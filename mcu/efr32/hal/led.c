/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    led.c
 * \brief   Board-specific LED functions for efr32
 */

#include "led.h"
#include "mcu.h"
#include "board.h"

#ifndef BOARD_LED_PIN_LIST
// Must be defined from board.h
#error "Please define BOARD_LED_PIN_LIST from your board.h"
#endif

typedef struct
{
    uint8_t port;
    uint8_t pin;
} led_gpio_t;

/** \brief  Board-dependent LED number to pin mapping */
static const led_gpio_t pin_map[] = BOARD_LED_PIN_LIST;

/** \brief  Compute number of leds on the board */
#define NUMBER_OF_LEDS  (sizeof(pin_map) / sizeof(pin_map[0]))

static void led_configure(uint8_t led_id)
{
    led_gpio_t led = pin_map[led_id];
    if (led.pin > 15 || led.port > GPIOF)
    {
        return;
    }

    /* Enable clocks */
    CMU->HFBUSCLKEN0 |= CMU_HFBUSCLKEN0_GPIO;

    /* Set pin mode */
    hal_gpio_set_mode(led.port,
                      led.pin,
                      GPIO_MODE_OUT_PP);

    /* Off by default */
    hal_gpio_clear(led.port, led.pin);
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
    if (led_id >= NUMBER_OF_LEDS)
    {
        return LED_RES_INVALID_ID;
    }

    led_gpio_t led = pin_map[led_id];
    if (led.pin > 15 || led.port > GPIOF)
    {
        return LED_RES_INVALID_ID;
    }

    if (state)
    {
        hal_gpio_clear(led.port, led.pin);
    }
    else
    {
        hal_gpio_set(led.port, led.pin);
    }

    return LED_RES_OK;
}

led_res_e Led_toggle(uint8_t led_id)
{
    if (led_id >= NUMBER_OF_LEDS)
    {
        return LED_RES_INVALID_ID;
    }

    led_gpio_t led = pin_map[led_id];
    if (led.pin > 15 || led.port > GPIOF)
    {
        return LED_RES_INVALID_ID;
    }

    hal_gpio_toggle(led.port, led.pin);

    return LED_RES_OK;
}

uint8_t Led_getNumber()
{
    return NUMBER_OF_LEDS;
}
