/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    button.c
 * \brief   Board-specific button module for efr32
 */

#include "button.h"
#include "mcu.h"
#include "board.h"
#include "api.h"


#ifdef BOARD_BUTTON_PIN_LIST

/*
 * The selected board has buttons
 */

#ifndef BOARD_DEBOUNCE_TIME_MS
/** \brief  Debounce time of button in ms. It can be overwritten from board.h */
#define BOARD_DEBOUNCE_TIME_MS      100
#endif

#ifndef BOARD_BUTTON_ACTIVE_LOW
/** \brief  Is button active low. It can be overwritten from board.h */
#define BOARD_BUTTON_ACTIVE_LOW     true
#endif

#ifndef BOARD_BUTTON_INTERNAL_PULL
/** \brief  Does the driver needs to activate internal pull-up/down.
 *          If true; pull-up (down) is enabled if BOARD_BUTTON_ACTIVE_LOW is
 *          true (false). It can be overwritten from board.h
 */
#define BOARD_BUTTON_INTERNAL_PULL     true
#endif

#ifndef BOARD_BUTTON_USE_EVEN_INT
/** \brief  Use even (or odd) ext int. It can be overwritten from board.h */
#define BOARD_BUTTON_USE_EVEN_INT   true
#endif

/** \brief  Each button use a GPIOTE channel Define first one */
#define GPIOTE_START_CHANNEL        0

/** \brief GPIO port and pin number */
typedef struct
{
    uint8_t ext_int;
    uint8_t port;
    uint8_t pin;
} button_gpio_t;

/** \brief  Board-dependent Button number to pin mapping */
static const button_gpio_t m_pin_map[] = BOARD_BUTTON_PIN_LIST;

/** \brief  Compute number of buttons on the board */
#define BOARD_BUTTON_NUMBER   (sizeof(m_pin_map) / sizeof(m_pin_map[0]))

typedef struct
{
    // Callback when button pressed
    on_button_event_cb on_pressed;
    // Callback when button released
    on_button_event_cb on_released;
    // Used for debounce
    app_lib_time_timestamp_hp_t last_button_event;
    // True if button GPIO is active low
    bool active_low;
} button_internal_t;

/** \brief  Table to manage the button list */
static button_internal_t m_button_conf[BOARD_BUTTON_NUMBER];

static void button_interrupt_handler(void);

static void button_enable_interrupt(uint8_t ext_int,
                                    uint8_t port,
                                    uint8_t pin,
                                    bool enable)
{
    bool high_reg = ext_int >= 8;
    uint8_t shift = (ext_int & 7) * 4;
    uint32_t mask = ~((uint32_t)0xf << shift);
    uint32_t set = 0;

    // Select port
    set = enable ? (uint32_t)port << shift : 0;
    if (high_reg)
    {
#if !defined(_SILICON_LABS_32B_SERIES_2_CONFIG_1)
        GPIO->EXTIPSELH = (GPIO->EXTIPSELH & mask) | set;
#endif
    }
    else
    {
        GPIO->EXTIPSELL = (GPIO->EXTIPSELL & mask) | set;
    }

    // Select pin
    set = enable ? (uint32_t)(pin & 3) << shift : 0;
    if (high_reg)
    {
#if !defined(_SILICON_LABS_32B_SERIES_2_CONFIG_1)
        GPIO->EXTIPINSELH = (GPIO->EXTIPINSELH & mask) | set;
#endif
    }
    else
    {
        GPIO->EXTIPINSELL = (GPIO->EXTIPINSELL & mask) | set;
    }

    // Set rising and falling edge sensitivity
    shift = ext_int;
    mask = ~((uint32_t)1 << shift);
    set = enable ? (uint32_t)1 << shift : 0;
    GPIO->EXTIRISE = (GPIO->EXTIRISE & mask) | set;
    GPIO->EXTIFALL = (GPIO->EXTIFALL & mask) | set;

    //Clear external interrupt flag
#if defined(_SILICON_LABS_32B_SERIES_1)
    GPIO->IFC = (uint32_t)1 << shift;
#else
    GPIO->IF_CLR = (uint32_t)1 << shift;
#endif

    // Enable or disable external interrupt
    GPIO->IEN = (GPIO->IEN & mask) | set;
}

