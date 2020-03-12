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

#ifndef BOARD_LED_PIN_LIST
// Must be defined from board.h
#error "Please define BOARD_LED_PIN_LIST from your board.h"
#endif

/** \brief  Board-dependent LED number to pin mapping */
static const uint8_t pin_map[] = BOARD_LED_PIN_LIST;

/** \brief  Compute number of leds on the board */
#define NUMBER_OF_LEDS  (sizeof(pin_map) / sizeof(pin_map[0]))

static void led_configure(uint_fast8_t led_num)
{
    uint_fast8_t pin_num = pin_map[led_num];

    nrf_gpio_cfg_output(pin_num);
    nrf_gpio_pin_set(pin_num);  // Off by default
}

void Led_init()
{
    for (uint8_t i = 0; i < NUMBER_OF_LEDS; i++)
    {
        // Set up LED GPIO
        led_configure(i);
    }
}

led_res_e Led_set(uint8_t led_id, bool state)
{
    if (led_id >= (sizeof(pin_map) / sizeof(pin_map[0])))
    {
        return LED_RES_INVALID_ID;
    }

    uint_fast8_t pin_num = pin_map[led_id];

    if (state)
    {
        nrf_gpio_pin_clear(pin_num);
    }
    else
    {
        nrf_gpio_pin_set(pin_num);
    }

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

uint8_t Led_getNumber()
{
    return NUMBER_OF_LEDS;
}
