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

#ifndef BOARD_BUTTON_INTERNAL_PULL
/** \brief  Does the driver needs to activate internal pull-up/down.
 *          If true; pull-up (down) is enabled if BOARD_BUTTON_ACTIVE_LOW is
 *          true (false). It can be overwritten from board.h
 */
#define BOARD_BUTTON_INTERNAL_PULL     true
#endif

/** \brief  Each button use a GPIOTE channel Define first one */
#define GPIOTE_START_CHANNEL        0

/** \brief  Board-dependent Button number to pin mapping */
static const uint8_t pin_map[] = BOARD_BUTTON_PIN_LIST;

/** \brief  Compute number of button on the board */
#define BOARD_BUTTON_NUMBER         (sizeof(pin_map) / sizeof(pin_map[0]))

static void gpiote_interrupt_handler(void);

typedef enum
{
    NO_INTERRUPT,
    ON_PRESSED_ONLY,
    ON_RELEASED_ONLY,
    ON_PRESSED_AND_RELEASED
} button_interrupt_type_e;

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

/* Button events and sense truth table
 *
 * Type : interrupt type see button_interrupt_type_e
 *        N:No_it, P:pressed_only, R:released_only, PR:pressed&released
 * act_low: Button is active low
 * Pin:     IO state (Low or High)
 * Sense:   Value to configure in sense register
 *
 *  Type | act_low | Pin | Sense
 *    N  |    X    |  X  | Disabled
 *
 *    P  |    Y    |  X  | Low
 *    P  |    N    |  X  | High
 *
 *    R  |    Y    |  X  | High
 *    R  |    N    |  X  | Low
 *
 *   PR  |    X    |  H  | Low
 *   PR  |    X    |  L  | Low
 *
 */
static void button_enable_interrupt(uint8_t button_id,
                                    button_interrupt_type_e type)
{
    bool is_sense_low = false;

    switch(type)
    {
        case NO_INTERRUPT:
        {
            //Disable SENSE
            NRF_GPIO->PIN_CNF[m_button_conf[button_id].gpio] = 0;
            NRF_GPIO->LATCH = 1 << m_button_conf[button_id].gpio;

            // Check if port IRQ must be masked
            for (uint8_t i = 0; i < BOARD_BUTTON_NUMBER; i++)
            {
                if (m_button_conf[i].on_pressed ||
                    m_button_conf[i].on_released)
                {
                    // At least one line still enabled
                    return;
                }
            }
            NRF_GPIOTE->INTENCLR = GPIOTE_INTENCLR_PORT_Msk;
            return;
        }
        case ON_PRESSED_ONLY:
        {
            is_sense_low = m_button_conf[button_id].active_low;
            break;
        }
        case ON_RELEASED_ONLY:
        {
            is_sense_low = !m_button_conf[button_id].active_low;
            break;
        }
        case ON_PRESSED_AND_RELEASED:
        {
            is_sense_low =
                nrf_gpio_pin_read(m_button_conf[button_id].gpio) == 0 ?
                                                                false : true;
            break;
        }
    }

    // Configure interrupt (type != NO_INTERRUPT)

    //Disable IRQ
    NRF_GPIO->PIN_CNF[m_button_conf[button_id].gpio] &=
                                                ~(GPIO_PIN_CNF_SENSE_Msk);

    if(is_sense_low)
    {
        NRF_GPIO->PIN_CNF[m_button_conf[button_id].gpio] |=
                        (GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos);
    }
    else
    {
        NRF_GPIO->PIN_CNF[m_button_conf[button_id].gpio] |=
                    (GPIO_PIN_CNF_SENSE_High << GPIO_PIN_CNF_SENSE_Pos);
    }
    // Clear the line before enabling IRQ
    NRF_GPIO->LATCH = 1 << m_button_conf[button_id].gpio;
    NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_PORT_Msk;
}

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

        // Enable the Pull-Up/Down on Buttons (Input; Pull; SENSE:Disabled)
        if (BOARD_BUTTON_INTERNAL_PULL)
        {
            uint32_t pull;

            if (BOARD_BUTTON_ACTIVE_LOW)
            {
                pull = (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos);
            }
            else
            {
                pull = (GPIO_PIN_CNF_PULL_Pulldown << GPIO_PIN_CNF_PULL_Pos);
            }
            NRF_GPIO->PIN_CNF[m_button_conf[i].gpio] = pull;
        }
        else
        {
            // Default config (Input; No Pull; SENSE:Disabled).
            NRF_GPIO->PIN_CNF[m_button_conf[i].gpio] = 0;
        }
    }

    NRF_GPIOTE->INTENCLR = GPIOTE_INTENCLR_PORT_Msk;
    NRF_GPIOTE->EVENTS_PORT = 0;

    // Enable interrupt
    lib_system->clearPendingFastAppIrq(GPIOTE_IRQn);
    lib_system->enableAppIrq(true,
                             GPIOTE_IRQn,
                             APP_LIB_SYSTEM_IRQ_PRIO_LO,
                             gpiote_interrupt_handler);
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
    button_interrupt_type_e type;

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

    if (m_button_conf[button_id].on_pressed &&
        m_button_conf[button_id].on_released)
    {
        type = ON_PRESSED_AND_RELEASED;
    }
    else if (m_button_conf[button_id].on_pressed)
    {
        type = ON_PRESSED_ONLY;
    }
    else if (m_button_conf[button_id].on_released)
    {
        type = ON_RELEASED_ONLY;
    }
    else
    {
        type = NO_INTERRUPT;
    }

    button_enable_interrupt(button_id, type);

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

    if (NRF_GPIOTE->EVENTS_PORT == 0)
    {
        return;
    }

    NRF_GPIOTE->EVENTS_PORT = 0;
    // read any event from peripheral to flush the write buffer:
    EVENT_READBACK = NRF_GPIOTE->EVENTS_PORT;

    // Check all possible sources
    for (uint8_t i = 0; i < BOARD_BUTTON_NUMBER; i ++)
    {
        uint8_t pin = m_button_conf[i].gpio;
        if (NRF_GPIO->LATCH & (1 << pin))
        {
            if (lib_time->getTimeDiffUs(now,
                                        m_button_conf[i].last_button_event)
                > (BOARD_DEBOUNCE_TIME_MS * 1000))
            {
                bool pressed;
                Button_getState(i, &pressed);
                m_button_conf[i].last_button_event = now;

                if (m_button_conf[i].on_pressed && pressed)
                {
                    // Call callback
                    m_button_conf[i].on_pressed(i, BUTTON_PRESSED);
                }
                else if (m_button_conf[i].on_released && !pressed)
                {
                    //Call callback
                    m_button_conf[i].on_released(i, BUTTON_RELEASED);
                }

                // Change SENSE polarity
                if (m_button_conf[i].on_pressed &&
                    m_button_conf[i].on_released)
                {
                    NRF_GPIO->PIN_CNF[pin] &= ~(GPIO_PIN_CNF_SENSE_Msk);
                    if (nrf_gpio_pin_read(pin) == 0)
                    {
                        NRF_GPIO->PIN_CNF[pin] |=
                            (GPIO_PIN_CNF_SENSE_High << GPIO_PIN_CNF_SENSE_Pos);
                    }
                    else
                    {
                        NRF_GPIO->PIN_CNF[pin] |=
                            (GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos);
                    }
                }
            }
            // Clear the line
            NRF_GPIO->LATCH = 1 << pin;
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
