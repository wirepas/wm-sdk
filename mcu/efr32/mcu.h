#ifndef MCU_H_
#define MCU_H_

#include "vendor/em_bitband.h"
#include "vendor/em_device.h"
#include "vendor/em_system.h"
#ifdef EFR32FG12
#include "vendor/efr32fg12/efr32_gpio.h"
#elif defined EFR32FG13
#include "vendor/efr32fg13/efr32_gpio.h"
#else
#error("EFR32FG12 or EFR32FG13 must be defined")
#endif


#define MAX_IRQ_NUMBER TRNG0_IRQn

#endif /* MCU_H_ */
