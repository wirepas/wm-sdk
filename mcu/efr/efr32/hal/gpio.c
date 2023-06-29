/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file    gpio.c
 * \brief   Board-specific GPIO functions for efr32.
 * \attention Should be compatible with the gpio.h interface.
 */

#include "gpio.h"
#include "mcu.h"
#include "board.h"
#include "api.h"
#include "em_cmu.h"
#include "em_gpio.h"

/*
 * If some GPIOs are mapped
 *
 * Note:
 * If BOARD_GPIO_PIN_LIST is not defined,
 * then the dummy functions defined in the gpio_weak.c file will be used instead
 */
#ifdef BOARD_GPIO_PIN_LIST

/** \brief Interrupt identified as free (meaning it is available) */
#define IT_FREE (0x1F) // must be greater than GPIO_EXTINTNO_MAX

/** \brief GPIO external interrupt number */
typedef uint8_t gpio_it_t;

/** \brief GPIO direction */
typedef enum
{
    /** input direction */
    DIR_INPUT,
    /** output direction */
    DIR_OUTPUT
} direction_e;

/** \brief GPIO internal configuration */
typedef struct
{
    /** GPIO direction (either input or output)  */
    direction_e direction : 1;
    /** Callback called on GPIO events. Only used on input GPIO   */
    gpio_in_event_e event_cfg : 2;
    /** GPIO interrupt number. Only used on input GPIO */
    gpio_it_t it : 5; // bitsize = 5 to hold interrupt number (see GPIO_EXTINTNO_MAX) and IT_FREE value
} gpio_cfg_t;

/** \brief GPIO id to GPIO pin map (array index: GPIO id ; array value: GPIO port & pin) */
static const struct
{
    gpio_port_t port;
    gpio_pin_t pin;
} m_id_to_pin_map[] = BOARD_GPIO_PIN_LIST;

/** \brief Compute number of GPIOs that are mapped (= total number of used GPIOs) */
#define BOARD_GPIO_NUMBER (sizeof(m_id_to_pin_map) / sizeof(m_id_to_pin_map[0]))

/** \brief GPIO id to GPIO internal config map (array index: GPIO id ; array value: GPIO internal config) */
static gpio_cfg_t m_id_to_cfg_map[BOARD_GPIO_NUMBER];

/** \brief GPIO id to GPIO event callback map (array index: GPIO id ; array value: GPIO event callback) */
static gpio_in_event_cb_f m_id_to_event_cb_map[BOARD_GPIO_NUMBER];

/** \brief Has the library been initialized */
static bool m_initialized = false;

/**
 * \brief   Check if the pin numbers of the mapped GPIOs are valid (if they exist on the MCU)
 * \return  True if all the pin numbers are valid ; False otherwise
 */
static bool check_pins(void);

/**
 * \brief       Configure mode (e.g.: pull-down) of input GPIO
 * \param       id
 *              Id of the GPIO
 * \param[in]   in_cfg
 *              GPIO input configuration
 */
static void input_set_cfg_mode(gpio_id_t id, const gpio_in_cfg_t *in_cfg);

/**
 * \brief       Configure interrupt of input GPIO
 * \param       id
 *              Id of the GPIO
 * \param[in]   in_cfg
 *              GPIO input configuration
 * \return      Return code of operation
 */
static gpio_res_e input_set_cfg_irq(gpio_id_t id, const gpio_in_cfg_t *in_cfg);

/** \brief Initialize GPIOs interrupts */
static void input_init_irq(void);

/**
 * \brief       Allocate a interrupt number to a GPIO
 * \param       id
 *              Id of the GPIO
 * \param[out]  it
 *              Returned interrupt number
 * \return      Return code of operation
 */
static gpio_res_e alloc_it(gpio_id_t id, gpio_it_t *it);

/**
 * \brief       Free a interrupt number allocated to a GPIO
 * \param       id
 *              Id of the GPIO
 * \param[out]  it
 *              Freed interrupt number
 * \return      Return code of operation
 */
static void free_it(gpio_id_t id, gpio_it_t *it);

/**
 * \brief   Check if the given interrupt number is already in use or if it is free
 * \param   it
 *          interrupt number to check
 * \return  True if the given interrupt number is free ; False otherwise (meaning it is already in use)
 */
static bool is_it_free(gpio_it_t it);

/** \brief function called when a GPIO interrupt is raised. Used to generate the appropriate GPIO event */
static void gpio_event_handle(void);

/** \brief Enable GPIO clock */
static inline void enable_gpio_clock(void)
{
#if (_SILICON_LABS_32B_SERIES == 1)
    CMU->HFBUSCLKEN0 |= CMU_HFBUSCLKEN0_GPIO;
#elif (_SILICON_LABS_32B_SERIES == 2) && (_SILICON_LABS_32B_SERIES_2_CONFIG > 1)
    CMU->CLKEN0_SET = CMU_CLKEN0_GPIO;
#endif
}

