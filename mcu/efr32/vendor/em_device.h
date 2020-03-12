/*
 * em_device.h
 *
 *  Chip variant include file selection.
 */

#ifndef EM_DEVICE_H_
#define EM_DEVICE_H_

#if defined(EFR32FG13)
#include "efr32fg13/em_device_fg13.h"
#define GPIO_PORT_MAX 5
#elif defined(EFR32FG12)
#include "efr32fg12/em_device_fg12.h"
#define GPIO_PORT_MAX 10
#else
#error "em_device.h: Unknown EFR32 PART"
#endif

#ifndef __SYSTEM_CLOCK
#define __SYSTEM_CLOCK                 (38400000UL)
#endif

#endif /* EM_DEVICE_H_ */
