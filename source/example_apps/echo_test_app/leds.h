#ifndef LED_H
#define LED_H

#include <stdint.h>

#ifdef NRF52840

#define LED_1_PIN   13
#define LED_2_PIN   14
#define LED_3_PIN   15
#define LED_4_PIN   16

#else

#define LED_1_PIN   17
#define LED_2_PIN   18
#define LED_3_PIN   19
#define LED_4_PIN   20

#endif // TARGET_BOARD

void led_off(uint32_t pin);
void led_toggle(uint32_t pin);
void led_on(uint32_t pin);
void configure_leds(void);

#endif // LED_H