gpio_res_e Gpio_init(void)
{
    gpio_id_t id;
    const gpio_in_cfg_t in_cfg =
    {
        .event_cb = NULL,
        .event_cfg = GPIO_IN_EVENT_NONE,
        .in_mode_cfg = GPIO_IN_DISABLED
    };

    if (m_initialized)
    {
        /* return if GPIO initialization has already been performed */
        return GPIO_RES_OK;
    }
    if (!check_pins())
    {
        return GPIO_RES_INVALID_PIN;
    }

    Sys_enterCriticalSection();

    enable_gpio_clock();
    /* Configure each mapped GPIOs to a default configuration  */
    for (id = 0; id < BOARD_GPIO_NUMBER; id++)
    {
        input_set_cfg_mode(id, &in_cfg);
        /* set interrupt number as unused/free */
        m_id_to_cfg_map[id].it = IT_FREE;
        /* Store direction and part of configuration that is used internally */
        m_id_to_event_cb_map[id] = in_cfg.event_cb;
        m_id_to_cfg_map[id].direction = DIR_INPUT;
        m_id_to_cfg_map[id].event_cfg = in_cfg.event_cfg;
    }
    input_init_irq();

    Sys_exitCriticalSection();

    m_initialized = true;

    return GPIO_RES_OK;
}

gpio_res_e Gpio_inputSetCfg(gpio_id_t id, const gpio_in_cfg_t *in_cfg)
{
    gpio_res_e res;

    if (!m_initialized)
    {
        return GPIO_RES_UNINITIALIZED;
    }
    if (id >= BOARD_GPIO_NUMBER || in_cfg == NULL)
    {
        return GPIO_RES_INVALID_PARAM;
    }

    Sys_enterCriticalSection();

    input_set_cfg_mode(id, in_cfg);
    res = input_set_cfg_irq(id, in_cfg);
    if (res != GPIO_RES_OK)
    {
        Sys_exitCriticalSection();
        return res;
    }
    /* Store direction and part of configuration that is used internally */
    m_id_to_event_cb_map[id] = in_cfg->event_cb;
    m_id_to_cfg_map[id].direction = DIR_INPUT;
    m_id_to_cfg_map[id].event_cfg = in_cfg->event_cfg;

    Sys_exitCriticalSection();

    return GPIO_RES_OK;
}

gpio_res_e Gpio_inputRead(gpio_id_t id, gpio_level_e *level)
{
    gpio_port_t port;
    gpio_pin_t pin;
    bool  read_val;

    if (!m_initialized)
    {
        return GPIO_RES_UNINITIALIZED;
    }
    if (id >= BOARD_GPIO_NUMBER || level == NULL)
    {
        return GPIO_RES_INVALID_PARAM;
    }
    if (m_id_to_cfg_map[id].direction != DIR_INPUT)
    {
        return GPIO_RES_INVALID_DIRECTION;
    }

    Gpio_getPin(id, &port, &pin);
    read_val = (GPIO_PinInGet(port, pin) != 0);
    *level = read_val ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW;

    return GPIO_RES_OK;
}

gpio_res_e Gpio_outputSetCfg(gpio_id_t id, const gpio_out_cfg_t *out_cfg)
{
    GPIO_Mode_TypeDef mode;
    gpio_port_t port;
    gpio_pin_t pin;

    if (!m_initialized)
    {
        return GPIO_RES_UNINITIALIZED;
    }
    if (id >= BOARD_GPIO_NUMBER || out_cfg == NULL)
    {
        return GPIO_RES_INVALID_PARAM;
    }

    /*
     * convert board independant operating mode enum,
     * to board specific operating mode enum
     */
    switch (out_cfg->out_mode_cfg)
    {
        case GPIO_OUT_MODE_PUSH_PULL:
            mode = gpioModePushPull;
            break;
        case GPIO_OUT_MODE_OPEN_DRAIN:
            mode = gpioModeWiredAndFilter;
            break;
        case GPIO_OUT_MODE_OPEN_DRAIN_WITH_PULL_UP:
            mode = gpioModeWiredAndPullUpFilter;
            break;
        default:
            return GPIO_RES_INVALID_PARAM;
    }

    Sys_enterCriticalSection();

    Gpio_getPin(id, &port, &pin);
    GPIO_PinModeSet(port, pin, mode, (out_cfg->level_default != GPIO_LEVEL_LOW));
    /* Store direction */
    m_id_to_cfg_map[id].direction = DIR_OUTPUT;

    Sys_exitCriticalSection();

    return GPIO_RES_OK;
}

