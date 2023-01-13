/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file    gpio.c
 * \brief   Board-specific GPIO functions for nrf52.
 * \attention Should be compatible with the gpio.h interface.
 */

#include "gpio.h"
#include "mcu.h"
#include "board.h"
#include "api.h"

/*
 * If some GPIOs are mapped
 *
 * Note:
 * If BOARD_GPIO_PIN_LIST is not defined,
 * then the dummy functions defined in the gpio_weak.c file will be used instead
 */
#ifdef BOARD_GPIO_PIN_LIST

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
    /** Callback called on GPIO events  */
    gpio_in_event_e event_cfg : 2;
} gpio_cfg_intern_t;

/** \brief GPIO id to GPIO pin map (array index: GPIO id ; array value: GPIO pin) */
static const gpio_pin_t m_id_to_pin_map[] = BOARD_GPIO_PIN_LIST;

/** \brief Compute number of GPIOs that are mapped (= total number of used GPIOs) */
#define BOARD_GPIO_NUMBER   (sizeof(m_id_to_pin_map) / sizeof(m_id_to_pin_map[0]))

/** \brief GPIO id to GPIO internal config map (array index: GPIO id ; array value: GPIO internal config) */
static gpio_cfg_intern_t m_id_to_cfg_map[BOARD_GPIO_NUMBER];

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
 */
static void input_set_cfg_irq(gpio_id_t id, const gpio_in_cfg_t *in_cfg);

/** \brief Initialize GPIOs interrupts */
static void input_init_irq(void);

/**
 * \brief   Check if any of the GPIO "SENSE" is enabled
 * \return  True if any of the  GPIO "SENSE" is enabled ; False otherwise
 */
static bool any_sense_enabled(void);

/** \brief function called when a GPIO interrupt is raised. Used to generate the appropriate GPIO event */
static void gpio_event_handle(void);

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

    /* Configure each mapped GPIOs to a default configuration  */
    for (id = 0; id < BOARD_GPIO_NUMBER; id++)
    {
        input_set_cfg_mode(id, &in_cfg);
        input_set_cfg_irq(id, &in_cfg);
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
    input_set_cfg_irq(id, in_cfg);
    /* Store direction and part of configuration that is used internally */
    m_id_to_event_cb_map[id] = in_cfg->event_cb;
    m_id_to_cfg_map[id].direction = DIR_INPUT;
    m_id_to_cfg_map[id].event_cfg = in_cfg->event_cfg;

    Sys_exitCriticalSection();

    return GPIO_RES_OK;
}

gpio_res_e Gpio_inputRead(gpio_id_t id, gpio_level_e *level)
{
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

    Gpio_getPin(id, NULL, &pin);
    read_val = (nrf_gpio_pin_read(pin) != 0);
    *level = read_val ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW;

    return GPIO_RES_OK;
}

gpio_res_e Gpio_outputSetCfg(gpio_id_t id, const gpio_out_cfg_t *out_cfg)
{
    nrf_gpio_pin_pull_t pull;
    nrf_gpio_pin_drive_t drive;
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
            drive = NRF_GPIO_PIN_S0S1;
            pull = NRF_GPIO_PIN_NOPULL;
            break;
        case GPIO_OUT_MODE_OPEN_DRAIN:
            drive = NRF_GPIO_PIN_S0D1;
            pull = NRF_GPIO_PIN_NOPULL;
            break;
        case GPIO_OUT_MODE_OPEN_DRAIN_WITH_PULL_UP:
            drive = NRF_GPIO_PIN_S0D1;
            pull = NRF_GPIO_PIN_PULLUP;
            break;
        default:
            return GPIO_RES_INVALID_PARAM;
    }

    Sys_enterCriticalSection();

    Gpio_getPin(id, NULL, &pin);
    nrf_gpio_cfg(pin,
                NRF_GPIO_PIN_DIR_OUTPUT,
                NRF_GPIO_PIN_INPUT_DISCONNECT,
                pull,
                drive,
                NRF_GPIO_PIN_NOSENSE);
    /* Set pin default state */
    nrf_gpio_pin_write(pin, (out_cfg->level_default != GPIO_LEVEL_LOW));
    /* Store direction */
    m_id_to_cfg_map[id].direction = DIR_OUTPUT;

    Sys_exitCriticalSection();

    return GPIO_RES_OK;
}

