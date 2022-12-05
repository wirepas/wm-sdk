/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    button.c
 * \brief   Board-independent button functions.
 */

#include "button.h"
#include "gpio.h"
#include "board.h"
#include "api.h"


#ifdef BOARD_BUTTON_ID_LIST

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

/** \brief Button id to GPIO id map (array index: button id ; array value: GPIO id) */
static const uint8_t m_id_map[] = BOARD_BUTTON_ID_LIST;

/** \brief  Compute number of button on the board */
#define BOARD_BUTTON_NUMBER         (sizeof(m_id_map) / sizeof(m_id_map[0]))

typedef struct
{
    // Callback when button pressed
    on_button_event_cb on_pressed;
    // Callback when button released
    on_button_event_cb on_released;
    // Used for debounce
    app_lib_time_timestamp_hp_t last_button_event;
} button_internal_t;

/** \brief Button id to button conf map (array index: button id ; array value: button conf) */
static button_internal_t m_button_conf[BOARD_BUTTON_NUMBER];

/** \brief Has the button library been initialized */
static bool m_initialized = false;

static void button_event_handle(gpio_id_t gpio_id, gpio_in_event_e gpio_event);
static bool get_button_id_from_gpio_id(gpio_id_t gpio_id, uint8_t *button_id);

static inline gpio_in_cfg_t get_button_gpio_cfg(void)
{
    gpio_in_cfg_t gpio_in_cfg;

    gpio_in_cfg.event_cb = NULL;
    gpio_in_cfg.event_cfg = GPIO_IN_EVENT_NONE;
#if BOARD_BUTTON_INTERNAL_PULL
#if BOARD_BUTTON_ACTIVE_LOW
    gpio_in_cfg.in_mode_cfg = GPIO_IN_PULL_UP;
#else
    gpio_in_cfg.in_mode_cfg = GPIO_IN_PULL_DOWN;
#endif // BOARD_BUTTON_ACTIVE_LOW
#else
    gpio_in_cfg.in_mode_cfg = GPIO_IN_PULL_NONE;
#endif // BOARD_BUTTON_INTERNAL_PULL

    return gpio_in_cfg;
}

void Button_init(void)
{
    uint8_t button_id;
    app_lib_time_timestamp_hp_t now = lib_time->getTimestampHp();
    button_internal_t button_conf =
    {
        .on_pressed = NULL,
        .on_released = NULL,
        .last_button_event = now
    };
    gpio_id_t gpio_id;
    gpio_in_cfg_t gpio_in_cfg;

    if (m_initialized)
    {
        /* return if button initialization has already been performed */
        return;
    }

    gpio_in_cfg = get_button_gpio_cfg();

    for (button_id = 0; button_id < BOARD_BUTTON_NUMBER; button_id++)
    {
        gpio_id = m_id_map[button_id];
        Gpio_inputSetCfg(gpio_id, &gpio_in_cfg);
        m_button_conf[button_id] = button_conf;
    }

    m_initialized = true;
}

button_res_e Button_getState(uint8_t button_id, bool * state_p)
{
    gpio_level_e gpio_level = GPIO_LEVEL_LOW;
    gpio_id_t gpio_id;

    if (!m_initialized)
    {
        return BUTTON_RES_UNINITIALIZED;
    }
    if (button_id >= BOARD_BUTTON_NUMBER)
    {
        return BUTTON_RES_INVALID_ID;
    }

    gpio_id = m_id_map[button_id];
    Gpio_inputRead(gpio_id, &gpio_level);

   /*
    *  level  | active_low | state |
    *  clear  |      0     |   0   |
    *  set    |      0     |   1   |
    *  clear  |      1     |   1   |
    *  set    |      1     |   0   |
    */
    *state_p = ((gpio_level != GPIO_LEVEL_LOW) != BOARD_BUTTON_ACTIVE_LOW);

    return BUTTON_RES_OK;
}

