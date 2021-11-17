#include "hal_api.h"
#include "leds.h"

void led_on(uint32_t pin)
{
    nrf_gpio_pin_clear(pin);
}

void led_off(uint32_t pin)
{
    nrf_gpio_pin_set(pin);
}

void led_toggle(uint32_t pin)
{
    nrf_gpio_pin_toggle(pin);
}

void configure_leds()
{

    nrf_gpio_pin_set(LED_1_PIN);
    nrf_gpio_pin_set(LED_2_PIN);
    nrf_gpio_pin_set(LED_3_PIN);
    nrf_gpio_pin_set(LED_4_PIN);

    nrf_gpio_cfg(LED_1_PIN,
                 NRF_GPIO_PIN_DIR_OUTPUT,
                 NRF_GPIO_PIN_INPUT_CONNECT,
                 NRF_GPIO_PIN_NOPULL,
                 NRF_GPIO_PIN_S0S1,
                 NRF_GPIO_PIN_NOSENSE);

    nrf_gpio_cfg(LED_2_PIN,
                 NRF_GPIO_PIN_DIR_OUTPUT,
                 NRF_GPIO_PIN_INPUT_CONNECT,
                 NRF_GPIO_PIN_NOPULL,
                 NRF_GPIO_PIN_S0S1,
                 NRF_GPIO_PIN_NOSENSE);

    nrf_gpio_cfg(LED_3_PIN,
                 NRF_GPIO_PIN_DIR_OUTPUT,
                 NRF_GPIO_PIN_INPUT_CONNECT,
                 NRF_GPIO_PIN_NOPULL,
                 NRF_GPIO_PIN_S0S1,
                 NRF_GPIO_PIN_NOSENSE);

    nrf_gpio_cfg(LED_4_PIN,
                 NRF_GPIO_PIN_DIR_OUTPUT,
                 NRF_GPIO_PIN_INPUT_CONNECT,
                 NRF_GPIO_PIN_NOPULL,
                 NRF_GPIO_PIN_S0S1,
                 NRF_GPIO_PIN_NOSENSE);   

}