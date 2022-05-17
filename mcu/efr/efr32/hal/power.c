/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include "power.h"
#include "mcu.h"
#include "board.h"

/** Since SDK v1.2 (stack v5.1.x) this option has been move to
 *  board/<board_name>/config.mk. Set board_hw_dcdc to yes to enable DCDC.
 *  Warning: the logic is inversed compared to #define MCU_NO_DCDC.
 */
#ifdef MCU_NO_DCDC
#error This option has been moved to board/<board_name>/config.mk
#endif

/** Silabs reference radio boards use DCDC for MCU. Thus DCDC ON for MCU is
 *  treated as default option. DCDC voltage is 1V8.
 *
 *  Silabs reference radio boards use either DCDC (1V8) or VBAT (3V3) for
 *  radio PA. Most boards have configuration resistors for the PA voltage
 *  selection. The default configuration is board dependent:
 *      +10dBm boards have DCDC selected for radio PA
 *      +19dBm boards have VBAT selected for radio PA
 *  (Profiles over ~13.5dBm TX power need PA voltage 3V3, DCDC is not enough).
 */

#if defined(_SILICON_LABS_32B_SERIES_1)
#define EMU_DCDCINIT                                                                        \
{                                                                                           \
  emuPowerConfig_DcdcToDvdd,     /* DCDC to DVDD */                                         \
  emuDcdcMode_LowNoise,          /* Low-noise mode in EM0 */                                \
  1800,                          /* Nominal output voltage for DVDD mode, 1.8V  */          \
  15,                            /* Nominal EM0/1 load current of less than 15mA */         \
  10,                            /* Nominal EM2/3/4 load current less than 10uA  */         \
  200,                           /* Maximum average current of 200mA
                                 (assume strong battery or other power source) */           \
  emuDcdcAnaPeripheralPower_DCDC,/* Select DCDC as analog power supply (lower power) */     \
  160,                           /* Maximum reverse current of 160mA */                     \
  emuDcdcLnCompCtrl_4u7F,        /* 4.7uF DCDC capacitor */                                 \
}
#elif defined(_SILICON_LABS_32B_SERIES_2_CONFIG_3)
#define EMU_DCDCINIT                                                         \
{                                                                            \
    emuDcdcMode_Regulation,        /**< DCDC regulator on. */                \
    emuVreginCmpThreshold_2v3,     /**< 2.3V VREGIN comparator threshold. */ \
    emuDcdcTonMaxTimeout_1P19us,   /**< Ton max is 1.19us. */                \
    emuDcdcDriveSpeed_Default,     /**< Default efficiency in EM0/1. */      \
    emuDcdcDriveSpeed_Default,     /**< Default efficiency in EM2/3. */      \
    emuDcdcPeakCurrent_Load36mA,   /**< Default peak current in EM0/1. */    \
    emuDcdcPeakCurrent_Load36mA    /**< Default peak current in EM2/3. */    \
}
#else //other  _SILICON_LABS_32B_SERIES_2 device
#define EMU_DCDCINIT                                                        \
{                                                                           \
    emuDcdcMode_Regulation,        /**< DCDC regulator on. */               \
    emuVreginCmpThreshold_2v3,     /**< 2.3V VREGIN comparator treshold. */ \
    emuDcdcTonMaxTimeout_1P19us,   /**< Ton max is 1.19us. */               \
    true,                          /**< Enable DCM only mode. */            \
    emuDcdcDriveSpeed_Default,     /**< Default efficiency in EM0/1. */     \
    emuDcdcDriveSpeed_Default,     /**< Default efficiency in EM2/3. */     \
    emuDcdcPeakCurrent_Load60mA,   /**< Default peak current in EM0/1. */   \
    emuDcdcPeakCurrent_Load36mA    /**< Default peak current in EM2/3. */   \
}
#endif

void Power_enableDCDC()
{
#if BOARD_HW_DCDC
    EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT;
    EMU_DCDCInit(&dcdcInit);
#else
    // No DCDC. DVDD must be externally wired or you are doomed!
#if !defined(EFR32MG21)
    EMU_DCDCPowerOff();
#endif
#endif
}
