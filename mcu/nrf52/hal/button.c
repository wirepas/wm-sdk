/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    button.c
 * \brief   Board-specific button module for nrf52
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

/** \brief  Each button use a GPIOTE channel Define first one */
#define GPIOTE_START_CHANNEL        0

/** \brief  Board-dependent Button number to pin mapping */
static const uint8_t pin_map[] = BOARD_BUTTON_PIN_LIST;

/** \brief  Compute number of button on the board */
#define BOARD_BUTTON_NUMBER         (sizeof(pin_map) / sizeof(pin_map[0]))

static void gpiote_interrupt_handler(void);

typedef struct
{
    // Callback when button pressed
    on_button_event_cb on_pressed;
    // Callback when button released
    on_button_event_cb on_released;
    // Used for debounce
    app_lib_time_timestamp_hp_t last_button_event;
    uint8_t gpio;
    uint8_t channel;
    bool active_low;
} button_internal_t;

/** \brief  Table to manage the button list */
static button_internal_t m_button_conf[BOARD_BUTTON_NUMBER];

void Button_init(void)
{
    app_lib_time_timestamp_hp_t now = lib_time->getTimestampHp();

    for (uint8_t i = 0; i < BOARD_BUTTON_NUMBER; i ++)
    {
        m_button_conf[i].gpio = pin_map[i];
        m_button_conf[i].channel = GPIOTE_START_CHANNEL + i;
        m_button_conf[i].on_pressed = NULL;
        m_button_conf[i].on_released = NULL;
        m_button_conf[i].last_button_event = now;
        m_button_conf[i].active_low = BOARD_BUTTON_ACTIVE_LOW;

        // Enable the Pull-Up on Buttons . Otherwise when the button is not pressed the pin is not connected (undefined value)
        NRF_GPIO->PIN_CNF[m_button_conf[i].gpio] =
            NRF_GPIO->PIN_CNF[m_button_conf[i].gpio] | (NRF_GPIO_PIN_PULLUP << 2);

        NRF_GPIOTE->CONFIG[m_button_conf[i].channel] =
                    ((GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) & GPIOTE_CONFIG_MODE_Msk)
                    |((GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) & GPIOTE_CONFIG_POLARITY_Msk)
                    |((m_button_conf[i].gpio  << GPIOTE_CONFIG_PSEL_Pos) & GPIOTE_CONFIG_PSEL_Msk);
    }

    NRF_GPIOTE->INTENCLR = 0xFFFFFFFF;

    // Enable interrupt
    lib_system->enableAppIrq(true, GPIOTE_IRQn, APP_LIB_SYSTEM_IRQ_PRIO_LO, gpiote_interrupt_handler);
}

button_res_e Button_getState(uint8_t button_id, bool * state_p)
{
    if (button_id >= BOARD_BUTTON_NUMBER)
    {
        // Invalid button number, just answer not pressed
        return BUTTON_RES_INVALID_ID;
    }

    *state_p = ((nrf_gpio_pin_read(m_button_conf[button_id].gpio) != 0)
               != m_button_conf[button_id].active_low);

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

    if (m_button_conf[button_id].on_pressed || m_button_conf[button_id].on_released)
    {
        // At least one event registered, enable interrupt on given channel
        NRF_GPIOTE->INTENSET = NRF_GPIOTE->INTENSET | (1 << m_button_conf[button_id].channel);
        NRF_GPIOTE->EVENTS_IN[m_button_conf[button_id].channel] = 0;
    }
    else
    {
        // Disable interrupt
        NRF_GPIOTE->INTENSET = NRF_GPIOTE->INTENSET & ~(1 << m_button_conf[button_id].channel);
    }
    Sys_exitCriticalSection();

    return BUTTON_RES_OK;
}

uint8_t Button_get_number(void)
{
    return BOARD_BUTTON_NUMBER;
}

static void gpiote_interrupt_handler(void)
{
    app_lib_time_timestamp_hp_t now = lib_time->getTimestampHp();

    // Check all possible sources
    for (uint8_t i = 0; i < BOARD_BUTTON_NUMBER; i ++)
    {
        if (NRF_GPIOTE->EVENTS_IN[m_button_conf[i].channel] != 0)
        {
            if (lib_time->getTimeDiffUs(now, m_button_conf[i].last_button_event) > (BOARD_DEBOUNCE_TIME_MS * 1000))
            {
                // Button is pressed if gpio is in active state
                bool pressed = ((nrf_gpio_pin_read(m_button_conf[i].gpio) != 0) != m_button_conf[i].active_low);
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
             NRF_GPIOTE->EVENTS_IN[m_button_conf[i].channel] = 0;
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
