/**************************************************************************//**
 * @file em_device.h
 * @brief CMSIS Cortex-M Peripheral Access Layer for Silicon Laboratories
 *        microcontroller devices
 *
 * This is a convenience header file for defining the part number on the
 * build command line, instead of specifying the part specific header file.
 *
 * @verbatim
 * Example: Add "-DEFM32G890F128" to your build options, to define part
 *          Add "#include "em_device.h" to your source files
 *
 *
 * @endverbatim
 * @version 5.1.3
 ******************************************************************************
 * @section License
 * <b>Copyright 2017 Silicon Laboratories, Inc. http://www.silabs.com</b>
 ******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.@n
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.@n
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Laboratories, Inc.
 * has no obligation to support this Software. Silicon Laboratories, Inc. is
 * providing the Software "AS IS", with no express or implied warranties of any
 * kind, including, but not limited to, any implied warranties of
 * merchantability or fitness for any particular purpose or warranties against
 * infringement of any proprietary rights of a third party.
 *
 * Silicon Laboratories, Inc. will not be liable for any consequential,
 * incidental, or special damages, or any other relief, or for any claim by
 * any third party, arising from your use of this Software.
 *
 *****************************************************************************/

#ifndef EM_DEVICE_FG12_H
#define EM_DEVICE_FG12_H

#if defined(EFR32FG12P231F1024GL125)
#include "efr32fg12p231f1024gl125.h"

#elif defined(EFR32FG12P231F1024GM48)
#include "efr32fg12p231f1024gm48.h"

#elif defined(EFR32FG12P232F1024GL125)
#include "efr32fg12p232f1024gl125.h"

#elif defined(EFR32FG12P232F1024GM48)
#include "efr32fg12p232f1024gm48.h"

#elif defined(EFR32FG12P431F1024GL125)
#include "efr32fg12p431f1024gl125.h"

#elif defined(EFR32FG12P431F1024GM48)
#include "efr32fg12p431f1024gm48.h"

#elif defined(EFR32FG12P432F1024GL125)
#include "efr32fg12p432f1024gl125.h"

#elif defined(EFR32FG12P432F1024GM48)
#include "efr32fg12p432f1024gm48.h"

#elif defined(EFR32FG12P433F1024GL125)
#include "efr32fg12p433f1024gl125.h"

#elif defined(EFR32FG12P433F1024GM48)
#include "efr32fg12p433f1024gm48.h"

#else
#error "em_device.h: PART NUMBER undefined"
#endif
#endif /* EM_DEVICE_FG12_H */
