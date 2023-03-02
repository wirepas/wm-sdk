#ifndef MCU_H_
#define MCU_H_

#include "em_bus.h"
#include "em_device.h"
#include "em_system.h"
#include "em_emu.h"
#ifdef EFR32FG12
#include "efr32fg12/efr32_gpio.h"
#elif defined EFR32FG13
#include "efr32fg13/efr32_gpio.h"
#elif defined EFR32MG21
#include "efr32mg21/Include/efr32mg21_gpio.h"
#include "efr32mg21/efr32_gpio.h"
#elif defined EFR32MG22
#include "efr32mg22/Include/efr32mg22_gpio.h"
#include "efr32mg22/efr32_gpio.h"
#elif defined EFR32FG23
#include "efr32fg23/Include/efr32fg23_gpio.h"
#include "efr32fg23/efr32_gpio.h"
#elif defined EFR32MG12

#else
#error("EFR32FG12, EFR32MG12, EFR32FG13, EFR32MG21, EFR32MG22 or EFR32FG23 must be defined")
#endif

#endif /* MCU_H_ */