void Button_init(void)
{
    // Enable clocks
#if defined(_SILICON_LABS_32B_SERIES_1)
    CMU->HFBUSCLKEN0 |= CMU_HFBUSCLKEN0_GPIO;
#elif !defined (EFR32MG21)
    CMU->CLKEN0_SET = CMU_CLKEN0_GPIO;
#endif

    // Get current timestamp
    app_lib_time_timestamp_hp_t now = lib_time->getTimestampHp();

    // Configure button GPIOs
    for (uint8_t i = 0; i < BOARD_BUTTON_NUMBER; i++)
    {
        // Set button configuration
        m_button_conf[i].on_pressed = NULL;
        m_button_conf[i].on_released = NULL;
        m_button_conf[i].last_button_event = now;
        m_button_conf[i].active_low = BOARD_BUTTON_ACTIVE_LOW;

        // Enable the Pull-Up/Down on Buttons.
        if (BOARD_BUTTON_INTERNAL_PULL)
        {
            // Set pin mode: input enabled with filter, DOUT sets pull dir.
            hal_gpio_set_mode(m_pin_map[i].port,
                              m_pin_map[i].pin,
                              GPIO_MODE_IN_PP);

            if (m_button_conf[i].active_low)
            {
                hal_gpio_set(m_pin_map[i].port, m_pin_map[i].pin);
            }
            else
            {
                hal_gpio_clear(m_pin_map[i].port, m_pin_map[i].pin);
            }
        }
        else
        {
            // Default config: Input No Pull, DOUT enables filter.
            hal_gpio_set_mode(m_pin_map[i].port,
                              m_pin_map[i].pin,
                              GPIO_MODE_IN_OD_NOPULL);
            hal_gpio_set(m_pin_map[i].port, m_pin_map[i].pin);
        }
    }

    // Enable even or odd external interrupts
    lib_system->enableAppIrq(
        false,
        BOARD_BUTTON_USE_EVEN_INT ? GPIO_EVEN_IRQn : GPIO_ODD_IRQn,
        APP_LIB_SYSTEM_IRQ_PRIO_LO,
        button_interrupt_handler);
}

button_res_e Button_getState(uint8_t button_id, bool * state_p)
{
    if (button_id >= BOARD_BUTTON_NUMBER)
    {
        // Invalid button number
        return BUTTON_RES_INVALID_ID;
    }

    uint32_t b = hal_gpio_get(m_pin_map[button_id].port,
                              m_pin_map[button_id].pin);

    if (m_button_conf[button_id].active_low)
    {
        b ^= 0x01;
    }

    *state_p = (b != 0);

    return BUTTON_RES_OK;
}

button_res_e Button_register_for_event(uint8_t button_id,
                                       button_event_e event,
                                       on_button_event_cb cb)
{
    if ((button_id >= BOARD_BUTTON_NUMBER)
        || (event != BUTTON_PRESSED && event != BUTTON_RELEASED ))
    {
        // Invalid button number
        return BUTTON_RES_INVALID_ID;
    }

    Sys_enterCriticalSection();

    if (event == BUTTON_PRESSED)
    {
        m_button_conf[button_id].on_pressed = cb;
    }
    else
    {
        m_button_conf[button_id].on_released = cb;
    }

    // Enable interrupt if at least one event registered, disable otherwise
    button_enable_interrupt(m_pin_map[button_id].ext_int,
                            m_pin_map[button_id].port,
                            m_pin_map[button_id].pin,
                            m_button_conf[button_id].on_pressed ||
                            m_button_conf[button_id].on_released);

    Sys_exitCriticalSection();

    return BUTTON_RES_OK;
}

uint8_t Button_get_number(void)
{
    return BOARD_BUTTON_NUMBER;
}

static void button_interrupt_handler(void)
{
    app_lib_time_timestamp_hp_t now = lib_time->getTimestampHp();

    // Check all possible sources
    for (uint8_t i = 0; i < BOARD_BUTTON_NUMBER; i++)
    {
        // Read external interrupt flag
        if ((GPIO->IF & (((uint32_t)1) << m_pin_map[i].ext_int)) != 0)
        {
            if (lib_time->getTimeDiffUs(now, m_button_conf[i].last_button_event)
                > (BOARD_DEBOUNCE_TIME_MS * 1000))
            {
                // Button is pressed if gpio is in active state
                bool pressed = hal_gpio_get(m_pin_map[i].port,
                                            m_pin_map[i].pin);
                if (m_button_conf[i].active_low)
                {
                    pressed = !pressed;
                }
                m_button_conf[i].last_button_event = now;

                if (m_button_conf[i].on_pressed && pressed)
                {
                    m_button_conf[i].on_pressed(i, BUTTON_PRESSED);
                }
                else if (m_button_conf[i].on_released && !pressed)
                {
                    m_button_conf[i].on_released(i, BUTTON_RELEASED);
                }
            }

            // Clear external interrupt flag
#if defined(_SILICON_LABS_32B_SERIES_1)
            GPIO->IFC = (uint32_t)1 << m_pin_map[i].ext_int;
#else
            GPIO->IF_CLR = (uint32_t)1 << m_pin_map[i].ext_int;
#endif
        }
    }
}

#else // BOARD_BUTTON_PIN_LIST

/*
 * The selected board has no buttons
 *
 * As some example apps support such boards but also provide extra features
 * when a board has buttons, the button driver has this dummy implementation
 * to simplify the build process.
 */

void Button_init(void)
{
    // Do nothing
}

button_res_e Button_getState(uint8_t button_id, bool * state_p)
{
    (void) button_id;
    *state_p = false;

    // Invalid button number
    return BUTTON_RES_INVALID_ID;
}

uint8_t Button_get_number(void)
{
    return 0;
}

button_res_e Button_register_for_event(uint8_t button_id,
                                       button_event_e event,
                                       on_button_event_cb cb)
{
    (void) button_id;
    (void) event;
    (void) cb;

    // Invalid button number
    return BUTTON_RES_INVALID_ID;
}

#endif // BOARD_BUTTON_PIN_LIST