gpio_res_e Gpio_outputWrite(gpio_id_t id, gpio_level_e level)
{
    gpio_port_t port;
    gpio_pin_t pin;

    if (!m_initialized)
    {
        return GPIO_RES_UNINITIALIZED;
    }
    if (id >= BOARD_GPIO_NUMBER)
    {
        return GPIO_RES_INVALID_PARAM;
    }
    if (m_id_to_cfg_map[id].direction != DIR_OUTPUT)
    {
        return GPIO_RES_INVALID_DIRECTION;
    }

    Gpio_getPin(id, &port, &pin);
    if (level == GPIO_LEVEL_HIGH)
    {
        GPIO_PinOutSet(port, pin);
    }
    else
    {
        GPIO_PinOutClear(port, pin);
    }

    return GPIO_RES_OK;
}

gpio_res_e Gpio_outputToggle(gpio_id_t id)
{
    gpio_port_t port;
    gpio_pin_t pin;

    if (!m_initialized)
    {
        return GPIO_RES_UNINITIALIZED;
    }
    if (id >= BOARD_GPIO_NUMBER)
    {
        return GPIO_RES_INVALID_PARAM;
    }
    if (m_id_to_cfg_map[id].direction != DIR_OUTPUT)
    {
        return GPIO_RES_INVALID_DIRECTION;
    }

    Gpio_getPin(id, &port, &pin);
    GPIO_PinOutToggle(port, pin);

    return GPIO_RES_OK;
}

gpio_res_e Gpio_outputRead(gpio_id_t id, gpio_level_e *level)
{
    gpio_port_t port;
    gpio_pin_t pin;
    bool  read_val;

    if (!m_initialized)
    {
        return GPIO_RES_UNINITIALIZED;
    }
    if (id >= BOARD_GPIO_NUMBER || level == NULL)
    {
        return GPIO_RES_INVALID_PARAM;
    }
    if (m_id_to_cfg_map[id].direction != DIR_OUTPUT)
    {
        return GPIO_RES_INVALID_DIRECTION;
    }

    Gpio_getPin(id, &port, &pin);
    read_val = GPIO_PinOutGet(port, pin) != 0;
    *level = read_val ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW;

    return GPIO_RES_OK;
}

gpio_res_e Gpio_getPin(gpio_id_t id, gpio_port_t *port, gpio_pin_t *pin)
{
    if (id >= BOARD_GPIO_NUMBER)
    {
        return GPIO_RES_INVALID_PARAM;
    }

    if (port)
    {
        *port = m_id_to_pin_map[id].port;
    }
    if (pin)
    {
        *pin = m_id_to_pin_map[id].pin;
    }

    return GPIO_RES_OK;
}

uint8_t Gpio_getNumber(void)
{
    return BOARD_GPIO_NUMBER;
}

static bool check_pins(void)
{
    gpio_id_t id;
    gpio_port_t port;
    gpio_pin_t pin;

    for (id = 0; id < BOARD_GPIO_NUMBER; id++)
    {
        Gpio_getPin(id, &port, &pin);
        if (!GPIO_PORT_PIN_VALID(port, pin))
        {
            return false;
        }
    }
    return true;
}

static void input_set_cfg_mode(gpio_id_t id, const gpio_in_cfg_t *in_cfg)
{
    gpio_port_t port;
    gpio_pin_t pin;
    GPIO_Mode_TypeDef efr_mode;
    bool out;

    /*
     * convert board independant mode enum,
     * to board specific mode enum
     */
    switch (in_cfg->in_mode_cfg)
    {
        case GPIO_IN_DISABLED:
            efr_mode = gpioModeDisabled;
            /* set "out" to 0 to disable pull-up */
            out = false;
            break;
        case GPIO_IN_PULL_NONE:
            efr_mode = gpioModeInput;
            /* set "out" to 1 to enable filter */
            out = true;
            break;
        case GPIO_IN_PULL_DOWN:
            efr_mode = gpioModeInputPullFilter;
            /* "out" determines pull direction (0 -> pull-down)  */
            out = false;
            break;
        case GPIO_IN_PULL_UP:
            efr_mode = gpioModeInputPullFilter;
            /* "out" determines pull direction (1 -> pull-up)  */
            out = true;
            break;
    }
    Gpio_getPin(id, &port, &pin);
    GPIO_PinModeSet(port, pin, efr_mode, out);
}

static gpio_res_e input_set_cfg_irq(gpio_id_t id, const gpio_in_cfg_t *in_cfg)
{
    gpio_port_t port;
    gpio_pin_t pin;
    gpio_it_t it;
    gpio_res_e res;

    /* if no event on this GPIO, ... */
    if (in_cfg->event_cfg == GPIO_IN_EVENT_NONE)
    {
        /* ...then free/unalloc its interrupt number */
        free_it(id, &it);
    }
    /* else if any event on this GPIO, ... */
    else
    {
        /* ...then allocate an interrupt number to the GPIO */
        res = alloc_it(id, &it);
        if (res != GPIO_RES_OK)
        {
            return res;
        }
    }

    /* update the interrupt registers, if got a valid interrupt number  */
    if (it != IT_FREE)
    {
        Gpio_getPin(id, &port, &pin);
        GPIO_ExtIntConfig(port,
                          pin,
                          it,
                          IS_RISING_EDGE(in_cfg->event_cfg),
                          IS_FALLING_EDGE(in_cfg->event_cfg),
                          in_cfg->event_cfg != GPIO_IN_EVENT_NONE);
    }
    return GPIO_RES_OK;
}

