#ifndef MCU_H_
#define MCU_H_

#define NRF_STATIC_INLINE __STATIC_INLINE
#if MCU_SUB == 832
#define NRF52832_XXAA
#include "vendor/nrf52832_peripherals.h"
#define MAX_IRQ_NUMBER  I2S_IRQn
#elif MCU_SUB == 840
#undef NRF52
#define NRF52840_XXAA
#include "vendor/nrf52840_peripherals.h"
#define MAX_IRQ_NUMBER  SPIM3_IRQn
#elif MCU_SUB == 833
#undef NRF52
#define NRF52833_XXAA
#include "vendor/nrf52833_peripherals.h"
#define MAX_IRQ_NUMBER  SPIM3_IRQn
#else
#error SUB_MCU Must be 832 or 840
#endif

#include "vendor/nrf.h"
#include "vendor/nrf_gpio.h"

#endif /* MCU_H_ */
