/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include "early_init.h"
#include "mcu.h"

/** Note: This file is compiled and linked to bootloader */

/** Silabs reference radio boards use DCDC for MCU. Thus DCDC ON for MCU is
 *  treated as default option. DCDC voltage is 1V8.
 *
 *  Silabs reference radio boards use either DCDC (1V8) or VBAT (3V3) for
 *  radio PA. Most boards have configuration resistors for the PA voltage
 *  selection. The default configuration is board dependent:
 *      +10dBm boards have DCDC selected for radio PA
 *      +19dBm boards have VBAT selected for radio PA
 *  (Profiles over ~13.5dBm TX power need PA voltage 3V3, DCDC is not enough).
 *
 *  Define MCU_NO_DCDC if using hardware without DCDC support. Can be defined
 *  locally here or via bootloader build (CFLAGS += -DMCU_NO_DCDC).
 */
//#define MCU_NO_DCDC

/** Implementation of early_init function for EFR32, for DCDC mode select.
 *
 *  Firmware does the actual setting. Information to firmware is passed by
 *  using (otherwise unused) register EMU->EM4CTRL.EM4IORETMODE
 *  This is possible because EM-state EM4 will never be used.
 *  EM4IORETMODE    0   DCDC in use (required components exist).
 *                      Radio PA voltage determined by actual profile.
 *                  1   NO-DCDC (DVDD externally connected to VBAT).
 *                      Radio PA voltage assumed to be VBAT (3V3).
 *                  2   (reserved for future use)
 */
void early_init(void)
{
    volatile uint32_t x;
    x = EMU->EM4CTRL;
#ifdef MCU_NO_DCDC
    x |= (1 << _EMU_EM4CTRL_EM4IORETMODE_SHIFT); // 1
#else
    x &= ~(_EMU_EM4CTRL_EM4IORETMODE_MASK);      // 0
#endif
    EMU->EM4CTRL = x;
}