gpio_res_e Gpio_outputWrite(gpio_id_t id, gpio_level_e level)
{
    gpio_pin_t pin;
    bool  write_val;

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

    Gpio_getPin(id, NULL, &pin);
    write_val = (level != GPIO_LEVEL_LOW);
    nrf_gpio_pin_write(pin, write_val);

    return GPIO_RES_OK;
}

gpio_res_e Gpio_outputToggle(gpio_id_t id)
{
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

    Gpio_getPin(id, NULL, &pin);
    nrf_gpio_pin_toggle(pin);

    return GPIO_RES_OK;
}

gpio_res_e Gpio_outputRead(gpio_id_t id, gpio_level_e *level)
{
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

    Gpio_getPin(id, NULL, &pin);
    read_val = nrf_gpio_pin_out_read(pin) != 0;
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
        /*
         * With NRF chips, all the information is stored in pin.
         * Thus port is always set to 0 and it can be ignored.
         */
        *port = 0;
    }
    if (pin)
    {
        *pin = m_id_to_pin_map[id];
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
    gpio_pin_t pin;

    for (id = 0; id < BOARD_GPIO_NUMBER; id++)
    {
        Gpio_getPin(id, NULL, &pin);
        if (!nrf_gpio_pin_present_check(pin))
        {
            return false;
        }
    }
    return true;
}

static void input_set_cfg_mode(gpio_id_t id, const gpio_in_cfg_t *in_cfg)
{
    gpio_pin_t pin;
    nrf_gpio_pin_pull_t nrf_pin_pull;
    nrf_gpio_pin_input_t nrf_connect;

    /*
     * convert board independant mode enum,
     * to board specific pull enum
     */
    switch (in_cfg->in_mode_cfg)
    {
        case GPIO_IN_DISABLED:
            nrf_pin_pull = NRF_GPIO_PIN_NOPULL;
            nrf_connect = NRF_GPIO_PIN_INPUT_DISCONNECT;
            break;
        case GPIO_IN_PULL_NONE:
            nrf_pin_pull = NRF_GPIO_PIN_NOPULL;
            nrf_connect = NRF_GPIO_PIN_INPUT_CONNECT;
            break;
        case GPIO_IN_PULL_DOWN:
            nrf_pin_pull = NRF_GPIO_PIN_PULLDOWN;
            nrf_connect = NRF_GPIO_PIN_INPUT_CONNECT;
            break;
        case GPIO_IN_PULL_UP:
            nrf_pin_pull = NRF_GPIO_PIN_PULLUP;
            nrf_connect = NRF_GPIO_PIN_INPUT_CONNECT;
            break;
    }
    Gpio_getPin(id, NULL, &pin);

    nrf_gpio_cfg(pin,
                NRF_GPIO_PIN_DIR_INPUT,
                nrf_connect,
                nrf_pin_pull,
                NRF_GPIO_PIN_S0S1,
                NRF_GPIO_PIN_NOSENSE);
}

static void input_set_cfg_irq(gpio_id_t id, const gpio_in_cfg_t *in_cfg)
{
    gpio_pin_t pin;
    nrf_gpio_pin_sense_t sense;

    Gpio_getPin(id, NULL, &pin);

    /* if no event on this GPIO, ... */
    if (in_cfg->event_cfg == GPIO_IN_EVENT_NONE)
    {
        /* ...then disable its sense feature */
        nrf_gpio_cfg_sense_set(pin, NRF_GPIO_PIN_NOSENSE);
        nrf_gpio_pin_latch_clear(pin);

        /* if not a single GPIO uses its sense feature, ... */
        if(!any_sense_enabled())
        {
            /* ...then disable the port interrupt (since it is no longer used by any GPIO) */
            nrf_gpiote_int_disable(NRF_GPIOTE, GPIOTE_INTENCLR_PORT_Msk);
        }
    }
    /*  if event on this GPIO (rising edge, falling edge, or both),... */
    else
    {
        /* ...then read the current pin state and set for next sense to oposit */
        sense = (nrf_gpio_pin_read(pin)) ? NRF_GPIO_PIN_SENSE_LOW : NRF_GPIO_PIN_SENSE_HIGH;
        nrf_gpio_cfg_sense_set(pin, sense);
        nrf_gpio_pin_latch_clear(pin);
        nrf_gpiote_int_enable(NRF_GPIOTE, GPIOTE_INTENCLR_PORT_Msk);
    }
}

