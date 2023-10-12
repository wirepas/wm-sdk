#ifndef MCU_H_
#define MCU_H_

#define NRF_STATIC_INLINE __STATIC_INLINE

#if MCU_SUB == 832
#define NRF52832_XXAA
//#include "nrf52832_peripherals.h"
#elif MCU_SUB == 840
#undef NRF52
#define NRF52840_XXAA
//#include "nrf52840_peripherals.h"
#elif MCU_SUB == 833
#undef NRF52
#define NRF52833_XXAA
//#include "nrf52833_peripherals.h"
#elif MCU_SUB == 60
#define NRF9160_XXAA
/** Define NRF_GPIO and GPIOTE_IRQn for nrf9160. */
#define NRF_GPIO        NRF_P0
#define GPIOTE_IRQn     GPIOTE0_IRQn
#define UART0_IRQn      UARTE0_SPIM0_SPIS0_TWIM0_TWIS0_IRQn
/** Define the last IRQ number in the interrupt vector */
#elif MCU_SUB == 61
#define NRF9120_XXAA
/** Define NRF_GPIO and GPIOTE_IRQn for nrf9160. */
#define NRF_GPIO        NRF_P0
#define GPIOTE_IRQn     GPIOTE0_IRQn
#define UART0_IRQn      UARTE0_SPIM0_SPIS0_TWIM0_TWIS0_IRQn
/** Define the last IRQ number in the interrupt vector */
#else
#error SUB_MCU Must be 832 or 840 or 60 or 61
#endif

#include "nrfx.h"
#include "mdk/nrf.h"
#include "hal/nrf_gpio.h"
#include "hal/nrf_gpiote.h"
#include "hal/nrf_uarte.h"
#include "hal/nrf_timer.h"

extern __IO uint32_t EVENT_READBACK;

#endif /* MCU_H_ */
