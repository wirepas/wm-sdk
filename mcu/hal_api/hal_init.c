/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include "hal_init.h"

// HAL drivers are considered in use if their associated interface (.h)
// file is in the include list. It could be done with a dedicated C flag
// but it would have the same effect

#if __has_include("gpio.h")
#include "gpio.h"
#endif

#if __has_include("button.h")
#include "button.h"
#endif

#if __has_include("led.h")
#include "led.h"
#endif

void Hal_init(void)
{
#if __has_include("gpio.h")
    Gpio_init();
#endif

#if __has_include("button.h")
    Button_init();
#endif

#if __has_include("led.h")
    Led_init();
#endif
}