static void input_init_irq(void)
{
    nrf_gpiote_event_clear(NRF_GPIOTE, NRF_GPIOTE_EVENT_PORT);
    nrf_gpiote_int_disable(NRF_GPIOTE, GPIOTE_INTENCLR_PORT_Msk);
    lib_system->enableAppIrq(true,
                             GPIOTE_IRQn,
                             APP_LIB_SYSTEM_IRQ_PRIO_LO,
                             gpio_event_handle);
    lib_system->clearPendingFastAppIrq(GPIOTE_IRQn);
}

static bool any_sense_enabled(void)
{
    gpio_id_t id;
    gpio_pin_t pin;
    nrf_gpio_pin_sense_t sense;

    for (id = 0; id < BOARD_GPIO_NUMBER; id++)
    {
        Gpio_getPin(id, NULL, &pin);
        sense = nrf_gpio_pin_sense_get(pin);
        if (sense != NRF_GPIO_PIN_NOSENSE)
        {
            return true;
        }
    }
    return false;
}

static void gpio_event_handle(void)
{
    gpio_id_t id;
    gpio_pin_t pin;
    gpio_in_event_cb_f event_cb;
    gpio_in_event_e event_cfg;
    nrf_gpio_pin_sense_t sense;
    gpio_in_event_e event;

    /* leave if the port interrupt flag is not set (should not happen) */
    if (!nrf_gpiote_event_check(NRF_GPIOTE, NRF_GPIOTE_EVENT_PORT))
    {
        return;
    }

    /* clear the port interrupt */
    nrf_gpiote_event_clear(NRF_GPIOTE, NRF_GPIOTE_EVENT_PORT);

    /* for each GPIO */
    for (id = 0; id < BOARD_GPIO_NUMBER; id++)
    {
        Gpio_getPin(id, NULL, &pin);
        /* check if the GPIO latch (= GPIO interrupt flag) is set */
        if (nrf_gpio_pin_latch_get(pin) != 0)
        {
            /* get stored/internal config  */
            event_cb = m_id_to_event_cb_map[id];
            event_cfg = m_id_to_cfg_map[id].event_cfg;

            sense = nrf_gpio_pin_sense_get(pin);
            /*
             * Invoke user handler only if the sensed pin level
             * matches its polarity configuration.
             */
            if (event_cb &&
               ((IS_RISING_EDGE(event_cfg) && IS_FALLING_EDGE(event_cfg)) ||
               (sense == NRF_GPIO_PIN_SENSE_HIGH && IS_RISING_EDGE(event_cfg)) ||
               (sense == NRF_GPIO_PIN_SENSE_LOW && IS_FALLING_EDGE(event_cfg))))
            {
                event = (sense == NRF_GPIO_PIN_SENSE_HIGH) ? GPIO_IN_EVENT_RISING_EDGE : GPIO_IN_EVENT_FALLING_EDGE;
                event_cb(id, event);
            }

            /*
             * Reconfigure sense to the opposite level, so the internal PINx.DETECT signal
             * can be deasserted. Therefore PORT event generated again,
             * unless some other PINx.DETECT signal is still active.
             */
            sense = (sense == NRF_GPIO_PIN_SENSE_HIGH) ? NRF_GPIO_PIN_SENSE_LOW : NRF_GPIO_PIN_SENSE_HIGH;
            nrf_gpio_cfg_sense_set(pin, sense);

            /*
             * Try to clear LATCH bit corresponding to currently processed pin.
             * This may not succeed if the pin's state changed during the interrupt processing
             * and now it matches the new sense configuration. In such case,
             * the pin will be processed again in another iteration of the outer loop.
             */
            nrf_gpio_pin_latch_clear(pin);
        }
    }
}

#endif // BOARD_GPIO_PIN_LIST