button_res_e Button_register_for_event(uint8_t button_id,
                                       button_event_e event,
                                       on_button_event_cb cb)
{
    gpio_id_t gpio_id;
    gpio_in_cfg_t gpio_in_cfg;

    if (!m_initialized)
    {
        return BUTTON_RES_UNINITIALIZED;
    }
    if (button_id >= BOARD_BUTTON_NUMBER)
    {
        return BUTTON_RES_INVALID_ID;
    }
    if ((event != BUTTON_PRESSED && event != BUTTON_RELEASED) ||
       (cb == NULL))
    {
        return BUTTON_RES_INVALID_PARAM;
    }

    gpio_in_cfg = get_button_gpio_cfg();

    Sys_enterCriticalSection();

    if (event == BUTTON_PRESSED)
    {
        m_button_conf[button_id].on_pressed = cb;
    }
    else
    {
        m_button_conf[button_id].on_released = cb;
    }

    /* if on both press and release */
    if (m_button_conf[button_id].on_pressed &&
        m_button_conf[button_id].on_released)
    {
        gpio_in_cfg.event_cfg = GPIO_IN_EVENT_RISING_EDGE | GPIO_IN_EVENT_FALLING_EDGE;
    }
    /* else if on press only or on release only */
    else
    {
       /*
        *  button_event  | active_low |    gpio_event    |
        *  on_pressed    |      0     |  on_rising_edge  |
        *  on_released   |      0     |  on_falling_edge |
        *  on_pressed    |      1     |  on_falling_edge |
        *  on_released   |      1     |  on_rising_edge  |
        */
        gpio_in_cfg.event_cfg = ((m_button_conf[button_id].on_pressed != NULL) != BOARD_BUTTON_ACTIVE_LOW) ?
                                GPIO_IN_EVENT_RISING_EDGE : GPIO_IN_EVENT_FALLING_EDGE;
    }
    gpio_in_cfg.event_cb = button_event_handle;

    gpio_id = m_id_map[button_id];
    Gpio_inputSetCfg(gpio_id, &gpio_in_cfg);

    Sys_exitCriticalSection();

    return BUTTON_RES_OK;
}

uint8_t Button_get_number(void)
{
    return BOARD_BUTTON_NUMBER;
}

static void button_event_handle(gpio_id_t gpio_id, gpio_in_event_e gpio_event)
{
    uint8_t button_id;
    bool button_id_found;
    app_lib_time_timestamp_hp_t now = lib_time->getTimestampHp();

    if (gpio_id >= Gpio_getNumber())
    {
        return;
    }

    /* get button id from gpio id */
    button_id_found = get_button_id_from_gpio_id(gpio_id, &button_id);
    if (button_id_found == false)
    {
        return;
    }

    if (lib_time->getTimeDiffUs(now, m_button_conf[button_id].last_button_event)
        > (BOARD_DEBOUNCE_TIME_MS * 1000))
    {
        m_button_conf[button_id].last_button_event = now;

        if ((m_button_conf[button_id].on_pressed) &&
           (((gpio_event == GPIO_IN_EVENT_RISING_EDGE) && !BOARD_BUTTON_ACTIVE_LOW) ||
           ((gpio_event == GPIO_IN_EVENT_FALLING_EDGE) && BOARD_BUTTON_ACTIVE_LOW)))
        {
            m_button_conf[button_id].on_pressed(button_id, BUTTON_PRESSED);
        }
        else if ((m_button_conf[button_id].on_released) &&
                (((gpio_event == GPIO_IN_EVENT_FALLING_EDGE) && !BOARD_BUTTON_ACTIVE_LOW) ||
                ((gpio_event == GPIO_IN_EVENT_RISING_EDGE) && BOARD_BUTTON_ACTIVE_LOW)))
        {
            m_button_conf[button_id].on_released(button_id, BUTTON_RELEASED);
        }
    }
}

static bool get_button_id_from_gpio_id(gpio_id_t gpio_id, uint8_t *button_id)
{
    uint8_t button_id_tmp;

    if (button_id == NULL)
    {
        return false;
    }

    /* browse the button IDs and find the one that is mapped to the given GPIO id */
    for (button_id_tmp = 0; button_id_tmp < BOARD_BUTTON_NUMBER; button_id_tmp++)
    {
        if (m_id_map[button_id_tmp] == gpio_id)
        {
            *button_id = button_id_tmp;
            return true;
        }
    }
    return false;
}

#else // BOARD_BUTTON_ID_LIST
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

#endif // BOARD_BUTTON_ID_LIST
