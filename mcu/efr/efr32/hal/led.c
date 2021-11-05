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

#ifdef BOARD_LED_PIN_LIST

/*
 * The selected board has LEDs
 */

#ifndef BOARD_LED_ACTIVE_LOW
/** \brief  Are LEDs active low. It can be overwritten from board.h */
#define BOARD_LED_ACTIVE_LOW    false
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
    if (led.pin > 15 || led.port > GPIO_PORT_MAX)
    {
        return;
    }

#if defined(_SILICON_LABS_32B_SERIES_1)
    /* Enable clocks */
    CMU->HFBUSCLKEN0 |= CMU_HFBUSCLKEN0_GPIO;
#elif !defined (EFR32MG21)
    CMU->CLKEN0_SET = CMU_CLKEN0_GPIO;
#endif

    /* Set pin mode */
    hal_gpio_set_mode(led.port,
                      led.pin,
                      GPIO_MODE_OUT_PP);

    /* Off by default */
#if BOARD_LED_ACTIVE_LOW
    hal_gpio_set(led.port, led.pin);
#else // BOARD_LED_ACTIVE_LOW
    hal_gpio_clear(led.port, led.pin);
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
    if (led_id >= NUMBER_OF_LEDS)
    {
        return LED_RES_INVALID_ID;
    }
    led_gpio_t led = pin_map[led_id];
    if (led.pin > 15 || led.port > GPIO_PORT_MAX)
    {
        return LED_RES_INVALID_ID;
    }

#if BOARD_LED_ACTIVE_LOW   
    return (hal_gpio_get(led.port, led.pin) == 0);
#else //BOARD_LED_ACTIVE_HIGH 
    return (hal_gpio_get(led.port, led.pin) != 0);
#endif // BOARD_LED_ACTIVE_LOW    
}

led_res_e Led_set(uint8_t led_id, bool state)
{
    if (led_id >= NUMBER_OF_LEDS)
    {
        return LED_RES_INVALID_ID;
    }

    led_gpio_t led = pin_map[led_id];
    if (led.pin > 15 || led.port > GPIO_PORT_MAX)
    {
        return LED_RES_INVALID_ID;
    }

#if BOARD_LED_ACTIVE_LOW
    if (state)
    {
        hal_gpio_clear(led.port, led.pin);
    }
    else
    {
        hal_gpio_set(led.port, led.pin);
    }
#else // BOARD_LED_ACTIVE_LOW
    if (state)
    {
        hal_gpio_set(led.port, led.pin);
    }
    else
    {
        hal_gpio_clear(led.port, led.pin);
    }
#endif // BOARD_LED_ACTIVE_LOW

    return LED_RES_OK;
}

led_res_e Led_toggle(uint8_t led_id)
{
    if (led_id >= NUMBER_OF_LEDS)
    {
        return LED_RES_INVALID_ID;
    }

    led_gpio_t led = pin_map[led_id];

    if (led.pin > 15 || led.port > GPIO_PORT_MAX)
    {
        return LED_RES_INVALID_ID;
    }

    hal_gpio_toggle(led.port, led.pin);

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
