/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    led.c
 * \brief   Board-specific LED functions for nrf52
 */

#include "led.h"
#include "mcu.h"
#include "board.h"

#ifdef BOARD_LED_PIN_LIST

/*
 * The selected board has LEDs
 */

#ifndef BOARD_LED_ACTIVE_LOW
/** \brief  Are LEDs active low. It can be overwritten from board.h */
#define BOARD_LED_ACTIVE_LOW    true
#endif

/** \brief  Board-dependent LED number to pin mapping */
static const uint8_t pin_map[] = BOARD_LED_PIN_LIST;

/** \brief  Compute number of leds on the board */
#define NUMBER_OF_LEDS  (sizeof(pin_map) / sizeof(pin_map[0]))

static void led_configure(uint_fast8_t led_num)
{
    uint_fast8_t pin_num = pin_map[led_num];

    nrf_gpio_cfg_output(pin_num);

    // Off by default
#if BOARD_LED_ACTIVE_LOW
    nrf_gpio_pin_set(pin_num);
#else // BOARD_LED_ACTIVE_LOW
    nrf_gpio_pin_clear(pin_num);
#endif // BOARD_LED_ACTIVE_LOW
}

void Led_init(void)
{
    for (uint8_t i = 0; i < NUMBER_OF_LEDS; i++)
    {
        // Set up LED GPIO
        led_configure(i);
    }
}

bool Led_get(uint8_t led_id)
{
    if (led_id >= (sizeof(pin_map) / sizeof(pin_map[0])))
    {
        return LED_RES_INVALID_ID;
    }
    uint_fast8_t pin_num = pin_map[led_id];
#if BOARD_LED_ACTIVE_LOW
    return (nrf_gpio_pin_out_read(pin_num) == 0);
#else //BOARD_LED_ACTIVE_HIGH
    return (nrf_gpio_pin_out_read(pin_num) != 0);
#endif // BOARD_LED_ACTIVE_LOW
}

led_res_e Led_set(uint8_t led_id, bool state)
{
    if (led_id >= (sizeof(pin_map) / sizeof(pin_map[0])))
    {
        return LED_RES_INVALID_ID;
    }

    uint_fast8_t pin_num = pin_map[led_id];

#if BOARD_LED_ACTIVE_LOW
    if (state)
    {
        nrf_gpio_pin_clear(pin_num);
    }
    else
    {
        nrf_gpio_pin_set(pin_num);
    }
#else // BOARD_LED_ACTIVE_LOW
    if (state)
    {
        nrf_gpio_pin_set(pin_num);
    }
    else
    {
        nrf_gpio_pin_clear(pin_num);
    }
#endif // BOARD_LED_ACTIVE_LOW

    return LED_RES_OK;
}

led_res_e Led_toggle(uint8_t led_id)
{
    if (led_id >= (sizeof(pin_map) / sizeof(pin_map[0])))
    {
        return LED_RES_INVALID_ID;
    }

    uint_fast8_t pin_num = pin_map[led_id];

    nrf_gpio_pin_toggle(pin_num);

    return LED_RES_OK;
}

uint8_t Led_getNumber(void)
{
    return NUMBER_OF_LEDS;
}

#else // BOARD_LED_PIN_LIST

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

#endif // BOARD_LED_PIN_LIST
