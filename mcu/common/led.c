/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    led.c
 * \brief   Board-independent led functions.
 */

#include "led.h"
#include "gpio.h"
#include "board.h"

#ifdef BOARD_LED_ID_LIST

/*
 * The selected board has LEDs
 */

#ifndef BOARD_LED_ACTIVE_LOW
/** \brief  Are LEDs active low. It can be overwritten from board.h */
#define BOARD_LED_ACTIVE_LOW    true
#endif

/** \brief Button id to GPIO id map (array index: button id ; array value: GPIO id) */
static const uint8_t m_id_map[] = BOARD_LED_ID_LIST;

/** \brief  Compute number of leds on the board */
#define NUMBER_OF_LEDS  (sizeof(m_id_map) / sizeof(m_id_map[0]))

/** \brief Has the LED library been initialized */
static bool m_initialized = false;

void Led_init(void)
{
    gpio_id_t gpio_id;
    uint8_t led_id;
    gpio_out_cfg_t gpio_out_cfg = {
        .out_mode_cfg = GPIO_OUT_MODE_PUSH_PULL,
        .level_default = BOARD_LED_ACTIVE_LOW ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW
    };

    if (m_initialized)
    {
        /* return if Led initialization has already been performed */
        return;
    }

    for (led_id = 0; led_id < NUMBER_OF_LEDS; led_id++)
    {
        gpio_id = m_id_map[led_id];
        Gpio_outputSetCfg(gpio_id, &gpio_out_cfg);
    }

    m_initialized = true;
}

led_res_e Led_set(uint8_t led_id, bool state)
{
    gpio_id_t gpio_id;
    gpio_level_e gpio_level;

    if (!m_initialized)
    {
        return LED_RES_UNINITIALIZED;
    }
    if (led_id >= NUMBER_OF_LEDS)
    {
        return LED_RES_INVALID_ID;
    }

    /*
     *  state  | active_low |   level  |
     *    0    |      0     |   clear  |
     *    1    |      0     |   set    |
     *    0    |      1     |   set    |
     *    1    |      1     |   clear  |
     */
    gpio_level = (state != BOARD_LED_ACTIVE_LOW) ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW;

    gpio_id = m_id_map[led_id];
    Gpio_outputWrite(gpio_id, gpio_level);

    return LED_RES_OK;
}

bool Led_get(uint8_t led_id)
{
    gpio_level_e level;
    gpio_id_t gpio_id;
    bool state;

    if (!m_initialized)
    {
        return LED_RES_UNINITIALIZED;
    }
    if (led_id >= NUMBER_OF_LEDS)
    {
        return LED_RES_INVALID_ID;
    }

    gpio_id = m_id_map[led_id];
    Gpio_outputRead(gpio_id, &level);

    /*
    *  level    | active_low | state |
    *  clear    |      0     |   0   |
    *  set      |      0     |   1   |
    *  clear    |      1     |   1   |
    *  set      |      1     |   0   |
    */
    state = (level != GPIO_LEVEL_LOW) != BOARD_LED_ACTIVE_LOW;

    return state;
}

led_res_e Led_toggle(uint8_t led_id)
{
    gpio_id_t gpio_id;

    if (!m_initialized)
    {
        return LED_RES_UNINITIALIZED;
    }
    if (led_id >= NUMBER_OF_LEDS)
    {
        return LED_RES_INVALID_ID;
    }

    gpio_id = m_id_map[led_id];
    Gpio_outputToggle(gpio_id);

    return LED_RES_OK;
}

uint8_t Led_getNumber(void)
{
    return NUMBER_OF_LEDS;
}

#else // BOARD_LED_ID_LIST

/*
 * The selected board has no LEDs
 *
 * As some example apps support such boards but also provide extra status
 * information when a board has LEDs, the LED driver has this dummy
 * implementation to simplify the build process.
 */

void Led_init(void)
{
    // Do nothing
}

led_res_e Led_set(uint8_t led_id, bool state)
{
    (void) led_id;
    (void) state;

    // Invalid LED number
    return LED_RES_INVALID_ID;
}

led_res_e Led_toggle(uint8_t led_id)
{
    (void) led_id;

    // Invalid LED number
    return LED_RES_INVALID_ID;
}

uint8_t Led_getNumber(void)
{
    return 0;
}

#endif // BOARD_LED_ID_LIST