static void input_init_irq(void)
{
    GPIO_IntDisable(_GPIO_IEN_MASK & 0x0000FFFF);
    lib_system->enableAppIrq(true, GPIO_EVEN_IRQn, APP_LIB_SYSTEM_IRQ_PRIO_LO, gpio_event_handle);
    lib_system->enableAppIrq(true, GPIO_ODD_IRQn, APP_LIB_SYSTEM_IRQ_PRIO_LO, gpio_event_handle);
    lib_system->clearPendingFastAppIrq(GPIO_EVEN_IRQn);
    lib_system->clearPendingFastAppIrq(GPIO_ODD_IRQn);
}

static gpio_res_e alloc_it(gpio_id_t id, gpio_it_t *it)
{
    gpio_pin_t pin;
    gpio_it_t it_tmp, it_min, it_max;
    bool it_found = false;

    /*
     * if an interrupt number was already allocated for this GPIO,
     * then just return it.
     */
    it_tmp = m_id_to_cfg_map[id].it;
    if (it_tmp != IT_FREE)
    {
        *it = it_tmp;
        return GPIO_RES_OK;
    }

    /*
     * Refer to chapter "Edge Interrupt Generation" in the MCU reference manual
     * for an explanation about the interrupt number allocation mechanism
     *
     * Example:
     * GPIO pin = Px5. in group Px[4:7] -> so one interrupt in the EXTI[4-7] interrupt group can be used.
     */
    Gpio_getPin(id, NULL, &pin);
    it_min = SL_FLOOR(pin, 4);
    it_max = SL_FLOOR(pin, 4) + 3;

    /* Browse the interrupt group, and allocate the first interrupt that is free */
    for (it_tmp = it_min; it_tmp <= it_max; it_tmp++)
    {
        if (is_it_free(it_tmp))
        {
            m_id_to_cfg_map[id].it = it_tmp;
            *it = it_tmp;
            it_found = true;
            break;
        }
    }
    return it_found ? GPIO_RES_OK : GPIO_RES_NO_FREE_IT;
}

static void free_it(gpio_id_t id, gpio_it_t *it)
{
    *it = m_id_to_cfg_map[id].it;
    m_id_to_cfg_map[id].it = IT_FREE;
}

static bool is_it_free(gpio_it_t it)
{
    gpio_id_t id;

    for (id = 0; id < BOARD_GPIO_NUMBER; id++)
    {
        if (m_id_to_cfg_map[id].it == it)
        {
            return false;
        }
    }
    return true;
}

static void gpio_event_handle(void)
{
    gpio_id_t id;
    gpio_it_t it;
    bool it_raised;
    gpio_level_e level;
    gpio_in_event_cb_f event_cb;
    gpio_in_event_e event_cfg;
    gpio_in_event_e event;

    /* for each GPIO */
    for (id = 0; id < BOARD_GPIO_NUMBER; id++)
    {
        /* get the interrupt number of the GPIO */
        it = m_id_to_cfg_map[id].it;
        if (it == IT_FREE)
        {
            continue;
        }

        /* check if the GPIO interrupt flag is set */
        it_raised = (GPIO_IntGet() & (((uint32_t) 1) << it)) != 0;
        if (it_raised)
        {
            Gpio_inputRead(id, &level);
            /* get stored/internal config  */
            event_cb = m_id_to_event_cb_map[id];
            event_cfg = m_id_to_cfg_map[id].event_cfg;
            /*
             * Invoke user handler only if the pin level
             * matches its polarity configuration.
             */
            if (event_cb &&
               ((IS_RISING_EDGE(event_cfg) && IS_FALLING_EDGE(event_cfg)) ||
               (level == GPIO_LEVEL_HIGH && IS_RISING_EDGE(event_cfg)) ||
               (level == GPIO_LEVEL_LOW && IS_FALLING_EDGE(event_cfg))))
            {
                event = (level == GPIO_LEVEL_HIGH) ? GPIO_IN_EVENT_RISING_EDGE : GPIO_IN_EVENT_FALLING_EDGE;
                event_cb(id, event);
            }

            /* clear the GPIO interrupt flag */
            GPIO_IntClear((uint32_t) 1 << it);
        }
    }
}

#endif // BOARD_GPIO_PIN_LIST
