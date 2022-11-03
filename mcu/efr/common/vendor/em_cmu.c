/***************************************************************************//**
 * @file
 * @brief Clock management unit (CMU) Peripheral API
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#include "em_cmu.h"
#if defined(CMU_PRESENT)

#include <stddef.h>
#include <limits.h>
#include "em_assert.h"
#include "em_bus.h"
#include "em_cmu.h"
#include "em_common.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_system.h"

/***************************************************************************//**
 * @addtogroup cmu CMU - Clock Management Unit
 * @brief Clock management unit (CMU) Peripheral API
 * @details
 *  This module contains functions for the CMU peripheral of Silicon Labs 32-bit
 *  MCUs and SoCs. The CMU module controls oscillators, clocks gates, clock
 *  multiplexers, pre-scalers, calibration modules and wait-states.
 * @{
 ******************************************************************************/
#if defined(_SILICON_LABS_32B_SERIES_2)

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/*******************************************************************************
 ******************************   DEFINES   ************************************
 ******************************************************************************/

#if defined(_SILICON_LABS_32B_SERIES_2_CONFIG_1)
// Maximum allowed core frequency vs. wait-states on flash accesses.
#define CMU_MAX_FLASHREAD_FREQ_0WS        39000000UL
#define CMU_MAX_FLASHREAD_FREQ_1WS        80000000UL

// Maximum allowed core frequency vs. wait-states on sram accesses.
#define CMU_MAX_SRAM_FREQ_0WS             50000000UL
#define CMU_MAX_SRAM_FREQ_1WS             80000000UL

#elif (_SILICON_LABS_32B_SERIES_2_CONFIG > 1)
// Maximum allowed core frequency vs. wait-states on flash accesses.
//#define CMU_MAX_FLASHREAD_FREQ_0WS        40000000UL
//#define CMU_MAX_FLASHREAD_FREQ_1WS        80000000UL

// Maximum allowed core frequency vs. wait-states and vscale on flash accesses.
#define CMU_MAX_FREQ_0WS_1V1              40000000UL
#define CMU_MAX_FREQ_1WS_1V1              80000000UL
#define CMU_MAX_FREQ_0WS_1V0              80000000UL
#define CMU_MAX_FREQ_1WS_1V0              80000000UL
#endif

// Maximum allowed PCLK frequency.
#define CMU_MAX_PCLK_FREQ                 50000000UL

// Maximum allowed RHCLK frequency.
#define CMU_MAX_RHCLK_FREQ                40000000UL

#if defined(EMU_VSCALE_EM01_PRESENT)
// This macro is intended to be used as input to CMU_UpdateWaitStates()
#define VSCALE_DEFAULT    (2 - (int)EMU_VScaleGet())
#else
#define VSCALE_DEFAULT    VSCALE_EM01_HIGH_PERFORMANCE
#endif

#if defined(PLFRCO_PRESENT)
// Typical frequency for HFXO as recommanded in the datasheets.
// see AN0016: Oscillator Design Considerations
//
// Recommended for most wireless applications
// to meet transceiver electrical specifications
#define XTAL_38M4                         38400000UL
// Recommended for implementing Z-Wave devices
#define XTAL_39M0                         39000000UL
// High Precision mode calibration Counts for 38.4MHz
#define LFRCO_NOMCAL_XTAL_38M4            _LFRCO_NOMCAL_RESETVALUE
#define LFRCO_NOMCALINV_XTAL_38M4         _LFRCO_NOMCALINV_RESETVALUE
// High Precision mode calibration Counts for 39.0MHz.
#define LFRCO_NOMCAL_XTAL_39M0            0x0005CFBBUL
#define LFRCO_NOMCALINV_XTAL_39M0         0x0000581AUL
#endif

#define PLL0_USB_OUTPUT_FREQ              48000000UL

/*******************************************************************************
 **************************   LOCAL VARIABLES   ********************************
 ******************************************************************************/

// Table of HFRCOCAL values and their associated min/max frequencies and
// optional band enumerator.
static const struct hfrcoCalTableElement{
  uint32_t  minFreq;
  uint32_t  maxFreq;
  uint32_t  value;
  CMU_HFRCODPLLFreq_TypeDef band;
} hfrcoCalTable[] =
{
  //  minFreq   maxFreq    HFRCOCAL value  band
  {  900000UL, 1080000UL, 0x82401F00UL, cmuHFRCODPLLFreq_1M0Hz         },
  { 1080000UL, 1300000UL, 0xA2411F3AUL, (CMU_HFRCODPLLFreq_TypeDef)0   },
  { 1300000UL, 1530000UL, 0xA2421F3AUL, (CMU_HFRCODPLLFreq_TypeDef)0   },
  { 1530000UL, 1800000UL, 0xB6439F3AUL, (CMU_HFRCODPLLFreq_TypeDef)0   },
  { 1800000UL, 2150000UL, 0x81401F00UL, cmuHFRCODPLLFreq_2M0Hz         },
  { 2150000UL, 2600000UL, 0xA1411F3AUL, (CMU_HFRCODPLLFreq_TypeDef)0   },
  { 2600000UL, 3050000UL, 0xA1421F3AUL, (CMU_HFRCODPLLFreq_TypeDef)0   },
  { 3050000UL, 3600000UL, 0xB5439F3AUL, (CMU_HFRCODPLLFreq_TypeDef)0   },
  { 3600000UL, 4300000UL, 0x80401F00UL, cmuHFRCODPLLFreq_4M0Hz         },
  { 4300000UL, 5200000UL, 0xA0411F3AUL, (CMU_HFRCODPLLFreq_TypeDef)0   },
  { 5200000UL, 6100000UL, 0xA0421F3AUL, (CMU_HFRCODPLLFreq_TypeDef)0   },
  { 6100000UL, 7800000UL, 0xB4439F00UL, cmuHFRCODPLLFreq_7M0Hz         },
  { 7800000UL, 9800000UL, 0xB4449F3AUL, (CMU_HFRCODPLLFreq_TypeDef)0   },
  { 9800000UL, 11800000UL, 0xB4459F3AUL, (CMU_HFRCODPLLFreq_TypeDef)0  },
  { 11800000UL, 14400000UL, 0xB4669F00UL, cmuHFRCODPLLFreq_13M0Hz      },
  { 14400000UL, 17200000UL, 0xB4679F00UL, cmuHFRCODPLLFreq_16M0Hz      },
  { 17200000UL, 19700000UL, 0xA8689F00UL, cmuHFRCODPLLFreq_19M0Hz      },
  { 19700000UL, 23800000UL, 0xB8899F3AUL, (CMU_HFRCODPLLFreq_TypeDef)0 },
  { 23800000UL, 28700000UL, 0xB88A9F00UL, cmuHFRCODPLLFreq_26M0Hz      },
  { 28700000UL, 34800000UL, 0xB8AB9F00UL, cmuHFRCODPLLFreq_32M0Hz      },
  { 34800000UL, 42800000UL, 0xA8CC9F00UL, cmuHFRCODPLLFreq_38M0Hz      },
  { 42800000UL, 51600000UL, 0xACED9F00UL, cmuHFRCODPLLFreq_48M0Hz      },
  { 51600000UL, 60500000UL, 0xBCEE9F00UL, cmuHFRCODPLLFreq_56M0Hz      },
  { 60500000UL, 72600000UL, 0xBCEF9F00UL, cmuHFRCODPLLFreq_64M0Hz      },
  { 72600000UL, 80000000UL, 0xCCF09F00UL, cmuHFRCODPLLFreq_80M0Hz      },
#if defined(_SILICON_LABS_32B_SERIES_2_CONFIG_5)
  { 80000000UL, 100000000UL, 0xCCF19F00UL, cmuHFRCODPLLFreq_100M0Hz   }
#endif
};

static uint16_t lfxo_precision = 0xFFFF;

#define HFRCOCALTABLE_ENTRIES (sizeof(hfrcoCalTable) \
                               / sizeof(struct hfrcoCalTableElement))

// CTUNE delta needed for some series 2 chips. This delta is added to the tuning capacitance on XO.
// The inter-chip buffered crystal sharing feature added from the EFR32xG23 creates an imbalance
// between XI and XO capacitance load internally on the chip. The delta allows to compensate for
// the difference.
#if defined(_SILICON_LABS_32B_SERIES_2)
#if defined(_SILICON_LABS_32B_SERIES_2_CONFIG_3)  \
  || defined(_SILICON_LABS_32B_SERIES_2_CONFIG_4) \
  || defined(_SILICON_LABS_32B_SERIES_2_CONFIG_5)
static int8_t ctuneDelta = 40; // Recommendation from analog team to counter the internal chip imbalance.
#else
static int8_t ctuneDelta = 0;
#endif
#endif

/*******************************************************************************
 **************************   LOCAL PROTOTYPES   *******************************
 ******************************************************************************/
#if (_SILICON_LABS_32B_SERIES_2_CONFIG > 1)
#if defined(PDM_PRESENT)
static void     em01GrpbClkGet(uint32_t *freq, CMU_Select_TypeDef *sel);
#endif
#if defined(EUART_PRESENT)
static void     euart0ClkGet(uint32_t *freq, CMU_Select_TypeDef *sel);
#endif
#if defined(EUSART_PRESENT)
static void     eusart0ClkGet(uint32_t *freq, CMU_Select_TypeDef *sel);
static void     em01GrpcClkGet(uint32_t *freq, CMU_Select_TypeDef *sel);
#endif
#if defined(LCD_PRESENT)
static void     lcdClkGet(uint32_t *freq, CMU_Select_TypeDef *sel);
#endif
#if defined(VDAC_PRESENT)
static void     vdac0ClkGet(uint32_t *freq, CMU_Select_TypeDef *sel);
#if (VDAC_COUNT > 1)
static void     vdac1ClkGet(uint32_t *freq, CMU_Select_TypeDef *sel);
#endif
#endif
#if defined(PCNT_PRESENT)
static void     pcnt0ClkGet(uint32_t *freq, CMU_Select_TypeDef *sel);
#endif
#if defined(LESENSE_PRESENT)
static void     lesenseHFClkGet(uint32_t *freq, CMU_Select_TypeDef *sel);
#endif
#if (defined(CMU_SYSCLKCTRL_RHCLKPRESC) \
  && (_SILICON_LABS_EFR32_RADIO_TYPE != _SILICON_LABS_EFR32_RADIO_NONE))
static void     rhclkPrescMax(void);
static void     rhclkPrescOptimize(void);
#endif
#endif

#if defined(HFRCOEM23_PRESENT)
static uint32_t HFRCOEM23DevinfoGet(CMU_HFRCOEM23Freq_TypeDef freq);
#endif
static void     traceClkGet(uint32_t *freq, CMU_Select_TypeDef *sel);
static void     dpllRefClkGet(uint32_t *freq, CMU_Select_TypeDef *sel);
static void     em01GrpaClkGet(uint32_t *freq, CMU_Select_TypeDef *sel);
static void     em23GrpaClkGet(uint32_t *freq, CMU_Select_TypeDef *sel);
static void     em4GrpaClkGet(uint32_t *freq, CMU_Select_TypeDef *sel);
static void     flashWaitStateControl(uint32_t coreFreq, int vscale);
static uint32_t HFRCODPLLDevinfoGet(CMU_HFRCODPLLFreq_TypeDef freq);
#if defined(IADC_PRESENT)
static void     iadcClkGet(uint32_t *freq, CMU_Select_TypeDef *sel);
#endif
static void     pclkDivMax(void);
static void     pclkDivOptimize(void);
#if defined(RTCC_PRESENT)
static void     rtccClkGet(uint32_t *freq, CMU_Select_TypeDef *sel);
#elif defined(SYSRTC_PRESENT)
static void     sysrtcClkGet(uint32_t *freq, CMU_Select_TypeDef *sel);
#endif
static void     waitStateMax(void);
static void     wdog0ClkGet(uint32_t *freq, CMU_Select_TypeDef *sel);
#if WDOG_COUNT > 1
static void     wdog1ClkGet(uint32_t *freq, CMU_Select_TypeDef *sel);
#endif
static void     sysTickClkGet(uint32_t *freq, CMU_Select_TypeDef *sel);
#if defined(USB_PRESENT)
static void     usbClkGet(uint32_t *freq, CMU_Select_TypeDef *sel);
#endif
/** @endcond */

// The following code is common for all SERIES_2 configurations.

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Calibrate an oscillator.
 *
 * @details
 *   Run a calibration of a selectable reference clock againt HCLK. Please
 *   refer to the reference manual, CMU chapter, for further details.
 *
 * @note
 *   This function will not return until calibration measurement is completed.
 *
 * @param[in] cycles
 *   The number of HCLK cycles to run calibration. Increasing this number
 *   increases precision, but the calibration will take more time.
 *
 * @param[in] ref
 *   The reference clock used to compare against HCLK.
 *
 * @return
 *   The number of ticks the selected reference clock ticked while running
 *   cycles ticks of the HCLK clock.
 ******************************************************************************/
uint32_t CMU_Calibrate(uint32_t cycles, CMU_Select_TypeDef ref)
{
  // Check for cycle count overflow
#if defined(_CMU_CALCTRL_CALTOP_MASK)
  EFM_ASSERT(cycles <= (_CMU_CALCTRL_CALTOP_MASK
                        >> _CMU_CALCTRL_CALTOP_SHIFT));
#elif defined(_CMU_CALTOP_CALTOP_MASK)
  EFM_ASSERT(cycles <= (_CMU_CALTOP_CALTOP_MASK
                        >> _CMU_CALTOP_CALTOP_SHIFT));
#endif

  CMU_CalibrateConfig(cycles, cmuSelect_HCLK, ref);
  CMU_CalibrateStart();
  return CMU_CalibrateCountGet();
}

/***************************************************************************//**
 * @brief
 *   Configure clock calibration.
 *
 * @details
 *   Configure a calibration for a selectable clock source against another
 *   selectable reference clock.
 *   Refer to the reference manual, CMU chapter, for further details.
 *
 * @note
 *   After configuration, a call to @ref CMU_CalibrateStart() is required, and
 *   the resulting calibration value can be read with the
 *   @ref CMU_CalibrateCountGet() function call.
 *
 * @param[in] downCycles
 *   The number of downSel clock cycles to run calibration. Increasing this
 *   number increases precision, but the calibration will take more time.
 *
 * @param[in] downSel
 *   The clock which will be counted down downCycles cycles.
 *
 * @param[in] upSel
 *   The reference clock, the number of cycles generated by this clock will
 *   be counted and added up, the result can be given with the
 *   @ref CMU_CalibrateCountGet() function call.
 ******************************************************************************/
void CMU_CalibrateConfig(uint32_t downCycles, CMU_Select_TypeDef downSel,
                         CMU_Select_TypeDef upSel)
{
  // Keep untouched configuration settings
  uint32_t calCtrl = CMU->CALCTRL
                     & ~(_CMU_CALCTRL_UPSEL_MASK
                         | _CMU_CALCTRL_DOWNSEL_MASK
#if defined(_CMU_CALCTRL_CALTOP_MASK)
                         | _CMU_CALCTRL_CALTOP_MASK
#endif
                         );

  // Check for cycle count overflow
#if defined(_CMU_CALCTRL_CALTOP_MASK)
  EFM_ASSERT(downCycles <= (_CMU_CALCTRL_CALTOP_MASK
                            >> _CMU_CALCTRL_CALTOP_SHIFT));
  calCtrl |= downCycles;
#elif defined(_CMU_CALTOP_CALTOP_MASK)
  EFM_ASSERT(downCycles <= (_CMU_CALTOP_CALTOP_MASK >> _CMU_CALTOP_CALTOP_SHIFT));
  CMU->CALTOP = downCycles << _CMU_CALTOP_CALTOP_SHIFT;
#endif

  // Set down counting clock source selector
  switch (downSel) {
    case cmuSelect_HCLK:
      calCtrl |= CMU_CALCTRL_DOWNSEL_HCLK;
      break;

    case cmuSelect_PRS:
      calCtrl |= CMU_CALCTRL_DOWNSEL_PRS;
      break;

    case cmuSelect_HFXO:
      calCtrl |= CMU_CALCTRL_DOWNSEL_HFXO;
      break;

    case cmuSelect_LFXO:
      calCtrl |= CMU_CALCTRL_DOWNSEL_LFXO;
      break;

    case cmuSelect_HFRCODPLL:
      calCtrl |= CMU_CALCTRL_DOWNSEL_HFRCODPLL;
      break;

#if defined(HFRCOEM23_PRESENT)
    case cmuSelect_HFRCOEM23:
      calCtrl |= CMU_CALCTRL_DOWNSEL_HFRCOEM23;
      break;
#endif

    case cmuSelect_FSRCO:
      calCtrl |= CMU_CALCTRL_DOWNSEL_FSRCO;
      break;

    case cmuSelect_LFRCO:
#if defined(PLFRCO_PRESENT)
    case cmuSelect_PLFRCO:
#endif
      calCtrl |= CMU_CALCTRL_DOWNSEL_LFRCO;
      break;

    case cmuSelect_ULFRCO:
      calCtrl |= CMU_CALCTRL_DOWNSEL_ULFRCO;
      break;

    case cmuSelect_Disabled:
      break;

    default:
      EFM_ASSERT(false);
      break;
  }

  // Set up counting clock source selector
  switch (upSel) {
    case cmuSelect_PRS:
      calCtrl |= CMU_CALCTRL_UPSEL_PRS;
      break;

    case cmuSelect_HFXO:
      calCtrl |= CMU_CALCTRL_UPSEL_HFXO;
      break;

    case cmuSelect_LFXO:
      calCtrl |= CMU_CALCTRL_UPSEL_LFXO;
      break;

    case cmuSelect_HFRCODPLL:
      calCtrl |= CMU_CALCTRL_UPSEL_HFRCODPLL;
      break;

#if defined(HFRCOEM23_PRESENT)
    case cmuSelect_HFRCOEM23:
      calCtrl |= CMU_CALCTRL_UPSEL_HFRCOEM23;
      break;
#endif

    case cmuSelect_FSRCO:
      calCtrl |= CMU_CALCTRL_UPSEL_FSRCO;
      break;

    case cmuSelect_LFRCO:
#if defined(PLFRCO_PRESENT)
    case cmuSelect_PLFRCO:
#endif
      calCtrl |= CMU_CALCTRL_UPSEL_LFRCO;
      break;

    case cmuSelect_ULFRCO:
      calCtrl |= CMU_CALCTRL_UPSEL_ULFRCO;
      break;

    case cmuSelect_Disabled:
      break;

    default:
      EFM_ASSERT(false);
      break;
  }

  CMU->CALCTRL = calCtrl;
}

/***************************************************************************//**
 * @brief
 *    Get calibration count value.
 *
 * @note
 *    If continuous calibration mode is active, calibration busy will almost
 *    always be off, and reading the value will be just needed, where the normal
 *    case would be that this function call has been triggered by the CALRDY
 *    interrupt flag.
 *
 * @return
 *    Calibration count, the number of UPSEL clocks (see @ref CMU_CalibrateConfig())
 *    in the period of DOWNSEL oscillator clock cycles configured by a previous
 *    write operation to CMU->CALCNT.
 ******************************************************************************/
uint32_t CMU_CalibrateCountGet(void)
{
  // Wait until calibration completes, UNLESS continuous calibration mode is on
  if ((CMU->CALCTRL & CMU_CALCTRL_CONT) == 0UL) {
    // Wait until calibration completes
    while ((CMU->STATUS & CMU_STATUS_CALRDY) == 0UL) {
    }
  }
  return CMU->CALCNT;
}

/***************************************************************************//**
 * @brief
 *    Direct a clock to a GPIO pin.
 *
 * @param[in] clkNo
 *   Selects between CLKOUT0, CLKOUT1 or CLKOUT2 outputs. Use values 0,1or 2.
 *
 * @param[in] sel
 *   Select clock source.
 *
 * @param[in] clkDiv
 *   Select a clock divisor (1..32). Only applicable when cmuSelect_EXPCLK is
 *   slexted as clock source.
 *
 * @param[in] port
 *   GPIO port.
 *
 * @param[in] pin
 *   GPIO pin.
 *
 * @note
 *    Refer to the reference manual and the datasheet for details on which
 *    GPIO port/pins that are available.
 ******************************************************************************/
void CMU_ClkOutPinConfig(uint32_t           clkNo,
                         CMU_Select_TypeDef sel,
                         CMU_ClkDiv_TypeDef clkDiv,
                         GPIO_Port_TypeDef  port,
                         unsigned int       pin)
{
  uint32_t tmp = 0U, mask;

  EFM_ASSERT(clkNo <= 2U);
  EFM_ASSERT(clkDiv <= 32U);
  EFM_ASSERT(port <= 3U);
  EFM_ASSERT(pin <= 15U);

  switch (sel) {
    case cmuSelect_Disabled:
      tmp = CMU_EXPORTCLKCTRL_CLKOUTSEL0_DISABLED;
      break;

    case cmuSelect_FSRCO:
      tmp = CMU_EXPORTCLKCTRL_CLKOUTSEL0_FSRCO;
      break;

    case cmuSelect_HFXO:
      tmp = CMU_EXPORTCLKCTRL_CLKOUTSEL0_HFXO;
      break;

    case cmuSelect_HFRCODPLL:
      tmp = CMU_EXPORTCLKCTRL_CLKOUTSEL0_HFRCODPLL;
      break;

#if defined(HFRCOEM23_PRESENT)
    case cmuSelect_HFRCOEM23:
      tmp = CMU_EXPORTCLKCTRL_CLKOUTSEL0_HFRCOEM23;
      break;
#endif

    case cmuSelect_EXPCLK:
      tmp  = CMU_EXPORTCLKCTRL_CLKOUTSEL0_HFEXPCLK;
      break;

    case cmuSelect_LFXO:
      tmp = CMU_EXPORTCLKCTRL_CLKOUTSEL0_LFXO;
      break;

    case cmuSelect_LFRCO:
#if defined(PLFRCO_PRESENT)
    case cmuSelect_PLFRCO:
#endif
      tmp = CMU_EXPORTCLKCTRL_CLKOUTSEL0_LFRCO;
      break;

    case cmuSelect_ULFRCO:
      tmp = CMU_EXPORTCLKCTRL_CLKOUTSEL0_ULFRCO;
      break;

    case cmuSelect_HCLK:
      tmp = CMU_EXPORTCLKCTRL_CLKOUTSEL0_HCLK;
      break;

    default:
      EFM_ASSERT(false);
      break;
  }

  mask  = _CMU_EXPORTCLKCTRL_CLKOUTSEL0_MASK
          << (clkNo * _CMU_EXPORTCLKCTRL_CLKOUTSEL1_SHIFT);
  tmp <<= clkNo * _CMU_EXPORTCLKCTRL_CLKOUTSEL1_SHIFT;

  if (sel == cmuSelect_EXPCLK) {
    tmp  |= (clkDiv - 1U) << _CMU_EXPORTCLKCTRL_PRESC_SHIFT;
    mask |= _CMU_EXPORTCLKCTRL_PRESC_MASK;
  }

  CMU->EXPORTCLKCTRL = (CMU->EXPORTCLKCTRL & ~mask) | tmp;

  if (sel == cmuSelect_Disabled) {
    GPIO->CMUROUTE_CLR.ROUTEEN = GPIO_CMU_ROUTEEN_CLKOUT0PEN << clkNo;
    GPIO_PinModeSet(port, pin, gpioModeDisabled, 0);
  } else {
    GPIO->CMUROUTE_SET.ROUTEEN = GPIO_CMU_ROUTEEN_CLKOUT0PEN << clkNo;
    if (clkNo == 0U) {
      GPIO->CMUROUTE.CLKOUT0ROUTE = (port << _GPIO_CMU_CLKOUT0ROUTE_PORT_SHIFT)
                                    | (pin << _GPIO_CMU_CLKOUT0ROUTE_PIN_SHIFT);
    } else if (clkNo == 1) {
      GPIO->CMUROUTE.CLKOUT1ROUTE = (port << _GPIO_CMU_CLKOUT1ROUTE_PORT_SHIFT)
                                    | (pin << _GPIO_CMU_CLKOUT1ROUTE_PIN_SHIFT);
    } else {
      GPIO->CMUROUTE.CLKOUT2ROUTE = (port << _GPIO_CMU_CLKOUT2ROUTE_PORT_SHIFT)
                                    | (pin << _GPIO_CMU_CLKOUT2ROUTE_PIN_SHIFT);
    }
    GPIO_PinModeSet(port, pin, gpioModePushPull, 0);
  }
}

/***************************************************************************//**
 * @brief
 *   Get clock divisor.
 *
 * @param[in] clock
 *   Clock point to get divisor for. Notice that not all clock points
 *   have a divisors. Please refer to CMU overview in reference manual.
 *
 * @return
 *   The current clock point divisor. 1 is returned
 *   if @p clock specifies a clock point without divisor.
 ******************************************************************************/
CMU_ClkDiv_TypeDef CMU_ClockDivGet(CMU_Clock_TypeDef clock)
{
  uint32_t ret = 0U;

  switch (clock) {
    case cmuClock_HCLK:
    case cmuClock_CORE:
      ret = (CMU->SYSCLKCTRL & _CMU_SYSCLKCTRL_HCLKPRESC_MASK)
            >> _CMU_SYSCLKCTRL_HCLKPRESC_SHIFT;
#if defined(_SILICON_LABS_32B_SERIES_2_CONFIG_1)
      if (ret == 2U ) {   // Unused value, illegal prescaler
        EFM_ASSERT(false);
      }
#elif (_SILICON_LABS_32B_SERIES_2_CONFIG > 1)
      if (!((ret == _CMU_SYSCLKCTRL_HCLKPRESC_DIV1)
            || (ret == _CMU_SYSCLKCTRL_HCLKPRESC_DIV2)
            || (ret == _CMU_SYSCLKCTRL_HCLKPRESC_DIV4)
            || (ret == _CMU_SYSCLKCTRL_HCLKPRESC_DIV8)
            || (ret == _CMU_SYSCLKCTRL_HCLKPRESC_DIV16))) {
        // Illegal prescaler
        EFM_ASSERT(false);
      }
#endif
      break;

#if defined(_CMU_TRACECLKCTRL_PRESC_MASK)
    case cmuClock_TRACECLK:
      ret = (CMU->TRACECLKCTRL & _CMU_TRACECLKCTRL_PRESC_MASK)
            >> _CMU_TRACECLKCTRL_PRESC_SHIFT;
      if (!((ret == _CMU_TRACECLKCTRL_PRESC_DIV1)
            || (ret == _CMU_TRACECLKCTRL_PRESC_DIV2)
#if defined(_CMU_TRACECLKCTRL_PRESC_DIV3)
            || (ret == _CMU_TRACECLKCTRL_PRESC_DIV3)
#endif
            || (ret == _CMU_TRACECLKCTRL_PRESC_DIV4))) {
        // Illegal prescaler
        EFM_ASSERT(false);
      }
      break;
#endif

    case cmuClock_EXPCLK:
      ret = (CMU->EXPORTCLKCTRL & _CMU_EXPORTCLKCTRL_PRESC_MASK)
            >> _CMU_EXPORTCLKCTRL_PRESC_SHIFT;
      break;

    case cmuClock_PCLK:
      ret = (CMU->SYSCLKCTRL & _CMU_SYSCLKCTRL_PCLKPRESC_MASK)
            >> _CMU_SYSCLKCTRL_PCLKPRESC_SHIFT;
      break;

    default:
      break;
  }
  return 1U + ret;
}

/***************************************************************************//**
 * @brief
 *   Set clock divisor.
 *
 * @param[in] clock
 *   Clock point to set divisor for. Notice that not all clock points
 *   have a divisor, please refer to CMU overview in the reference
 *   manual.
 *
 * @param[in] div
 *   The clock divisor to use.
 ******************************************************************************/
void CMU_ClockDivSet(CMU_Clock_TypeDef clock, CMU_ClkDiv_TypeDef div)
{
#if (defined(_SILICON_LABS_32B_SERIES_2_CONFIG_4) \
  || defined(_SILICON_LABS_32B_SERIES_2_CONFIG_5) \
  && defined(CoreDebug_DEMCR_TRCENA_Msk))
  bool restoreTrace;
#endif

  switch (clock) {
    case cmuClock_HCLK:
    case cmuClock_CORE:
#if defined(_SILICON_LABS_32B_SERIES_2_CONFIG_1)
      EFM_ASSERT((div == 1U) || (div == 2U) || (div == 4U));
#elif (_SILICON_LABS_32B_SERIES_2_CONFIG > 1)
      EFM_ASSERT((div == 1U) || (div == 2U) || (div == 4U)
                 || (div == 8U) || (div == 16U));
#endif

      // Set max wait-states and PCLK divisor while changing core clock
      waitStateMax();
      pclkDivMax();
#if ((defined(CMU_SYSCLKCTRL_RHCLKPRESC)) \
      && (_SILICON_LABS_EFR32_RADIO_TYPE != _SILICON_LABS_EFR32_RADIO_NONE))
      // Set largest prescaler for radio clock tree
      rhclkPrescMax();
#endif

      // Set new divisor
      CMU->SYSCLKCTRL = (CMU->SYSCLKCTRL & ~_CMU_SYSCLKCTRL_HCLKPRESC_MASK)
                        | ((div - 1U) << _CMU_SYSCLKCTRL_HCLKPRESC_SHIFT);

      // Update CMSIS core clock variable and set optimum wait-states
      CMU_UpdateWaitStates(SystemCoreClockGet(), VSCALE_DEFAULT);

      // Set optimal PCLK divisor
      pclkDivOptimize();
#if (defined(CMU_SYSCLKCTRL_RHCLKPRESC) \
      && (_SILICON_LABS_EFR32_RADIO_TYPE != _SILICON_LABS_EFR32_RADIO_NONE))
      // Set optimal RHCLK prescaler
      rhclkPrescOptimize();
#endif
      break;

#if defined(_CMU_TRACECLKCTRL_PRESC_MASK)
    case cmuClock_TRACECLK:
#if (defined(_SILICON_LABS_32B_SERIES_2_CONFIG_4)     \
      || defined(_SILICON_LABS_32B_SERIES_2_CONFIG_5) \
      && defined(CoreDebug_DEMCR_TRCENA_Msk))
      restoreTrace = CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk;
      if (restoreTrace) {
        CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
      }
#endif
#if defined(_CMU_TRACECLKCTRL_PRESC_DIV3)
      EFM_ASSERT((div == 1U) || (div == 2U) || (div == 3U) || (div == 4U));
#else
      EFM_ASSERT((div == 1U) || (div == 2U) || (div == 4U));
#endif
      CMU->TRACECLKCTRL = (CMU->TRACECLKCTRL & ~_CMU_TRACECLKCTRL_PRESC_MASK)
                          | ((div - 1U) << _CMU_TRACECLKCTRL_PRESC_SHIFT);
#if (defined(_SILICON_LABS_32B_SERIES_2_CONFIG_4)     \
      || defined(_SILICON_LABS_32B_SERIES_2_CONFIG_5) \
      && defined(CoreDebug_DEMCR_TRCENA_Msk))
      if (restoreTrace) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
      }
#endif
      break;
#endif

    case cmuClock_EXPCLK:
      EFM_ASSERT((div >= 1U) && (div <= 32U));
      CMU->EXPORTCLKCTRL = (CMU->EXPORTCLKCTRL & ~_CMU_EXPORTCLKCTRL_PRESC_MASK)
                           | ((div - 1U) << _CMU_EXPORTCLKCTRL_PRESC_SHIFT);
      break;

    case cmuClock_PCLK:
      EFM_ASSERT((div == 1U) || (div == 2U));
      CMU->SYSCLKCTRL = (CMU->SYSCLKCTRL & ~_CMU_SYSCLKCTRL_PCLKPRESC_MASK)
                        | ((div - 1U) << _CMU_SYSCLKCTRL_PCLKPRESC_SHIFT);
      break;

    default:
      EFM_ASSERT(false);
      break;
  }
}

#if (_SILICON_LABS_32B_SERIES_2_CONFIG > 1)
/***************************************************************************//**
 * @brief
 *   Enable/disable a clock.
 *
 * @details
 *   Module clocks sre disabled after reset. If a module clock is disabled, the
 *   registers of that module are not accessible and accessing such registers
 *   will hardfault the Cortex core.
 *
 * @param[in] clock
 *   The clock to enable/disable.
 *
 * @param[in] enable
 *   @li true - enable specified clock.
 *   @li false - disable specified clock.
 ******************************************************************************/
void CMU_ClockEnable(CMU_Clock_TypeDef clock, bool enable)
{
  volatile uint32_t *reg = NULL;
  uint32_t          bit;

  /* Identify the enable register. */
  if (((unsigned)clock >> CMU_EN_REG_POS) == CMU_NO_EN_REG) {
    EFM_ASSERT(false);                      /* No enable for this clock. */
  } else if (((unsigned)clock >> CMU_EN_REG_POS) == CMU_CLKEN0_EN_REG) {
    reg = &CMU->CLKEN0;
  } else if (((unsigned)clock >> CMU_EN_REG_POS) == CMU_CLKEN1_EN_REG) {
    reg = &CMU->CLKEN1;
  } else {
#if defined(CRYPTOACC_PRESENT)
    reg = &CMU->CRYPTOACCCLKCTRL;
#else
    // No register to enable clock. Possible hard fault exception.
    EFM_ASSERT(false);
#endif
  }

  /* Get the bit position used to enable/disable. */
  bit = ((unsigned)clock >> CMU_EN_BIT_POS) & CMU_EN_BIT_MASK;

  /* Set/clear bit as requested. */
  BUS_RegBitWrite(reg, bit, (uint32_t)enable);
}
#endif // (_SILICON_LABS_32B_SERIES_2_CONFIG > 1)

#if defined(_SILICON_LABS_32B_SERIES_2_CONFIG_1)
/***************************************************************************//**
 * @brief
 *   Get clock frequency for a clock point.
 *
 * @param[in] clock
 *   Clock point to fetch frequency for.
 *
 * @return
 *   The current frequency in Hz.
 ******************************************************************************/
uint32_t CMU_ClockFreqGet(CMU_Clock_TypeDef clock)
{
  uint32_t ret = 0U;

  switch (clock) {
    case cmuClock_SYSCLK:
      ret = SystemSYSCLKGet();
      break;

    case cmuClock_CORE:
    case cmuClock_HCLK:
    case cmuClock_LDMA:
    case cmuClock_GPCRC:
      ret = SystemHCLKGet();
      break;

    case cmuClock_EXPCLK:
      ret = SystemSYSCLKGet() / CMU_ClockDivGet(cmuClock_EXPCLK);
      break;

    case cmuClock_I2C1:
    case cmuClock_PRS:
    case cmuClock_PCLK:
    case cmuClock_GPIO:
    case cmuClock_USART0:
    case cmuClock_USART1:
    case cmuClock_USART2:
      ret = SystemHCLKGet() / CMU_ClockDivGet(cmuClock_PCLK);
      break;

    case cmuClock_I2C0:
    case cmuClock_LSPCLK:
      ret = SystemHCLKGet() / CMU_ClockDivGet(cmuClock_PCLK) / 2U;
      break;

#if defined(IADC_PRESENT)
    case cmuClock_IADC0:
    case cmuClock_IADCCLK:
      iadcClkGet(&ret, NULL);
      break;
#endif

    case cmuClock_TIMER0:
    case cmuClock_TIMER1:
    case cmuClock_TIMER2:
    case cmuClock_TIMER3:
    case cmuClock_EM01GRPACLK:
      em01GrpaClkGet(&ret, NULL);
      break;

    case cmuClock_SYSTICK:
      sysTickClkGet(&ret, NULL);
      break;

    case cmuClock_LETIMER0:
    case cmuClock_EM23GRPACLK:
      em23GrpaClkGet(&ret, NULL);
      break;

    case cmuClock_BURTC:
    case cmuClock_EM4GRPACLK:
      em4GrpaClkGet(&ret, NULL);
      break;

    case cmuClock_WDOG0:
    case cmuClock_WDOG0CLK:
      wdog0ClkGet(&ret, NULL);
      break;

    case cmuClock_WDOG1:
    case cmuClock_WDOG1CLK:
      wdog1ClkGet(&ret, NULL);
      break;

    case cmuClock_DPLLREFCLK:
      dpllRefClkGet(&ret, NULL);
      break;

    case cmuClock_TRACECLK:
      traceClkGet(&ret, NULL);
      break;

    case cmuClock_RTCC:
    case cmuClock_RTCCCLK:
      rtccClkGet(&ret, NULL);
      break;

    default:
      EFM_ASSERT(false);
      break;
  }
  return ret;
}
#endif // defined(_SILICON_LABS_32B_SERIES_2_CONFIG_1)

#if (_SILICON_LABS_32B_SERIES_2_CONFIG > 1)
/***************************************************************************//**
 * @brief
 *   Get clock frequency for a clock point.
 *
 * @param[in] clock
 *   Clock point to fetch frequency for.
 *
 * @return
 *   The current frequency in Hz.
 ******************************************************************************/
uint32_t CMU_ClockFreqGet(CMU_Clock_TypeDef clock)
{
  uint32_t ret = 0U;
  uint32_t freq = 0U;

  switch (clock) {
    case cmuClock_SYSCLK:
      ret = SystemSYSCLKGet();
      break;

    case cmuClock_HCLK:
    case cmuClock_CORE:
    case cmuClock_ICACHE:
    case cmuClock_MSC:
    case cmuClock_LDMA:
    case cmuClock_SMU:
#if defined(RADIOAES_PRESENT)
    case cmuClock_RADIOAES:
#endif
#if defined(CRYPTOACC_PRESENT)
    case cmuClock_CRYPTOACC:
    case cmuClock_CRYPTOAES:
    case cmuClock_CRYPTOPK:
#endif
      ret = SystemHCLKGet();
      break;

    case cmuClock_EXPCLK:
      ret = SystemSYSCLKGet() / CMU_ClockDivGet(cmuClock_EXPCLK);
      break;

    case cmuClock_PCLK:
#if defined(USART_PRESENT)
    case cmuClock_USART0:
#if USART_COUNT > 1
    case cmuClock_USART1:
#endif
#endif
    case cmuClock_I2C1:
    case cmuClock_PRS:
    case cmuClock_GPIO:
    case cmuClock_GPCRC:
    case cmuClock_LDMAXBAR:
    case cmuClock_SYSCFG:
    case cmuClock_DCDC:
    case cmuClock_BURAM:
    case cmuClock_DPLL0:
      ret = SystemHCLKGet() / CMU_ClockDivGet(cmuClock_PCLK);
      break;

    case cmuClock_LSPCLK:
    case cmuClock_I2C0:
    case cmuClock_AMUXCP0:
#if defined(ACMP_PRESENT)
    case cmuClock_ACMP0:
#if ACMP_COUNT > 1
    case cmuClock_ACMP1:
#endif
#endif
      ret = SystemHCLKGet() / CMU_ClockDivGet(cmuClock_PCLK) / 2U;
      break;

    case cmuClock_TRACECLK:
      traceClkGet(&freq, NULL);
      ret = freq / CMU_ClockDivGet(cmuClock_TRACECLK);
      break;

    case cmuClock_TIMER0:
    case cmuClock_TIMER1:
    case cmuClock_TIMER2:
    case cmuClock_TIMER3:
#if TIMER_COUNT > 4
    case cmuClock_TIMER4:
#endif
#if TIMER_COUNT > 7
    case cmuClock_TIMER5:
    case cmuClock_TIMER6:
    case cmuClock_TIMER7:
#endif
#if defined(KEYSCAN_PRESENT)
    case cmuClock_KEYSCAN:
#endif
    case cmuClock_EM01GRPACLK:
      em01GrpaClkGet(&ret, NULL);
      break;
#if defined(PDM_PRESENT)
    case cmuClock_PDM:
    case cmuClock_EM01GRPBCLK:
      em01GrpbClkGet(&ret, NULL);
      break;
#endif
#if defined(EUART_PRESENT)
    case cmuClock_EUART0:
    case cmuClock_EUART0CLK:
      euart0ClkGet(&ret, NULL);
      break;
#endif
#if defined(EUSART_PRESENT) && EUSART_COUNT > 0
    case cmuClock_EUSART0:
    case cmuClock_EUSART0CLK:
      eusart0ClkGet(&ret, NULL);
      break;
#if defined(EUSART_PRESENT) && EUSART_COUNT > 1
    case cmuClock_EUSART1:
#endif
#if defined(EUSART_PRESENT) && EUSART_COUNT > 2
    case cmuClock_EUSART2:
#endif
#if defined(EUSART_PRESENT) && EUSART_COUNT > 3
    case cmuClock_EUSART3:
#endif
#if defined(EUSART_PRESENT) && EUSART_COUNT > 4
    case cmuClock_EUSART4:
#endif
    case cmuClock_EM01GRPCCLK:
      em01GrpcClkGet(&ret, NULL);
      break;
#endif

#if defined(IADC_PRESENT)
    case cmuClock_IADC0:
    case cmuClock_IADCCLK:
      iadcClkGet(&ret, NULL);
      break;
#endif

    case cmuClock_SYSTICK:
      sysTickClkGet(&ret, NULL);
      break;

    case cmuClock_LETIMER0:
    case cmuClock_EM23GRPACLK:
#if defined(LESENSE_PRESENT)
    case cmuClock_LESENSE:
    case cmuClock_LESENSECLK:
#endif
      em23GrpaClkGet(&ret, NULL);
      break;

    case cmuClock_WDOG0:
    case cmuClock_WDOG0CLK:
      wdog0ClkGet(&ret, NULL);
      break;
#if WDOG_COUNT > 1
    case cmuClock_WDOG1:
    case cmuClock_WDOG1CLK:
      wdog1ClkGet(&ret, NULL);
      break;
#endif
#if defined(RTCC_PRESENT)
    case cmuClock_RTCC:
    case cmuClock_RTCCCLK:
      rtccClkGet(&ret, NULL);
      break;
#endif
#if defined(SYSRTC_PRESENT)
    case cmuClock_SYSRTC:
    case cmuClock_SYSRTCCLK:
      sysrtcClkGet(&ret, NULL);
      break;
#endif
#if defined(LCD_PRESENT)
    case cmuClock_LCD:
    case cmuClock_LCDCLK:
      lcdClkGet(&ret, NULL);
      break;
#endif
#if defined(VDAC_PRESENT)
    case cmuClock_VDAC0:
    case cmuClock_VDAC0CLK:
      vdac0ClkGet(&ret, NULL);
      break;
#if (VDAC_COUNT > 1)
    case cmuClock_VDAC1:
    case cmuClock_VDAC1CLK:
      vdac1ClkGet(&ret, NULL);
      break;
#endif
#endif /* VDAC_PRESENT */
#if defined(PCNT_PRESENT)
    case cmuClock_PCNT0:
    case cmuClock_PCNT0CLK:
      pcnt0ClkGet(&ret, NULL);
      break;
#endif
#if defined(LESENSE_PRESENT)
    case cmuClock_LESENSEHFCLK:
      lesenseHFClkGet(&ret, NULL);
      break;
#endif
    case cmuClock_BURTC:
    case cmuClock_EM4GRPACLK:
      em4GrpaClkGet(&ret, NULL);
      break;

#if defined(USB_PRESENT)
    case cmuClock_USB:
      usbClkGet(&ret, NULL);
      break;
#endif

    default:
      EFM_ASSERT(false);
      break;
  }

  return ret;
}
#endif // (_SILICON_LABS_32B_SERIES_2_CONFIG > 1)

/***************************************************************************//**
 * @brief
 *   Get currently selected reference clock used for a clock branch.
 *
 * @param[in] clock
 *   Clock branch to fetch selected ref. clock for.
 *
 * @return
 *   Reference clock used for clocking selected branch, #cmuSelect_Error if
 *   invalid @p clock provided.
 ******************************************************************************/
CMU_Select_TypeDef CMU_ClockSelectGet(CMU_Clock_TypeDef clock)
{
  CMU_Select_TypeDef ret = cmuSelect_Error;

  switch (clock) {
// -----------------------------------------------------------------------------
    case cmuClock_SYSCLK:
      switch (CMU->SYSCLKCTRL & _CMU_SYSCLKCTRL_CLKSEL_MASK) {
        case _CMU_SYSCLKCTRL_CLKSEL_HFRCODPLL:
          ret = cmuSelect_HFRCODPLL;
          break;

        case _CMU_SYSCLKCTRL_CLKSEL_HFXO:
          ret = cmuSelect_HFXO;
          break;

        case _CMU_SYSCLKCTRL_CLKSEL_CLKIN0:
          ret = cmuSelect_CLKIN0;
          break;

        case _CMU_SYSCLKCTRL_CLKSEL_FSRCO:
          ret = cmuSelect_FSRCO;
          break;

#if defined(RFFPLL_PRESENT)
        case _CMU_SYSCLKCTRL_CLKSEL_RFFPLL0SYS:
          ret = cmuSelect_RFFPLLSYS;
          break;
#endif
        default:
          ret = cmuSelect_Error;
          EFM_ASSERT(false);
          break;
      }
      break;

// -----------------------------------------------------------------------------
#if defined(IADC_PRESENT)
    case cmuClock_IADC0:
    case cmuClock_IADCCLK:
      iadcClkGet(NULL, &ret);
      break;
#endif
// -----------------------------------------------------------------------------
    case cmuClock_TIMER0:
    case cmuClock_TIMER1:
    case cmuClock_TIMER2:
    case cmuClock_TIMER3:
#if TIMER_COUNT > 4
    case cmuClock_TIMER4:
#endif
#if TIMER_COUNT > 7
    case cmuClock_TIMER5:
    case cmuClock_TIMER6:
    case cmuClock_TIMER7:
#endif
#if defined(KEYSCAN_PRESENT)
    case cmuClock_KEYSCAN:
#endif
    case cmuClock_EM01GRPACLK:
      em01GrpaClkGet(NULL, &ret);
      break;

// -----------------------------------------------------------------------------
    case cmuClock_SYSTICK:
      sysTickClkGet(NULL, &ret);
      break;

    case cmuClock_LETIMER0:
    case cmuClock_EM23GRPACLK:
#if defined(LESENSE_PRESENT)
    case cmuClock_LESENSE:
    case cmuClock_LESENSECLK:
#endif
      em23GrpaClkGet(NULL, &ret);
      break;

// -----------------------------------------------------------------------------
    case cmuClock_BURTC:
    case cmuClock_EM4GRPACLK:
      em4GrpaClkGet(NULL, &ret);
      break;

// -----------------------------------------------------------------------------
#if defined(_CMU_EM01GRPBCLKCTRL_CLKSEL_MASK)
    case cmuClock_PDMREF:
    case cmuClock_EM01GRPBCLK:
      em01GrpbClkGet(NULL, &ret);
      break;
#endif

// -----------------------------------------------------------------------------
    case cmuClock_WDOG0:
    case cmuClock_WDOG0CLK:
      wdog0ClkGet(NULL, &ret);
      break;

// -----------------------------------------------------------------------------
#if defined(_CMU_WDOG1CLKCTRL_CLKSEL_MASK)
    case cmuClock_WDOG1:
    case cmuClock_WDOG1CLK:
      wdog1ClkGet(NULL, &ret);
      break;
#endif

// -----------------------------------------------------------------------------
    case cmuClock_DPLLREFCLK:
      dpllRefClkGet(NULL, &ret);
      break;

// -----------------------------------------------------------------------------
#if defined(_CMU_TRACECLKCTRL_CLKSEL_MASK)
    case cmuClock_TRACECLK:
      traceClkGet(NULL, &ret);
      break;
#endif

// -----------------------------------------------------------------------------
#if defined(_CMU_EUART0CLKCTRL_CLKSEL_MASK)
    case cmuClock_EUART0:
    case cmuClock_EUART0CLK:
      euart0ClkGet(NULL, &ret);
      break;
#elif defined(EUSART_PRESENT)
    case cmuClock_EUSART0:
    case cmuClock_EUSART0CLK:
      eusart0ClkGet(NULL, &ret);
      break;

#if defined(EUSART_PRESENT) && EUSART_COUNT > 1
    case cmuClock_EUSART1:
#endif
#if defined(EUSART_PRESENT) && EUSART_COUNT > 2
    case cmuClock_EUSART2:
#endif
#if defined(EUSART_PRESENT) && EUSART_COUNT > 3
    case cmuClock_EUSART3:
#endif
#if defined(EUSART_PRESENT) && EUSART_COUNT > 4
    case cmuClock_EUSART4:
#endif
    case cmuClock_EM01GRPCCLK:
      em01GrpcClkGet(NULL, &ret);
      break;
 #endif

// -----------------------------------------------------------------------------
#if defined(RTCC_PRESENT)
    case cmuClock_RTCC:
    case cmuClock_RTCCCLK:
      rtccClkGet(NULL, &ret);
      break;
#endif
// -----------------------------------------------------------------------------
#if defined(SYSRTC_PRESENT)
    case cmuClock_SYSRTC:
    case cmuClock_SYSRTCCLK:
      sysrtcClkGet(NULL, &ret);
      break;
#endif
// -----------------------------------------------------------------------------
#if defined(LCD_PRESENT)
    case cmuClock_LCD:
    case cmuClock_LCDCLK:
      lcdClkGet(NULL, &ret);
      break;
#endif
// -----------------------------------------------------------------------------
#if defined(VDAC_PRESENT)
    case cmuClock_VDAC0:
    case cmuClock_VDAC0CLK:
      vdac0ClkGet(NULL, &ret);
      break;
#if (VDAC_COUNT > 1)
    case cmuClock_VDAC1:
    case cmuClock_VDAC1CLK:
      vdac1ClkGet(NULL, &ret);
      break;
#endif
#endif
// -----------------------------------------------------------------------------
#if defined(PCNT_PRESENT)
    case cmuClock_PCNT0:
    case cmuClock_PCNT0CLK:
      pcnt0ClkGet(NULL, &ret);
      break;
#endif
// -----------------------------------------------------------------------------
#if defined(LESENSE_PRESENT)
    case cmuClock_LESENSEHFCLK:
      lesenseHFClkGet(NULL, &ret);
      break;
#endif
// -----------------------------------------------------------------------------
#if defined(USB_PRESENT)
    case cmuClock_USB:
      usbClkGet(NULL, &ret);
      break;
#endif
// -----------------------------------------------------------------------------
    default:
      EFM_ASSERT(false);
      break;
  }
  return ret;
}

/***************************************************************************//**
 * @brief
 *   Select reference clock/oscillator used for a clock branch.
 *
 * @param[in] clock
 *   Clock branch to select reference clock for.
 *
 * @param[in] ref
 *   Reference selected for clocking, please refer to reference manual for
 *   for details on which reference is available for a specific clock branch.
 ******************************************************************************/
void CMU_ClockSelectSet(CMU_Clock_TypeDef clock, CMU_Select_TypeDef ref)
{
  uint32_t tmp = 0U;
  bool oscForceEnStatus = false;
#if (_SILICON_LABS_32B_SERIES_2_CONFIG > 1)
  bool syscfgClkIsOff = false;
#endif
#if (defined(_SILICON_LABS_32B_SERIES_2_CONFIG_4) \
  || defined(_SILICON_LABS_32B_SERIES_2_CONFIG_5) \
  && defined(CoreDebug_DEMCR_TRCENA_Msk))
  bool restoreTrace;
#endif

  switch (clock) {
// -----------------------------------------------------------------------------
    case cmuClock_SYSCLK:
      switch (ref) {
        case cmuSelect_HFRCODPLL:
          tmp = CMU_SYSCLKCTRL_CLKSEL_HFRCODPLL;
          break;

        case cmuSelect_HFXO:
          tmp = CMU_SYSCLKCTRL_CLKSEL_HFXO;
#if defined(_CMU_CLKEN0_MASK)
#if defined(CMU_CLKEN0_HFXO0)
          CMU->CLKEN0_SET = CMU_CLKEN0_HFXO0;
#endif
#endif
          // Make sure HFXO is enabled.
          oscForceEnStatus = (HFXO0->CTRL & HFXO_CTRL_DISONDEMAND) != 0;
          HFXO0->CTRL_SET = HFXO_CTRL_FORCEEN;
          break;

        case cmuSelect_CLKIN0:
          tmp = CMU_SYSCLKCTRL_CLKSEL_CLKIN0;
          break;

        case cmuSelect_FSRCO:
          tmp = CMU_SYSCLKCTRL_CLKSEL_FSRCO;
          break;

#if defined(RFFPLL_PRESENT)
        case cmuSelect_RFFPLLSYS:
          tmp = CMU_SYSCLKCTRL_CLKSEL_RFFPLL0SYS;
          break;
#endif

        default:
          EFM_ASSERT(false);
          break;
      }

#if defined(EMU_VSCALE_EM01_PRESENT)
      // VSCALE up before changing clock.
      EMU_VScaleEM01(emuVScaleEM01_HighPerformance, true);
#endif

      // Set max wait-states and PCLK divisor while changing core clock.
      waitStateMax();
      pclkDivMax();
#if ((defined(CMU_SYSCLKCTRL_RHCLKPRESC)) \
      && (_SILICON_LABS_EFR32_RADIO_TYPE != _SILICON_LABS_EFR32_RADIO_NONE))
      // Set largest prescaler for radio clock tree
      rhclkPrescMax();
#endif

      // Switch to selected oscillator.
      CMU->SYSCLKCTRL = (CMU->SYSCLKCTRL & ~_CMU_SYSCLKCTRL_CLKSEL_MASK) | tmp;

      // Update CMSIS core clock variable and set optimum wait-states.
      CMU_UpdateWaitStates(SystemCoreClockGet(), VSCALE_DEFAULT);

#if defined(EMU_VSCALE_EM01_PRESENT)
      // Check if possible to downscale VSCALE setting.
      EMU_VScaleEM01ByClock(0, true);
#endif

      // Set optimal PCLK divisor
      pclkDivOptimize();
#if (defined(CMU_SYSCLKCTRL_RHCLKPRESC) \
      && (_SILICON_LABS_EFR32_RADIO_TYPE != _SILICON_LABS_EFR32_RADIO_NONE))
      // Set optimal RHCLK prescaler
      rhclkPrescOptimize();
#endif

      if (oscForceEnStatus == false) {
        switch (ref) {
          case cmuSelect_HFXO:
            HFXO0->CTRL_CLR = HFXO_CTRL_FORCEEN;
            break;

          default:
            break;
        }
      }
      break;

// -----------------------------------------------------------------------------
#if defined(IADC_PRESENT)
    case cmuClock_IADC0:
    case cmuClock_IADCCLK:
      switch (ref) {
        case cmuSelect_EM01GRPACLK:
          tmp = CMU_IADCCLKCTRL_CLKSEL_EM01GRPACLK;
          break;

#if defined(HFRCOEM23_PRESENT)
        case cmuSelect_HFRCOEM23:
          tmp = CMU_IADCCLKCTRL_CLKSEL_HFRCOEM23;
          break;
#endif

        case cmuSelect_FSRCO:
          tmp = CMU_IADCCLKCTRL_CLKSEL_FSRCO;
          break;

        default:
          EFM_ASSERT(false);
          break;
      }
      CMU->IADCCLKCTRL = (CMU->IADCCLKCTRL & ~_CMU_IADCCLKCTRL_CLKSEL_MASK)
                         | tmp;
      break;
#endif

// -----------------------------------------------------------------------------
    case cmuClock_TIMER0:
    case cmuClock_TIMER1:
    case cmuClock_TIMER2:
    case cmuClock_TIMER3:
#if TIMER_COUNT > 4
    case cmuClock_TIMER4:
#endif
#if TIMER_COUNT > 7
    case cmuClock_TIMER5:
    case cmuClock_TIMER6:
    case cmuClock_TIMER7:
#endif
#if defined(KEYSCAN_PRESENT)
    case cmuClock_KEYSCAN:
#endif
    case cmuClock_EM01GRPACLK:
      switch (ref) {
        case cmuSelect_HFRCODPLL:
          tmp = CMU_EM01GRPACLKCTRL_CLKSEL_HFRCODPLL;
          break;

        case cmuSelect_HFXO:
          tmp = CMU_EM01GRPACLKCTRL_CLKSEL_HFXO;
          break;

#if defined(HFRCOEM23_PRESENT)
        case cmuSelect_HFRCOEM23:
          tmp = CMU_EM01GRPACLKCTRL_CLKSEL_HFRCOEM23;
          break;
#endif

        case cmuSelect_FSRCO:
          tmp = CMU_EM01GRPACLKCTRL_CLKSEL_FSRCO;
          break;

#if defined(CMU_EM01GRPACLKCTRL_CLKSEL_HFRCODPLLRT)
        case cmuSelect_HFRCODPLLRT:
          tmp = CMU_EM01GRPACLKCTRL_CLKSEL_HFRCODPLLRT;
          break;
#endif
#if defined(CMU_EM01GRPACLKCTRL_CLKSEL_HFXORT)
        case cmuSelect_HFXORT:
          tmp = CMU_EM01GRPACLKCTRL_CLKSEL_HFXORT;
          break;
#endif
        default:
          EFM_ASSERT(false);
          break;
      }
      CMU->EM01GRPACLKCTRL = (CMU->EM01GRPACLKCTRL
                              & ~_CMU_EM01GRPACLKCTRL_CLKSEL_MASK) | tmp;
      break;

// -----------------------------------------------------------------------------
    case cmuClock_SYSTICK:
      switch (ref) {
        case cmuSelect_EM23GRPACLK:
        case cmuSelect_LFXO:
        case cmuSelect_LFRCO:
        case cmuSelect_ULFRCO:
#if defined(_SILICON_LABS_32B_SERIES_2_CONFIG_1)
          SYSCFG->CFGSYSTIC = (SYSCFG->CFGSYSTIC | _SYSCFG_CFGSYSTIC_SYSTICEXTCLKEN_MASK);
#else
          syscfgClkIsOff = ((CMU->CLKEN0 & CMU_CLKEN0_SYSCFG) == 0);
          CMU->CLKEN0_SET = CMU_CLKEN0_SYSCFG;
          SYSCFG->CFGSYSTIC = (SYSCFG->CFGSYSTIC | _SYSCFG_CFGSYSTIC_SYSTICEXTCLKEN_MASK);
          if (syscfgClkIsOff) {
            CMU->CLKEN0_CLR = CMU_CLKEN0_SYSCFG;
          }
#endif
          SysTick->CTRL = (SysTick->CTRL & ~SysTick_CTRL_CLKSOURCE_Msk);
          break;
        case cmuSelect_HCLK:
#if defined(_SILICON_LABS_32B_SERIES_2_CONFIG_1)
          SYSCFG->CFGSYSTIC = (SYSCFG->CFGSYSTIC & ~_SYSCFG_CFGSYSTIC_SYSTICEXTCLKEN_MASK);
#else
          syscfgClkIsOff = ((CMU->CLKEN0 & CMU_CLKEN0_SYSCFG) == 0);
          CMU->CLKEN0_SET = CMU_CLKEN0_SYSCFG;
          SYSCFG->CFGSYSTIC = (SYSCFG->CFGSYSTIC & ~_SYSCFG_CFGSYSTIC_SYSTICEXTCLKEN_MASK);
          if (syscfgClkIsOff) {
            CMU->CLKEN0_CLR = CMU_CLKEN0_SYSCFG;
          }
#endif
          SysTick->CTRL = (SysTick->CTRL | SysTick_CTRL_CLKSOURCE_Msk);
          break;
        default:
          EFM_ASSERT(false);
          break;
      }
      break;

    case cmuClock_LETIMER0:
    case cmuClock_EM23GRPACLK:
#if defined(LESENSE_PRESENT)
    case cmuClock_LESENSE:
    case cmuClock_LESENSECLK:
#endif
      switch (ref) {
        case cmuSelect_LFRCO:
#if defined(PLFRCO_PRESENT)
        case cmuSelect_PLFRCO:
#endif
          tmp = CMU_EM23GRPACLKCTRL_CLKSEL_LFRCO;
          break;

        case cmuSelect_LFXO:
          tmp = CMU_EM23GRPACLKCTRL_CLKSEL_LFXO;
          break;

        case cmuSelect_ULFRCO:
          tmp = CMU_EM23GRPACLKCTRL_CLKSEL_ULFRCO;
          break;

        default:
          EFM_ASSERT(false);
          break;
      }
      CMU->EM23GRPACLKCTRL = (CMU->EM23GRPACLKCTRL
                              & ~_CMU_EM23GRPACLKCTRL_CLKSEL_MASK) | tmp;
      break;

// -----------------------------------------------------------------------------
    case cmuClock_BURTC:
    case cmuClock_EM4GRPACLK:
      switch (ref) {
        case cmuSelect_LFRCO:
#if defined(PLFRCO_PRESENT)
        case cmuSelect_PLFRCO:
#endif
          tmp = CMU_EM4GRPACLKCTRL_CLKSEL_LFRCO;
          break;

        case cmuSelect_LFXO:
          tmp = CMU_EM4GRPACLKCTRL_CLKSEL_LFXO;
          break;

        case cmuSelect_ULFRCO:
          tmp = CMU_EM4GRPACLKCTRL_CLKSEL_ULFRCO;
          break;

        default:
          EFM_ASSERT(false);
          break;
      }
      CMU->EM4GRPACLKCTRL = (CMU->EM4GRPACLKCTRL
                             & ~_CMU_EM4GRPACLKCTRL_CLKSEL_MASK) | tmp;
      break;

#if defined(_CMU_EM01GRPBCLKCTRL_CLKSEL_MASK)
// -----------------------------------------------------------------------------
    case cmuClock_PDMREF:
    case cmuClock_EM01GRPBCLK:
      switch (ref) {
        case cmuSelect_HFRCODPLL:
          tmp = CMU_EM01GRPBCLKCTRL_CLKSEL_HFRCODPLL;
          break;

        case cmuSelect_HFXO:
          tmp = CMU_EM01GRPBCLKCTRL_CLKSEL_HFXO;
          break;

        case cmuSelect_FSRCO:
          tmp = CMU_EM01GRPBCLKCTRL_CLKSEL_FSRCO;
          break;

        case cmuSelect_CLKIN0:
          tmp = CMU_EM01GRPBCLKCTRL_CLKSEL_CLKIN0;
          break;

        case cmuSelect_HFRCODPLLRT:
          tmp = CMU_EM01GRPBCLKCTRL_CLKSEL_HFRCODPLLRT;
          break;

        case cmuSelect_HFXORT:
          tmp = CMU_EM01GRPBCLKCTRL_CLKSEL_HFXORT;
          break;

        default:
          EFM_ASSERT(false);
          break;
      }
      CMU->EM01GRPBCLKCTRL = (CMU->EM01GRPBCLKCTRL
                              & ~_CMU_EM01GRPBCLKCTRL_CLKSEL_MASK) | tmp;
      break;
#endif

// -----------------------------------------------------------------------------
    case cmuClock_WDOG0:
    case cmuClock_WDOG0CLK:
      switch (ref) {
        case cmuSelect_LFRCO:
#if defined(PLFRCO_PRESENT)
        case cmuSelect_PLFRCO:
#endif
          tmp = CMU_WDOG0CLKCTRL_CLKSEL_LFRCO;
          break;

        case cmuSelect_LFXO:
          tmp = CMU_WDOG0CLKCTRL_CLKSEL_LFXO;
          break;

        case cmuSelect_ULFRCO:
          tmp = CMU_WDOG0CLKCTRL_CLKSEL_ULFRCO;
          break;

        case cmuSelect_HCLKDIV1024:
          tmp = CMU_WDOG0CLKCTRL_CLKSEL_HCLKDIV1024;
          break;

        default:
          EFM_ASSERT(false);
          break;
      }
      CMU->WDOG0CLKCTRL = (CMU->WDOG0CLKCTRL & ~_CMU_WDOG0CLKCTRL_CLKSEL_MASK)
                          | tmp;
      break;

#if defined(_CMU_WDOG1CLKCTRL_CLKSEL_MASK)
// -----------------------------------------------------------------------------
    case cmuClock_WDOG1:
    case cmuClock_WDOG1CLK:
      switch (ref) {
        case cmuSelect_LFRCO:
#if defined(PLFRCO_PRESENT)
        case cmuSelect_PLFRCO:
#endif
          tmp = CMU_WDOG1CLKCTRL_CLKSEL_LFRCO;
          break;

        case cmuSelect_LFXO:
          tmp = CMU_WDOG1CLKCTRL_CLKSEL_LFXO;
          break;

        case cmuSelect_ULFRCO:
          tmp = CMU_WDOG1CLKCTRL_CLKSEL_ULFRCO;
          break;

        case cmuSelect_HCLKDIV1024:
          tmp = CMU_WDOG1CLKCTRL_CLKSEL_HCLKDIV1024;
          break;

        default:
          EFM_ASSERT(false);
          break;
      }
      CMU->WDOG1CLKCTRL = (CMU->WDOG1CLKCTRL & ~_CMU_WDOG1CLKCTRL_CLKSEL_MASK)
                          | tmp;
      break;
#endif

// -----------------------------------------------------------------------------
    case cmuClock_DPLLREFCLK:
      switch (ref) {
        case cmuSelect_HFXO:
          tmp = CMU_DPLLREFCLKCTRL_CLKSEL_HFXO;
          break;

        case cmuSelect_LFXO:
          tmp = CMU_DPLLREFCLKCTRL_CLKSEL_LFXO;
          break;

        case cmuSelect_CLKIN0:
          tmp = CMU_DPLLREFCLKCTRL_CLKSEL_CLKIN0;
          break;

        case cmuSelect_Disabled:
          tmp = CMU_DPLLREFCLKCTRL_CLKSEL_DISABLED;
          break;

        default:
          EFM_ASSERT(false);
          break;
      }
      CMU->DPLLREFCLKCTRL = (CMU->DPLLREFCLKCTRL
                             & ~_CMU_DPLLREFCLKCTRL_CLKSEL_MASK) | tmp;
      break;

#if defined(_CMU_TRACECLKCTRL_CLKSEL_MASK)
// -----------------------------------------------------------------------------
    case cmuClock_TRACECLK:
#if (defined(_SILICON_LABS_32B_SERIES_2_CONFIG_4)     \
      || defined(_SILICON_LABS_32B_SERIES_2_CONFIG_5) \
      && defined(CoreDebug_DEMCR_TRCENA_Msk))
      restoreTrace = CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk;
      if (restoreTrace) {
        CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
      }
#endif
      switch (ref) {
#if defined(_SILICON_LABS_32B_SERIES_2_CONFIG_1)
        case cmuSelect_HCLK:
          tmp = CMU_TRACECLKCTRL_CLKSEL_HCLK;
          break;
#endif

#if defined(_SILICON_LABS_32B_SERIES_2_CONFIG_4) \
        || defined(_SILICON_LABS_32B_SERIES_2_CONFIG_5)
        case cmuSelect_SYSCLK:
          tmp = CMU_TRACECLKCTRL_CLKSEL_SYSCLK;
          break;

        case cmuSelect_HFRCODPLLRT:
          tmp = CMU_TRACECLKCTRL_CLKSEL_HFRCODPLLRT;
          break;
#endif

        case cmuSelect_HFRCOEM23:
          tmp = CMU_TRACECLKCTRL_CLKSEL_HFRCOEM23;
          break;

        default:
          EFM_ASSERT(false);
          break;
      }
      CMU->TRACECLKCTRL = (CMU->TRACECLKCTRL & ~_CMU_TRACECLKCTRL_CLKSEL_MASK)
                          | tmp;
#if (defined(_SILICON_LABS_32B_SERIES_2_CONFIG_4)     \
      || defined(_SILICON_LABS_32B_SERIES_2_CONFIG_5) \
      && defined(CoreDebug_DEMCR_TRCENA_Msk))
      if (restoreTrace) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
      }
#endif
      break;
#endif

#if defined(_CMU_EUART0CLKCTRL_CLKSEL_MASK)
// -----------------------------------------------------------------------------
    case cmuClock_EUART0:
    case cmuClock_EUART0CLK:
      switch (ref) {
        case cmuSelect_EM01GRPACLK:
          tmp = _CMU_EUART0CLKCTRL_CLKSEL_EM01GRPACLK;
          break;

        case cmuSelect_EM23GRPACLK:
          tmp = _CMU_EUART0CLKCTRL_CLKSEL_EM23GRPACLK;
          break;

        default:
          EFM_ASSERT(false);
          break;
      }
      CMU->EUART0CLKCTRL = (CMU->EUART0CLKCTRL & ~_CMU_EUART0CLKCTRL_CLKSEL_MASK)
                           | tmp;
      break;
#elif defined(EUSART_PRESENT)
    case cmuClock_EUSART0:
    case cmuClock_EUSART0CLK:
      switch (ref) {
        case cmuSelect_EM01GRPCCLK:
          tmp = _CMU_EUSART0CLKCTRL_CLKSEL_EM01GRPCCLK;
          break;
        case cmuSelect_HFRCOEM23:
          tmp = _CMU_EUSART0CLKCTRL_CLKSEL_HFRCOEM23;
          break;
        case cmuSelect_LFRCO:
#if defined(PLFRCO_PRESENT)
        case cmuSelect_PLFRCO:
#endif
          tmp = _CMU_EUSART0CLKCTRL_CLKSEL_LFRCO;
          break;
        case cmuSelect_LFXO:
          tmp = _CMU_EUSART0CLKCTRL_CLKSEL_LFXO;
          break;

        default:
          EFM_ASSERT(false);
          break;
      }
      CMU->EUSART0CLKCTRL = (CMU->EUSART0CLKCTRL & ~_CMU_EUSART0CLKCTRL_CLKSEL_MASK)
                            | tmp;
      break;

#if defined(EUSART_PRESENT) && EUSART_COUNT > 1
    case cmuClock_EUSART1:
#endif
#if defined(EUSART_PRESENT) && EUSART_COUNT > 2
    case cmuClock_EUSART2:
#endif
#if defined(EUSART_PRESENT) && EUSART_COUNT > 3
    case cmuClock_EUSART3:
#endif
#if defined(EUSART_PRESENT) && EUSART_COUNT > 4
    case cmuClock_EUSART4:
#endif
    case cmuClock_EM01GRPCCLK:
      switch (ref) {
        case cmuSelect_HFRCODPLL:
          tmp = _CMU_EM01GRPCCLKCTRL_CLKSEL_HFRCODPLL;
          break;
        case cmuSelect_HFRCOEM23:
          tmp = _CMU_EM01GRPCCLKCTRL_CLKSEL_HFRCOEM23;
          break;
        case cmuSelect_FSRCO:
          tmp = _CMU_EM01GRPCCLKCTRL_CLKSEL_FSRCO;
          break;
        case cmuSelect_HFXO:
          tmp = _CMU_EM01GRPCCLKCTRL_CLKSEL_HFXO;
          break;
        default:
          EFM_ASSERT(false);
          break;
      }
      CMU->EM01GRPCCLKCTRL = (CMU->EM01GRPCCLKCTRL & ~_CMU_EM01GRPCCLKCTRL_CLKSEL_MASK)
                             | tmp;
      break;
#endif

// -----------------------------------------------------------------------------
#if defined (RTCC_PRESENT)
    case cmuClock_RTCC:
    case cmuClock_RTCCCLK:
      switch (ref) {
        case cmuSelect_LFRCO:
#if defined(PLFRCO_PRESENT)
        case cmuSelect_PLFRCO:
#endif
          tmp = CMU_RTCCCLKCTRL_CLKSEL_LFRCO;
          break;

        case cmuSelect_LFXO:
          tmp = CMU_RTCCCLKCTRL_CLKSEL_LFXO;
          break;

        case cmuSelect_ULFRCO:
          tmp = CMU_RTCCCLKCTRL_CLKSEL_ULFRCO;
          break;

        default:
          EFM_ASSERT(false);
          break;
      }
      CMU->RTCCCLKCTRL = (CMU->RTCCCLKCTRL & ~_CMU_RTCCCLKCTRL_CLKSEL_MASK)
                         | tmp;
      break;
#endif
// -----------------------------------------------------------------------------
#if defined (SYSRTC_PRESENT)
    case cmuClock_SYSRTC:
    case cmuClock_SYSRTCCLK:
      switch (ref) {
        case cmuSelect_LFRCO:
#if defined(PLFRCO_PRESENT)
        case cmuSelect_PLFRCO:
#endif
          tmp = CMU_SYSRTC0CLKCTRL_CLKSEL_LFRCO;
          break;

        case cmuSelect_LFXO:
          tmp = CMU_SYSRTC0CLKCTRL_CLKSEL_LFXO;
          break;

        case cmuSelect_ULFRCO:
          tmp = CMU_SYSRTC0CLKCTRL_CLKSEL_ULFRCO;
          break;

        default:
          EFM_ASSERT(false);
          break;
      }
      CMU->SYSRTC0CLKCTRL = (CMU->SYSRTC0CLKCTRL & ~_CMU_SYSRTC0CLKCTRL_CLKSEL_MASK)
                            | tmp;
      break;
#endif
// -----------------------------------------------------------------------------
#if defined(LCD_PRESENT)
    case cmuClock_LCD:
    case cmuClock_LCDCLK:
      switch (ref) {
        case cmuSelect_LFRCO:
#if defined(PLFRCO_PRESENT)
        case cmuSelect_PLFRCO:
#endif
          tmp = CMU_LCDCLKCTRL_CLKSEL_LFRCO;
          break;

        case cmuSelect_LFXO:
          tmp = CMU_LCDCLKCTRL_CLKSEL_LFXO;
          break;

        case cmuSelect_ULFRCO:
          tmp = CMU_LCDCLKCTRL_CLKSEL_ULFRCO;
          break;

        default:
          EFM_ASSERT(false);
          break;
      }
      CMU->LCDCLKCTRL =  (CMU->LCDCLKCTRL  & ~_CMU_LCDCLKCTRL_CLKSEL_MASK)
                        | tmp;
      break;
#endif
// -----------------------------------------------------------------------------
#if defined(VDAC_PRESENT)
    case cmuClock_VDAC0:
    case cmuClock_VDAC0CLK:
      switch (ref) {
        case cmuSelect_FSRCO:
          tmp = CMU_VDAC0CLKCTRL_CLKSEL_FSRCO;
          break;
        case cmuSelect_HFRCOEM23:
          tmp = CMU_VDAC0CLKCTRL_CLKSEL_HFRCOEM23;
          break;
        case cmuSelect_EM01GRPACLK:
          tmp = CMU_VDAC0CLKCTRL_CLKSEL_EM01GRPACLK;
          break;
        case cmuSelect_EM23GRPACLK:
          tmp = CMU_VDAC0CLKCTRL_CLKSEL_EM23GRPACLK;
          break;

        default:
          EFM_ASSERT(false);
          break;
      }
      CMU->VDAC0CLKCTRL = (CMU->VDAC0CLKCTRL & ~_CMU_VDAC0CLKCTRL_CLKSEL_MASK)
                          | tmp;
      break;
#if (VDAC_COUNT > 1)
    case cmuClock_VDAC1:
    case cmuClock_VDAC1CLK:
      switch (ref) {
        case cmuSelect_FSRCO:
          tmp = CMU_VDAC1CLKCTRL_CLKSEL_FSRCO;
          break;
        case cmuSelect_HFRCOEM23:
          tmp = CMU_VDAC1CLKCTRL_CLKSEL_HFRCOEM23;
          break;
        case cmuSelect_EM01GRPACLK:
          tmp = CMU_VDAC1CLKCTRL_CLKSEL_EM01GRPACLK;
          break;
        case cmuSelect_EM23GRPACLK:
          tmp = CMU_VDAC1CLKCTRL_CLKSEL_EM23GRPACLK;
          break;

        default:
          EFM_ASSERT(false);
          break;
      }
      CMU->VDAC1CLKCTRL = (CMU->VDAC1CLKCTRL & ~_CMU_VDAC1CLKCTRL_CLKSEL_MASK)
                          | tmp;
      break;
#endif
#endif /* VDAC_PRESENT */
// -----------------------------------------------------------------------------
#if defined(PCNT_PRESENT)
    case cmuClock_PCNT0:
    case cmuClock_PCNT0CLK:
      switch (ref) {
        case cmuSelect_EM23GRPACLK:
          tmp = CMU_PCNT0CLKCTRL_CLKSEL_EM23GRPACLK;
          break;

        case cmuSelect_PCNTEXTCLK:
          tmp = CMU_PCNT0CLKCTRL_CLKSEL_PCNTS0;
          break;

        default:
          EFM_ASSERT(false);
          break;
      }
      CMU->PCNT0CLKCTRL = (CMU->PCNT0CLKCTRL & ~_CMU_PCNT0CLKCTRL_CLKSEL_MASK)
                          | tmp;
      break;
#endif
// -----------------------------------------------------------------------------
#if defined(LESENSE_PRESENT)
    case cmuClock_LESENSEHFCLK:
      switch (ref) {
        case cmuSelect_FSRCO:
          tmp = CMU_LESENSEHFCLKCTRL_CLKSEL_FSRCO;
          break;
        case cmuSelect_HFRCOEM23:
          tmp = CMU_LESENSEHFCLKCTRL_CLKSEL_HFRCOEM23;
          break;

        default:
          EFM_ASSERT(false);
          break;
      }
      CMU->LESENSEHFCLKCTRL = (CMU->LESENSEHFCLKCTRL & ~_CMU_LESENSEHFCLKCTRL_CLKSEL_MASK)
                              | tmp;
      break;
#endif
// -----------------------------------------------------------------------------
#if defined(USB_PRESENT)
    case cmuClock_USB:
      switch (ref) {
        case cmuSelect_PLL0:
          tmp = CMU_USB0CLKCTRL_CLKSEL_PLL0;
          break;
        case cmuSelect_LFXO:
          tmp = CMU_USB0CLKCTRL_CLKSEL_LFXO;
          break;
        case cmuSelect_LFRCO:
          tmp = CMU_USB0CLKCTRL_CLKSEL_LFRCO;
          break;

        default:
          EFM_ASSERT(false);
          break;
      }
      CMU->USB0CLKCTRL = (CMU->USB0CLKCTRL & ~_CMU_USB0CLKCTRL_CLKSEL_MASK)
                         | tmp;
      break;
#endif
// -----------------------------------------------------------------------------
    default:
      EFM_ASSERT(false);
      break;
  }
}

/***************************************************************************//**
 * @brief
 *   Gets the precision (in PPM) of the specified low frequency clock branch.
 *
 * @param[in] clock
 *   Clock branch.
 *
 * @return
 *   Precision, in PPM, of the specified clock branch.
 *
 * @note
 *   This function is only for internal usage.
 *
 * @note
 *   The current implementation of this function is used to determine if the
 *   clock has a precision <= 500 ppm or not (which is the minimum required
 *   for BLE). Future version of this function should provide more accurate
 *   precision numbers to allow for further optimizations from the stacks.
 ******************************************************************************/
uint16_t CMU_LF_ClockPrecisionGet(CMU_Clock_TypeDef clock)
{
  CMU_Select_TypeDef src = CMU_ClockSelectGet(clock);
  uint16_t precision;

  switch (src) {
    case cmuSelect_LFXO:
      precision = lfxo_precision;
      break;

#if defined(PLFRCO_PRESENT)
#if defined(_SILICON_LABS_32B_SERIES_1)
    case cmuSelect_PLFRCO:
      precision = 500;
      break;
#endif

#if defined(LFRCO_CFG_HIGHPRECEN)
    case cmuSelect_LFRCO:
    case cmuSelect_PLFRCO:

      CMU->CLKEN0_SET = CMU_CLKEN0_LFRCO;

      if (LFRCO->CFG & _LFRCO_CFG_HIGHPRECEN_MASK) {
        precision = 500;
      } else {
        precision = 0xFFFF;
      }
      break;
#endif
#endif

    default:
      precision = 0xFFFF;
      break;
  }

  return precision;
}

/***************************************************************************//**
 * @brief
 *   Get HFRCODPLL band in use.
 *
 * @return
 *   HFRCODPLL band in use.
 ******************************************************************************/
CMU_HFRCODPLLFreq_TypeDef CMU_HFRCODPLLBandGet(void)
{
  return (CMU_HFRCODPLLFreq_TypeDef)SystemHFRCODPLLClockGet();
}

/***************************************************************************//**
 * @brief
 *   Set HFRCODPLL band and the tuning value based on the value in the
 *   calibration table made during production.
 *
 * @param[in] freq
 *   HFRCODPLL frequency band to activate.
 ******************************************************************************/
void CMU_HFRCODPLLBandSet(CMU_HFRCODPLLFreq_TypeDef freq)
{
  uint32_t hfrcoFreqRangeExpected;
  uint32_t hfrcoFreqRangeActual;
  uint32_t hfrcoCalCurrent;
  uint32_t freqCal, sysFreq;
#if defined(EMU_VSCALE_EM01_PRESENT)
  uint32_t prevFreq;
#endif

  // Get calibration data from DEVINFO
  freqCal = HFRCODPLLDevinfoGet(freq);
  EFM_ASSERT((freqCal != 0UL) && (freqCal != UINT_MAX));

#if defined(CMU_CLKEN0_DPLL0)
  CMU->CLKEN0_SET = CMU_CLKEN0_DPLL0 | CMU_CLKEN0_HFRCO0;
#endif

  // Make sure DPLL is disabled before configuring
  if (DPLL0->EN == DPLL_EN_EN) {
    DPLL0->EN_CLR = DPLL_EN_EN;
#if defined(DPLL_EN_DISABLING)
    while (DPLL0->EN & DPLL_EN_DISABLING) {
    }
#else
    while ((DPLL0->STATUS & (DPLL_STATUS_ENS | DPLL_STATUS_RDY)) != 0UL) {
    }
#endif
  }

  // Set max wait-states and PCLK divisor while changing core clock
  if (CMU_ClockSelectGet(cmuClock_SYSCLK) == cmuSelect_HFRCODPLL) {
    waitStateMax();
    pclkDivMax();
#if ((defined(CMU_SYSCLKCTRL_RHCLKPRESC)) \
    && (_SILICON_LABS_EFR32_RADIO_TYPE != _SILICON_LABS_EFR32_RADIO_NONE))
    // Set largest prescaler for radio clock tree
    rhclkPrescMax();
#endif
  }

  // Set divider for 1, 2 and 4MHz bands
  freqCal &= ~_HFRCO_CAL_CLKDIV_MASK;
  switch (freq) {
    case cmuHFRCODPLLFreq_1M0Hz:
      freqCal |= HFRCO_CAL_CLKDIV_DIV4;
      break;

    case cmuHFRCODPLLFreq_2M0Hz:
      freqCal |= HFRCO_CAL_CLKDIV_DIV2;
      break;

    default:
      break;
  }

#if defined(EMU_VSCALE_EM01_PRESENT)
  prevFreq = SystemHFRCODPLLClockGet();

  if ((uint32_t)freq > prevFreq) {
    /* When increasing frequency voltage scale must be done before the change. */
    EMU_VScaleEM01ByClock((uint32_t)freq, true);
  }
#endif

  // updates to the CAL register are deferred if FREQBSY is high, so wait
  // until HFRCO is not busy to keep going
  while (HFRCO0->STATUS & (HFRCO_STATUS_SYNCBUSY | HFRCO_STATUS_FREQBSY)) {
  }

  /*
   * Some devices have clamped frequency ranges, so instead of the usual [0:16]
   * interval, the upper limit is 12. Hardware takes care of clamping the value,
   * but a situation might occur where tuning and frequency range are not
   * in sync. So try to detect if the value has been clamped, and if it happened
   * revert back to the previous value.
   */
  hfrcoCalCurrent = HFRCO0->CAL;
  HFRCO0->CAL = freqCal;

  // values are not shifted, not necessary for comparison
  hfrcoFreqRangeExpected = (freqCal     & _HFRCO_CAL_FREQRANGE_MASK);
  hfrcoFreqRangeActual   = (HFRCO0->CAL & _HFRCO_CAL_FREQRANGE_MASK);

  EFM_ASSERT(hfrcoFreqRangeExpected == hfrcoFreqRangeActual);
  if (hfrcoFreqRangeExpected == hfrcoFreqRangeActual) {
    // Update CMSIS HFRCODPLL frequency.
    SystemHFRCODPLLClockSet(freq);
  } else {
    // revert back to previous value
    HFRCO0->CAL = hfrcoCalCurrent;
#if defined(EMU_VSCALE_EM01_PRESENT)
    freq = (CMU_HFRCODPLLFreq_TypeDef)prevFreq;
#endif
  }

  // If HFRCODPLL is selected as SYSCLK (and HCLK), optimize flash access
  // wait-state configuration and PCLK divisor for this frequency.
  if (CMU_ClockSelectGet(cmuClock_SYSCLK) == cmuSelect_HFRCODPLL) {
    // Call @ref SystemCoreClockGet() to update CMSIS core clock variable.
    sysFreq = SystemCoreClockGet();
    EFM_ASSERT(sysFreq <= (uint32_t)freq);
    CMU_UpdateWaitStates(sysFreq, VSCALE_DEFAULT);
    pclkDivOptimize();
#if (defined(CMU_SYSCLKCTRL_RHCLKPRESC) \
    && (_SILICON_LABS_EFR32_RADIO_TYPE != _SILICON_LABS_EFR32_RADIO_NONE))
    // Set optimal RHCLK prescaler
    rhclkPrescOptimize();
#endif
  }

#if defined(EMU_VSCALE_EM01_PRESENT)
  if ((uint32_t)freq <= prevFreq) {
    /* When decreasing frequency voltage scale must be done after the change. */
    EMU_VScaleEM01ByClock(0, true);
  }
#endif
}

/**************************************************************************//**
 * @brief
 *   Lock the DPLL to a given frequency.
 *   The frequency is given by: Fout = Fref * (N+1) / (M+1).
 *
 * @note
 *   This function does not check if the given N & M values will actually
 *   produce the desired target frequency. @n
 *   N & M limitations: @n
 *     300 < N <= 4095 @n
 *     0 <= M <= 4095 @n
 *   Any peripheral running off HFRCODPLL should be switched to a lower
 *   frequency clock (if possible) prior to calling this function to avoid
 *   over-clocking.
 *
 * @param[in] init
 *    DPLL setup parameter struct.
 *
 * @return
 *   Returns false on invalid target frequency or DPLL locking error.
 *****************************************************************************/
bool CMU_DPLLLock(const CMU_DPLLInit_TypeDef *init)
{
  int index = 0;
  unsigned int i;
  bool hclkDivIncreased = false;
  uint32_t hfrcoCalVal, lockStatus = 0, hclkDiv = 0, sysFreq;
  uint32_t hfrcoFreqRangeExpected;
  uint32_t hfrcoFreqRangeActual;
  uint32_t hfrcoCalCurrent;
  bool hfrcoClamped = false;
  bool restoreDpll;

#if defined(CMU_CLKEN0_DPLL0)
  CMU->CLKEN0_SET = CMU_CLKEN0_DPLL0 | CMU_CLKEN0_HFRCO0;
#endif

  restoreDpll = DPLL0->EN & _DPLL_EN_EN_MASK;

  // Make sure DPLL is disabled before configuring
  DPLL0->EN_CLR = DPLL_EN_EN;
#if defined(DPLL_EN_DISABLING)
  while (DPLL0->EN & DPLL_EN_DISABLING) {
  }
#else
  while ((DPLL0->STATUS & (DPLL_STATUS_ENS | DPLL_STATUS_RDY)) != 0UL) {
  }
#endif
  EFM_ASSERT(init->frequency >= hfrcoCalTable[0].minFreq);
  EFM_ASSERT(init->frequency
             <= hfrcoCalTable[HFRCOCALTABLE_ENTRIES - 1U].maxFreq);

  EFM_ASSERT(init->n > 300U);
  EFM_ASSERT(init->n <= (_DPLL_CFG1_N_MASK >> _DPLL_CFG1_N_SHIFT));
  EFM_ASSERT(init->m <= (_DPLL_CFG1_M_MASK >> _DPLL_CFG1_M_SHIFT));

#if defined(EMU_VSCALE_EM01_PRESENT)
  if ((EMU_VScaleGet() == emuVScaleEM01_LowPower)
      && (init->frequency > CMU_VSCALEEM01_LOWPOWER_VOLTAGE_CLOCK_MAX)) {
    EFM_ASSERT(false);
    return false;
  }
#endif

  // Find correct HFRCODPLL band, and retrieve a HFRCOCAL value.
  for (i = 0; i < HFRCOCALTABLE_ENTRIES; i++) {
    if ((init->frequency    >= hfrcoCalTable[i].minFreq)
        && (init->frequency <= hfrcoCalTable[i].maxFreq)) {
      index = (int)i;                       // Correct band found
      break;
    }
  }
  if ((uint32_t)index == HFRCOCALTABLE_ENTRIES) {
    EFM_ASSERT(false);
    return false;                           // Target frequency out of spec.
  }
  hfrcoCalVal = hfrcoCalTable[index].value;

  // Check if a calibrated HFRCOCAL.TUNING value is present in device DI page.
  if (hfrcoCalTable[index].band != (CMU_HFRCODPLLFreq_TypeDef)0) {
    uint32_t tuning;

    tuning = (HFRCODPLLDevinfoGet(hfrcoCalTable[index].band)
              & _HFRCO_CAL_TUNING_MASK)
             >> _HFRCO_CAL_TUNING_SHIFT;
    hfrcoCalVal |= tuning << _HFRCO_CAL_TUNING_SHIFT;
  }

  if (CMU_ClockSelectGet(cmuClock_SYSCLK) == cmuSelect_HFRCODPLL) {
    // Set max wait-states and PCLK divisor while changing core clock
    waitStateMax();
    pclkDivMax();
#if ((defined(CMU_SYSCLKCTRL_RHCLKPRESC)) \
    && (_SILICON_LABS_EFR32_RADIO_TYPE != _SILICON_LABS_EFR32_RADIO_NONE))
    // Set largest prescaler for radio clock tree
    rhclkPrescMax();
#endif

    // Increase HCLK divider value (if possible) while locking DPLL to
    // avoid over-clocking.
    hclkDiv = CMU_ClockDivGet(cmuClock_HCLK);
    hclkDivIncreased = true;
    if (hclkDiv == 1U) {
      CMU_ClockDivSet(cmuClock_HCLK, 2U);
    } else if (hclkDiv == 2U) {
      CMU_ClockDivSet(cmuClock_HCLK, 4U);
    } else {
      hclkDivIncreased = false;
    }
  }

  // updates to the CAL register are deferred if FREQBSY is high, so wait
  // until HFRCO is not busy to keep going
  while (HFRCO0->STATUS & (HFRCO_STATUS_SYNCBUSY | HFRCO_STATUS_FREQBSY)) {
  }

  /*
   * Some devices have clamped frequency ranges, so instead of the usual [0:16]
   * interval, the upper limit is 12. Hardware takes care of clamping the value,
   * but a situation might occur where tuning and frequency range are not
   * in sync. So try to detect if the value has been clamped, and if it happened
   * revert back to the previous value.
   */
  hfrcoCalCurrent = HFRCO0->CAL;
  HFRCO0->CAL = hfrcoCalVal;

  // values are not shifted, not necessary for comparison
  hfrcoFreqRangeExpected = (hfrcoCalVal & _HFRCO_CAL_FREQRANGE_MASK);
  hfrcoFreqRangeActual   = (HFRCO0->CAL & _HFRCO_CAL_FREQRANGE_MASK);

  EFM_ASSERT(hfrcoFreqRangeExpected == hfrcoFreqRangeActual);
  if (hfrcoFreqRangeExpected == hfrcoFreqRangeActual) {
    DPLL0->CFG1   = ((uint32_t)init->n   << _DPLL_CFG1_N_SHIFT)
                    | ((uint32_t)init->m << _DPLL_CFG1_M_SHIFT);
    CMU_ClockSelectSet(cmuClock_DPLLREFCLK, init->refClk);
    DPLL0->CFG = ((init->autoRecover ? 1UL : 0UL) << _DPLL_CFG_AUTORECOVER_SHIFT)
                 | ((init->ditherEn ? 1UL : 0UL)  << _DPLL_CFG_DITHEN_SHIFT)
                 | ((uint32_t)init->edgeSel  << _DPLL_CFG_EDGESEL_SHIFT)
                 | ((uint32_t)init->lockMode << _DPLL_CFG_MODE_SHIFT);

    // Update CMSIS HFRCODPLL frequency.
    SystemHFRCODPLLClockSet(init->frequency);
  } else {
    hfrcoClamped = true;
    HFRCO0->CAL = hfrcoCalCurrent;
  }

  /*
   * if HFRCO frequency range has been clamped, re-enable DPLL only if it was
   * previously enabled
   */
  if (!hfrcoClamped || restoreDpll) {
    DPLL0->IF_CLR = DPLL_IF_LOCK | DPLL_IF_LOCKFAILLOW | DPLL_IF_LOCKFAILHIGH;

    // Lock DPLL
    DPLL0->EN_SET = DPLL_EN_EN;
    while ((lockStatus = (DPLL0->IF & (DPLL_IF_LOCK
                                       | DPLL_IF_LOCKFAILLOW
                                       | DPLL_IF_LOCKFAILHIGH))) == 0UL) {
    }
  }

  if (CMU_ClockSelectGet(cmuClock_SYSCLK) == cmuSelect_HFRCODPLL) {
    if (hclkDivIncreased) {
      // Restore original HCLK divider
      CMU_ClockDivSet(cmuClock_HCLK, hclkDiv);
    }

    // Call @ref SystemCoreClockGet() to update CMSIS core clock variable.
    sysFreq = SystemCoreClockGet();
    EFM_ASSERT(sysFreq <= init->frequency);
    EFM_ASSERT(sysFreq <= SystemHFRCODPLLClockGet());
    EFM_ASSERT(init->frequency == SystemHFRCODPLLClockGet());

    // Set optimal wait-states and PCLK divisor
    CMU_UpdateWaitStates(sysFreq, VSCALE_DEFAULT);
    pclkDivOptimize();
#if (defined(CMU_SYSCLKCTRL_RHCLKPRESC) \
    && (_SILICON_LABS_EFR32_RADIO_TYPE != _SILICON_LABS_EFR32_RADIO_NONE))
    // Set optimal RHCLK prescaler
    rhclkPrescOptimize();
#endif
  }

  if (hfrcoClamped) {
    return false;
  } else if (lockStatus == DPLL_IF_LOCK) {
    return true;
  }
  return false;
}

#if defined(USBPLL_PRESENT)
/***************************************************************************//**
 * @brief
 *   Initialize the PLL control registers.
 *
 * @note
 *  The HFXO reference frequency must be updated if crystal value is
 *  different from default value.
 *
 * @param[in] pllInit
 *   USB PLL parameters
 ******************************************************************************/
void CMU_USBPLLInit(const CMU_PLL_Init_TypeDef *pllInit)
{
  CMU->CLKEN1_SET = CMU_CLKEN1_USB;

  USBPLL0->LOCK = PLL_LOCK_LOCKKEY_UNLOCK;

  // Stop the PLL for configuration purposes
  USBPLL0->CTRL_SET = PLL_CTRL_DISONDEMAND;
  USBPLL0->CTRL_CLR = PLL_CTRL_FORCEEN;

  while (USBPLL0->STATUS & PLL_STATUS_PLLLOCK) ;

  if (pllInit->hfxoRefFreq == cmuHFXORefFreq_39M0Hz) {
    // Set DCO in low frequency mode for 39 MHz crystal.
    USBPLL0->DCOCTRL_SET = _PLL_DCOCTRL_DCOBIASHALF_MASK;
  } else {
    USBPLL0->DCOCTRL_CLR = _PLL_DCOCTRL_DCOBIASHALF_MASK;
  }

  while (USBPLL0->STATUS & PLL_STATUS_SYNCBUSY) ;

  USBPLL0->CTRL = (USBPLL0->CTRL & ~(_PLL_CTRL_SHUNTREGLPEN_MASK
                                     | _PLL_CTRL_DIVR_MASK
                                     | _PLL_CTRL_DIVX_MASK
                                     | _PLL_CTRL_DIVN_MASK
                                     | _PLL_CTRL_DISONDEMAND_MASK
                                     | _PLL_CTRL_FORCEEN_MASK))
                  | pllInit->hfxoRefFreq
                  | pllInit->shuntRegEn  << _PLL_CTRL_SHUNTREGLPEN_SHIFT
                  | pllInit->disOnDemand << _PLL_CTRL_DISONDEMAND_SHIFT
                  | pllInit->forceEn     << _PLL_CTRL_FORCEEN_SHIFT;

  while (USBPLL0->STATUS & PLL_STATUS_SYNCBUSY) ;

  if (pllInit->forceEn) {
    CMU_WaitUSBPLLLock();
  }

  if (pllInit->regLock) {
    USBPLL0->LOCK = ~PLL_LOCK_LOCKKEY_UNLOCK;
  }
}
#endif

#if defined(RFFPLL_PRESENT)
/***************************************************************************//**
 * @brief
 *   Initialize the RFFPLL control registers.
 *
 * @param[in] pllInit
 *   RFF PLL parameters
 ******************************************************************************/
void CMU_RFFPLLInit(const CMU_RFFPLL_Init_TypeDef *pllInit)
{
  EFM_ASSERT(CMU_ClockSelectGet(cmuClock_SYSCLK) != cmuSelect_RFFPLLSYS);
  EFM_ASSERT(pllInit->dividerY >= 8 && pllInit->dividerY <= 31);
  EFM_ASSERT(pllInit->dividerX >= 4 && pllInit->dividerX <= 15);
  EFM_ASSERT(pllInit->dividerN >= 32 && pllInit->dividerN <= 127);

  CMU->CLKEN1_SET = CMU_CLKEN1_RFFPLL0;

  RFFPLL0->LOCK = RFFPLL_LOCK_LOCKKEY_UNLOCK;

  RFFPLL0->CTRL = (RFFPLL0->CTRL & ~(_RFFPLL_CTRL_DISONDEMAND_MASK | _RFFPLL_CTRL_FORCEEN_MASK))
                  | (pllInit->disOnDemand << _RFFPLL_CTRL_DISONDEMAND_SHIFT)
                  | (pllInit->forceEn     << _RFFPLL_CTRL_FORCEEN_SHIFT);

  RFFPLL0->RFFPLLCTRL1 = (RFFPLL0->RFFPLLCTRL1 & ~(_RFFPLL_RFFPLLCTRL1_DIVY_MASK | _RFFPLL_RFFPLLCTRL1_DIVX_MASK | _RFFPLL_RFFPLLCTRL1_DIVN_MASK))
                         | (pllInit->dividerY << _RFFPLL_RFFPLLCTRL1_DIVY_SHIFT)
                         | (pllInit->dividerX << _RFFPLL_RFFPLLCTRL1_DIVX_SHIFT)
                         | (pllInit->dividerN << _RFFPLL_RFFPLLCTRL1_DIVN_SHIFT);

  // Update CMSIS RFFDPLL frequency.
  SystemRFFPLLClockSet(pllInit->frequency);

  if (pllInit->forceEn) {
    CMU_WaitRFFPLLLock();
  }

  if (pllInit->regLock) {
    RFFPLL0->LOCK = ~PLL_LOCK_LOCKKEY_UNLOCK;
  }
}
#endif

/**************************************************************************//**
 * @brief
 *   Initialize all HFXO control registers.
 *
 * @note
 *   HFXO configuration should be obtained from a configuration tool,
 *   app note or crystal datasheet. This function returns early if HFXO is
 *   already selected as SYSCLK.
 *
 * @param[in] hfxoInit
 *    HFXO setup parameters.
 *****************************************************************************/
void CMU_HFXOInit(const CMU_HFXOInit_TypeDef *hfxoInit)
{
  // Check all initialization structure members which may overflow target
  // bitfield.
  EFM_ASSERT(hfxoInit->timeoutCbLsb
             <= (_HFXO_XTALCFG_TIMEOUTCBLSB_MASK
                 >> _HFXO_XTALCFG_TIMEOUTCBLSB_SHIFT));
  EFM_ASSERT(hfxoInit->timeoutSteadyFirstLock
             <= (_HFXO_XTALCFG_TIMEOUTSTEADY_MASK
                 >> _HFXO_XTALCFG_TIMEOUTSTEADY_SHIFT));
  EFM_ASSERT(hfxoInit->timeoutSteady
             <= (_HFXO_XTALCFG_TIMEOUTSTEADY_MASK
                 >> _HFXO_XTALCFG_TIMEOUTSTEADY_SHIFT));
  EFM_ASSERT(hfxoInit->ctuneXoStartup
             <= (_HFXO_XTALCFG_CTUNEXOSTARTUP_MASK
                 >> _HFXO_XTALCFG_CTUNEXOSTARTUP_SHIFT));
  EFM_ASSERT(hfxoInit->ctuneXiStartup
             <= (_HFXO_XTALCFG_CTUNEXISTARTUP_MASK
                 >> _HFXO_XTALCFG_CTUNEXISTARTUP_SHIFT));
  EFM_ASSERT(hfxoInit->coreBiasStartup
             <= (_HFXO_XTALCFG_COREBIASSTARTUP_MASK
                 >> _HFXO_XTALCFG_COREBIASSTARTUP_SHIFT));
  EFM_ASSERT(hfxoInit->imCoreBiasStartup
             <= (_HFXO_XTALCFG_COREBIASSTARTUPI_MASK
                 >> _HFXO_XTALCFG_COREBIASSTARTUPI_SHIFT));
  EFM_ASSERT(hfxoInit->coreDegenAna
             <= (_HFXO_XTALCTRL_COREDGENANA_MASK
                 >> _HFXO_XTALCTRL_COREDGENANA_SHIFT));
  EFM_ASSERT(hfxoInit->ctuneFixAna
             <= (_HFXO_XTALCTRL_CTUNEFIXANA_MASK
                 >> _HFXO_XTALCTRL_CTUNEFIXANA_SHIFT));
  EFM_ASSERT(hfxoInit->mode
             <= (_HFXO_CFG_MODE_MASK >> _HFXO_CFG_MODE_SHIFT));

  // Return early if HFXO is already selected as SYSCLK.
  if (CMU_ClockSelectGet(cmuClock_SYSCLK) == cmuSelect_HFXO) {
    return;
  }

#if defined(CMU_CLKEN0_HFXO0)
  // Enable HFXO module clock.
  CMU->CLKEN0_SET = CMU_CLKEN0_HFXO0;
#endif

  // Unlock register interface.
  HFXO0->LOCK = HFXO_LOCK_LOCKKEY_UNLOCK;

  // Disable HFXO.
  HFXO0->CTRL_SET = HFXO_CTRL_DISONDEMAND
#if defined(_HFXO_CTRL_DISONDEMANDBUFOUT_MASK)
                    | HFXO_CTRL_DISONDEMANDBUFOUT
#endif
  ;
  HFXO0->CTRL_CLR = HFXO_CTRL_FORCEEN;
  while ((HFXO0->STATUS & _HFXO_STATUS_ENS_MASK) != 0U) {
  }

#if defined(_DEVINFO_HFXOCAL_VTRTRIMANA_MASK) && defined(_HFXO_BUFOUTTRIM_MASK) && defined(_HFXO_SWRST_MASK)
  {
    uint32_t tmp;

    tmp = BUS_RegMaskedRead(&DEVINFO->HFXOCAL,
                            _DEVINFO_HFXOCAL_VTRTRIMANA_MASK);
    tmp >>= _DEVINFO_HFXOCAL_VTRTRIMANA_SHIFT;
    BUS_RegMaskedWrite(&HFXO0->BUFOUTTRIM,
                       _HFXO_BUFOUTTRIM_VTRTRIMANA_MASK,
                       tmp << _HFXO_BUFOUTTRIM_VTRTRIMANA_SHIFT);
  }
#endif

#if defined(_DEVINFO_HFXOCAL_SHUNTBIASANA_MASK) && defined(_HFXO_LOWPWRCTRL_MASK) && defined(_HFXO_SWRST_MASK)
  {
    uint32_t tmp;

    tmp = BUS_RegMaskedRead(&DEVINFO->HFXOCAL,
                            _DEVINFO_HFXOCAL_SHUNTBIASANA_MASK);
    tmp >>= _DEVINFO_HFXOCAL_SHUNTBIASANA_SHIFT;
    BUS_RegMaskedWrite(&HFXO0->LOWPWRCTRL,
                       _HFXO_LOWPWRCTRL_SHUNTBIASANA_MASK,
                       tmp << _HFXO_LOWPWRCTRL_SHUNTBIASANA_SHIFT);
  }
#endif

  // Configure HFXO as specified in initialization struct, use
  // timeoutSteadyFirstLock as TIMEOUTSTEADY value.
  HFXO0->XTALCFG =
    (uint32_t)((hfxoInit->timeoutCbLsb           << _HFXO_XTALCFG_TIMEOUTCBLSB_SHIFT)
               | (hfxoInit->timeoutSteadyFirstLock << _HFXO_XTALCFG_TIMEOUTSTEADY_SHIFT)
               | (hfxoInit->ctuneXoStartup         << _HFXO_XTALCFG_CTUNEXOSTARTUP_SHIFT)
               | (hfxoInit->ctuneXiStartup         << _HFXO_XTALCFG_CTUNEXISTARTUP_SHIFT)
               | (hfxoInit->coreBiasStartup        << _HFXO_XTALCFG_COREBIASSTARTUP_SHIFT)
               | (hfxoInit->imCoreBiasStartup      << _HFXO_XTALCFG_COREBIASSTARTUPI_SHIFT));

  HFXO0->XTALCTRL = (HFXO0->XTALCTRL & _HFXO_XTALCTRL_SKIPCOREBIASOPT_MASK)
                    | (hfxoInit->coreDegenAna    << _HFXO_XTALCTRL_COREDGENANA_SHIFT)
                    | (hfxoInit->ctuneFixAna     << _HFXO_XTALCTRL_CTUNEFIXANA_SHIFT)
                    | (hfxoInit->ctuneXoAna      << _HFXO_XTALCTRL_CTUNEXOANA_SHIFT)
                    | (hfxoInit->ctuneXiAna      << _HFXO_XTALCTRL_CTUNEXIANA_SHIFT)
                    | (hfxoInit->coreBiasAna     << _HFXO_XTALCTRL_COREBIASANA_SHIFT);

#if defined(_SILICON_LABS_32B_SERIES_2_CONFIG_3)
  // See [PM-2871] for details.
  BUS_RegMaskedWrite((volatile uint32_t*)(HFXO0_BASE + 0x38U),
                     0x00000C00UL,
                     0x00000002UL << 10);
#endif

  HFXO0->CFG = (HFXO0->CFG & ~(_HFXO_CFG_SQBUFSCHTRGANA_MASK
                               | _HFXO_CFG_ENXIDCBIASANA_MASK
                               | _HFXO_CFG_MODE_MASK))
               | ((hfxoInit->mode == cmuHfxoOscMode_Crystal)
                  ? 0 : HFXO_CFG_SQBUFSCHTRGANA)
               | (hfxoInit->enXiDcBiasAna    << _HFXO_CFG_ENXIDCBIASANA_SHIFT)
               | (hfxoInit->mode             << _HFXO_CFG_MODE_SHIFT);

  if (hfxoInit->mode == cmuHfxoOscMode_Crystal) {
    // Lock HFXO with FORCEEN bit set and DISONDEMAND bit cleared.
    HFXO0->CTRL = (HFXO0->CTRL & ~(_HFXO_CTRL_FORCEXO2GNDANA_MASK
                                   | _HFXO_CTRL_FORCEXI2GNDANA_MASK
                                   | _HFXO_CTRL_DISONDEMAND_MASK
#if defined(HFXO_CTRL_EM23ONDEMAND)
                                   | _HFXO_CTRL_EM23ONDEMAND_MASK
#endif
                                   | _HFXO_CTRL_FORCEEN_MASK))
                  | (hfxoInit->forceXo2GndAna << _HFXO_CTRL_FORCEXO2GNDANA_SHIFT)
                  | (hfxoInit->forceXi2GndAna << _HFXO_CTRL_FORCEXI2GNDANA_SHIFT)
#if defined(HFXO_CTRL_EM23ONDEMAND)
                  | (hfxoInit->em23OnDemand   << _HFXO_CTRL_EM23ONDEMAND_SHIFT)
#endif
                  | HFXO_CTRL_FORCEEN;

    // Wait for HFXO lock and core bias algorithm to complete.
#if defined(HFXO_STATUS_FSMLOCK)
    while ((HFXO0->STATUS & (HFXO_STATUS_RDY | HFXO_STATUS_COREBIASOPTRDY
                             | HFXO_STATUS_ENS | HFXO_STATUS_FSMLOCK))
           != (HFXO_STATUS_RDY | HFXO_STATUS_COREBIASOPTRDY | HFXO_STATUS_ENS
               | HFXO_STATUS_FSMLOCK)) {
    }
#else
    while ((HFXO0->STATUS & (HFXO_STATUS_RDY | HFXO_STATUS_COREBIASOPTRDY
                             | HFXO_STATUS_ENS))
           != (HFXO_STATUS_RDY | HFXO_STATUS_COREBIASOPTRDY | HFXO_STATUS_ENS)) {
    }
#endif
    // Set DISONDEMAND to be able to enter new values for use on subsequent locks.
    HFXO0->CTRL_SET = HFXO_CTRL_DISONDEMAND;
#if defined(HFXO_CMD_MANUALOVERRIDE)
    HFXO0->CMD = HFXO_CMD_MANUALOVERRIDE;
#endif
#if defined(HFXO_STATUS_FSMLOCK)
    while ((HFXO0->STATUS & HFXO_STATUS_FSMLOCK) != 0) {
    }
#endif
    // Set new TIMEOUTSTEADY value for use on subsequent locks.
    HFXO0->XTALCFG = (HFXO0->XTALCFG & ~_HFXO_XTALCFG_TIMEOUTSTEADY_MASK)
                     | (hfxoInit->timeoutSteady
                        << _HFXO_XTALCFG_TIMEOUTSTEADY_SHIFT);

    // Skip core bias algorithm on subsequent locks.
    HFXO0->XTALCTRL_SET = HFXO_XTALCTRL_SKIPCOREBIASOPT;

    if (hfxoInit->disOnDemand == false) {
      HFXO0->CTRL_CLR = HFXO_CTRL_DISONDEMAND;
    }

    if (hfxoInit->forceEn == false) {
      HFXO0->CTRL_CLR = HFXO_CTRL_FORCEEN;
    }
  } else {
    // Lock HFXO in EXTERNAL SINE mode.

#if defined(_SILICON_LABS_32B_SERIES_2_CONFIG_3) || defined(_SILICON_LABS_32B_SERIES_2_CONFIG_4)
    //See [PM-3665] for details.
    if (hfxoInit->mode == cmuHfxoOscMode_ExternalSinePkDet) {
      HFXO0->XTALCFG = 0;
      HFXO0->XTALCTRL = 0;

      const uint32_t PKDETTHSTARTUP_PARAMETER_1 = 2UL;
      BUS_RegMaskedWrite((volatile uint32_t *)(HFXO0_BASE + 0x34UL),
                         0x0000F000UL | 0x00000F00UL,
                         (PKDETTHSTARTUP_PARAMETER_1 << 12) | (PKDETTHSTARTUP_PARAMETER_1 << 8));
    }
#endif

    HFXO0->CTRL = (HFXO0->CTRL & ~(_HFXO_CTRL_FORCEXO2GNDANA_MASK
                                   | _HFXO_CTRL_FORCEXI2GNDANA_MASK
                                   | _HFXO_CTRL_DISONDEMAND_MASK
#if defined(HFXO_CTRL_EM23ONDEMAND)
                                   | _HFXO_CTRL_EM23ONDEMAND_MASK
#endif
                                   | _HFXO_CTRL_FORCEEN_MASK))
                  | (hfxoInit->forceXo2GndAna << _HFXO_CTRL_FORCEXO2GNDANA_SHIFT)
                  | (hfxoInit->disOnDemand    << _HFXO_CTRL_DISONDEMAND_SHIFT)
#if defined(HFXO_CTRL_EM23ONDEMAND)
                  | (hfxoInit->em23OnDemand   << _HFXO_CTRL_EM23ONDEMAND_SHIFT)
#endif
                  | (hfxoInit->forceEn        << _HFXO_CTRL_FORCEEN_SHIFT);
  }

  if (hfxoInit->regLock) {
    HFXO0->LOCK = ~HFXO_LOCK_LOCKKEY_UNLOCK;
  }
}

#if defined(HFXO0_BUFOUT)
/**************************************************************************//**
 * @brief
 *   Initialize HFXO Bufout (Crystal sharing) leader control registers.
 *   Configure the bufout request input GPIO as a clock request signal
 *   to add the crystal sharing follower chip as a source of clock request.
 *
 * @warning
 *   If EM2 capabilities are needed, a GPIO that fully retains its
 *   capabilities while in EM2 must be selected.
 *
 * @param[in] bufoutInit
 *   Bufout setup parameters.
 *
 * @param[in] port
 *   Bufout request GPIO port.
 *
 * @param[in] pin
 *   Bufout request GPIO pin.
 *****************************************************************************/
void CMU_HFXOStartCrystalSharingLeader(const CMU_BUFOUTLeaderInit_TypeDef *bufoutInit,
                                       GPIO_Port_TypeDef                   port,
                                       unsigned int                        pin)
{
  EFM_ASSERT(GPIO_PORT_PIN_VALID(port, pin));

  // Configure Bufout request GPIO.
  GPIO_PinModeSet(port, pin, gpioModeInput, 0U);
  GPIO->SYXOROUTE[0].BUFOUTREQINASYNCROUTE = pin    << _GPIO_SYXO_BUFOUTREQINASYNCROUTE_PIN_SHIFT
                                             | port << _GPIO_SYXO_BUFOUTREQINASYNCROUTE_PORT_SHIFT;

  bool wasLocked = HFXO0->STATUS & HFXO_STATUS_LOCK_LOCKED ? true : false;

  // Unlock register interface.
  HFXO0->LOCK = HFXO_LOCK_LOCKKEY_UNLOCK;

  HFXO0->CTRL_CLR = _HFXO_CTRL_BUFOUTFREEZE_MASK | _HFXO_CTRL_DISONDEMANDBUFOUT_MASK;

  BUS_RegMaskedWrite(&HFXO0->BUFOUTCTRL,
                     _HFXO_BUFOUTCTRL_MINIMUMSTARTUPDELAY_MASK
                     | _HFXO_BUFOUTCTRL_TIMEOUTSTARTUP_MASK,
                     bufoutInit->minimalStartupDelay  << _HFXO_BUFOUTCTRL_MINIMUMSTARTUPDELAY_SHIFT
                     | bufoutInit->timeoutStartup       << _HFXO_BUFOUTCTRL_TIMEOUTSTARTUP_SHIFT);

  if (wasLocked) {
    HFXO0->LOCK = ~HFXO_LOCK_LOCKKEY_UNLOCK;
  }
}
#endif

#if defined(_HFXO_CTRL_PRSSTATUSSEL0_MASK)
/**************************************************************************//**
 * @brief
 *   Initialize HFXO Bufout (Crystal sharing) follower control registers.
 *   Configure the clock request signal to a specified GPIO to automatically
 *   request the high frequency crystal oscillator sine wave clock.
 *   This function must be used in conjunction with CMU_HFXOInit() configured
 *   with EXTERNAL_SINE or EXTERNAL_SINEPKDET mode.
 *
 * @warning
 *   If EM2 capabilities are needed, a GPIO that fully retains its
 *   capabilities while in EM2 must be selected.
 *
 * @note
 *   This function can be emulated on XG21/XG22 chips by controlling the clock
 *   request GPIO to ask the crystal sharing leader clock when needed.
 *
 * @param[in] prsStatusSelectOutput
 *    Selected HFXO PRS signal output.
 *
 * @param[in] prsAsyncCh
 *    PRS producer asynchronous signal channel.
 *
 * @param[in] port
 *    Bufout request GPIO port.
 *
 * @param[in] pin
 *    Bufout request GPIO pin.
 *****************************************************************************/
void CMU_HFXOCrystalSharingFollowerInit(CMU_PRS_Status_Output_Select_TypeDef prsStatusSelectOutput,
                                        unsigned int                         prsAsyncCh,
                                        GPIO_Port_TypeDef                    port,
                                        unsigned int                         pin)
{
  EFM_ASSERT(prsAsyncCh < PRS_ASYNC_CH_NUM);
  EFM_ASSERT(GPIO_PORT_PIN_VALID(port, pin));

  uint32_t mask = 0U, prsSignal = 0U, value = 0U;

  switch (prsStatusSelectOutput) {
    case PRS_Status_select_0:
      mask      = _HFXO_CTRL_PRSSTATUSSEL0_MASK;
      value     = _HFXO_CTRL_PRSSTATUSSEL0_ENS << _HFXO_CTRL_PRSSTATUSSEL0_SHIFT;
      prsSignal = _PRS_ASYNC_CH_CTRL_SIGSEL_HFXO0LSTATUS;
      break;

    case PRS_Status_select_1:
      mask      = _HFXO_CTRL_PRSSTATUSSEL1_MASK;
      value     = _HFXO_CTRL_PRSSTATUSSEL1_ENS << _HFXO_CTRL_PRSSTATUSSEL1_SHIFT;
      prsSignal = _PRS_ASYNC_CH_CTRL_SIGSEL_HFXO0LSTATUS1;
      break;

    default:
      EFM_ASSERT(false);
      break;
  }

  bool wasLocked = HFXO0->STATUS & HFXO_STATUS_LOCK_LOCKED ? true : false;

  // Unlock register interface.
  HFXO0->LOCK = HFXO_LOCK_LOCKKEY_UNLOCK;

  BUS_RegMaskedWrite(&HFXO0->CTRL, mask, value);

  if (wasLocked) {
    HFXO0->LOCK = ~HFXO_LOCK_LOCKKEY_UNLOCK;
  }

  value = _PRS_ASYNC_CH_CTRL_AUXSEL_DEFAULT     << _PRS_ASYNC_CH_CTRL_AUXSEL_SHIFT
          | _PRS_ASYNC_CH_CTRL_FNSEL_A          << _PRS_ASYNC_CH_CTRL_FNSEL_SHIFT
          | _PRS_ASYNC_CH_CTRL_SOURCESEL_HFXO0L << _PRS_ASYNC_CH_CTRL_SOURCESEL_SHIFT
          | prsSignal                           << _PRS_ASYNC_CH_CTRL_SIGSEL_SHIFT;

  mask = _PRS_ASYNC_CH_CTRL_AUXSEL_MASK
         | _PRS_ASYNC_CH_CTRL_FNSEL_MASK
         | _PRS_ASYNC_CH_CTRL_SOURCESEL_MASK
         | _PRS_ASYNC_CH_CTRL_SIGSEL_MASK;

  BUS_RegMaskedWrite(&(PRS->ASYNC_CH[prsAsyncCh].CTRL), mask, value);

  GPIO_PinModeSet(port, pin, gpioModeWiredOrPullDown, 0U);

  (&(GPIO->PRSROUTE[0].ASYNCH0ROUTE))[prsAsyncCh] = pin    << _GPIO_PRS_ASYNCH0ROUTE_PIN_SHIFT
                                                    | port << _GPIO_PRS_ASYNCH0ROUTE_PORT_SHIFT;
  GPIO->PRSROUTE[0].ROUTEEN = 1U << (_GPIO_PRS_ROUTEEN_ASYNCH0PEN_SHIFT + prsAsyncCh);
}
#endif

#if defined(_SILICON_LABS_32B_SERIES_2)
/**************************************************************************//**
 * @brief
 *   Set the HFXO crystal tuning delta.
 *
 * @param[in] delta
 *    Chip dependent crystal capacitor bank delta between HFXO XI and XO.
 *
 * @note
 *   The delta between XI and XO is applicable for the series 2 EFR32xG2x
 *   devices only.
 *****************************************************************************/
void CMU_HFXOCTuneDeltaSet(int32_t delta)
{
  ctuneDelta = (int8_t)delta;
}

/**************************************************************************//**
 * @brief
 *   Get the HFXO crystal tuning delta.
 *
 * @return
 *   Chip dependent crystal capacitor bank tuning delta.
 *****************************************************************************/
int32_t CMU_HFXOCTuneDeltaGet(void)
{
  return (int32_t)ctuneDelta;
}
#endif /* defined(_SILICON_LABS_32B_SERIES_2) */

/**************************************************************************//**
 * @brief
 *   Initialize LFXO control registers.
 *
 * @note
 *   LFXO configuration should be obtained from a configuration tool,
 *   app note or crystal datasheet. This function disables the LFXO to ensure
 *   a valid state before update.
 *
 * @param[in] lfxoInit
 *    LFXO setup parameters
 *****************************************************************************/
void CMU_LFXOInit(const CMU_LFXOInit_TypeDef *lfxoInit)
{
  EFM_ASSERT(lfxoInit->timeout
             <= (_LFXO_CFG_TIMEOUT_MASK  >> _LFXO_CFG_TIMEOUT_SHIFT));
  EFM_ASSERT(lfxoInit->mode
             <= (_LFXO_CFG_MODE_MASK >> _LFXO_CFG_MODE_SHIFT));
  EFM_ASSERT(lfxoInit->gain
             <= (_LFXO_CAL_GAIN_MASK >> _LFXO_CAL_GAIN_SHIFT));
  EFM_ASSERT(lfxoInit->capTune
             <= (_LFXO_CAL_CAPTUNE_MASK >> _LFXO_CAL_CAPTUNE_SHIFT));

  // Max internal capacitance tuning value is 0x4F (20 pF)
  uint8_t ctune = (uint8_t) SL_MIN(0x4FU, lfxoInit->capTune);

#if defined(CMU_CLKEN0_LFXO)
  // Enable LFXO module clock.
  CMU->CLKEN0_SET = CMU_CLKEN0_LFXO;
#endif

  // Unlock register interface
  LFXO->LOCK = LFXO_LOCK_LOCKKEY_UNLOCK;

  // Disable LFXO
  LFXO->CTRL_SET = LFXO_CTRL_DISONDEMAND;
  LFXO->CTRL_CLR = LFXO_CTRL_FORCEEN;
  while ((LFXO->STATUS & _LFXO_STATUS_ENS_MASK) != 0U) {
  }

  // Configure LFXO as specified
  LFXO->CAL = ((uint32_t)lfxoInit->gain  << _LFXO_CAL_GAIN_SHIFT)
              | ((uint32_t)ctune         << _LFXO_CAL_CAPTUNE_SHIFT);

  LFXO->CFG = (uint32_t)((lfxoInit->timeout           << _LFXO_CFG_TIMEOUT_SHIFT)
                         | (lfxoInit->mode            << _LFXO_CFG_MODE_SHIFT)
                         | (lfxoInit->highAmplitudeEn << _LFXO_CFG_HIGHAMPL_SHIFT)
                         | (lfxoInit->agcEn           << _LFXO_CFG_AGC_SHIFT));

  LFXO->CTRL = (uint32_t)((lfxoInit->failDetEM4WUEn   << _LFXO_CTRL_FAILDETEM4WUEN_SHIFT)
                          | (lfxoInit->failDetEn      << _LFXO_CTRL_FAILDETEN_SHIFT)
                          | (lfxoInit->disOnDemand    << _LFXO_CTRL_DISONDEMAND_SHIFT)
                          | (lfxoInit->forceEn        << _LFXO_CTRL_FORCEEN_SHIFT));

  if (lfxoInit->regLock) {
    LFXO->LOCK = ~LFXO_LOCK_LOCKKEY_UNLOCK;
  }
}

/**************************************************************************//**
 * @brief
 *   Sets LFXO's crystal precision, in PPM.
 *
 * @note
 *   LFXO precision should be obtained from a crystal datasheet.
 *
 * @param[in] precision
 *    LFXO's crystal precision, in PPM.
 *****************************************************************************/
void CMU_LFXOPrecisionSet(uint16_t precision)
{
  lfxo_precision = precision;
}

#if defined(PLFRCO_PRESENT)
/**************************************************************************//**
 * @brief
 *   Configure the LFRCO precision.
 *
 * @details
 *   When enabling high precision mode on the LFRCO the hardware will tune
 *   the oscillator automatically using the HFXO as a reference.
 *
 * @note
 *   Refer to the reference manual and the datasheet for details about
 *   NOMCAL and NOMCALINV calibration count values.
 *
 * @param[in] precision
 *    LFRCO precision, this can be either high or default.
 *****************************************************************************/
void CMU_LFRCOSetPrecision(CMU_Precision_TypeDef precision)
{
  uint32_t ref = 0;
  uint32_t nomcal = 0;
  uint32_t nomcalinv = 0;

  CMU->CLKEN0_SET = CMU_CLKEN0_LFRCO;

  LFRCO->CTRL = LFRCO_CTRL_DISONDEMAND; // Force disable
  while ((LFRCO->STATUS & LFRCO_STATUS_ENS) != 0U) {
    // Wait for LFRCO to stop
  }

  if (precision == cmuPrecisionHigh) {
    ref = SystemHFXOClockGet();
    // Use precomputed value for HFXO typical frequencies
    if (ref == XTAL_38M4) {
      nomcal = LFRCO_NOMCAL_XTAL_38M4;
      nomcalinv = LFRCO_NOMCALINV_XTAL_38M4;
    } else if (ref == XTAL_39M0) {
      nomcal = LFRCO_NOMCAL_XTAL_39M0;
      nomcalinv = LFRCO_NOMCALINV_XTAL_39M0;
    } else {
      // Compute calibration count, based on HFXO frequency
      nomcal = (5 * ref) >> 9;
      nomcalinv = ((1UL << 31) / 5) << 2;
      nomcalinv /= (ref >> 9);
    }

    LFRCO->NOMCAL = nomcal;
    LFRCO->NOMCALINV = nomcalinv;

    LFRCO->CFG = LFRCO_CFG_HIGHPRECEN;
  } else {
    LFRCO->CFG = 0;
  }
  LFRCO->CTRL = _LFRCO_CTRL_RESETVALUE;
}
#endif

/***************************************************************************//**
 * @brief
 *   Get oscillator frequency tuning setting.
 *
 * @param[in] osc
 *   Oscillator to get tuning value for.
 *
 * @return
 *   The oscillator frequency tuning setting in use.
 ******************************************************************************/
uint32_t CMU_OscillatorTuningGet(CMU_Osc_TypeDef osc)
{
  uint32_t ret = 0U;

  switch (osc) {
#if defined(_LFRCO_CAL_FREQTRIM_MASK)
    case cmuOsc_LFRCO:
#if defined(CMU_CLKEN0_LFRCO)
      CMU->CLKEN0_SET = CMU_CLKEN0_LFRCO;
#endif
      ret = (LFRCO->CAL & _LFRCO_CAL_FREQTRIM_MASK) >> _LFRCO_CAL_FREQTRIM_SHIFT;
      break;
#endif

    case cmuOsc_HFRCODPLL:
#if defined(CMU_CLKEN0_HFRCO0)
      CMU->CLKEN0_SET = CMU_CLKEN0_HFRCO0;
#endif
      ret = (HFRCO0->CAL & _HFRCO_CAL_TUNING_MASK) >> _HFRCO_CAL_TUNING_SHIFT;
      break;

#if defined(HFRCOEM23_PRESENT)
    case cmuOsc_HFRCOEM23:
      ret = (HFRCOEM23->CAL & _HFRCO_CAL_TUNING_MASK) >> _HFRCO_CAL_TUNING_SHIFT;
      break;
#endif

    case cmuOsc_HFXO:
#if defined(CMU_CLKEN0_HFXO0)
      CMU->CLKEN0_SET = CMU_CLKEN0_HFXO0;
#endif
      ret = (HFXO0->XTALCTRL & _HFXO_XTALCTRL_COREBIASANA_MASK) >> _HFXO_XTALCTRL_COREBIASANA_SHIFT;
      break;

    default:
      EFM_ASSERT(false);
      break;
  }

  return ret;
}

/***************************************************************************//**
 * @brief
 *   Set the oscillator frequency tuning control.
 *
 * @note
 *   Oscillator tuning is done during production, and the tuning value is
 *   automatically loaded after a reset. Changing the tuning value from the
 *   calibrated value is for more advanced use. Certain oscillators also have
 *   build-in tuning optimization.
 *
 * @param[in] osc
 *   Oscillator to set tuning value for.
 *
 * @param[in] val
 *   The oscillator frequency tuning setting to use.
 ******************************************************************************/
void CMU_OscillatorTuningSet(CMU_Osc_TypeDef osc, uint32_t val)
{
  bool disondemand = false;

  switch (osc) {
#if defined(_LFRCO_CAL_FREQTRIM_MASK)
    case cmuOsc_LFRCO:
#if defined(CMU_CLKEN0_LFRCO)
      CMU->CLKEN0_SET = CMU_CLKEN0_LFRCO;
#endif
      EFM_ASSERT(val <= (_LFRCO_CAL_FREQTRIM_MASK
                         >> _LFRCO_CAL_FREQTRIM_SHIFT));
      val &= _LFRCO_CAL_FREQTRIM_MASK >> _LFRCO_CAL_FREQTRIM_SHIFT;
      while (LFRCO->SYNCBUSY != 0U) {
      }
      LFRCO->CAL = (LFRCO->CAL & ~_LFRCO_CAL_FREQTRIM_MASK)
                   | (val << _LFRCO_CAL_FREQTRIM_SHIFT);
      break;
#endif

    case cmuOsc_HFRCODPLL:
#if defined(CMU_CLKEN0_HFRCO0)
      CMU->CLKEN0_SET = CMU_CLKEN0_HFRCO0;
#endif
      EFM_ASSERT(val <= (_HFRCO_CAL_TUNING_MASK >> _HFRCO_CAL_TUNING_SHIFT));
      val &= _HFRCO_CAL_TUNING_MASK >> _HFRCO_CAL_TUNING_SHIFT;
      while ((HFRCO0->STATUS & HFRCO_STATUS_SYNCBUSY) != 0UL) {
      }
      HFRCO0->CAL = (HFRCO0->CAL & ~_HFRCO_CAL_TUNING_MASK)
                    | (val << _HFRCO_CAL_TUNING_SHIFT);
      break;

#if defined(HFRCOEM23_PRESENT)
    case cmuOsc_HFRCOEM23:
      EFM_ASSERT(val <= (_HFRCO_CAL_TUNING_MASK >> _HFRCO_CAL_TUNING_SHIFT));
      val &= _HFRCO_CAL_TUNING_MASK >> _HFRCO_CAL_TUNING_SHIFT;
      while ((HFRCOEM23->STATUS & HFRCO_STATUS_SYNCBUSY) != 0UL) {
      }
      HFRCOEM23->CAL = (HFRCOEM23->CAL & ~_HFRCO_CAL_TUNING_MASK)
                       | (val << _HFRCO_CAL_TUNING_SHIFT);
      break;
#endif

    case cmuOsc_HFXO:
#if defined(CMU_CLKEN0_HFXO0)
      CMU->CLKEN0_SET = CMU_CLKEN0_HFXO0;
#endif
      EFM_ASSERT(val <= (_HFXO_XTALCTRL_COREBIASANA_MASK >> _HFXO_XTALCTRL_COREBIASANA_SHIFT));
      // Make sure HFXO is disable
      EFM_ASSERT((HFXO0->STATUS & HFXO_STATUS_ENS) == 0);

      // Set DISONDEMAND if not already set and wait for FSMLOCK to be clear so that
      // software can write to register
      disondemand = (HFXO0->CTRL & _HFXO_CTRL_DISONDEMAND_MASK) >> _HFXO_CTRL_DISONDEMAND_SHIFT;
      if (disondemand == false) {
        HFXO0->CTRL_SET = HFXO_CTRL_DISONDEMAND;
      }
#if defined(HFXO_STATUS_FSMLOCK)
      while ((HFXO0->STATUS & HFXO_STATUS_FSMLOCK) != 0) {
      }
#endif
      // Update Core Bias Ana setting and enable Optimization skip
      HFXO0->XTALCTRL = (HFXO0->XTALCTRL & ~_HFXO_XTALCTRL_COREBIASANA_MASK)
                        | (val << _HFXO_XTALCTRL_COREBIASANA_SHIFT)
                        | HFXO_XTALCTRL_SKIPCOREBIASOPT;
      // Clear back DISONDEMAND if needed
      if (disondemand == false) {
        HFXO0->CTRL_CLR = HFXO_CTRL_DISONDEMAND;
      }
      break;

    default:
      EFM_ASSERT(false);
      break;
  }
}

/***************************************************************************//**
 * @brief
 *   Configure wait state settings necessary to switch to a given core clock
 *   frequency at a certain voltage scale level.
 *
 * @details
 *   This function will set up the necessary flash wait states. Updating the
 *   wait state configuration must be done before increasing the clock
 *   frequency and it must be done after decreasing the clock frequency.
 *   Updating the wait state configuration must be done before core voltage is
 *   decreased and it must be done after a core voltage is increased.
 *
 * @param[in] freq
 *   The core clock frequency to configure wait-states.
 *
 * @param[in] vscale
 *   The voltage scale to configure wait-states. Expected values are
 *   0 or 1, higher number is lower voltage.
 *   @li 0 = 1.1 V (VSCALE2)
 *   @li 1 = 1.0 V (VSCALE1)
 ******************************************************************************/
void CMU_UpdateWaitStates(uint32_t freq, int vscale)
{
  if (vscale > 0) {
    flashWaitStateControl(freq, VSCALE_EM01_LOW_POWER);
  } else {
    flashWaitStateControl(freq, VSCALE_EM01_HIGH_PERFORMANCE);
  }
}

/**************************************************************************//**
 * @brief
 *   Select the PCNTn clock.
 *
 * @param[in] instance
 *   PCNT instance number to set selected clock source for.
 *
 * @param[in] external
 *   Set to true to select the external clock, false to select EM23GRPACLK.
 *****************************************************************************/
void CMU_PCNTClockExternalSet(unsigned int instance, bool external)
{
  (void)instance;
#if defined(PCNT_PRESENT)
  if (external) {
    CMU->PCNT0CLKCTRL = CMU_PCNT0CLKCTRL_CLKSEL_PCNTS0;
  } else {
    CMU->PCNT0CLKCTRL = CMU_PCNT0CLKCTRL_CLKSEL_EM23GRPACLK;
  }
#else
  (void)external;
#endif
}

#if defined(HFRCOEM23_PRESENT)
/***************************************************************************//**
 * @brief
 *   Get HFRCOEM23 band in use.
 *
 * @return
 *   HFRCOEM23 band in use.
 ******************************************************************************/
CMU_HFRCOEM23Freq_TypeDef CMU_HFRCOEM23BandGet(void)
{
  return (CMU_HFRCOEM23Freq_TypeDef)SystemHFRCOEM23ClockGet();
}

/***************************************************************************//**
 * @brief
 *   Set HFRCOEM23 band and the tuning value based on the value in the
 *   calibration table made during production.
 *
 * @param[in] freq
 *   HFRCOEM23 frequency band to activate.
 ******************************************************************************/
void CMU_HFRCOEM23BandSet(CMU_HFRCOEM23Freq_TypeDef freq)
{
  uint32_t freqCal;

  // Get calibration data from DEVINFO
  freqCal = HFRCOEM23DevinfoGet(freq);
  EFM_ASSERT((freqCal != 0UL) && (freqCal != UINT_MAX));
#if defined(CMU_CLKEN0_HFRCOEM23)
  CMU->CLKEN0_SET = CMU_CLKEN0_HFRCOEM23;
#endif

  // Set divider for 1, 2 and 4MHz bands
  freqCal &= ~_HFRCO_CAL_CLKDIV_MASK;
  switch (freq) {
    case cmuHFRCOEM23Freq_1M0Hz:
      freqCal |= HFRCO_CAL_CLKDIV_DIV4;
      break;

    case cmuHFRCOEM23Freq_2M0Hz:
      freqCal |= HFRCO_CAL_CLKDIV_DIV2;
      break;

    default:
      break;
  }

  // Activate new band selection
  HFRCOEM23->CAL = freqCal;
}
#endif // defined(HFRCOEM23_PRESENT)

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/*******************************************************************************
 **************************   LOCAL FUNCTIONS   ********************************
 ******************************************************************************/

#if (_SILICON_LABS_32B_SERIES_2_CONFIG > 1)
#if defined(PDM_PRESENT)
/***************************************************************************//**
 * @brief
 *   Get selected oscillator and frequency for @ref cmuClock_EM01GRPBCLK
 *   clock tree.
 *
 * @param[out] freq
 *   The frequency.
 *
 * @param[out] sel
 *   The selected oscillator.
 ******************************************************************************/
static void em01GrpbClkGet(uint32_t *freq, CMU_Select_TypeDef *sel)
{
  uint32_t f = 0U;
  CMU_Select_TypeDef s;
  switch (CMU->EM01GRPBCLKCTRL & _CMU_EM01GRPBCLKCTRL_CLKSEL_MASK) {
    case _CMU_EM01GRPBCLKCTRL_CLKSEL_HFRCODPLL:
      f = SystemHFRCODPLLClockGet();
      s = cmuSelect_HFRCODPLL;
      break;

    case _CMU_EM01GRPBCLKCTRL_CLKSEL_HFRCODPLLRT:
      f = SystemHFRCODPLLClockGet();
      s = cmuSelect_HFRCODPLLRT;
      break;

    case _CMU_EM01GRPBCLKCTRL_CLKSEL_HFXO:
      f = SystemHFXOClockGet();
      s = cmuSelect_HFXO;
      break;

    case _CMU_EM01GRPBCLKCTRL_CLKSEL_HFXORT:
      f = SystemHFXOClockGet();
      s = cmuSelect_HFXORT;
      break;

    case _CMU_EM01GRPBCLKCTRL_CLKSEL_FSRCO:
      f = SystemFSRCOClockGet();
      s = cmuSelect_FSRCO;
      break;

    case _CMU_EM01GRPBCLKCTRL_CLKSEL_CLKIN0:
      f = SystemCLKIN0Get();
      s = cmuSelect_CLKIN0;
      break;

    default:
      s = cmuSelect_Error;
      EFM_ASSERT(false);
      break;
  }
  if (freq != NULL) {
    *freq = f;
  }
  if (sel != NULL) {
    *sel = s;
  }
}
#endif

#if defined(EUART_PRESENT)
/***************************************************************************//**
 * @brief
 *   Get selected oscillator and frequency for @ref cmuClock_EUART0CLK
 *   clock tree.
 *
 * @param[out] freq
 *   The frequency.
 *
 * @param[out] sel
 *   The selected oscillator.
 ******************************************************************************/
static void euart0ClkGet(uint32_t *freq, CMU_Select_TypeDef *sel)
{
  switch (CMU->EUART0CLKCTRL & _CMU_EUART0CLKCTRL_CLKSEL_MASK) {
    case _CMU_EUART0CLKCTRL_CLKSEL_EM01GRPACLK:
      em01GrpaClkGet(freq, sel);
      break;

    case _CMU_EUART0CLKCTRL_CLKSEL_EM23GRPACLK:
      em23GrpaClkGet(freq, sel);
      break;

    default:
      if (freq != NULL) {
        *freq = 0U;
      }
      if (sel != NULL) {
        *sel = cmuSelect_Error;
      }
      EFM_ASSERT(false);
      break;
  }
}
#endif

#if defined(EUSART_PRESENT)
/***************************************************************************//**
 * @brief
 *   Get selected oscillator and frequency for @ref cmuClock_EUSART0CLK
 *   clock tree.
 *
 * @param[out] freq
 *   The frequency.
 *
 * @param[out] sel
 *   The selected oscillator.
 ******************************************************************************/
static void eusart0ClkGet(uint32_t *freq, CMU_Select_TypeDef *sel)
{
  uint32_t f = 0U;
  CMU_Select_TypeDef s;

  switch (CMU->EUSART0CLKCTRL & _CMU_EUSART0CLKCTRL_CLKSEL_MASK) {
    case CMU_EUSART0CLKCTRL_CLKSEL_EM01GRPCCLK:
      em01GrpcClkGet(&f, NULL);
      s = cmuSelect_EM01GRPCCLK;
      break;

    case CMU_EUSART0CLKCTRL_CLKSEL_HFRCOEM23:
      f = SystemHFRCOEM23ClockGet();
      s = cmuSelect_HFRCOEM23;
      break;

    case CMU_EUSART0CLKCTRL_CLKSEL_LFRCO:
      f = SystemLFRCOClockGet();
      s = cmuSelect_LFRCO;
      break;

    case CMU_EUSART0CLKCTRL_CLKSEL_LFXO:
      f = SystemLFXOClockGet();
      s = cmuSelect_LFXO;
      break;

    default:
      s = cmuSelect_Error;
      EFM_ASSERT(false);
      break;
  }
  if (freq != NULL) {
    *freq = f;
  }
  if (sel != NULL) {
    *sel = s;
  }
}

/**************************************************************************//**
* @brief
*   Get selected oscillator and frequency for @ref cmuClock_EM01GRPCCLK
*   clock tree.
*
* @param[out] freq
*   The frequency.
*
* @param[out] sel
*   The selected oscillator.
******************************************************************************/
static void em01GrpcClkGet(uint32_t *freq, CMU_Select_TypeDef *sel)
{
  uint32_t f = 0U;
  CMU_Select_TypeDef s;

  switch (CMU->EM01GRPCCLKCTRL & _CMU_EM01GRPCCLKCTRL_CLKSEL_MASK) {
    case _CMU_EM01GRPCCLKCTRL_CLKSEL_HFRCODPLL:
      f = SystemHFRCODPLLClockGet();
      s = cmuSelect_HFRCODPLL;
      break;

    case _CMU_EM01GRPCCLKCTRL_CLKSEL_HFRCOEM23:
      f = SystemHFRCOEM23ClockGet();
      s = cmuSelect_HFRCOEM23;
      break;

    case _CMU_EM01GRPCCLKCTRL_CLKSEL_HFXO:
      f = SystemHFXOClockGet();
      s = cmuSelect_HFXO;
      break;

    case _CMU_EM01GRPCCLKCTRL_CLKSEL_FSRCO:
      f = SystemFSRCOClockGet();
      s = cmuSelect_FSRCO;
      break;

    default:
      s = cmuSelect_Error;
      EFM_ASSERT(false);
      break;
  }
  if (freq != NULL) {
    *freq = f;
  }
  if (sel != NULL) {
    *sel = s;
  }
}
#endif // defined(EUSART_PRESENT)

#if defined(LCD_PRESENT)
/***************************************************************************//**
 * @brief
 *   Get selected oscillator and frequency for @ref cmuClock_LCDCLK
 *   clock tree.
 *
 * @param[out] freq
 *   The frequency.
 *
 * @param[out] sel
 *   The selected oscillator.
 ******************************************************************************/
static void lcdClkGet(uint32_t *freq, CMU_Select_TypeDef *sel)
{
  uint32_t f = 0U;
  CMU_Select_TypeDef s;

  switch (CMU->LCDCLKCTRL & _CMU_LCDCLKCTRL_CLKSEL_MASK) {
    case _CMU_LCDCLKCTRL_CLKSEL_ULFRCO:
      f = SystemULFRCOClockGet();
      s = cmuSelect_ULFRCO;
      break;

    case _CMU_LCDCLKCTRL_CLKSEL_LFRCO:
      f = SystemLFRCOClockGet();
      s = cmuSelect_LFRCO;
      break;

    case _CMU_LCDCLKCTRL_CLKSEL_LFXO:
      f = SystemLFXOClockGet();
      s = cmuSelect_LFXO;
      break;

    default:
      s = cmuSelect_Error;
      EFM_ASSERT(false);
      break;
  }
  if (freq != NULL) {
    *freq = f;
  }
  if (sel != NULL) {
    *sel = s;
  }
}
#endif // defined(LCD_PRESENT)

#if defined(VDAC_PRESENT)
/***************************************************************************//**
 * @brief
 *   Get selected oscillator and frequency for @ref cmuClock_VDAC0CLK
 *   clock tree.
 *
 * @param[out] freq
 *   The frequency.
 *
 * @param[out] sel
 *   The selected oscillator.
 ******************************************************************************/
static void vdac0ClkGet(uint32_t *freq, CMU_Select_TypeDef *sel)
{
  uint32_t f = 0U;
  CMU_Select_TypeDef s;

  switch (CMU->VDAC0CLKCTRL & _CMU_VDAC0CLKCTRL_CLKSEL_MASK) {
    case _CMU_VDAC0CLKCTRL_CLKSEL_EM01GRPACLK:
      em01GrpaClkGet(&f, NULL);
      s = cmuSelect_EM01GRPACLK;
      break;

    case _CMU_VDAC0CLKCTRL_CLKSEL_EM23GRPACLK:
      em23GrpaClkGet(&f, NULL);
      s = cmuSelect_EM23GRPACLK;
      break;

    case _CMU_VDAC0CLKCTRL_CLKSEL_HFRCOEM23:
      f = SystemHFRCOEM23ClockGet();
      s = cmuSelect_HFRCOEM23;
      break;

    case _CMU_VDAC0CLKCTRL_CLKSEL_FSRCO:
      f = SystemFSRCOClockGet();
      s = cmuSelect_FSRCO;
      break;

    default:
      s = cmuSelect_Error;
      EFM_ASSERT(false);
      break;
  }
  if (freq != NULL) {
    *freq = f;
  }
  if (sel != NULL) {
    *sel = s;
  }
}

#if (VDAC_COUNT > 1)
/***************************************************************************//**
 * @brief
 *   Get selected oscillator and frequency for @ref cmuClock_VDAC1CLK
 *   clock tree.
 *
 * @param[out] freq
 *   The frequency.
 *
 * @param[out] sel
 *   The selected oscillator.
 ******************************************************************************/
static void vdac1ClkGet(uint32_t *freq, CMU_Select_TypeDef *sel)
{
  uint32_t f = 0U;
  CMU_Select_TypeDef s;

  switch (CMU->VDAC1CLKCTRL & _CMU_VDAC1CLKCTRL_CLKSEL_MASK) {
    case _CMU_VDAC1CLKCTRL_CLKSEL_EM01GRPACLK:
      em01GrpaClkGet(&f, NULL);
      s = cmuSelect_EM01GRPACLK;
      break;

    case _CMU_VDAC1CLKCTRL_CLKSEL_EM23GRPACLK:
      em23GrpaClkGet(&f, NULL);
      s = cmuSelect_EM23GRPACLK;
      break;

    case _CMU_VDAC1CLKCTRL_CLKSEL_HFRCOEM23:
      f = SystemHFRCOEM23ClockGet();
      s = cmuSelect_HFRCOEM23;
      break;

    case _CMU_VDAC1CLKCTRL_CLKSEL_FSRCO:
      f = SystemFSRCOClockGet();
      s = cmuSelect_FSRCO;
      break;

    default:
      s = cmuSelect_Error;
      EFM_ASSERT(false);
      break;
  }
  if (freq != NULL) {
    *freq = f;
  }
  if (sel != NULL) {
    *sel = s;
  }
}
#endif
#endif /* VDAC_PRESENT */

#if defined(PCNT_PRESENT)
/***************************************************************************//**
 * @brief
 *   Get selected oscillator and frequency for @ref cmuClock_PCNT0CLK
 *   clock tree.
 *
 * @param[out] freq
 *   The frequency.
 *
 * @param[out] sel
 *   The selected oscillator.
 ******************************************************************************/
static void pcnt0ClkGet(uint32_t *freq, CMU_Select_TypeDef *sel)
{
  uint32_t f = 0U;
  CMU_Select_TypeDef s;

  switch (CMU->PCNT0CLKCTRL & _CMU_PCNT0CLKCTRL_CLKSEL_MASK) {
    case _CMU_PCNT0CLKCTRL_CLKSEL_EM23GRPACLK:
      em23GrpaClkGet(&f, NULL);
      s = cmuSelect_EM23GRPACLK;
      break;

    case _CMU_PCNT0CLKCTRL_CLKSEL_PCNTS0:
      f = 0U; // external or PRS source so the frequency is undefined.
      s = cmuSelect_PCNTEXTCLK;
      break;

    default:
      s = cmuSelect_Error;
      EFM_ASSERT(false);
      break;
  }
  if (freq != NULL) {
    *freq = f;
  }
  if (sel != NULL) {
    *sel = s;
  }
}
#endif

#if defined(LESENSE_PRESENT)
/***************************************************************************//**
 * @brief
 *   Get selected oscillator and frequency for @ref cmuClock_LESENSEHFCLK
 *   clock tree.
 *
 * @param[out] freq
 *   The frequency.
 *
 * @param[out] sel
 *   The selected oscillator.
 ******************************************************************************/
static void lesenseHFClkGet(uint32_t *freq, CMU_Select_TypeDef *sel)
{
  uint32_t f = 0U;
  CMU_Select_TypeDef s;

  switch (CMU->LESENSEHFCLKCTRL & _CMU_LESENSEHFCLKCTRL_CLKSEL_MASK) {
    case _CMU_LESENSEHFCLKCTRL_CLKSEL_HFRCOEM23:
      f = SystemHFRCOEM23ClockGet();
      s = cmuSelect_HFRCOEM23;
      break;

    case _CMU_LESENSEHFCLKCTRL_CLKSEL_FSRCO:
      f = SystemFSRCOClockGet();
      s = cmuSelect_FSRCO;
      break;

    default:
      s = cmuSelect_Error;
      EFM_ASSERT(false);
      break;
  }
  if (freq != NULL) {
    *freq = f;
  }
  if (sel != NULL) {
    *sel = s;
  }
}
#endif

#if ((defined(CMU_SYSCLKCTRL_RHCLKPRESC)) \
  && (_SILICON_LABS_EFR32_RADIO_TYPE != _SILICON_LABS_EFR32_RADIO_NONE))
/***************************************************************************//**
 * @brief
 *   Set maximum allowed prescaler for radio clock tree (RHCLK).
 ******************************************************************************/
static void rhclkPrescMax(void)
{
  // Set largest prescaler (DIV2).
  CMU->SYSCLKCTRL_SET = CMU_SYSCLKCTRL_RHCLKPRESC;
}

#if defined(_SILICON_LABS_32B_SERIES_2_CONFIG_2)  \
  || defined(_SILICON_LABS_32B_SERIES_2_CONFIG_3) \
  || defined(_SILICON_LABS_32B_SERIES_2_CONFIG_4)
/***************************************************************************//**
 * @brief
 *   Set radio clock tree prescaler to achieve highest possible frequency
 *   and still be within spec.
 ******************************************************************************/
static void rhclkPrescOptimize(void)
{
  if (CMU_ClockFreqGet(cmuClock_SYSCLK) <= CMU_MAX_RHCLK_FREQ) {
    // Set smallest prescaler (DIV1).
    CMU->SYSCLKCTRL_CLR = CMU_SYSCLKCTRL_RHCLKPRESC;
  }
}
#endif
#endif // ((defined(CMU_SYSCLKCTRL_RHCLKPRESC)) && (_SILICON_LABS_EFR32_RADIO_TYPE != _SILICON_LABS_EFR32_RADIO_NONE))
#endif // #if (_SILICON_LABS_32B_SERIES_2_CONFIG > 1)

#if defined(HFRCOEM23_PRESENT)
/***************************************************************************//**
 * @brief
 *   Get calibrated HFRCOEM23 tuning value from Device information (DI) page
 *   for a given frequency. Calibration value is not available for all frequency
 *   bands.
 *
 * @param[in] freq
 *   HFRCOEM23 frequency band
 ******************************************************************************/
static uint32_t HFRCOEM23DevinfoGet(CMU_HFRCOEM23Freq_TypeDef freq)
{
  uint32_t ret = 0U;

  switch (freq) {
    // 1, 2 and 4MHz share the same calibration word
    case cmuHFRCOEM23Freq_1M0Hz:
    case cmuHFRCOEM23Freq_2M0Hz:
    case cmuHFRCOEM23Freq_4M0Hz:
      ret = DEVINFO->HFRCOEM23CAL[0].HFRCOEM23CAL;
      break;

    case cmuHFRCOEM23Freq_13M0Hz:
      ret = DEVINFO->HFRCOEM23CAL[6].HFRCOEM23CAL;
      break;

    case cmuHFRCOEM23Freq_16M0Hz:
      ret = DEVINFO->HFRCOEM23CAL[7].HFRCOEM23CAL;
      break;

    case cmuHFRCOEM23Freq_19M0Hz:
      ret = DEVINFO->HFRCOEM23CAL[8].HFRCOEM23CAL;
      break;

    case cmuHFRCOEM23Freq_26M0Hz:
      ret = DEVINFO->HFRCOEM23CAL[10].HFRCOEM23CAL;
      break;

    case cmuHFRCOEM23Freq_32M0Hz:
      ret = DEVINFO->HFRCOEM23CAL[11].HFRCOEM23CAL;
      break;

    case cmuHFRCOEM23Freq_40M0Hz:
      ret = DEVINFO->HFRCOEM23CAL[12].HFRCOEM23CAL;
      break;

    case cmuHFRCOEM23Freq_UserDefined:
      break;

    default:
      EFM_ASSERT(false);
      break;
  }
  return ret;
}
#endif // defined(HFRCOEM23_PRESENT)

/***************************************************************************//**
 * @brief
 *   Get selected oscillator and frequency for @ref cmuClock_TRACECLK
 *   clock tree.
 *
 * @param[out] freq
 *   The frequency.
 *
 * @param[out] sel
 *   The selected oscillator.
 ******************************************************************************/
static void traceClkGet(uint32_t *freq, CMU_Select_TypeDef *sel)
{
  uint32_t f = 0U;
  CMU_Select_TypeDef s;

#if defined(_CMU_TRACECLKCTRL_CLKSEL_MASK)
  switch (CMU->TRACECLKCTRL & _CMU_TRACECLKCTRL_CLKSEL_MASK) {
#if defined(_CMU_TRACECLKCTRL_CLKSEL_HCLK)
    case _CMU_TRACECLKCTRL_CLKSEL_HCLK:
      f = SystemHCLKGet();
      s = cmuSelect_HCLK;
      break;
#endif
#if defined(_CMU_TRACECLKCTRL_CLKSEL_SYSCLK)
    case _CMU_TRACECLKCTRL_CLKSEL_SYSCLK:
      f = SystemSYSCLKGet();
      s = cmuSelect_SYSCLK;
      break;
#endif

    case _CMU_TRACECLKCTRL_CLKSEL_HFRCOEM23:
      f = SystemHFRCOEM23ClockGet();
      s = cmuSelect_HFRCOEM23;
      break;

    default:
      s = cmuSelect_Error;
      EFM_ASSERT(false);
      break;
  }
#else
  f = SystemSYSCLKGet();
  s = cmuSelect_SYSCLK;
#endif

  if (freq != NULL) {
    *freq = f;
  }
  if (sel != NULL) {
    *sel = s;
  }
}

/***************************************************************************//**
 * @brief
 *   Get selected oscillator and frequency for @ref cmuClock_DPLLREFCLK
 *   clock tree.
 *
 * @param[out] freq
 *   The frequency.
 *
 * @param[out] sel
 *   The selected oscillator.
 ******************************************************************************/
static void dpllRefClkGet(uint32_t *freq, CMU_Select_TypeDef *sel)
{
  uint32_t f = 0U;
  CMU_Select_TypeDef s;

  switch (CMU->DPLLREFCLKCTRL & _CMU_DPLLREFCLKCTRL_CLKSEL_MASK) {
    case _CMU_DPLLREFCLKCTRL_CLKSEL_HFXO:
      f = SystemHFXOClockGet();
      s = cmuSelect_HFXO;
      break;

    case _CMU_DPLLREFCLKCTRL_CLKSEL_LFXO:
      f = SystemLFXOClockGet();
      s = cmuSelect_LFXO;
      break;

    case _CMU_DPLLREFCLKCTRL_CLKSEL_CLKIN0:
      f = SystemCLKIN0Get();
      s = cmuSelect_CLKIN0;
      break;

    case _CMU_DPLLREFCLKCTRL_CLKSEL_DISABLED:
      s = cmuSelect_Disabled;
      break;

    default:
      s = cmuSelect_Error;
      EFM_ASSERT(false);
      break;
  }

  if (freq != NULL) {
    *freq = f;
  }
  if (sel != NULL) {
    *sel = s;
  }
}

/***************************************************************************//**
 * @brief
 *   Get selected oscillator and frequency for @ref cmuClock_EM01GRPACLK
 *   clock tree.
 *
 * @param[out] freq
 *   The frequency.
 *
 * @param[out] sel
 *   The selected oscillator.
 ******************************************************************************/
static void em01GrpaClkGet(uint32_t *freq, CMU_Select_TypeDef *sel)
{
  uint32_t f = 0U;
  CMU_Select_TypeDef s;

  switch (CMU->EM01GRPACLKCTRL & _CMU_EM01GRPACLKCTRL_CLKSEL_MASK) {
    case _CMU_EM01GRPACLKCTRL_CLKSEL_HFRCODPLL:
      f = SystemHFRCODPLLClockGet();
      s = cmuSelect_HFRCODPLL;
      break;

    case _CMU_EM01GRPACLKCTRL_CLKSEL_HFXO:
      f = SystemHFXOClockGet();
      s = cmuSelect_HFXO;
      break;

#if defined(HFRCOEM23_PRESENT)
    case _CMU_EM01GRPACLKCTRL_CLKSEL_HFRCOEM23:
      f = SystemHFRCOEM23ClockGet();
      s = cmuSelect_HFRCOEM23;
      break;
#endif

    case _CMU_EM01GRPACLKCTRL_CLKSEL_FSRCO:
      f = SystemFSRCOClockGet();
      s = cmuSelect_FSRCO;
      break;

#if defined(CMU_EM01GRPACLKCTRL_CLKSEL_HFRCODPLLRT)
    case CMU_EM01GRPACLKCTRL_CLKSEL_HFRCODPLLRT:
      f = SystemHFRCODPLLClockGet();
      s = cmuSelect_HFRCODPLLRT;
      break;
#endif

#if defined(CMU_EM01GRPACLKCTRL_CLKSEL_HFXORT)
    case CMU_EM01GRPACLKCTRL_CLKSEL_HFXORT:
      f = SystemHFXOClockGet();
      s = cmuSelect_HFXORT;
      break;
#endif

    default:
      s = cmuSelect_Error;
      EFM_ASSERT(false);
      break;
  }

  if (freq != NULL) {
    *freq = f;
  }
  if (sel != NULL) {
    *sel = s;
  }
}

/***************************************************************************//**
 * @brief
 *   Get selected oscillator and frequency for @ref cmuClock_EM23GRPACLK
 *   clock tree.
 *
 * @param[out] freq
 *   The frequency.
 *
 * @param[out] sel
 *   The selected oscillator.
 ******************************************************************************/
static void em23GrpaClkGet(uint32_t *freq, CMU_Select_TypeDef *sel)
{
  uint32_t f = 0U;
  CMU_Select_TypeDef s;

  switch (CMU->EM23GRPACLKCTRL & _CMU_EM23GRPACLKCTRL_CLKSEL_MASK) {
    case _CMU_EM23GRPACLKCTRL_CLKSEL_LFRCO:
      f = SystemLFRCOClockGet();
      s = cmuSelect_LFRCO;
      break;

    case _CMU_EM23GRPACLKCTRL_CLKSEL_LFXO:
      f = SystemLFXOClockGet();
      s = cmuSelect_LFXO;
      break;

    case _CMU_EM23GRPACLKCTRL_CLKSEL_ULFRCO:
      f = SystemULFRCOClockGet();
      s = cmuSelect_ULFRCO;
      break;

    default:
      s = cmuSelect_Error;
      EFM_ASSERT(false);
      break;
  }

  if (freq != NULL) {
    *freq = f;
  }
  if (sel != NULL) {
    *sel = s;
  }
}

/***************************************************************************//**
 * @brief
 *   Get selected oscillator and frequency for @ref cmuClock_EM4GRPACLK
 *   clock tree.
 *
 * @param[out] freq
 *   The frequency.
 *
 * @param[out] sel
 *   The selected oscillator.
 ******************************************************************************/
static void em4GrpaClkGet(uint32_t *freq, CMU_Select_TypeDef *sel)
{
  uint32_t f = 0U;
  CMU_Select_TypeDef s;

  switch (CMU->EM4GRPACLKCTRL & _CMU_EM4GRPACLKCTRL_CLKSEL_MASK) {
    case _CMU_EM4GRPACLKCTRL_CLKSEL_LFRCO:
      f = SystemLFRCOClockGet();
      s = cmuSelect_LFRCO;
      break;

    case _CMU_EM4GRPACLKCTRL_CLKSEL_LFXO:
      f = SystemLFXOClockGet();
      s = cmuSelect_LFXO;
      break;

    case _CMU_EM4GRPACLKCTRL_CLKSEL_ULFRCO:
      f = SystemULFRCOClockGet();
      s = cmuSelect_ULFRCO;
      break;

    default:
      s = cmuSelect_Error;
      EFM_ASSERT(false);
      break;
  }

  if (freq != NULL) {
    *freq = f;
  }
  if (sel != NULL) {
    *sel = s;
  }
}

/***************************************************************************//**
 * @brief
 *   Configure flash access wait states to support the given core clock
 *   frequency and vscale level.
 *
 * @note Current implementation sets wait states depending on frequency only.
 *   This assumes that applications running on Series 2 Config 2 devices
 *   never attemtps to set core frequency above 40MHz at VSCALE1 (1.0V).
 *   Series 2 Config 1 devices does not support vscale.
 *
 * @param[in] coreFreq
 *   The core clock frequency to configure flash wait-states.
 *
 * @param[in] vscale
 *   Voltage Scale level. Supported levels are 0 and 1 where 0 is the default.
 *   @li 0 = 1.1 V (VSCALE2)
 *   @li 1 = 1.0 V (VSCALE1)
 ******************************************************************************/
static void flashWaitStateControl(uint32_t coreFreq, int vscale)
{
  (void)vscale;

  uint32_t mode;
  bool mscLocked;

#if defined(CMU_CLKEN1_MSC)
  CMU->CLKEN1_SET = CMU_CLKEN1_MSC;
#endif

#if (_SILICON_LABS_32B_SERIES_2_CONFIG > 1)
  coreFreq *= CMU_ClockDivGet(cmuClock_CORE);
#endif

  // Make sure the MSC is unlocked
  mscLocked = (MSC->STATUS & _MSC_STATUS_REGLOCK_MASK)
              == MSC_STATUS_REGLOCK_LOCKED;
  MSC->LOCK = MSC_LOCK_LOCKKEY_UNLOCK;

  // Get current flash read setting
  mode = MSC->READCTRL & ~_MSC_READCTRL_MODE_MASK;

  // Set new mode based on the core clock frequency
#if defined(_SILICON_LABS_32B_SERIES_2_CONFIG_1)
  if (coreFreq <= CMU_MAX_FLASHREAD_FREQ_0WS) {
#elif defined(EMU_VSCALE_EM01_PRESENT)
  if ((coreFreq <= CMU_MAX_FREQ_0WS_1V1) || (vscale >= 1)) {
#else
  if (coreFreq <= CMU_MAX_FREQ_0WS_1V1) {
#endif
    mode |= MSC_READCTRL_MODE_WS0;
  } else {
    mode |= MSC_READCTRL_MODE_WS1;
  }
  MSC->READCTRL = mode;

#if defined(_SILICON_LABS_32B_SERIES_2_CONFIG_1)
  // Get current sram read setting
  mode = SYSCFG->DMEM0RAMCTRL & ~_SYSCFG_DMEM0RAMCTRL_RAMWSEN_MASK;

  // Set new mode based on the core clock frequency
  if (coreFreq > CMU_MAX_SRAM_FREQ_0WS) {
    mode |= 1 << _SYSCFG_DMEM0RAMCTRL_RAMWSEN_SHIFT;
  }
  SYSCFG->DMEM0RAMCTRL = mode;
#endif
  if (mscLocked) {
    MSC->LOCK = MSC_LOCK_LOCKKEY_LOCK;
  }
}

/***************************************************************************//**
 * @brief
 *   Get calibrated HFRCODPLL tuning value from Device information (DI) page
 *   for a given frequency. Calibration value is not available for all frequency
 *   bands.
 *
 * @param[in] freq
 *   HFRCODPLL frequency band
 ******************************************************************************/
static uint32_t HFRCODPLLDevinfoGet(CMU_HFRCODPLLFreq_TypeDef freq)
{
  uint32_t ret = 0U;

  switch (freq) {
    // 1, 2 and 4MHz share the same calibration word
    case cmuHFRCODPLLFreq_1M0Hz:
    case cmuHFRCODPLLFreq_2M0Hz:
    case cmuHFRCODPLLFreq_4M0Hz:
      ret = DEVINFO->HFRCODPLLCAL[0].HFRCODPLLCAL;
      break;

    case cmuHFRCODPLLFreq_7M0Hz:
      ret = DEVINFO->HFRCODPLLCAL[3].HFRCODPLLCAL;
      break;

    case cmuHFRCODPLLFreq_13M0Hz:
      ret = DEVINFO->HFRCODPLLCAL[6].HFRCODPLLCAL;
      break;

    case cmuHFRCODPLLFreq_16M0Hz:
      ret = DEVINFO->HFRCODPLLCAL[7].HFRCODPLLCAL;
      break;

    case cmuHFRCODPLLFreq_19M0Hz:
      ret = DEVINFO->HFRCODPLLCAL[8].HFRCODPLLCAL;
      break;

    case cmuHFRCODPLLFreq_26M0Hz:
      ret = DEVINFO->HFRCODPLLCAL[10].HFRCODPLLCAL;
      break;

    case cmuHFRCODPLLFreq_32M0Hz:
      ret = DEVINFO->HFRCODPLLCAL[11].HFRCODPLLCAL;
      break;

    case cmuHFRCODPLLFreq_38M0Hz:
      ret = DEVINFO->HFRCODPLLCAL[12].HFRCODPLLCAL;
      break;

    case cmuHFRCODPLLFreq_48M0Hz:
      ret = DEVINFO->HFRCODPLLCAL[13].HFRCODPLLCAL;
      break;

    case cmuHFRCODPLLFreq_56M0Hz:
      ret = DEVINFO->HFRCODPLLCAL[14].HFRCODPLLCAL;
      break;

    case cmuHFRCODPLLFreq_64M0Hz:
      ret = DEVINFO->HFRCODPLLCAL[15].HFRCODPLLCAL;
      break;

    case cmuHFRCODPLLFreq_80M0Hz:
      ret = DEVINFO->HFRCODPLLCAL[16].HFRCODPLLCAL;
      break;

#if defined(_SILICON_LABS_32B_SERIES_2_CONFIG_5)
    case cmuHFRCODPLLFreq_100M0Hz:
      ret = DEVINFO->HFRCODPLLCAL[17].HFRCODPLLCAL;
      break;
#endif

    case cmuHFRCODPLLFreq_UserDefined:
      break;

    default:
      EFM_ASSERT(false);
      break;
  }
  return ret;
}

#if defined(IADC_PRESENT)
/***************************************************************************//**
 * @brief
 *   Get selected oscillator and frequency for @ref cmuClock_IADCCLK
 *   clock tree.
 *
 * @param[out] freq
 *   The frequency.
 *
 * @param[out] sel
 *   The selected oscillator.
 ******************************************************************************/
static void iadcClkGet(uint32_t *freq, CMU_Select_TypeDef *sel)
{
  uint32_t f = 0U;
  CMU_Select_TypeDef s;

  switch (CMU->IADCCLKCTRL & _CMU_IADCCLKCTRL_CLKSEL_MASK) {
    case _CMU_IADCCLKCTRL_CLKSEL_EM01GRPACLK:
      em01GrpaClkGet(&f, NULL);
      s = cmuSelect_EM01GRPACLK;
      break;

#if defined(HFRCOEM23_PRESENT)
    case _CMU_IADCCLKCTRL_CLKSEL_HFRCOEM23:
      f = SystemHFRCOEM23ClockGet();
      s = cmuSelect_HFRCOEM23;
      break;
#endif

    case _CMU_IADCCLKCTRL_CLKSEL_FSRCO:
      f = SystemFSRCOClockGet();
      s = cmuSelect_FSRCO;
      break;

    default:
      s = cmuSelect_Error;
      EFM_ASSERT(false);
      break;
  }

  if (freq != NULL) {
    *freq = f;
  }
  if (sel != NULL) {
    *sel = s;
  }
}
#endif

/***************************************************************************//**
 * @brief
 *   Set maximum allowed divisor for @ref cmuClock_PCLK clock tree.
 ******************************************************************************/
static void pclkDivMax(void)
{
  // Set largest divisor for PCLK clock tree.
  CMU_ClockDivSet(cmuClock_PCLK, 2U);
}

/***************************************************************************//**
 * @brief
 *   Set @ref cmuClock_PCLK clock tree divisor to achieve highest possible
 *  frequency and still be within spec.
 ******************************************************************************/
static void pclkDivOptimize(void)
{
  CMU_ClkDiv_TypeDef div = 2U;

  if (CMU_ClockFreqGet(cmuClock_HCLK) <= CMU_MAX_PCLK_FREQ) {
    div = 1U;
  }
  CMU_ClockDivSet(cmuClock_PCLK, div);
}

#if defined(RTCC_PRESENT)
/***************************************************************************//**
 * @brief
 *   Get selected oscillator and frequency for @ref cmuClock_RTCCCLK
 *   clock tree.
 *
 * @param[out] freq
 *   The frequency.
 *
 * @param[out] sel
 *   The selected oscillator.
 ******************************************************************************/
static void rtccClkGet(uint32_t *freq, CMU_Select_TypeDef *sel)
{
  uint32_t f = 0U;
  CMU_Select_TypeDef s;

  switch (CMU->RTCCCLKCTRL & _CMU_RTCCCLKCTRL_CLKSEL_MASK) {
    case _CMU_RTCCCLKCTRL_CLKSEL_LFRCO:
      f = SystemLFRCOClockGet();
      s = cmuSelect_LFRCO;
      break;

    case _CMU_RTCCCLKCTRL_CLKSEL_LFXO:
      f = SystemLFXOClockGet();
      s = cmuSelect_LFXO;
      break;

    case _CMU_RTCCCLKCTRL_CLKSEL_ULFRCO:
      f = SystemULFRCOClockGet();
      s = cmuSelect_ULFRCO;
      break;

    default:
      s = cmuSelect_Error;
      EFM_ASSERT(false);
      break;
  }

  if (freq != NULL) {
    *freq = f;
  }
  if (sel != NULL) {
    *sel = s;
  }
}
#endif

#if defined(SYSRTC_PRESENT)
/***************************************************************************//**
 * @brief
 *   Get selected oscillator and frequency for @ref cmuClock_SYSRTCCLK
 *   clock tree.
 *
 * @param[out] freq
 *   The frequency.
 *
 * @param[out] sel
 *   The selected oscillator.
 ******************************************************************************/
static void sysrtcClkGet(uint32_t *freq, CMU_Select_TypeDef *sel)
{
  uint32_t f = 0U;
  CMU_Select_TypeDef s;

  switch (CMU->SYSRTC0CLKCTRL & _CMU_SYSRTC0CLKCTRL_CLKSEL_MASK) {
    case _CMU_SYSRTC0CLKCTRL_CLKSEL_LFRCO:
      f = SystemLFRCOClockGet();
      s = cmuSelect_LFRCO;
      break;

    case _CMU_SYSRTC0CLKCTRL_CLKSEL_LFXO:
      f = SystemLFXOClockGet();
      s = cmuSelect_LFXO;
      break;

    case _CMU_SYSRTC0CLKCTRL_CLKSEL_ULFRCO:
      f = SystemULFRCOClockGet();
      s = cmuSelect_ULFRCO;
      break;

    default:
      s = cmuSelect_Error;
      EFM_ASSERT(false);
      break;
  }

  if (freq != NULL) {
    *freq = f;
  }
  if (sel != NULL) {
    *sel = s;
  }
}
#endif

/***************************************************************************//**
 * @brief
 *   Set wait-states to values valid for maximum allowable core clock frequency.
 ******************************************************************************/
static void waitStateMax(void)
{
  flashWaitStateControl(SystemMaxCoreClockGet(), 0);
}

/***************************************************************************//**
 * @brief
 *   Get selected oscillator and frequency for @ref cmuClock_WDOG0CLK
 *   clock tree.
 *
 * @param[out] freq
 *   The frequency.
 *
 * @param[out] sel
 *   The selected oscillator.
 ******************************************************************************/
static void wdog0ClkGet(uint32_t *freq, CMU_Select_TypeDef *sel)
{
  uint32_t f = 0U;
  CMU_Select_TypeDef s;

  switch (CMU->WDOG0CLKCTRL & _CMU_WDOG0CLKCTRL_CLKSEL_MASK) {
    case _CMU_WDOG0CLKCTRL_CLKSEL_LFRCO:
      f = SystemLFRCOClockGet();
      s = cmuSelect_LFRCO;
      break;

    case _CMU_WDOG0CLKCTRL_CLKSEL_LFXO:
      f = SystemLFXOClockGet();
      s = cmuSelect_LFXO;
      break;

    case _CMU_WDOG0CLKCTRL_CLKSEL_ULFRCO:
      f = SystemULFRCOClockGet();
      s = cmuSelect_ULFRCO;
      break;

    case _CMU_WDOG0CLKCTRL_CLKSEL_HCLKDIV1024:
      f = SystemHCLKGet() / 1024U;
      s = cmuSelect_HCLKDIV1024;
      break;

    default:
      s = cmuSelect_Error;
      EFM_ASSERT(false);
      break;
  }

  if (freq != NULL) {
    *freq = f;
  }
  if (sel != NULL) {
    *sel = s;
  }
}

#if defined(_SILICON_LABS_32B_SERIES_2) && WDOG_COUNT > 1
/***************************************************************************//**
 * @brief
 *   Get selected oscillator and frequency for @ref cmuClock_WDOG1CLK
 *   clock tree.
 *
 * @param[out] freq
 *   The frequency.
 *
 * @param[out] sel
 *   The selected oscillator.
 ******************************************************************************/
static void wdog1ClkGet(uint32_t *freq, CMU_Select_TypeDef *sel)
{
  uint32_t f = 0U;
  CMU_Select_TypeDef s;

  switch (CMU->WDOG1CLKCTRL & _CMU_WDOG1CLKCTRL_CLKSEL_MASK) {
    case _CMU_WDOG1CLKCTRL_CLKSEL_LFRCO:
      f = SystemLFRCOClockGet();
      s = cmuSelect_LFRCO;
      break;

    case _CMU_WDOG1CLKCTRL_CLKSEL_LFXO:
      f = SystemLFXOClockGet();
      s = cmuSelect_LFXO;
      break;

    case _CMU_WDOG1CLKCTRL_CLKSEL_ULFRCO:
      f = SystemULFRCOClockGet();
      s = cmuSelect_ULFRCO;
      break;

    case _CMU_WDOG1CLKCTRL_CLKSEL_HCLKDIV1024:
      f = SystemHCLKGet() / 1024U;
      s = cmuSelect_HCLKDIV1024;
      break;

    default:
      s = cmuSelect_Error;
      EFM_ASSERT(false);
      break;
  }

  if (freq != NULL) {
    *freq = f;
  }
  if (sel != NULL) {
    *sel = s;
  }
}
#endif // defined(_SILICON_LABS_32B_SERIES_2) && WDOG_COUNT > 1

/***************************************************************************//**
 * @brief
 *   Get selected oscillator and frequency for @ref cmuClock_SYSTICK
 *   clock tree.
 *
 * @param[out] freq
 *   The frequency.
 *
 * @param[out] sel
 *   The selected oscillator.
 ******************************************************************************/
static void sysTickClkGet(uint32_t *freq, CMU_Select_TypeDef *sel)
{
  uint32_t f = 0U;
  CMU_Select_TypeDef s;

  if (SysTick->CTRL & SysTick_CTRL_CLKSOURCE_Msk) {
    f = SystemHCLKGet();
    s = cmuSelect_HCLK;
  } else {
    em23GrpaClkGet(&f, &s);
  }

  if (freq != NULL) {
    *freq = f;
  }
  if (sel != NULL) {
    *sel = s;
  }
}

#if defined(USB_PRESENT)
/***************************************************************************//**
 * @brief
 *   Get selected oscillator and frequency for @ref cmuClock_USB clock tree.
 *
 * @param[out] freq
 *   The frequency.
 *
 * @param[out] sel
 *   The selected oscillator.
 ******************************************************************************/
static void usbClkGet(uint32_t *freq, CMU_Select_TypeDef *sel)
{
  uint32_t f = 0U;
  CMU_Select_TypeDef s;

  switch (CMU->USB0CLKCTRL & _CMU_USB0CLKCTRL_CLKSEL_MASK) {
    case _CMU_USB0CLKCTRL_CLKSEL_PLL0:
      f = PLL0_USB_OUTPUT_FREQ;
      s = cmuSelect_PLL0;
      break;

    case _CMU_USB0CLKCTRL_CLKSEL_LFXO:
      f = SystemLFXOClockGet();
      s = cmuSelect_LFXO;
      break;

    case _CMU_USB0CLKCTRL_CLKSEL_LFRCO:
      f = SystemLFRCOClockGet();
      s = cmuSelect_LFRCO;
      break;

    default:
      s = cmuSelect_Error;
      EFM_ASSERT(false);
      break;
  }

  if (freq != NULL) {
    *freq = f;
  }
  if (sel != NULL) {
    *sel = s;
  }
}
#endif

/** @endcond */

#else // defined(_SILICON_LABS_32B_SERIES_2)

/*******************************************************************************
 ******************************   DEFINES   ************************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

#if defined(_SILICON_LABS_32B_SERIES_0)
/** The maximum allowed core frequency when using 0 wait-states on flash access. */
#define CMU_MAX_FREQ_0WS        16000000
/** The maximum allowed core frequency when using 1 wait-states on flash access */
#define CMU_MAX_FREQ_1WS        32000000

#elif (_SILICON_LABS_GECKO_INTERNAL_SDID == 80)
// EFR32xG1x and EFM32xG1x
#define CMU_MAX_FREQ_0WS_1V2    25000000
#define CMU_MAX_FREQ_1WS_1V2    40000000

#elif (_SILICON_LABS_GECKO_INTERNAL_SDID == 84)
// EFR32xG12x and EFM32xG12x
#define CMU_MAX_FREQ_0WS_1V2    25000000
#define CMU_MAX_FREQ_1WS_1V2    40000000
#define CMU_MAX_FREQ_0WS_1V1    21330000
#define CMU_MAX_FREQ_1WS_1V1    32000000
#define CMU_MAX_FREQ_0WS_1V0     7000000
#define CMU_MAX_FREQ_1WS_1V0    14000000
#define CMU_MAX_FREQ_2WS_1V0    20000000

#elif (_SILICON_LABS_GECKO_INTERNAL_SDID == 89)
// EFR32xG13x and EFM32xG13x
#define CMU_MAX_FREQ_0WS_1V2    25000000
#define CMU_MAX_FREQ_1WS_1V2    40000000
#define CMU_MAX_FREQ_0WS_1V0     7000000
#define CMU_MAX_FREQ_1WS_1V0    14000000
#define CMU_MAX_FREQ_2WS_1V0    20000000

#elif (_SILICON_LABS_GECKO_INTERNAL_SDID == 95)
// EFR32xG14x and EFM32xG14x
#define CMU_MAX_FREQ_0WS_1V2    25000000
#define CMU_MAX_FREQ_1WS_1V2    40000000
#define CMU_MAX_FREQ_0WS_1V0     7000000
#define CMU_MAX_FREQ_1WS_1V0    14000000
#define CMU_MAX_FREQ_2WS_1V0    20000000

#elif (_SILICON_LABS_GECKO_INTERNAL_SDID == 100)
// EFM32GG11x
#define CMU_MAX_FREQ_0WS_1V2    18000000
#define CMU_MAX_FREQ_1WS_1V2    36000000
#define CMU_MAX_FREQ_2WS_1V2    54000000
#define CMU_MAX_FREQ_3WS_1V2    72000000
#define CMU_MAX_FREQ_0WS_1V0     7000000
#define CMU_MAX_FREQ_1WS_1V0    14000000
#define CMU_MAX_FREQ_2WS_1V0    20000000

#elif (_SILICON_LABS_GECKO_INTERNAL_SDID == 103)
// EFM32TG11x
#define CMU_MAX_FREQ_0WS_1V2    25000000
#define CMU_MAX_FREQ_1WS_1V2    48000000
#define CMU_MAX_FREQ_0WS_1V0    10000000
#define CMU_MAX_FREQ_1WS_1V0    21000000
#define CMU_MAX_FREQ_2WS_1V0    20000000

#elif (_SILICON_LABS_GECKO_INTERNAL_SDID == 106)
// EFM32GG12x
#define CMU_MAX_FREQ_0WS_1V2    18000000
#define CMU_MAX_FREQ_1WS_1V2    36000000
#define CMU_MAX_FREQ_2WS_1V2    54000000
#define CMU_MAX_FREQ_3WS_1V2    72000000
#define CMU_MAX_FREQ_0WS_1V0    7000000
#define CMU_MAX_FREQ_1WS_1V0    14000000
#define CMU_MAX_FREQ_2WS_1V0    20000000

#else
#error "Max Flash wait-state frequencies are not defined for this platform."
#endif

/** The maximum frequency for the HFLE interface. */
#if defined(CMU_CTRL_HFLE)
/** The maximum HFLE frequency for series 0 EFM32 and EZR32 Wonder Gecko. */
#if defined(_SILICON_LABS_32B_SERIES_0) \
  && (defined(_EFM32_WONDER_FAMILY)     \
  || defined(_EZR32_WONDER_FAMILY))
#define CMU_MAX_FREQ_HFLE                       24000000UL
/** The maximum HFLE frequency for other series 0 parts with maximum core clock
    higher than 32 MHz. */
#elif defined(_SILICON_LABS_32B_SERIES_0) \
  && (defined(_EFM32_GIANT_FAMILY)        \
  || defined(_EZR32_LEOPARD_FAMILY))
#define CMU_MAX_FREQ_HFLE                       maxFreqHfle()
#endif
#elif defined(CMU_CTRL_WSHFLE)
/** The maximum HFLE frequency for series 1 parts. */
#define CMU_MAX_FREQ_HFLE                       32000000UL
#endif

#if defined(CMU_STATUS_HFXOSHUNTOPTRDY)
#define HFXO_TUNING_READY_FLAGS  (CMU_STATUS_HFXOPEAKDETRDY | CMU_STATUS_HFXOSHUNTOPTRDY)
#define HFXO_TUNING_MODE_AUTO    (_CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_AUTOCMD)
#define HFXO_TUNING_MODE_CMD     (_CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_CMD)
#elif defined(CMU_STATUS_HFXOPEAKDETRDY)
#define HFXO_TUNING_READY_FLAGS  (CMU_STATUS_HFXOPEAKDETRDY)
#define HFXO_TUNING_MODE_AUTO    (_CMU_HFXOCTRL_PEAKDETMODE_AUTOCMD)
#define HFXO_TUNING_MODE_CMD     (_CMU_HFXOCTRL_PEAKDETMODE_CMD)
#endif

#if defined(CMU_HFXOCTRL_MODE_EXTCLK)
/** HFXO external clock mode is renamed from EXTCLK to DIGEXTCLK. */
#define CMU_HFXOCTRL_MODE_DIGEXTCLK     CMU_HFXOCTRL_MODE_EXTCLK
#endif

#if defined(_EMU_CMD_EM01VSCALE0_MASK)
#define VSCALE_DEFAULT    ((int)EMU_VScaleGet())
#else
#define VSCALE_DEFAULT    VSCALE_EM01_HIGH_PERFORMANCE
#endif

/*******************************************************************************
 **************************   LOCAL VARIABLES   ********************************
 ******************************************************************************/

#if defined(_CMU_AUXHFRCOCTRL_FREQRANGE_MASK)
static CMU_AUXHFRCOFreq_TypeDef auxHfrcoFreq = cmuAUXHFRCOFreq_19M0Hz;
#endif
#if defined(_CMU_STATUS_HFXOSHUNTOPTRDY_MASK)
#define HFXO_INVALID_TRIM   (~_CMU_HFXOTRIMSTATUS_MASK)
#endif

#if defined(CMU_OSCENCMD_DPLLEN)
/** A table of HFRCOCTRL values and their associated minimum/maximum frequencies and
    an optional band enumerator. */
static const struct hfrcoCtrlTableElement{
  uint32_t              minFreq;
  uint32_t              maxFreq;
  uint32_t              value;
  CMU_HFRCOFreq_TypeDef band;
} hfrcoCtrlTable[] =
{
  // minFreq  maxFreq   HFRCOCTRL value  band
  {  860000UL, 1050000UL, 0xBC601F00UL, cmuHFRCOFreq_1M0Hz       },
  { 1050000UL, 1280000UL, 0xBC611F35UL, (CMU_HFRCOFreq_TypeDef)0 },
  { 1280000UL, 1480000UL, 0xBCA21F35UL, (CMU_HFRCOFreq_TypeDef)0 },
  { 1480000UL, 1800000UL, 0xAD231F35UL, (CMU_HFRCOFreq_TypeDef)0 },
  { 1800000UL, 2110000UL, 0xBA601F00UL, cmuHFRCOFreq_2M0Hz       },
  { 2110000UL, 2560000UL, 0xBA611F35UL, (CMU_HFRCOFreq_TypeDef)0 },
  { 2560000UL, 2970000UL, 0xBAA21F35UL, (CMU_HFRCOFreq_TypeDef)0 },
  { 2970000UL, 3600000UL, 0xAB231F35UL, (CMU_HFRCOFreq_TypeDef)0 },
  { 3600000UL, 4220000UL, 0xB8601F00UL, cmuHFRCOFreq_4M0Hz       },
  { 4220000UL, 5120000UL, 0xB8611F35UL, (CMU_HFRCOFreq_TypeDef)0 },
  { 5120000UL, 5930000UL, 0xB8A21F35UL, (CMU_HFRCOFreq_TypeDef)0 },
  { 5930000UL, 7520000UL, 0xA9231F00UL, cmuHFRCOFreq_7M0Hz       },
  { 7520000UL, 9520000UL, 0x99241F35UL, (CMU_HFRCOFreq_TypeDef)0 },
  { 9520000UL, 11800000UL, 0x99251F35UL, (CMU_HFRCOFreq_TypeDef)0 },
  { 11800000UL, 14400000UL, 0x99261F00UL, cmuHFRCOFreq_13M0Hz     },
  { 14400000UL, 17200000UL, 0x99271F00UL, cmuHFRCOFreq_16M0Hz     },
  { 17200000UL, 19700000UL, 0x99481F00UL, cmuHFRCOFreq_19M0Hz     },
  { 19700000UL, 23800000UL, 0x99491F35UL, (CMU_HFRCOFreq_TypeDef)0 },
  { 23800000UL, 28700000UL, 0x994A1F00UL, cmuHFRCOFreq_26M0Hz      },
  { 28700000UL, 34800000UL, 0x996B1F00UL, cmuHFRCOFreq_32M0Hz      },
#if defined(_SILICON_LABS_GECKO_INTERNAL_SDID_84)  \
  || defined(_SILICON_LABS_GECKO_INTERNAL_SDID_89) \
  || defined(_SILICON_LABS_GECKO_INTERNAL_SDID_95)
  { 34800000UL, 40000000UL, 0x996C1F00UL, cmuHFRCOFreq_38M0Hz      }
#elif defined(_SILICON_LABS_GECKO_INTERNAL_SDID_100) \
  || defined(_SILICON_LABS_GECKO_INTERNAL_SDID_106)
  { 34800000UL, 42800000UL, 0x996C1F00UL, cmuHFRCOFreq_38M0Hz      },
  { 42800000UL, 51600000UL, 0x996D1F00UL, cmuHFRCOFreq_48M0Hz      },
  { 51600000UL, 60500000UL, 0x998E1F00UL, cmuHFRCOFreq_56M0Hz      },
  { 60500000UL, 72000000UL, 0xA98F1F00UL, cmuHFRCOFreq_64M0Hz      }
#elif defined(_SILICON_LABS_GECKO_INTERNAL_SDID_103)
  { 34800000UL, 42800000UL, 0x996C1F00UL, cmuHFRCOFreq_38M0Hz      },
  { 42800000UL, 48000000UL, 0x996D1F00UL, cmuHFRCOFreq_48M0Hz      }
#else
  #error "HFRCOCTRL values not set for this platform."
#endif
};

#define HFRCOCTRLTABLE_ENTRIES (sizeof(hfrcoCtrlTable) \
                                / sizeof(struct hfrcoCtrlTableElement))
#endif // CMU_OSCENCMD_DPLLEN

#if defined(_SILICON_LABS_32B_SERIES_1) && defined(_EMU_STATUS_VSCALE_MASK)
/* Devices with Voltage Scaling needs extra handling of wait states. */
static const struct flashWsTableElement{
  uint32_t maxFreq;
  uint8_t  vscale;
  uint8_t  ws;
} flashWsTable[] =
{
#if (_SILICON_LABS_GECKO_INTERNAL_SDID == 100 || _SILICON_LABS_GECKO_INTERNAL_SDID == 106)
  { CMU_MAX_FREQ_0WS_1V2, 0, 0 },  /* 0 wait states at max frequency 18 MHz and 1.2V */
  { CMU_MAX_FREQ_1WS_1V2, 0, 1 },  /* 1 wait states at max frequency 36 MHz and 1.2V */
  { CMU_MAX_FREQ_2WS_1V2, 0, 2 },  /* 2 wait states at max frequency 54 MHz and 1.2V */
  { CMU_MAX_FREQ_3WS_1V2, 0, 3 },  /* 3 wait states at max frequency 72 MHz and 1.2V */
  { CMU_MAX_FREQ_0WS_1V0, 2, 0 },  /* 0 wait states at max frequency 7 MHz and 1.0V */
  { CMU_MAX_FREQ_1WS_1V0, 2, 1 },  /* 1 wait states at max frequency 14 MHz and 1.0V */
  { CMU_MAX_FREQ_2WS_1V0, 2, 2 },  /* 2 wait states at max frequency 21 MHz and 1.0V */
#else
  { CMU_MAX_FREQ_0WS_1V2, 0, 0 },  /* 0 wait states at 1.2V */
  { CMU_MAX_FREQ_1WS_1V2, 0, 1 },  /* 1 wait states at 1.2V */
  { CMU_MAX_FREQ_0WS_1V0, 2, 0 },  /* 0 wait states at 1.0V */
  { CMU_MAX_FREQ_1WS_1V0, 2, 1 },  /* 1 wait states at 1.0V */
  { CMU_MAX_FREQ_2WS_1V0, 2, 2 },  /* 2 wait states at 1.0V */
#endif
};

#define FLASH_WS_TABLE_ENTRIES (sizeof(flashWsTable) / sizeof(flashWsTable[0]))
#endif

#if defined(_CMU_USHFRCOCTRL_FREQRANGE_MASK) \
  || defined(_CMU_USHFRCOTUNE_MASK)
#ifndef EFM32_USHFRCO_STARTUP_FREQ
#define EFM32_USHFRCO_STARTUP_FREQ        (48000000UL)
#endif

static uint32_t ushfrcoFreq = EFM32_USHFRCO_STARTUP_FREQ;
#endif

/*******************************************************************************
 **************************   LOCAL PROTOTYPES   *******************************
 ******************************************************************************/
#if defined(_CMU_HFRCOCTRL_FREQRANGE_MASK)
static uint32_t CMU_HFRCODevinfoGet(CMU_HFRCOFreq_TypeDef freq);
#endif

#if defined(_CMU_USHFRCOCTRL_FREQRANGE_MASK)
static uint32_t CMU_USHFRCODevinfoGet(CMU_USHFRCOFreq_TypeDef freq);
#endif

static void hfperClkSafePrescaler(void);
static void hfperClkOptimizedPrescaler(void);

static uint16_t lfxo_precision = 0xFFFF;

/** @endcond */

/*******************************************************************************
 **************************   LOCAL FUNCTIONS   ********************************
 ******************************************************************************/

#if defined(_SILICON_LABS_32B_SERIES_0) \
  && (defined(_EFM32_GIANT_FAMILY)      \
  || defined(_EZR32_LEOPARD_FAMILY))
/***************************************************************************//**
 * @brief
 *   Return maximum allowed frequency for low energy peripherals.
 ******************************************************************************/
static uint32_t maxFreqHfle(void)
{
  uint16_t majorMinorRev;

  switch (SYSTEM_GetFamily()) {
    case systemPartFamilyEfm32Leopard:
    case systemPartFamilyEzr32Leopard:
      /* CHIP MAJOR bit [5:0] */
      majorMinorRev = (((ROMTABLE->PID0 & _ROMTABLE_PID0_REVMAJOR_MASK)
                        >> _ROMTABLE_PID0_REVMAJOR_SHIFT) << 8);
      /* CHIP MINOR bit [7:4] */
      majorMinorRev |= (((ROMTABLE->PID2 & _ROMTABLE_PID2_REVMINORMSB_MASK)
                         >> _ROMTABLE_PID2_REVMINORMSB_SHIFT) << 4);
      /* CHIP MINOR bit [3:0] */
      majorMinorRev |=  ((ROMTABLE->PID3 & _ROMTABLE_PID3_REVMINORLSB_MASK)
                         >> _ROMTABLE_PID3_REVMINORLSB_SHIFT);

      if (majorMinorRev >= 0x0204) {
        return 24000000;
      } else {
        return 32000000;
      }

    case systemPartFamilyEfm32Giant:
      return 32000000;

    default:
      /* Invalid device family. */
      EFM_ASSERT(false);
      return 0;
  }
}
#endif

#if defined(CMU_MAX_FREQ_HFLE)

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
/* Unified definitions for the HFLE wait-state and prescaler fields. */
#if defined(CMU_CTRL_HFLE)
#define _GENERIC_HFLE_WS_MASK           _CMU_CTRL_HFLE_MASK
#define _GENERIC_HFLE_WS_SHIFT          _CMU_CTRL_HFLE_SHIFT
#define GENERIC_HFLE_PRESC_REG          CMU->HFCORECLKDIV
#define _GENERIC_HFLE_PRESC_MASK        _CMU_HFCORECLKDIV_HFCORECLKLEDIV_MASK
#define _GENERIC_HFLE_PRESC_SHIFT       _CMU_HFCORECLKDIV_HFCORECLKLEDIV_SHIFT
#elif defined(CMU_CTRL_WSHFLE)
#define _GENERIC_HFLE_WS_MASK           _CMU_CTRL_WSHFLE_MASK
#define _GENERIC_HFLE_WS_SHIFT          _CMU_CTRL_WSHFLE_SHIFT
#define GENERIC_HFLE_PRESC_REG          CMU->HFPRESC
#define _GENERIC_HFLE_PRESC_MASK        _CMU_HFPRESC_HFCLKLEPRESC_MASK
#define _GENERIC_HFLE_PRESC_SHIFT       _CMU_HFPRESC_HFCLKLEPRESC_SHIFT
#endif
/** @endcond */

/***************************************************************************//**
 * @brief
 *   Set HFLE wait-states and HFCLKLE prescaler according to wanted HF clock.
 *
 * @param[in] hfFreq
 *   The HF clock frequency to use.
 *   This is:
 *     CORE clock on Series0 devices.
 *     HF clock on on Series1 devices.
 ******************************************************************************/
static void setHfLeConfig(uint32_t hfFreq)
{
  unsigned int hfleWs;
  uint32_t hflePresc;

  /* Check for 1 bit fields. @ref BUS_RegBitWrite() below are going to fail if the
     fields are changed to more than 1 bit. */
  EFM_ASSERT((_GENERIC_HFLE_WS_MASK >> _GENERIC_HFLE_WS_SHIFT) == 0x1U);

  /* - Enable HFLE wait-state to allow access to LE peripherals when HFBUSCLK is
       above maxLeFreq.
     - Set HFLE prescaler. Allowed HFLE clock frequency is maxLeFreq. */

  hfleWs = 1;
  if (hfFreq <= CMU_MAX_FREQ_HFLE) {
    hfleWs = 0;
    hflePresc = 0;
  } else if (hfFreq <= (2UL * CMU_MAX_FREQ_HFLE)) {
    hflePresc = 1;
  } else {
    hflePresc = 2;
  }
  BUS_RegBitWrite(&CMU->CTRL, _GENERIC_HFLE_WS_SHIFT, hfleWs);
  GENERIC_HFLE_PRESC_REG = (GENERIC_HFLE_PRESC_REG & ~_GENERIC_HFLE_PRESC_MASK)
                           | (hflePresc << _GENERIC_HFLE_PRESC_SHIFT);
}

#if defined(_CMU_CTRL_HFLE_MASK)
/***************************************************************************//**
 * @brief
 *   Get HFLE wait-state configuration.
 *
 * @return
 *   The current wait-state configuration.
 ******************************************************************************/
static uint32_t getHfLeConfig(void)
{
  uint32_t ws = BUS_RegBitRead(&CMU->CTRL, _GENERIC_HFLE_WS_SHIFT);
  return ws;
}
#endif
#endif

/***************************************************************************//**
 * @brief
 *   Get the AUX clock frequency. Used by MSC flash programming and LESENSE,
 *   by default also as a debug clock.
 *
 * @return
 *   AUX Frequency in Hz.
 ******************************************************************************/
static uint32_t auxClkGet(void)
{
  uint32_t ret;

#if defined(_CMU_AUXHFRCOCTRL_FREQRANGE_MASK)
  ret = (uint32_t)auxHfrcoFreq;

#elif defined(_CMU_AUXHFRCOCTRL_BAND_MASK)
  /* All series 0 families except EFM32G */
  switch (CMU->AUXHFRCOCTRL & _CMU_AUXHFRCOCTRL_BAND_MASK) {
    case CMU_AUXHFRCOCTRL_BAND_1MHZ:
      if ( SYSTEM_GetProdRev() >= 19 ) {
        ret = 1200000;
      } else {
        ret = 1000000;
      }
      break;

    case CMU_AUXHFRCOCTRL_BAND_7MHZ:
      if ( SYSTEM_GetProdRev() >= 19 ) {
        ret = 6600000;
      } else {
        ret = 7000000;
      }
      break;

    case CMU_AUXHFRCOCTRL_BAND_11MHZ:
      ret = 11000000;
      break;

    case CMU_AUXHFRCOCTRL_BAND_14MHZ:
      ret = 14000000;
      break;

    case CMU_AUXHFRCOCTRL_BAND_21MHZ:
      ret = 21000000;
      break;

#if defined(_CMU_AUXHFRCOCTRL_BAND_28MHZ)
    case CMU_AUXHFRCOCTRL_BAND_28MHZ:
      ret = 28000000;
      break;
#endif

    default:
      ret = 0;
      EFM_ASSERT(false);
      break;
  }

#else
  /* Gecko has a fixed 14 MHz AUXHFRCO clock. */
  ret = 14000000;

#endif

  return ret;
}

#if defined (_CMU_ADCCTRL_ADC0CLKSEL_HFSRCCLK) \
  || defined (_CMU_ADCCTRL_ADC1CLKSEL_HFSRCCLK)
/***************************************************************************//**
 * @brief
 *   Get the HFSRCCLK frequency.
 *
 * @return
 *   HFSRCCLK Frequency in Hz.
 ******************************************************************************/
static uint32_t hfSrcClkGet(void)
{
  uint32_t ret;

  ret = SystemHFClockGet();
  return ret * (1U + ((CMU->HFPRESC & _CMU_HFPRESC_PRESC_MASK)
                      >> _CMU_HFPRESC_PRESC_SHIFT));
}
#endif

/***************************************************************************//**
 * @brief
 *   Get the Debug Trace clock frequency.
 *
 * @return
 *   Debug Trace frequency in Hz.
 ******************************************************************************/
static uint32_t dbgClkGet(void)
{
  uint32_t ret;
  CMU_Select_TypeDef clk;

  /* Get selected clock source */
  clk = CMU_ClockSelectGet(cmuClock_DBG);

  switch (clk) {
    case cmuSelect_HFCLK:
      ret = SystemHFClockGet();
      break;

    case cmuSelect_AUXHFRCO:
      ret = auxClkGet();
      break;

    default:
      ret = 0;
      EFM_ASSERT(false);
      break;
  }
  return ret;
}

#if defined(_CMU_ADCCTRL_ADC0CLKSEL_MASK)
/***************************************************************************//**
 * @brief
 *   Get the ADC n asynchronous clock frequency.
 *
 * @return
 *   ADC n asynchronous frequency in Hz.
 ******************************************************************************/
static uint32_t adcAsyncClkGet(uint32_t adc)
{
  uint32_t ret;
  CMU_Select_TypeDef clk;

  /* Get the selected clock source. */
  switch (adc) {
    case 0:
      clk = CMU_ClockSelectGet(cmuClock_ADC0ASYNC);
      break;

#if defined(_CMU_ADCCTRL_ADC1CLKSEL_MASK)
    case 1:
      clk = CMU_ClockSelectGet(cmuClock_ADC1ASYNC);
      break;
#endif

    default:
      EFM_ASSERT(false);
      return 0;
  }

  switch (clk) {
    case cmuSelect_Disabled:
      ret = 0;
      break;

    case cmuSelect_AUXHFRCO:
      ret = auxClkGet();
      break;

    case cmuSelect_HFXO:
      ret = SystemHFXOClockGet();
      break;

    case cmuSelect_HFSRCCLK:
      ret = hfSrcClkGet();
      break;

    default:
      ret = 0;
      EFM_ASSERT(false);
      break;
  }
  return ret;
}
#endif

#if defined(_CMU_SDIOCTRL_MASK)
/***************************************************************************//**
 * @brief
 *   Get the SDIO reference clock frequency.
 *
 * @return
 *   SDIO reference clock frequency in Hz.
 ******************************************************************************/
static uint32_t sdioRefClkGet(void)
{
  uint32_t ret;
  CMU_Select_TypeDef clk;

  /* Get the selected clock source. */
  clk = CMU_ClockSelectGet(cmuClock_SDIOREF);

  switch (clk) {
    case cmuSelect_HFRCO:
      ret = SystemHfrcoFreq;
      break;

    case cmuSelect_HFXO:
      ret = SystemHFXOClockGet();
      break;

    case cmuSelect_AUXHFRCO:
      ret = auxClkGet();
      break;

    case cmuSelect_USHFRCO:
      ret = ushfrcoFreq;
      break;

    default:
      ret = 0;
      EFM_ASSERT(false);
      break;
  }
  return ret;
}
#endif

#if defined(_CMU_QSPICTRL_MASK)
/***************************************************************************//**
 * @brief
 *   Get the QSPI n reference clock frequency.
 *
 * @return
 *   QSPI n reference clock frequency in Hz.
 ******************************************************************************/
static uint32_t qspiRefClkGet(uint32_t qspi)
{
  uint32_t ret;
  CMU_Select_TypeDef clk;

  /* Get the selected clock source. */
  switch (qspi) {
    case 0:
      clk = CMU_ClockSelectGet(cmuClock_QSPI0REF);
      break;

    default:
      EFM_ASSERT(false);
      return 0;
  }

  switch (clk) {
    case cmuSelect_HFRCO:
      ret = SystemHfrcoFreq;
      break;

    case cmuSelect_HFXO:
      ret = SystemHFXOClockGet();
      break;

    case cmuSelect_AUXHFRCO:
      ret = auxClkGet();
      break;

    case cmuSelect_USHFRCO:
      ret = ushfrcoFreq;
      break;

    default:
      ret = 0;
      EFM_ASSERT(false);
      break;
  }
  return ret;
}
#endif

#if defined(_CMU_PDMCTRL_MASK)
/***************************************************************************//**
 * @brief
 *   Get the PDM reference clock frequency.
 *
 * @return
 *   PDM reference clock frequency in Hz.
 ******************************************************************************/
static uint32_t pdmRefClkGet(void)
{
  uint32_t ret;
  CMU_Select_TypeDef clk;

  /* Get the selected clock source. */
  clk = CMU_ClockSelectGet(cmuClock_PDMREF);

  switch (clk) {
    case cmuSelect_HFRCO:
      ret = SystemHfrcoFreq;
      break;

    case cmuSelect_HFXO:
      ret = SystemHFXOClockGet();
      break;

    case cmuSelect_USHFRCO:
      ret = ushfrcoFreq;
      break;

    default:
      ret = 0;
      EFM_ASSERT(false);
      break;
  }
  return ret;
}
#endif

#if defined(USBR_CLOCK_PRESENT)
/***************************************************************************//**
 * @brief
 *   Get the USB rate clock frequency.
 *
 * @return
 *   USB rate clock frequency in Hz.
 ******************************************************************************/
static uint32_t usbRateClkGet(void)
{
  uint32_t ret;
  CMU_Select_TypeDef clk;

  clk = CMU_ClockSelectGet(cmuClock_USBR);

  switch (clk) {
    case cmuSelect_USHFRCO:
      ret = ushfrcoFreq;
      break;

    case cmuSelect_HFXO:
      ret = SystemHFXOClockGet();
      break;

    case cmuSelect_HFXOX2:
      ret = 2u * SystemHFXOClockGet();
      break;

    case cmuSelect_HFRCO:
      ret = SystemHfrcoFreq;
      break;

    case cmuSelect_LFXO:
      ret = SystemLFXOClockGet();
      break;

    case cmuSelect_LFRCO:
      ret = SystemLFRCOClockGet();
      break;

    default:
      ret = 0;
      EFM_ASSERT(false);
      break;
  }
  return ret;
}
#endif

/***************************************************************************//**
 * @brief
 *   Configure flash access wait states to support the given core clock
 *   frequency.
 *
 * @param[in] coreFreq
 *   The core clock frequency to configure flash wait-states.
 *
 * @param[in] vscale
 *   Voltage Scale level. Supported levels are 0 and 2 where 0 is the default.
 ******************************************************************************/
static void flashWaitStateControl(uint32_t coreFreq, int vscale)
{
  uint32_t mode;
#if defined(MSC_READCTRL_MODE_WS0SCBTP)
  bool scbtpEn;   /* Suppressed Conditional Branch Target Prefetch setting. */
#endif
  (void) vscale;  /* vscale parameter is only used on some devices. */

  /* Get mode and SCBTP enable. */
  mode = MSC->READCTRL & _MSC_READCTRL_MODE_MASK;

#if defined(_SILICON_LABS_32B_SERIES_0)
#if defined(MSC_READCTRL_MODE_WS0SCBTP)
  /* Devices with MODE and SCBTP in the same register field. */
  switch (mode) {
    case MSC_READCTRL_MODE_WS0:
    case MSC_READCTRL_MODE_WS1:
#if defined(MSC_READCTRL_MODE_WS2)
    case MSC_READCTRL_MODE_WS2:
#endif
      scbtpEn = false;
      break;

    default: /* WSxSCBTP */
      scbtpEn = true;
      break;
  }

  /* Set mode based on the core clock frequency and SCBTP enable. */
  if (false) {
  }
#if defined(MSC_READCTRL_MODE_WS2)
  else if (coreFreq > CMU_MAX_FREQ_1WS) {
    mode = (scbtpEn ? MSC_READCTRL_MODE_WS2SCBTP : MSC_READCTRL_MODE_WS2);
  }
#endif
  else if ((coreFreq <= CMU_MAX_FREQ_1WS) && (coreFreq > CMU_MAX_FREQ_0WS)) {
    mode = (scbtpEn ? MSC_READCTRL_MODE_WS1SCBTP : MSC_READCTRL_MODE_WS1);
  } else {
    mode = (scbtpEn ? MSC_READCTRL_MODE_WS0SCBTP : MSC_READCTRL_MODE_WS0);
  }
#else /* defined(MSC_READCTRL_MODE_WS0SCBTP) */

  if (coreFreq <= CMU_MAX_FREQ_0WS) {
    mode = 0;
  } else if (coreFreq <= CMU_MAX_FREQ_1WS) {
    mode = 1;
  }
#endif /* defined(MSC_READCTRL_MODE_WS0SCBTP) */
// End defined(_SILICON_LABS_32B_SERIES_0)

#elif defined(_SILICON_LABS_32B_SERIES_1)
#if defined(_EMU_STATUS_VSCALE_MASK)

  /* These devices have specific requirements on the supported flash wait state
   * depending on the frequency and voltage scale level. */
  uint32_t i;
  for (i = 0; i < FLASH_WS_TABLE_ENTRIES; i++) {
    if ((flashWsTable[i].vscale == (uint8_t)vscale)
        && (coreFreq <= flashWsTable[i].maxFreq)) {
      break; // Found a matching entry.
    }
  }

  if (i == FLASH_WS_TABLE_ENTRIES) {
    mode = 3; // Worst case flash wait state for unsupported cases.
    EFM_ASSERT(false);
  } else {
    mode = flashWsTable[i].ws;
  }
  mode = mode << _MSC_READCTRL_MODE_SHIFT;

#else
  /* Devices where MODE and SCBTP are in separate fields and where the device
   * either does not support voltage scale or where the voltage scale does
   * not impact the flash wait state configuration. */
  if (coreFreq <= CMU_MAX_FREQ_0WS_1V2) {
    mode = 0;
  } else if (coreFreq <= CMU_MAX_FREQ_1WS_1V2) {
    mode = 1;
  }
#if defined(MSC_READCTRL_MODE_WS2)
  else if (coreFreq <= CMU_MAX_FREQ_2WS) {
    mode = 2;
  }
#endif
#if defined(MSC_READCTRL_MODE_WS3)
  else if (coreFreq <= CMU_MAX_FREQ_3WS) {
    mode = 3;
  }
#endif
  mode = mode << _MSC_READCTRL_MODE_SHIFT;
#endif
// End defined(_SILICON_LABS_32B_SERIES_1)

#else
#error "Undefined 32B SERIES!"
#endif

  /* BUS_RegMaskedWrite cannot be used as it would temporarily set the
     mode field to WS0. */
  MSC->READCTRL = (MSC->READCTRL & ~_MSC_READCTRL_MODE_MASK) | mode;
}

/***************************************************************************//**
 * @brief
 *   Configure flash access wait states to the most conservative setting for
 *   this target. Retain SCBTP (Suppressed Conditional Branch Target Prefetch)
 *   setting.
 ******************************************************************************/
static void flashWaitStateMax(void)
{
  /* Make sure the MSC is unlocked */
  bool mscLocked = MSC->LOCK != 0UL;
  MSC->LOCK = MSC_UNLOCK_CODE;

  flashWaitStateControl(SystemMaxCoreClockGet(), 0);

  if (mscLocked) {
    MSC->LOCK = 0;
  }
}

#if defined(_MSC_RAMCTRL_RAMWSEN_MASK)
/***************************************************************************//**
 * @brief
 *   Configure RAM access wait states to support the given core clock
 *   frequency.
 *
 * @param[in] coreFreq
 *   The core clock frequency to configure RAM wait-states.
 *
 * @param[in] vscale
 *   A voltage scale level. Supported levels are 0 and 2 where 0 is the default.
 ******************************************************************************/
static void setRamWaitState(uint32_t coreFreq, int vscale)
{
  uint32_t limit = 38000000;
  if (vscale == 2) {
    limit = 16000000;
  }

  if (coreFreq > limit) {
    BUS_RegMaskedSet(&MSC->RAMCTRL, (MSC_RAMCTRL_RAMWSEN
                                     | MSC_RAMCTRL_RAM1WSEN
                                     | MSC_RAMCTRL_RAM2WSEN));
  } else {
    BUS_RegMaskedClear(&MSC->RAMCTRL, (MSC_RAMCTRL_RAMWSEN
                                       | MSC_RAMCTRL_RAM1WSEN
                                       | MSC_RAMCTRL_RAM2WSEN));
  }
}
#endif

#if defined(_MSC_CTRL_WAITMODE_MASK)
/***************************************************************************//**
 * @brief
 *   Configure the wait state for peripheral accesses over the bus to support
 *   the given bus clock frequency.
 *
 * @param[in] busFreq
 *   A peripheral bus clock frequency to configure wait-states.
 *
 * @param[in] vscale
 *   The voltage scale to configure wait-states. Expected values are
 *   0 or 2.
 *
 *   @li 0 = 1.2 V (VSCALE2)
 *   @li 2 = 1.0 V (VSCALE0)
 * ******************************************************************************/
static void setBusWaitState(uint32_t busFreq, int vscale)
{
  if ((busFreq > 50000000) && (vscale == 0)) {
    BUS_RegMaskedSet(&MSC->CTRL, MSC_CTRL_WAITMODE_WS1);
  } else {
    BUS_RegMaskedClear(&MSC->CTRL, MSC_CTRL_WAITMODE_WS1);
  }
}
#endif

#if defined(PLFRCO_PRESENT)
static bool deviceHasPlfrco(void)
{
  SYSTEM_ChipRevision_TypeDef rev;

  if (_SILICON_LABS_GECKO_INTERNAL_SDID == 89) {
    // check the xG13 rev and check if it's >= Rev A3
    SYSTEM_ChipRevisionGet(&rev);
    return (rev.major > 1) || (rev.minor >= 3);
  } else {
    return false;
  }
}
#endif

/***************************************************************************//**
 * @brief
 *   Configure various wait states to switch to a certain frequency
 *   and a certain voltage scale.
 *
 * @details
 *   This function will set up the necessary flash, bus, and RAM wait states.
 *   Updating the wait state configuration must be done before
 *   increasing the clock frequency and it must be done after decreasing the
 *   clock frequency. Updating the wait state configuration must be done before
 *   core voltage is decreased and it must be done after a core voltage is
 *   increased.
 *
 * @param[in] freq
 *   The core clock frequency to configure wait-states.
 *
 * @param[in] vscale
 *   The voltage scale to configure wait-states. Expected values are
 *   0 or 2, higher number is lower voltage.
 *
 *   @li 0 = 1.2 V (VSCALE2)
 *   @li 2 = 1.0 V (VSCALE0)
 *
 ******************************************************************************/
void CMU_UpdateWaitStates(uint32_t freq, int vscale)
{
  /* Make sure the MSC is unlocked */
  bool mscLocked = MSC->LOCK != 0UL;
  MSC->LOCK = MSC_UNLOCK_CODE;

  flashWaitStateControl(freq, vscale);
#if defined(_MSC_RAMCTRL_RAMWSEN_MASK)
  setRamWaitState(freq, vscale);
#endif
#if defined(_MSC_CTRL_WAITMODE_MASK)
  setBusWaitState(freq, vscale);
#endif

  if (mscLocked) {
    MSC->LOCK = 0;
  }
}

#if defined(_CMU_HFXOSTEADYSTATECTRL_REGISHUPPER_MASK)
/***************************************************************************//**
 * @brief
 *   Return the upper value for CMU_HFXOSTEADYSTATECTRL_REGISH.
 ******************************************************************************/
static uint32_t getRegIshUpperVal(uint32_t steadyStateRegIsh)
{
  uint32_t regIshUpper;
  const uint32_t upperMax = _CMU_HFXOSTEADYSTATECTRL_REGISHUPPER_MASK
                            >> _CMU_HFXOSTEADYSTATECTRL_REGISHUPPER_SHIFT;
  /* Add 3 as specified in the register description for CMU_HFXOSTEADYSTATECTRL_REGISHUPPER. */
  regIshUpper = SL_MIN(steadyStateRegIsh + 3UL, upperMax);
  regIshUpper <<= _CMU_HFXOSTEADYSTATECTRL_REGISHUPPER_SHIFT;
  return regIshUpper;
}
#endif

#if defined(_CMU_HFXOCTRL_MASK)
/***************************************************************************//**
 * @brief
 *   Get the HFXO tuning mode.
 *
 * @return
 *   The current HFXO tuning mode from the HFXOCTRL register.
 ******************************************************************************/
__STATIC_INLINE uint32_t getHfxoTuningMode(void)
{
#if defined(_CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_MASK)
  return (CMU->HFXOCTRL & _CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_MASK)
         >> _CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_SHIFT;
#else
  return (CMU->HFXOCTRL & _CMU_HFXOCTRL_PEAKDETMODE_MASK)
         >> _CMU_HFXOCTRL_PEAKDETMODE_SHIFT;
#endif
}

/***************************************************************************//**
 * @brief
 *   Set the HFXO tuning mode.
 *
 * @param[in] mode
 *   The new HFXO tuning mode. This can be HFXO_TUNING_MODE_AUTO or
 *   HFXO_TUNING_MODE_CMD.
 ******************************************************************************/
__STATIC_INLINE void setHfxoTuningMode(uint32_t mode)
{
#if defined(_CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_MASK)
  CMU->HFXOCTRL = (CMU->HFXOCTRL & ~_CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_MASK)
                  | (mode << _CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_SHIFT);
#else
  CMU->HFXOCTRL = (CMU->HFXOCTRL & ~_CMU_HFXOCTRL_PEAKDETMODE_MASK)
                  | (mode << _CMU_HFXOCTRL_PEAKDETMODE_SHIFT);
#endif
}
#endif

/***************************************************************************//**
 * @brief
 *   Get the LFnCLK frequency based on the current configuration.
 *
 * @param[in] lfClkBranch
 *   Selected LF branch.
 *
 * @return
 *   The LFnCLK frequency in Hz. If no LFnCLK is selected (disabled), 0 is
 *   returned.
 ******************************************************************************/
static uint32_t lfClkGet(CMU_Clock_TypeDef lfClkBranch)
{
  uint32_t sel;
  uint32_t ret = 0;

  switch (lfClkBranch) {
    case cmuClock_LFA:
    case cmuClock_LFB:
#if defined(_CMU_LFCCLKEN0_MASK)
    case cmuClock_LFC:
#endif
#if defined(_CMU_LFECLKSEL_MASK)
    case cmuClock_LFE:
#endif
      break;

    default:
      EFM_ASSERT(false);
      break;
  }

  sel = (uint32_t)CMU_ClockSelectGet(lfClkBranch);

  /* Get clock select field */
  switch (lfClkBranch) {
    case cmuClock_LFA:
#if defined(_CMU_LFCLKSEL_MASK)
      sel = (CMU->LFCLKSEL & _CMU_LFCLKSEL_LFA_MASK) >> _CMU_LFCLKSEL_LFA_SHIFT;
#elif defined(_CMU_LFACLKSEL_MASK)
      sel = (CMU->LFACLKSEL & _CMU_LFACLKSEL_LFA_MASK) >> _CMU_LFACLKSEL_LFA_SHIFT;
#else
      EFM_ASSERT(false);
#endif
      break;

    case cmuClock_LFB:
#if defined(_CMU_LFCLKSEL_MASK)
      sel = (CMU->LFCLKSEL & _CMU_LFCLKSEL_LFB_MASK) >> _CMU_LFCLKSEL_LFB_SHIFT;
#elif defined(_CMU_LFBCLKSEL_MASK)
      sel = (CMU->LFBCLKSEL & _CMU_LFBCLKSEL_LFB_MASK) >> _CMU_LFBCLKSEL_LFB_SHIFT;
#else
      EFM_ASSERT(false);
#endif
      break;

#if defined(_CMU_LFCCLKEN0_MASK)
    case cmuClock_LFC:
#if defined(_CMU_LFCLKSEL_LFC_MASK)
      sel = (CMU->LFCLKSEL & _CMU_LFCLKSEL_LFC_MASK) >> _CMU_LFCLKSEL_LFC_SHIFT;
#elif defined(_CMU_LFCCLKSEL_LFC_MASK)
      sel = (CMU->LFCCLKSEL & _CMU_LFCCLKSEL_LFC_MASK) >> _CMU_LFCCLKSEL_LFC_SHIFT;
#else
      EFM_ASSERT(false);
#endif
      break;
#endif

#if defined(_CMU_LFECLKSEL_MASK)
    case cmuClock_LFE:
      sel = (CMU->LFECLKSEL & _CMU_LFECLKSEL_LFE_MASK) >> _CMU_LFECLKSEL_LFE_SHIFT;
      break;
#endif

    default:
      EFM_ASSERT(false);
      break;
  }

  /* Get the clock frequency. */
#if defined(_CMU_LFCLKSEL_MASK)
  switch (sel) {
    case _CMU_LFCLKSEL_LFA_LFRCO:
      ret = SystemLFRCOClockGet();
      break;

    case _CMU_LFCLKSEL_LFA_LFXO:
      ret = SystemLFXOClockGet();
      break;

#if defined(_CMU_LFCLKSEL_LFA_HFCORECLKLEDIV2)
    case _CMU_LFCLKSEL_LFA_HFCORECLKLEDIV2:
#if defined(CMU_MAX_FREQ_HFLE)
      /* HFLE bit is or'ed by hardware with HFCORECLKLEDIV to reduce the
       * frequency of CMU_HFCORECLKLEDIV2. */
      ret = SystemCoreClockGet() / (1U << (getHfLeConfig() + 1));
#else
      ret = SystemCoreClockGet() / 2U;
#endif
      break;
#endif

    case _CMU_LFCLKSEL_LFA_DISABLED:
      ret = 0;
#if defined(CMU_LFCLKSEL_LFAE)
      /* Check LF Extended bit setting for LFA or LFB ULFRCO clock. */
      if ((lfClkBranch == cmuClock_LFA) || (lfClkBranch == cmuClock_LFB)) {
        if (CMU->LFCLKSEL >> (lfClkBranch == cmuClock_LFA
                              ? _CMU_LFCLKSEL_LFAE_SHIFT
                              : _CMU_LFCLKSEL_LFBE_SHIFT)) {
          ret = SystemULFRCOClockGet();
        }
      }
#endif
      break;

    default:
      ret = 0U;
      EFM_ASSERT(false);
      break;
  }
#endif /* _CMU_LFCLKSEL_MASK */

#if defined(_CMU_LFACLKSEL_MASK)
  switch (sel) {
    case _CMU_LFACLKSEL_LFA_LFRCO:
      ret = SystemLFRCOClockGet();
      break;

    case _CMU_LFACLKSEL_LFA_LFXO:
      ret = SystemLFXOClockGet();
      break;

    case _CMU_LFACLKSEL_LFA_ULFRCO:
      ret = SystemULFRCOClockGet();
      break;

#if defined(PLFRCO_PRESENT)
    case _CMU_LFACLKSEL_LFA_PLFRCO:
      ret = SystemLFRCOClockGet();
      break;
#endif

#if defined(_CMU_LFBCLKSEL_LFB_HFCLKLE)
    case _CMU_LFBCLKSEL_LFB_HFCLKLE:
      ret = SystemHFClockGet()
            / SL_Log2ToDiv(((CMU->HFPRESC & _CMU_HFPRESC_HFCLKLEPRESC_MASK)
                            >> _CMU_HFPRESC_HFCLKLEPRESC_SHIFT) + 1UL);
      break;
#endif

    case _CMU_LFACLKSEL_LFA_DISABLED:
      ret = 0;
      break;

    default:
      ret = 0U;
      EFM_ASSERT(false);
      break;
  }
#endif

  return ret;
}

/***************************************************************************//**
 * @brief
 *   Wait for an ongoing sync of register(s) to low-frequency domain to complete.
 *
 * @param[in] mask
 *   A bitmask corresponding to SYNCBUSY register defined bits, indicating
 *   registers that must complete any ongoing synchronization.
 ******************************************************************************/
__STATIC_INLINE void syncReg(uint32_t mask)
{
  /* Avoid a deadlock if modifying the same register twice when freeze mode is */
  /* activated. */
  if ((CMU->FREEZE & CMU_FREEZE_REGFREEZE) != 0UL) {
    return;
  }

  /* Wait for any pending previous write operation to complete */
  /* in low-frequency domain. */
  while ((CMU->SYNCBUSY & mask) != 0UL) {
  }
}

#if defined(USBC_CLOCK_PRESENT)
/***************************************************************************//**
 * @brief
 *   Get the USBC frequency.
 *
 * @return
 *   USBC frequency in Hz.
 ******************************************************************************/
static uint32_t usbCClkGet(void)
{
  uint32_t ret;
  CMU_Select_TypeDef clk;

  /* Get the selected clock source. */
  clk = CMU_ClockSelectGet(cmuClock_USBC);

  switch (clk) {
    case cmuSelect_LFXO:
      ret = SystemLFXOClockGet();
      break;
    case cmuSelect_LFRCO:
      ret = SystemLFRCOClockGet();
      break;
#if defined (_CMU_USHFRCOCTRL_MASK)
    case cmuSelect_USHFRCO:
      ret = ushfrcoFreq;
      break;
#endif
    case cmuSelect_HFCLK:
      ret = SystemHFClockGet();
      break;
    default:
      /* Clock is not enabled */
      ret = 0;
      break;
  }
  return ret;
}
#endif

/***************************************************************************//**
 * @brief
 *   Set HFPER clock tree prescalers to safe values.
 *
 * @note
 *   This function applies to EFM32GG11B. There are 3 HFPER clock trees with
 *   these frequency limits:
 *     HFPERCLK  (A-tree): 20MHz in VSCALE0 mode, 50MHz in VSCALE2 mode.
 *     HFPERBCLK (B-tree): 20MHz in VSCALE0 mode, 72MHz in VSCALE2 mode.
 *     HFPERCCLK (C-tree): 20MHz in VSCALE0 mode, 50MHz in VSCALE2 mode.
 ******************************************************************************/
static void hfperClkSafePrescaler(void)
{
#if defined(_CMU_HFPERPRESC_MASK) && defined(_CMU_HFPERPRESCB_MASK) \
  && defined(_CMU_HFPERPRESCC_MASK)
  // Assuming a maximum HFCLK of 72MHz, set prescalers to DIV4.
  CMU_ClockPrescSet(cmuClock_HFPER, 3U);
  CMU_ClockPrescSet(cmuClock_HFPERB, 3U);
  CMU_ClockPrescSet(cmuClock_HFPERC, 3U);
#endif
}

/***************************************************************************//**
 * @brief
 *   Set HFPER clock tree prescalers to give highest possible clock node
 *   frequency while still beeing within spec.
 *
 * @note
 *   This function applies to EFM32GG11B. There are 3 HFPER clock trees with
 *   these frequency limits:
 *     HFPERCLK  (A-tree): 20MHz in VSCALE0 mode, 50MHz in VSCALE2 mode.
 *     HFPERBCLK (B-tree): 20MHz in VSCALE0 mode, 72MHz in VSCALE2 mode.
 *     HFPERCCLK (C-tree): 20MHz in VSCALE0 mode, 50MHz in VSCALE2 mode.
 ******************************************************************************/
static void hfperClkOptimizedPrescaler(void)
{
#if defined(_CMU_HFPERPRESC_MASK) && defined(_CMU_HFPERPRESCB_MASK) \
  && defined(_CMU_HFPERPRESCC_MASK)
  uint32_t hfClkFreq, divisor;

  hfClkFreq = SystemHFClockGet();

  if ( EMU_VScaleGet() == emuVScaleEM01_LowPower) {
    divisor = (hfClkFreq + 20000000U - 1U) / 20000000U; // ceil(x)
    if (divisor > 0U) {
      divisor--;                                        // Convert to prescaler
    }
    CMU_ClockPrescSet(cmuClock_HFPER, divisor);
    CMU_ClockPrescSet(cmuClock_HFPERB, divisor);
    CMU_ClockPrescSet(cmuClock_HFPERC, divisor);
  } else {
    divisor = (hfClkFreq + 50000000U - 1U) / 50000000U;
    if (divisor > 0U) {
      divisor--;
    }
    CMU_ClockPrescSet(cmuClock_HFPER, divisor);
    CMU_ClockPrescSet(cmuClock_HFPERC, divisor);

    divisor = (hfClkFreq + 72000000U - 1U) / 72000000U;
    if (divisor > 0U) {
      divisor--;
    }
    CMU_ClockPrescSet(cmuClock_HFPERB, divisor);
  }
#endif
}

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

#if defined(_CMU_AUXHFRCOCTRL_BAND_MASK)
/***************************************************************************//**
 * @brief
 *   Get the AUXHFRCO band in use.
 *
 * @return
 *   AUXHFRCO band in use.
 ******************************************************************************/
CMU_AUXHFRCOBand_TypeDef CMU_AUXHFRCOBandGet(void)
{
  return (CMU_AUXHFRCOBand_TypeDef)((CMU->AUXHFRCOCTRL
                                     & _CMU_AUXHFRCOCTRL_BAND_MASK)
                                    >> _CMU_AUXHFRCOCTRL_BAND_SHIFT);
}
#endif /* _CMU_AUXHFRCOCTRL_BAND_MASK */

#if defined(_CMU_AUXHFRCOCTRL_BAND_MASK)
/***************************************************************************//**
 * @brief
 *   Set the AUXHFRCO band and the tuning value based on the value in the
 *   calibration table made during production.
 *
 * @param[in] band
 *   AUXHFRCO band to activate.
 ******************************************************************************/
void CMU_AUXHFRCOBandSet(CMU_AUXHFRCOBand_TypeDef band)
{
  uint32_t tuning;

  /* Read a tuning value from the calibration table. */
  switch (band) {
    case cmuAUXHFRCOBand_1MHz:
      tuning = (DEVINFO->AUXHFRCOCAL0 & _DEVINFO_AUXHFRCOCAL0_BAND1_MASK)
               >> _DEVINFO_AUXHFRCOCAL0_BAND1_SHIFT;
      break;

    case cmuAUXHFRCOBand_7MHz:
      tuning = (DEVINFO->AUXHFRCOCAL0 & _DEVINFO_AUXHFRCOCAL0_BAND7_MASK)
               >> _DEVINFO_AUXHFRCOCAL0_BAND7_SHIFT;
      break;

    case cmuAUXHFRCOBand_11MHz:
      tuning = (DEVINFO->AUXHFRCOCAL0 & _DEVINFO_AUXHFRCOCAL0_BAND11_MASK)
               >> _DEVINFO_AUXHFRCOCAL0_BAND11_SHIFT;
      break;

    case cmuAUXHFRCOBand_14MHz:
      tuning = (DEVINFO->AUXHFRCOCAL0 & _DEVINFO_AUXHFRCOCAL0_BAND14_MASK)
               >> _DEVINFO_AUXHFRCOCAL0_BAND14_SHIFT;
      break;

    case cmuAUXHFRCOBand_21MHz:
      tuning = (DEVINFO->AUXHFRCOCAL1 & _DEVINFO_AUXHFRCOCAL1_BAND21_MASK)
               >> _DEVINFO_AUXHFRCOCAL1_BAND21_SHIFT;
      break;

#if defined(_CMU_AUXHFRCOCTRL_BAND_28MHZ)
    case cmuAUXHFRCOBand_28MHz:
      tuning = (DEVINFO->AUXHFRCOCAL1 & _DEVINFO_AUXHFRCOCAL1_BAND28_MASK)
               >> _DEVINFO_AUXHFRCOCAL1_BAND28_SHIFT;
      break;
#endif

    default:
      EFM_ASSERT(false);
      return;
  }

  /* Set band/tuning. */
  CMU->AUXHFRCOCTRL = (CMU->AUXHFRCOCTRL
                       & ~(_CMU_AUXHFRCOCTRL_BAND_MASK
                           | _CMU_AUXHFRCOCTRL_TUNING_MASK))
                      | (band << _CMU_AUXHFRCOCTRL_BAND_SHIFT)
                      | (tuning << _CMU_AUXHFRCOCTRL_TUNING_SHIFT);
}
#endif /* _CMU_AUXHFRCOCTRL_BAND_MASK */

#if defined(_CMU_AUXHFRCOCTRL_FREQRANGE_MASK)
/**************************************************************************//**
 * @brief
 *   Get the AUXHFRCO frequency calibration word in DEVINFO.
 *
 * @param[in] freq
 *   Frequency in Hz.
 *
 * @return
 *   AUXHFRCO calibration word for a given frequency.
 *****************************************************************************/
static uint32_t CMU_AUXHFRCODevinfoGet(CMU_AUXHFRCOFreq_TypeDef freq)
{
  switch (freq) {
    /* 1, 2, and 4 MHz share the same calibration word. */
    case cmuAUXHFRCOFreq_1M0Hz:
    case cmuAUXHFRCOFreq_2M0Hz:
    case cmuAUXHFRCOFreq_4M0Hz:
      return DEVINFO->AUXHFRCOCAL0;

    case cmuAUXHFRCOFreq_7M0Hz:
      return DEVINFO->AUXHFRCOCAL3;

    case cmuAUXHFRCOFreq_13M0Hz:
      return DEVINFO->AUXHFRCOCAL6;

    case cmuAUXHFRCOFreq_16M0Hz:
      return DEVINFO->AUXHFRCOCAL7;

    case cmuAUXHFRCOFreq_19M0Hz:
      return DEVINFO->AUXHFRCOCAL8;

    case cmuAUXHFRCOFreq_26M0Hz:
      return DEVINFO->AUXHFRCOCAL10;

    case cmuAUXHFRCOFreq_32M0Hz:
      return DEVINFO->AUXHFRCOCAL11;

    case cmuAUXHFRCOFreq_38M0Hz:
      return DEVINFO->AUXHFRCOCAL12;

#if defined(_DEVINFO_AUXHFRCOCAL13_MASK)
    case cmuAUXHFRCOFreq_48M0Hz:
      return DEVINFO->AUXHFRCOCAL13;
#endif
#if defined(_DEVINFO_AUXHFRCOCAL14_MASK)
    case cmuAUXHFRCOFreq_50M0Hz:
      return DEVINFO->AUXHFRCOCAL14;
#endif

    default: /* cmuAUXHFRCOFreq_UserDefined */
      return 0;
  }
}
#endif /* _CMU_AUXHFRCOCTRL_FREQRANGE_MASK */

#if defined(_CMU_AUXHFRCOCTRL_FREQRANGE_MASK)
/***************************************************************************//**
 * @brief
 *   Get the current AUXHFRCO frequency.
 *
 * @return
 *   AUXHFRCO frequency.
 ******************************************************************************/
CMU_AUXHFRCOFreq_TypeDef CMU_AUXHFRCOBandGet(void)
{
  return auxHfrcoFreq;
}
#endif /* _CMU_AUXHFRCOCTRL_FREQRANGE_MASK */

#if defined(_CMU_AUXHFRCOCTRL_FREQRANGE_MASK)
/***************************************************************************//**
 * @brief
 *   Set AUXHFRCO calibration for the selected target frequency.
 *
 * @param[in] setFreq
 *   AUXHFRCO frequency to set
 ******************************************************************************/
void CMU_AUXHFRCOBandSet(CMU_AUXHFRCOFreq_TypeDef setFreq)
{
  uint32_t freqCal;

  /* Get DEVINFO index and set global auxHfrcoFreq. */
  freqCal = CMU_AUXHFRCODevinfoGet(setFreq);
  EFM_ASSERT((freqCal != 0UL) && (freqCal != UINT_MAX));
  auxHfrcoFreq = setFreq;

  /* Wait for any previous sync to complete, then set calibration data
     for the selected frequency.  */
  while (BUS_RegBitRead(&CMU->SYNCBUSY,
                        _CMU_SYNCBUSY_AUXHFRCOBSY_SHIFT) != 0UL) {
  }

  /* Set a divider in AUXHFRCOCTRL for 1, 2, and 4 MHz. */
  switch (setFreq) {
    case cmuAUXHFRCOFreq_1M0Hz:
      freqCal = (freqCal & ~_CMU_AUXHFRCOCTRL_CLKDIV_MASK)
                | CMU_AUXHFRCOCTRL_CLKDIV_DIV4;
      break;

    case cmuAUXHFRCOFreq_2M0Hz:
      freqCal = (freqCal & ~_CMU_AUXHFRCOCTRL_CLKDIV_MASK)
                | CMU_AUXHFRCOCTRL_CLKDIV_DIV2;
      break;

    case cmuAUXHFRCOFreq_4M0Hz:
      freqCal = (freqCal & ~_CMU_AUXHFRCOCTRL_CLKDIV_MASK)
                | CMU_AUXHFRCOCTRL_CLKDIV_DIV1;
      break;

    default:
      break;
  }
  CMU->AUXHFRCOCTRL = freqCal;
}
#endif /* _CMU_AUXHFRCOCTRL_FREQRANGE_MASK */

/***************************************************************************//**
 * @brief
 *   Calibrate the clock.
 *
 * @details
 *   Run a calibration for HFCLK against a selectable reference clock.
 *   See the reference manual, CMU chapter, for more details.
 *
 * @note
 *   This function will not return until the calibration measurement is completed.
 *
 * @param[in] HFCycles
 *   The number of HFCLK cycles to run the calibration. Increasing this number
 *   increases precision but the calibration will take more time.
 *
 * @param[in] reference
 *   The reference clock used to compare HFCLK.
 *
 * @return
 *   The number of ticks the reference clock after HFCycles ticks on the HF
 *   clock.
 ******************************************************************************/
uint32_t CMU_Calibrate(uint32_t HFCycles, CMU_Osc_TypeDef reference)
{
  EFM_ASSERT(HFCycles <= (_CMU_CALCNT_CALCNT_MASK >> _CMU_CALCNT_CALCNT_SHIFT));

  /* Set the reference clock source. */
  switch (reference) {
    case cmuOsc_LFXO:
      CMU->CALCTRL = CMU_CALCTRL_UPSEL_LFXO;
      break;

    case cmuOsc_LFRCO:
      CMU->CALCTRL = CMU_CALCTRL_UPSEL_LFRCO;
      break;

#if defined(PLFRCO_PRESENT)
    case cmuOsc_PLFRCO:
      CMU->CALCTRL = CMU_CALCTRL_UPSEL_PLFRCO;
      break;
#endif

    case cmuOsc_HFXO:
      CMU->CALCTRL = CMU_CALCTRL_UPSEL_HFXO;
      break;

    case cmuOsc_HFRCO:
      CMU->CALCTRL = CMU_CALCTRL_UPSEL_HFRCO;
      break;

    case cmuOsc_AUXHFRCO:
      CMU->CALCTRL = CMU_CALCTRL_UPSEL_AUXHFRCO;
      break;

#if defined (_CMU_USHFRCOCTRL_MASK)
    case cmuOsc_USHFRCO:
      CMU->CALCTRL = CMU_CALCTRL_UPSEL_USHFRCO;
      break;
#endif

    default:
      EFM_ASSERT(false);
      return 0;
  }

  /* Set the top value. */
  CMU->CALCNT = HFCycles;

  /* Start the calibration. */
  CMU->CMD = CMU_CMD_CALSTART;

#if defined(CMU_STATUS_CALRDY)
  /* Wait until calibration completes. */
  while (BUS_RegBitRead(&CMU->STATUS, _CMU_STATUS_CALRDY_SHIFT) == 0UL) {
  }
#else
  /* Wait until calibration completes. */
  while (BUS_RegBitRead(&CMU->STATUS, _CMU_STATUS_CALBSY_SHIFT) != 0UL) {
  }
#endif

  return CMU->CALCNT;
}

#if defined(_CMU_CALCTRL_UPSEL_MASK) && defined(_CMU_CALCTRL_DOWNSEL_MASK)
/***************************************************************************//**
 * @brief
 *   Configure the clock calibration.
 *
 * @details
 *   Configure a calibration for a selectable clock source against another
 *   selectable reference clock.
 *   See the reference manual, CMU chapter, for more details.
 *
 * @note
 *   After configuration, a call to @ref CMU_CalibrateStart() is required and
 *   the resulting calibration value can be read out with the
 *   @ref CMU_CalibrateCountGet() function call.
 *
 * @param[in] downCycles
 *   The number of downSel clock cycles to run the calibration. Increasing this
 *   number increases precision but the calibration will take more time.
 *
 * @param[in] downSel
 *   The clock, which will be counted down downCycles.
 *
 * @param[in] upSel
 *   The reference clock; the number of cycles generated by this clock will
 *   be counted and added up and the result can be given with the
 *   @ref CMU_CalibrateCountGet() function call.
 ******************************************************************************/
void CMU_CalibrateConfig(uint32_t downCycles, CMU_Osc_TypeDef downSel,
                         CMU_Osc_TypeDef upSel)
{
  /* Keep configuration settings untouched. */
  uint32_t calCtrl = CMU->CALCTRL
                     & ~(_CMU_CALCTRL_UPSEL_MASK | _CMU_CALCTRL_DOWNSEL_MASK);

  /* 20 bits of precision to calibration count register. */
  EFM_ASSERT(downCycles <= (_CMU_CALCNT_CALCNT_MASK >> _CMU_CALCNT_CALCNT_SHIFT));

  /* Set down counting clock source - down counter. */
  switch (downSel) {
    case cmuOsc_LFXO:
      calCtrl |= CMU_CALCTRL_DOWNSEL_LFXO;
      break;

    case cmuOsc_LFRCO:
      calCtrl |= CMU_CALCTRL_DOWNSEL_LFRCO;
      break;

#if defined(PLFRCO_PRESENT)
    case cmuOsc_PLFRCO:
      calCtrl |= CMU_CALCTRL_DOWNSEL_PLFRCO;
      break;
#endif

    case cmuOsc_HFXO:
      calCtrl |= CMU_CALCTRL_DOWNSEL_HFXO;
      break;

    case cmuOsc_HFRCO:
      calCtrl |= CMU_CALCTRL_DOWNSEL_HFRCO;
      break;

    case cmuOsc_AUXHFRCO:
      calCtrl |= CMU_CALCTRL_DOWNSEL_AUXHFRCO;
      break;

#if defined (_CMU_USHFRCOCTRL_MASK)
    case cmuOsc_USHFRCO:
      calCtrl |= CMU_CALCTRL_DOWNSEL_USHFRCO;
      break;
#endif

    default:
      EFM_ASSERT(false);
      break;
  }

  /* Set the top value to be counted down by the downSel clock. */
  CMU->CALCNT = downCycles;

  /* Set the reference clock source - up counter. */
  switch (upSel) {
    case cmuOsc_LFXO:
      calCtrl |= CMU_CALCTRL_UPSEL_LFXO;
      break;

    case cmuOsc_LFRCO:
      calCtrl |= CMU_CALCTRL_UPSEL_LFRCO;
      break;

#if defined(PLFRCO_PRESENT)
    case cmuOsc_PLFRCO:
      calCtrl |= CMU_CALCTRL_UPSEL_PLFRCO;
      break;
#endif

    case cmuOsc_HFXO:
      calCtrl |= CMU_CALCTRL_UPSEL_HFXO;
      break;

    case cmuOsc_HFRCO:
      calCtrl |= CMU_CALCTRL_UPSEL_HFRCO;
      break;

    case cmuOsc_AUXHFRCO:
      calCtrl |= CMU_CALCTRL_UPSEL_AUXHFRCO;
      break;

#if defined (_CMU_USHFRCOCTRL_MASK)
    case cmuOsc_USHFRCO:
      calCtrl |= CMU_CALCTRL_UPSEL_USHFRCO;
      break;
#endif

    default:
      EFM_ASSERT(false);
      break;
  }

  CMU->CALCTRL = calCtrl;
}
#endif

/***************************************************************************//**
 * @brief
 *    Get the calibration count register.
 * @note
 *    If continuous calibration mode is active, calibration busy will almost
 *    always be off and only the value needs to be read. In a normal case,
 *    this function call is triggered by the CALRDY
 *    interrupt flag.
 * @return
 *    The calibration count, the number of UPSEL clocks
 *    in the period of DOWNSEL oscillator clock cycles configured by a previous
 *    write operation to CMU->CALCNT.
 ******************************************************************************/
uint32_t CMU_CalibrateCountGet(void)
{
  /* Wait until calibration completes, UNLESS continuous calibration mode is  */
  /* active. */
#if defined(CMU_CALCTRL_CONT)
  if (BUS_RegBitRead(&CMU->CALCTRL, _CMU_CALCTRL_CONT_SHIFT) == 0UL) {
#if defined(CMU_STATUS_CALRDY)
    /* Wait until calibration completes */
    while (BUS_RegBitRead(&CMU->STATUS, _CMU_STATUS_CALRDY_SHIFT) == 0UL) {
    }
#else
    /* Wait until calibration completes */
    while (BUS_RegBitRead(&CMU->STATUS, _CMU_STATUS_CALBSY_SHIFT) != 0UL) {
    }
#endif
  }
#else
  while (BUS_RegBitRead(&CMU->STATUS, _CMU_STATUS_CALBSY_SHIFT) != 0UL) {
  }
#endif
  return CMU->CALCNT;
}

/***************************************************************************//**
 * @brief
 *   Get the clock divisor/prescaler.
 *
 * @param[in] clock
 *   A clock point to get the divisor/prescaler for. Notice that not all clock points
 *   have a divisor/prescaler. See the CMU overview in the reference manual.
 *
 * @return
 *   The current clock point divisor/prescaler. 1 is returned
 *   if @p clock specifies a clock point without a divisor/prescaler.
 ******************************************************************************/
CMU_ClkDiv_TypeDef CMU_ClockDivGet(CMU_Clock_TypeDef clock)
{
#if defined(_SILICON_LABS_32B_SERIES_1)
  return 1UL + (uint32_t)CMU_ClockPrescGet(clock);

#elif defined(_SILICON_LABS_32B_SERIES_0)
  uint32_t           divReg;
  CMU_ClkDiv_TypeDef ret;

  /* Get divisor reg ID. */
  divReg = (clock >> CMU_DIV_REG_POS) & CMU_DIV_REG_MASK;

  switch (divReg) {
#if defined(_CMU_CTRL_HFCLKDIV_MASK)
    case CMU_HFCLKDIV_REG:
      ret = 1 + ((CMU->CTRL & _CMU_CTRL_HFCLKDIV_MASK)
                 >> _CMU_CTRL_HFCLKDIV_SHIFT);
      break;
#endif

    case CMU_HFPERCLKDIV_REG:
      ret = (CMU_ClkDiv_TypeDef)((CMU->HFPERCLKDIV
                                  & _CMU_HFPERCLKDIV_HFPERCLKDIV_MASK)
                                 >> _CMU_HFPERCLKDIV_HFPERCLKDIV_SHIFT);
      ret = SL_Log2ToDiv(ret);
      break;

    case CMU_HFCORECLKDIV_REG:
      ret = (CMU_ClkDiv_TypeDef)((CMU->HFCORECLKDIV
                                  & _CMU_HFCORECLKDIV_HFCORECLKDIV_MASK)
                                 >> _CMU_HFCORECLKDIV_HFCORECLKDIV_SHIFT);
      ret = SL_Log2ToDiv(ret);
      break;

#if defined(_CMU_HFCORECLKDIV_HFCORECLKLEDIV_MASK)
    case CMU_HFCORECLKLEDIV_REG:
      ret = (CMU_ClkDiv_TypeDef)((CMU->HFCORECLKDIV
                                  & _CMU_HFCORECLKDIV_HFCORECLKLEDIV_MASK)
                                 >> _CMU_HFCORECLKDIV_HFCORECLKLEDIV_SHIFT);
      ret = SL_Log2ToDiv(ret + 1U);
      break;
#endif

    case CMU_LFAPRESC0_REG:
      switch (clock) {
        case cmuClock_RTC:
          ret = (CMU_ClkDiv_TypeDef)((CMU->LFAPRESC0 & _CMU_LFAPRESC0_RTC_MASK)
                                     >> _CMU_LFAPRESC0_RTC_SHIFT);
          ret = SL_Log2ToDiv(ret);
          break;

#if defined(_CMU_LFAPRESC0_LETIMER0_MASK)
        case cmuClock_LETIMER0:
          ret = (CMU_ClkDiv_TypeDef)((CMU->LFAPRESC0 & _CMU_LFAPRESC0_LETIMER0_MASK)
                                     >> _CMU_LFAPRESC0_LETIMER0_SHIFT);
          ret = SL_Log2ToDiv(ret);
          break;
#endif

#if defined(_CMU_LFAPRESC0_LCD_MASK)
        case cmuClock_LCDpre:
          ret = (CMU_ClkDiv_TypeDef)(((CMU->LFAPRESC0 & _CMU_LFAPRESC0_LCD_MASK)
                                      >> _CMU_LFAPRESC0_LCD_SHIFT)
                                     + CMU_DivToLog2(cmuClkDiv_16));
          ret = SL_Log2ToDiv(ret);
          break;
#endif

#if defined(_CMU_LFAPRESC0_LESENSE_MASK)
        case cmuClock_LESENSE:
          ret = (CMU_ClkDiv_TypeDef)((CMU->LFAPRESC0 & _CMU_LFAPRESC0_LESENSE_MASK)
                                     >> _CMU_LFAPRESC0_LESENSE_SHIFT);
          ret = SL_Log2ToDiv(ret);
          break;
#endif

        default:
          ret = cmuClkDiv_1;
          EFM_ASSERT(false);
          break;
      }
      break;

    case CMU_LFBPRESC0_REG:
      switch (clock) {
#if defined(_CMU_LFBPRESC0_LEUART0_MASK)
        case cmuClock_LEUART0:
          ret = (CMU_ClkDiv_TypeDef)((CMU->LFBPRESC0 & _CMU_LFBPRESC0_LEUART0_MASK)
                                     >> _CMU_LFBPRESC0_LEUART0_SHIFT);
          ret = SL_Log2ToDiv(ret);
          break;
#endif

#if defined(_CMU_LFBPRESC0_LEUART1_MASK)
        case cmuClock_LEUART1:
          ret = (CMU_ClkDiv_TypeDef)((CMU->LFBPRESC0 & _CMU_LFBPRESC0_LEUART1_MASK)
                                     >> _CMU_LFBPRESC0_LEUART1_SHIFT);
          ret = SL_Log2ToDiv(ret);
          break;
#endif

        default:
          ret = cmuClkDiv_1;
          EFM_ASSERT(false);
          break;
      }
      break;

    default:
      ret = cmuClkDiv_1;
      EFM_ASSERT(false);
      break;
  }

  return ret;
#endif
}

/***************************************************************************//**
 * @brief
 *   Set the clock divisor/prescaler.
 *
 * @note
 *   If setting an LF clock prescaler, synchronization into the low-frequency
 *   domain is required. If the same register is modified before a previous
 *   update has completed, this function will stall until the previous
 *   synchronization has completed. See @ref CMU_FreezeEnable() for
 *   a suggestion on how to reduce the stalling time in some use cases.
 *
 *   HFCLKLE prescaler is automatically modified when peripherals with clock
 *   domain HFBUSCLK is chosen based on the maximum HFLE frequency allowed.
 *
 * @param[in] clock
 *   Clock point to set divisor/prescaler for. Notice that not all clock points
 *   have a divisor/prescaler. See the CMU overview in the reference
 *   manual.
 *
 * @param[in] div
 *   The clock divisor to use (<= cmuClkDiv_512).
 ******************************************************************************/
void CMU_ClockDivSet(CMU_Clock_TypeDef clock, CMU_ClkDiv_TypeDef div)
{
#if defined(_SILICON_LABS_32B_SERIES_1)
  CMU_ClockPrescSet(clock, (CMU_ClkPresc_TypeDef)(div - 1U));

#elif defined(_SILICON_LABS_32B_SERIES_0)
  uint32_t freq;
  uint32_t divReg;

  /* Get the divisor reg ID. */
  divReg = (clock >> CMU_DIV_REG_POS) & CMU_DIV_REG_MASK;

  switch (divReg) {
#if defined(_CMU_CTRL_HFCLKDIV_MASK)
    case CMU_HFCLKDIV_REG:
      EFM_ASSERT((div >= cmuClkDiv_1) && (div <= cmuClkDiv_8));

      /* Configure worst case wait states for flash access before setting divisor. */
      flashWaitStateMax();

      /* Set the divider. */
      CMU->CTRL = (CMU->CTRL & ~_CMU_CTRL_HFCLKDIV_MASK)
                  | ((div - 1) << _CMU_CTRL_HFCLKDIV_SHIFT);

      /* Update the CMSIS core clock variable. */
      /* (The function will update the global variable). */
      freq = SystemCoreClockGet();

      /* Optimize flash access wait state setting for the current core clk. */
      CMU_UpdateWaitStates(freq, VSCALE_DEFAULT);
      break;
#endif

    case CMU_HFPERCLKDIV_REG:
      EFM_ASSERT((div >= cmuClkDiv_1) && (div <= cmuClkDiv_512));
      /* Convert to the correct scale. */
      div = CMU_DivToLog2(div);
      CMU->HFPERCLKDIV = (CMU->HFPERCLKDIV & ~_CMU_HFPERCLKDIV_HFPERCLKDIV_MASK)
                         | (div << _CMU_HFPERCLKDIV_HFPERCLKDIV_SHIFT);
      break;

#if defined(_CMU_HFCORECLKDIV_HFCORECLKLEDIV_MASK)
    case CMU_HFCORECLKLEDIV_REG:
      /*
         This divisor is usually set when changing HF clock to keep HFLE clock
         within safe bounds. This code path ignore these constraints.
       */
      /* Convert to the correct scale. */
      div = CMU_DivToLog2(div) - 1U;
      CMU->HFCORECLKDIV = (CMU->HFCORECLKDIV
                           & ~_CMU_HFCORECLKDIV_HFCORECLKLEDIV_MASK)
                          | (div << _CMU_HFCORECLKDIV_HFCORECLKLEDIV_SHIFT);
      break;
#endif

    case CMU_HFCORECLKDIV_REG:
      EFM_ASSERT((div >= cmuClkDiv_1) && (div <= cmuClkDiv_512));

      /* Configure worst case wait states for flash access before setting the divisor. */
      flashWaitStateMax();

#if defined(CMU_MAX_FREQ_HFLE)
      setHfLeConfig(SystemCoreClockGet() / div);
#endif

      /* Convert to the correct scale. */
      div = CMU_DivToLog2(div);

      CMU->HFCORECLKDIV = (CMU->HFCORECLKDIV
                           & ~_CMU_HFCORECLKDIV_HFCORECLKDIV_MASK)
                          | (div << _CMU_HFCORECLKDIV_HFCORECLKDIV_SHIFT);

      /* Update the CMSIS core clock variable. */
      /* (The function will update the global variable). */
      freq = SystemCoreClockGet();

      /* Optimize wait state setting for the current core clk. */
      CMU_UpdateWaitStates(freq, VSCALE_DEFAULT);
#if defined(CMU_MAX_FREQ_HFLE)
      setHfLeConfig(freq);
#endif
      break;

    case CMU_LFAPRESC0_REG:
      switch (clock) {
        case cmuClock_RTC:
          EFM_ASSERT(div <= cmuClkDiv_32768);

          /* LF register about to be modified requires sync. busy check. */
          syncReg(CMU_SYNCBUSY_LFAPRESC0);

          /* Convert to the correct scale. */
          div = CMU_DivToLog2(div);

          CMU->LFAPRESC0 = (CMU->LFAPRESC0 & ~_CMU_LFAPRESC0_RTC_MASK)
                           | (div << _CMU_LFAPRESC0_RTC_SHIFT);
          break;

#if defined(_CMU_LFAPRESC0_LETIMER0_MASK)
        case cmuClock_LETIMER0:
          EFM_ASSERT(div <= cmuClkDiv_32768);

          /* LF register about to be modified requires sync. busy check. */
          syncReg(CMU_SYNCBUSY_LFAPRESC0);

          /* Convert to the correct scale. */
          div = CMU_DivToLog2(div);

          CMU->LFAPRESC0 = (CMU->LFAPRESC0 & ~_CMU_LFAPRESC0_LETIMER0_MASK)
                           | (div << _CMU_LFAPRESC0_LETIMER0_SHIFT);
          break;
#endif

#if defined(LCD_PRESENT)
        case cmuClock_LCDpre:
          EFM_ASSERT((div >= cmuClkDiv_16) && (div <= cmuClkDiv_128));

          /* LF register about to be modified requires sync. busy check. */
          syncReg(CMU_SYNCBUSY_LFAPRESC0);

          /* Convert to the correct scale. */
          div = CMU_DivToLog2(div);

          CMU->LFAPRESC0 = (CMU->LFAPRESC0 & ~_CMU_LFAPRESC0_LCD_MASK)
                           | ((div - CMU_DivToLog2(cmuClkDiv_16))
                              << _CMU_LFAPRESC0_LCD_SHIFT);
          break;
#endif /* defined(LCD_PRESENT) */

#if defined(LESENSE_PRESENT)
        case cmuClock_LESENSE:
          EFM_ASSERT(div <= cmuClkDiv_8);

          /* LF register about to be modified requires sync. busy check. */
          syncReg(CMU_SYNCBUSY_LFAPRESC0);

          /* Convert to the correct scale. */
          div = CMU_DivToLog2(div);

          CMU->LFAPRESC0 = (CMU->LFAPRESC0 & ~_CMU_LFAPRESC0_LESENSE_MASK)
                           | (div << _CMU_LFAPRESC0_LESENSE_SHIFT);
          break;
#endif /* defined(LESENSE_PRESENT) */

        default:
          EFM_ASSERT(false);
          break;
      }
      break;

    case CMU_LFBPRESC0_REG:
      switch (clock) {
#if defined(_CMU_LFBPRESC0_LEUART0_MASK)
        case cmuClock_LEUART0:
          EFM_ASSERT(div <= cmuClkDiv_8);

          /* LF register about to be modified requires sync. busy check. */
          syncReg(CMU_SYNCBUSY_LFBPRESC0);

          /* Convert to the correct scale. */
          div = CMU_DivToLog2(div);

          CMU->LFBPRESC0 = (CMU->LFBPRESC0 & ~_CMU_LFBPRESC0_LEUART0_MASK)
                           | (((uint32_t)div) << _CMU_LFBPRESC0_LEUART0_SHIFT);
          break;
#endif

#if defined(_CMU_LFBPRESC0_LEUART1_MASK)
        case cmuClock_LEUART1:
          EFM_ASSERT(div <= cmuClkDiv_8);

          /* LF register about to be modified requires sync. busy check. */
          syncReg(CMU_SYNCBUSY_LFBPRESC0);

          /* Convert to the correct scale. */
          div = CMU_DivToLog2(div);

          CMU->LFBPRESC0 = (CMU->LFBPRESC0 & ~_CMU_LFBPRESC0_LEUART1_MASK)
                           | (((uint32_t)div) << _CMU_LFBPRESC0_LEUART1_SHIFT);
          break;
#endif

        default:
          EFM_ASSERT(false);
          break;
      }
      break;

    default:
      EFM_ASSERT(false);
      break;
  }
#endif
}

/***************************************************************************//**
 * @brief
 *   Enable/disable a clock.
 *
 * @details
 *   In general, module clocking is disabled after a reset. If a module
 *   clock is disabled, the registers of that module are not accessible and
 *   reading from such registers may return undefined values. Writing to
 *   registers of clock-disabled modules has no effect.
 *   Avoid accessing module registers of a module with a disabled clock.
 *
 * @note
 *   If enabling/disabling an LF clock, synchronization into the low-frequency
 *   domain is required. If the same register is modified before a previous
 *   update has completed, this function will stall until the previous
 *   synchronization has completed. See @ref CMU_FreezeEnable() for
 *   a suggestion on how to reduce the stalling time in some use cases.
 *
 *   HFCLKLE prescaler is automatically modified when peripherals with clock
 *   domain HFBUSCLK is chosen based on the maximum HFLE frequency allowed.
 *
 * @param[in] clock
 *   The clock to enable/disable. Notice that not all defined clock
 *   points have separate enable/disable control. See the CMU overview
 *   in the reference manual.
 *
 * @param[in] enable
 *   @li true - enable specified clock.
 *   @li false - disable specified clock.
 ******************************************************************************/
void CMU_ClockEnable(CMU_Clock_TypeDef clock, bool enable)
{
  volatile uint32_t *reg;
  uint32_t          bit;
  uint32_t          sync = 0;

  /* Identify enable register */
  switch (((unsigned)clock >> CMU_EN_REG_POS) & CMU_EN_REG_MASK) {
#if defined(_CMU_CTRL_HFPERCLKEN_MASK)
    case CMU_CTRL_EN_REG:
      reg = &CMU->CTRL;
      break;
#endif

#if defined(_CMU_HFCORECLKEN0_MASK)
    case CMU_HFCORECLKEN0_EN_REG:
      reg = &CMU->HFCORECLKEN0;
#if defined(CMU_MAX_FREQ_HFLE)
      setHfLeConfig(SystemCoreClockGet());
#endif
      break;
#endif

#if defined(_CMU_HFBUSCLKEN0_MASK)
    case CMU_HFBUSCLKEN0_EN_REG:
      reg = &CMU->HFBUSCLKEN0;
      break;
#endif

#if defined(_CMU_HFPERCLKDIV_MASK)
    case CMU_HFPERCLKDIV_EN_REG:
      reg = &CMU->HFPERCLKDIV;
      break;
#endif

    case CMU_HFPERCLKEN0_EN_REG:
      reg = &CMU->HFPERCLKEN0;
      break;

#if defined(_CMU_HFPERCLKEN1_MASK)
    case CMU_HFPERCLKEN1_EN_REG:
      reg = &CMU->HFPERCLKEN1;
      break;
#endif

    case CMU_LFACLKEN0_EN_REG:
      reg  = &CMU->LFACLKEN0;
      sync = CMU_SYNCBUSY_LFACLKEN0;
      break;

    case CMU_LFBCLKEN0_EN_REG:
      reg  = &CMU->LFBCLKEN0;
      sync = CMU_SYNCBUSY_LFBCLKEN0;
      break;

#if defined(_CMU_LFCCLKEN0_MASK)
    case CMU_LFCCLKEN0_EN_REG:
      reg = &CMU->LFCCLKEN0;
      sync = CMU_SYNCBUSY_LFCCLKEN0;
      break;
#endif

#if defined(_CMU_LFECLKEN0_MASK)
    case CMU_LFECLKEN0_EN_REG:
      reg  = &CMU->LFECLKEN0;
      sync = CMU_SYNCBUSY_LFECLKEN0;
      break;
#endif

#if defined(_CMU_SDIOCTRL_MASK)
    case CMU_SDIOREF_EN_REG:
      reg = &CMU->SDIOCTRL;
      enable = !enable;
      break;
#endif

#if defined(_CMU_QSPICTRL_MASK)
    case CMU_QSPI0REF_EN_REG:
      reg = &CMU->QSPICTRL;
      enable = !enable;
      break;
#endif
#if defined(_CMU_USBCTRL_MASK)
    case CMU_USBRCLK_EN_REG:
      reg = &CMU->USBCTRL;
      break;
#endif
#if defined(_CMU_PDMCTRL_MASK)
    case CMU_PDMREF_EN_REG:
      reg = &CMU->PDMCTRL;
      break;
#endif

    case CMU_PCNT_EN_REG:
      reg = &CMU->PCNTCTRL;
      break;

    default: /* Cannot enable/disable a clock point. */
      EFM_ASSERT(false);
      return;
  }

  /* Get the bit position used to enable/disable. */
  bit = ((unsigned)clock >> CMU_EN_BIT_POS) & CMU_EN_BIT_MASK;

  /* LF synchronization required. */
  if (sync > 0UL) {
    syncReg(sync);
  }

  /* Set/clear bit as requested. */
  BUS_RegBitWrite(reg, bit, (uint32_t)enable);
}

/***************************************************************************//**
 * @brief
 *   Get the clock frequency for a clock point.
 *
 * @param[in] clock
 *   A clock point to fetch the frequency for.
 *
 * @return
 *   The current frequency in Hz.
 ******************************************************************************/
uint32_t CMU_ClockFreqGet(CMU_Clock_TypeDef clock)
{
  uint32_t ret;

  switch ((unsigned)clock & (CMU_CLK_BRANCH_MASK << CMU_CLK_BRANCH_POS)) {
    case (CMU_HF_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = SystemHFClockGet();
      break;

    case (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = SystemHFClockGet();
      /* Calculate frequency after HFPER divider. */
#if defined(_CMU_HFPERCLKDIV_HFPERCLKDIV_MASK)
      ret >>= (CMU->HFPERCLKDIV & _CMU_HFPERCLKDIV_HFPERCLKDIV_MASK)
              >> _CMU_HFPERCLKDIV_HFPERCLKDIV_SHIFT;
#endif
#if defined(_CMU_HFPERPRESC_PRESC_MASK)
      ret /= 1U + ((CMU->HFPERPRESC & _CMU_HFPERPRESC_PRESC_MASK)
                   >> _CMU_HFPERPRESC_PRESC_SHIFT);
#endif
      break;

#if defined(_CMU_HFPERPRESCB_MASK)
    case (CMU_HFPERB_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = SystemHFClockGet();
      /* Calculate frequency after HFPERB prescaler. */
      ret /= 1U + ((CMU->HFPERPRESCB & _CMU_HFPERPRESCB_PRESC_MASK)
                   >> _CMU_HFPERPRESCB_PRESC_SHIFT);
      break;
#endif

#if defined(_CMU_HFPERPRESCC_MASK)
    case (CMU_HFPERC_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = SystemHFClockGet();
      /* Calculate frequency after HFPERC prescaler. */
      ret /= 1U + ((CMU->HFPERPRESCC & _CMU_HFPERPRESCC_PRESC_MASK)
                   >> _CMU_HFPERPRESCC_PRESC_SHIFT);
      break;
#endif

#if defined(_SILICON_LABS_32B_SERIES_1)
#if defined(CRYPTO_PRESENT)     \
      || defined(LDMA_PRESENT)  \
      || defined(GPCRC_PRESENT) \
      || defined(PRS_PRESENT)   \
      || defined(GPIO_PRESENT)
    case (CMU_HFBUS_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = SystemHFClockGet();
#if defined(_CMU_HFBUSPRESC_MASK)
      ret /= 1U + ((CMU->HFBUSPRESC & _CMU_HFBUSPRESC_MASK)
                   >> _CMU_HFBUSPRESC_PRESC_SHIFT);
#endif
      break;
#endif

    case (CMU_HFCORE_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = SystemHFClockGet();
      ret /= 1U + ((CMU->HFCOREPRESC & _CMU_HFCOREPRESC_PRESC_MASK)
                   >> _CMU_HFCOREPRESC_PRESC_SHIFT);
      break;

    case (CMU_HFEXP_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = SystemHFClockGet();
      ret /= 1U + ((CMU->HFEXPPRESC & _CMU_HFEXPPRESC_PRESC_MASK)
                   >> _CMU_HFEXPPRESC_PRESC_SHIFT);
      break;
#endif

#if defined(_SILICON_LABS_32B_SERIES_0)
#if defined(AES_PRESENT)      \
      || defined(DMA_PRESENT) \
      || defined(EBI_PRESENT) \
      || defined(USB_PRESENT)
    case (CMU_HFCORE_CLK_BRANCH << CMU_CLK_BRANCH_POS):
    {
      ret = SystemCoreClockGet();
    } break;
#endif
#endif

    case (CMU_LFA_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = lfClkGet(cmuClock_LFA);
      break;

#if defined(_CMU_LFACLKEN0_RTC_MASK)
    case (CMU_RTC_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = lfClkGet(cmuClock_LFA);
      ret >>= (CMU->LFAPRESC0 & _CMU_LFAPRESC0_RTC_MASK)
              >> _CMU_LFAPRESC0_RTC_SHIFT;
      break;
#endif

#if defined(_CMU_LFECLKEN0_RTCC_MASK)
    case (CMU_RTCC_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = lfClkGet(cmuClock_LFE);
      ret >>= (CMU->LFEPRESC0 & _CMU_LFEPRESC0_RTCC_MASK)
              >> _CMU_LFEPRESC0_RTCC_SHIFT;
      break;
#endif

#if defined(_CMU_LFACLKEN0_LETIMER0_MASK)
    case (CMU_LETIMER0_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = lfClkGet(cmuClock_LFA);
#if defined(_SILICON_LABS_32B_SERIES_0)
      ret >>= (CMU->LFAPRESC0 & _CMU_LFAPRESC0_LETIMER0_MASK)
              >> _CMU_LFAPRESC0_LETIMER0_SHIFT;
#else
      ret /= SL_Log2ToDiv((CMU->LFAPRESC0 & _CMU_LFAPRESC0_LETIMER0_MASK)
                          >> _CMU_LFAPRESC0_LETIMER0_SHIFT);
#endif
      break;
#endif

#if defined(_CMU_LFACLKEN0_LETIMER1_MASK)
    case (CMU_LETIMER1_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = lfClkGet(cmuClock_LFA);
#if defined(_SILICON_LABS_32B_SERIES_0)
      ret >>= (CMU->LFAPRESC0 & _CMU_LFAPRESC0_LETIMER1_MASK)
              >> _CMU_LFAPRESC0_LETIMER1_SHIFT;
#else
      ret /= SL_Log2ToDiv((CMU->LFAPRESC0 & _CMU_LFAPRESC0_LETIMER1_MASK)
                          >> _CMU_LFAPRESC0_LETIMER1_SHIFT);
#endif
      break;
#endif

#if defined(_CMU_LFACLKEN0_LCD_MASK)
    case (CMU_LCDPRE_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = lfClkGet(cmuClock_LFA);
#if defined(_SILICON_LABS_32B_SERIES_0)
      ret >>= ((CMU->LFAPRESC0 & _CMU_LFAPRESC0_LCD_MASK)
               >> _CMU_LFAPRESC0_LCD_SHIFT)
              + CMU_DivToLog2(cmuClkDiv_16);
#else
      ret /= SL_Log2ToDiv((CMU->LFAPRESC0 & _CMU_LFAPRESC0_LCD_MASK)
                          >> _CMU_LFAPRESC0_LCD_SHIFT);
#endif
      break;

#if defined(_CMU_LCDCTRL_MASK)
    case (CMU_LCD_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = lfClkGet(cmuClock_LFA);
      ret >>= (CMU->LFAPRESC0 & _CMU_LFAPRESC0_LCD_MASK)
              >> _CMU_LFAPRESC0_LCD_SHIFT;
      ret /= 1U + ((CMU->LCDCTRL & _CMU_LCDCTRL_FDIV_MASK)
                   >> _CMU_LCDCTRL_FDIV_SHIFT);
      break;
#endif
#endif

#if defined(_CMU_LFACLKEN0_LESENSE_MASK)
    case (CMU_LESENSE_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = lfClkGet(cmuClock_LFA);
      ret >>= (CMU->LFAPRESC0 & _CMU_LFAPRESC0_LESENSE_MASK)
              >> _CMU_LFAPRESC0_LESENSE_SHIFT;
      break;
#endif

    case (CMU_LFB_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = lfClkGet(cmuClock_LFB);
      break;

#if defined(_CMU_LFBCLKEN0_LEUART0_MASK)
    case (CMU_LEUART0_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = lfClkGet(cmuClock_LFB);
#if defined(_SILICON_LABS_32B_SERIES_0)
      ret >>= (CMU->LFBPRESC0 & _CMU_LFBPRESC0_LEUART0_MASK)
              >> _CMU_LFBPRESC0_LEUART0_SHIFT;
#else
      ret /= SL_Log2ToDiv((CMU->LFBPRESC0 & _CMU_LFBPRESC0_LEUART0_MASK)
                          >> _CMU_LFBPRESC0_LEUART0_SHIFT);
#endif
      break;
#endif

#if defined(_CMU_LFBCLKEN0_LEUART1_MASK)
    case (CMU_LEUART1_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = lfClkGet(cmuClock_LFB);
#if defined(_SILICON_LABS_32B_SERIES_0)
      ret >>= (CMU->LFBPRESC0 & _CMU_LFBPRESC0_LEUART1_MASK)
              >> _CMU_LFBPRESC0_LEUART1_SHIFT;
#else
      ret /= SL_Log2ToDiv((CMU->LFBPRESC0 & _CMU_LFBPRESC0_LEUART1_MASK)
                          >> _CMU_LFBPRESC0_LEUART1_SHIFT);
#endif
      break;
#endif

#if defined(_CMU_LFBCLKEN0_CSEN_MASK)
    case (CMU_CSEN_LF_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = lfClkGet(cmuClock_LFB);
      ret /= SL_Log2ToDiv(((CMU->LFBPRESC0 & _CMU_LFBPRESC0_CSEN_MASK)
                           >> _CMU_LFBPRESC0_CSEN_SHIFT) + 4UL);
      break;
#endif

#if defined(CMU_LFCCLKEN0_USB)
    case (CMU_USBLE_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = lfClkGet(cmuClock_LFC);
      break;
#endif

#if defined(_SILICON_LABS_32B_SERIES_1)
    case (CMU_LFE_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = lfClkGet(cmuClock_LFE);
      break;
#endif

    case (CMU_DBG_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = dbgClkGet();
      break;

    case (CMU_AUX_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = auxClkGet();
      break;

#if defined(USBC_CLOCK_PRESENT)
    case (CMU_USBC_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = usbCClkGet();
      break;
#endif

#if defined(_CMU_ADCCTRL_ADC0CLKSEL_MASK)
    case (CMU_ADC0ASYNC_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = adcAsyncClkGet(0);
#if defined(_CMU_ADCCTRL_ADC0CLKDIV_MASK)
      ret /= 1U + ((CMU->ADCCTRL & _CMU_ADCCTRL_ADC0CLKDIV_MASK)
                   >> _CMU_ADCCTRL_ADC0CLKDIV_SHIFT);
#endif
      break;
#endif

#if defined(_CMU_ADCCTRL_ADC1CLKSEL_MASK)
    case (CMU_ADC1ASYNC_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = adcAsyncClkGet(1);
#if defined(_CMU_ADCCTRL_ADC1CLKDIV_MASK)
      ret /= 1U + ((CMU->ADCCTRL & _CMU_ADCCTRL_ADC1CLKDIV_MASK)
                   >> _CMU_ADCCTRL_ADC1CLKDIV_SHIFT);
#endif
      break;
#endif

#if defined(_CMU_SDIOCTRL_SDIOCLKSEL_MASK)
    case (CMU_SDIOREF_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = sdioRefClkGet();
      break;
#endif

#if defined(_CMU_QSPICTRL_QSPI0CLKSEL_MASK)
    case (CMU_QSPI0REF_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = qspiRefClkGet(0);
      break;
#endif

#if defined(USBR_CLOCK_PRESENT)
    case (CMU_USBR_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = usbRateClkGet();
      break;
#endif

#if defined(_CMU_PDMCTRL_PDMCLKSEL_MASK)
    case (CMU_PDMREF_CLK_BRANCH << CMU_CLK_BRANCH_POS):
      ret = pdmRefClkGet();
      break;
#endif

    case (CMU_HFLE_CLK_BRANCH << CMU_CLK_BRANCH_POS):
#if defined(_CMU_HFCORECLKDIV_HFCORECLKLEDIV_MASK)
      ret = SystemCoreClockGet() / CMU_ClockDivGet(clock);
#elif defined(_CMU_HFPRESC_HFCLKLEPRESC_MASK)
      ret = SystemHFClockGet() / CMU_ClockDivGet(clock);
#else
      ret = SystemCoreClockGet() / 2;
#endif
      break;

    default:
      ret = 0;
      EFM_ASSERT(false);
      break;
  }

  return ret;
}

#if defined(_SILICON_LABS_32B_SERIES_1)
/***************************************************************************//**
 * @brief
 *   Get the clock prescaler.
 *
 * @param[in] clock
 *   A clock point to get the prescaler for. Notice that not all clock points
 *   have a prescaler. See the CMU overview in the reference manual.
 *
 * @return
 *   The prescaler value of the current clock point. 0 is returned
 *   if @p clock specifies a clock point without a prescaler.
 ******************************************************************************/
uint32_t CMU_ClockPrescGet(CMU_Clock_TypeDef clock)
{
  uint32_t  prescReg;
  uint32_t  ret;

  /* Get the prescaler register ID. */
  prescReg = ((unsigned)clock >> CMU_PRESC_REG_POS) & CMU_PRESC_REG_MASK;

  switch (prescReg) {
    case CMU_HFPRESC_REG:
      ret = (CMU->HFPRESC & _CMU_HFPRESC_PRESC_MASK)
            >> _CMU_HFPRESC_PRESC_SHIFT;
      break;

    case CMU_HFEXPPRESC_REG:
      ret = (CMU->HFEXPPRESC & _CMU_HFEXPPRESC_PRESC_MASK)
            >> _CMU_HFEXPPRESC_PRESC_SHIFT;
      break;

    case CMU_HFCLKLEPRESC_REG:
      ret = (CMU->HFPRESC & _CMU_HFPRESC_HFCLKLEPRESC_MASK)
            >> _CMU_HFPRESC_HFCLKLEPRESC_SHIFT;
      ret = SL_Log2ToDiv(ret + 1U) - 1U;
      break;

    case CMU_HFPERPRESC_REG:
      ret = (CMU->HFPERPRESC & _CMU_HFPERPRESC_PRESC_MASK)
            >> _CMU_HFPERPRESC_PRESC_SHIFT;
      break;

#if defined(_CMU_HFPERPRESCB_MASK)
    case CMU_HFPERPRESCB_REG:
      ret = (CMU->HFPERPRESCB & _CMU_HFPERPRESCB_PRESC_MASK)
            >> _CMU_HFPERPRESCB_PRESC_SHIFT;
      break;
#endif

#if defined(_CMU_HFPERPRESCC_MASK)
    case CMU_HFPERPRESCC_REG:
      ret = (CMU->HFPERPRESCC & _CMU_HFPERPRESCC_PRESC_MASK)
            >> _CMU_HFPERPRESCC_PRESC_SHIFT;
      break;
#endif

    case CMU_HFCOREPRESC_REG:
      ret = (CMU->HFCOREPRESC & _CMU_HFCOREPRESC_PRESC_MASK)
            >> _CMU_HFCOREPRESC_PRESC_SHIFT;
      break;

    case CMU_LFAPRESC0_REG:
      switch (clock) {
#if defined(_CMU_LFAPRESC0_LETIMER0_MASK)
        case cmuClock_LETIMER0:
          ret = (CMU->LFAPRESC0 & _CMU_LFAPRESC0_LETIMER0_MASK)
                >> _CMU_LFAPRESC0_LETIMER0_SHIFT;
          /* Convert the exponent to a prescaler value. */
          ret = SL_Log2ToDiv(ret) - 1U;
          break;
#endif

#if defined(_CMU_LFAPRESC0_LESENSE_MASK)
        case cmuClock_LESENSE:
          ret = (CMU->LFAPRESC0 & _CMU_LFAPRESC0_LESENSE_MASK)
                >> _CMU_LFAPRESC0_LESENSE_SHIFT;
          /* Convert the exponent to a prescaler value. */
          ret = SL_Log2ToDiv(ret) - 1U;
          break;
#endif

#if defined(_CMU_LFAPRESC0_LETIMER1_MASK)
        case cmuClock_LETIMER1:
          ret = (CMU->LFAPRESC0 & _CMU_LFAPRESC0_LETIMER1_MASK)
                >> _CMU_LFAPRESC0_LETIMER1_SHIFT;
          ret = SL_Log2ToDiv(ret) - 1U;
          break;
#endif

#if defined(_CMU_LFAPRESC0_LCD_MASK)
        case cmuClock_LCD:
        case cmuClock_LCDpre:
          ret = (CMU->LFAPRESC0 & _CMU_LFAPRESC0_LCD_MASK)
                >> _CMU_LFAPRESC0_LCD_SHIFT;
          ret = SL_Log2ToDiv(ret) - 1U;
          break;
#endif

#if defined(_CMU_LFAPRESC0_RTC_MASK)
        case cmuClock_RTC:
          ret = (CMU->LFAPRESC0 & _CMU_LFAPRESC0_RTC_MASK)
                >> _CMU_LFAPRESC0_RTC_SHIFT;
          ret = SL_Log2ToDiv(ret) - 1U;
          break;
#endif

        default:
          ret = 0U;
          EFM_ASSERT(false);
          break;
      }
      break;

    case CMU_LFBPRESC0_REG:
      switch (clock) {
#if defined(_CMU_LFBPRESC0_LEUART0_MASK)
        case cmuClock_LEUART0:
          ret = (CMU->LFBPRESC0 & _CMU_LFBPRESC0_LEUART0_MASK)
                >> _CMU_LFBPRESC0_LEUART0_SHIFT;
          /* Convert the exponent to a prescaler value. */
          ret = SL_Log2ToDiv(ret) - 1U;
          break;
#endif

#if defined(_CMU_LFBPRESC0_LEUART1_MASK)
        case cmuClock_LEUART1:
          ret = (CMU->LFBPRESC0 & _CMU_LFBPRESC0_LEUART1_MASK)
                >> _CMU_LFBPRESC0_LEUART1_SHIFT;
          /* Convert the exponent to a prescaler value. */
          ret = SL_Log2ToDiv(ret) - 1U;
          break;
#endif

#if defined(_CMU_LFBPRESC0_CSEN_MASK)
        case cmuClock_CSEN_LF:
          ret = (CMU->LFBPRESC0 & _CMU_LFBPRESC0_CSEN_MASK)
                >> _CMU_LFBPRESC0_CSEN_SHIFT;
          /* Convert the exponent to a prescaler value. */
          ret = SL_Log2ToDiv(ret + 4U) - 1U;
          break;
#endif

        default:
          ret = 0U;
          EFM_ASSERT(false);
          break;
      }
      break;

    case CMU_LFEPRESC0_REG:
      switch (clock) {
#if defined(RTCC_PRESENT)
        case cmuClock_RTCC:
          ret = (CMU->LFEPRESC0 & _CMU_LFEPRESC0_RTCC_MASK)
                >> _CMU_LFEPRESC0_RTCC_SHIFT;
          break;

        default:
          ret = 0U;
          EFM_ASSERT(false);
          break;
#endif
      }
      break;

#if defined(_CMU_ADCCTRL_ADC0CLKDIV_MASK) \
      || defined(_CMU_ADCCTRL_ADC1CLKDIV_MASK)
    case CMU_ADCASYNCDIV_REG:
      switch (clock) {
#if defined(_CMU_ADCCTRL_ADC0CLKDIV_MASK)
        case cmuClock_ADC0ASYNC:
          ret = (CMU->ADCCTRL & _CMU_ADCCTRL_ADC0CLKDIV_MASK)
                >> _CMU_ADCCTRL_ADC0CLKDIV_SHIFT;
          break;
#endif
#if defined(_CMU_ADCCTRL_ADC1CLKDIV_MASK)
        case cmuClock_ADC1ASYNC:
          ret = (CMU->ADCCTRL & _CMU_ADCCTRL_ADC1CLKDIV_MASK)
                >> _CMU_ADCCTRL_ADC1CLKDIV_SHIFT;
          break;
#endif
        default:
          ret = 0U;
          EFM_ASSERT(false);
          break;
      }
      break;
#endif
#if defined(_CMU_HFBUSPRESC_MASK)
    case CMU_HFBUSPRESC_REG:
      ret = (CMU->HFBUSPRESC & _CMU_HFBUSPRESC_MASK)
            >> _CMU_HFBUSPRESC_PRESC_SHIFT;
      break;
#endif
    default:
      ret = 0U;
      EFM_ASSERT(false);
      break;
  }

  return ret;
}
#endif

#if defined(_SILICON_LABS_32B_SERIES_1)
/***************************************************************************//**
 * @brief
 *   Set the clock prescaler.
 *
 * @note
 *   If setting an LF clock prescaler, synchronization into the low-frequency
 *   domain is required. If the same register is modified before a previous
 *   update has completed, this function will stall until the previous
 *   synchronization has completed. See @ref CMU_FreezeEnable() for
 *   a suggestion on how to reduce the stalling time in some use cases.
 *
 *   HFCLKLE prescaler is automatically modified when peripherals with clock
 *   domain HFBUSCLK is chosen based on the maximum HFLE frequency allowed.
 *
 * @param[in] clock
 *   A clock point to set the prescaler for. Notice that not all clock points
 *   have a prescaler. See the CMU overview in the reference manual.
 *
 * @param[in] presc
 *   The clock prescaler.
 ******************************************************************************/
void CMU_ClockPrescSet(CMU_Clock_TypeDef clock, CMU_ClkPresc_TypeDef presc)
{
  uint32_t freq;
  uint32_t prescReg;

  /* Get the divisor reg ID. */
  prescReg = ((unsigned)clock >> CMU_PRESC_REG_POS) & CMU_PRESC_REG_MASK;

  switch (prescReg) {
    case CMU_HFPRESC_REG:
      EFM_ASSERT(presc < 32U);

      /* Configure worst case wait-states for flash and HFLE, set safe HFPER
         clock-tree prescalers. */
      flashWaitStateMax();
      setHfLeConfig(CMU_MAX_FREQ_HFLE + 1UL);
      hfperClkSafePrescaler();

      CMU->HFPRESC = (CMU->HFPRESC & ~_CMU_HFPRESC_PRESC_MASK)
                     | (presc << _CMU_HFPRESC_PRESC_SHIFT);

      /* Update the CMSIS core clock variable (this function updates the global
         variable). */
      freq = SystemCoreClockGet();
      /* Optimize flash and HFLE wait states and set optimized HFPER clock-tree
         prescalers. */
      CMU_UpdateWaitStates(freq, VSCALE_DEFAULT);
      setHfLeConfig(SystemHFClockGet());
      hfperClkOptimizedPrescaler();
      break;

    case CMU_HFEXPPRESC_REG:
      EFM_ASSERT(presc < 32U);

      CMU->HFEXPPRESC = (CMU->HFEXPPRESC & ~_CMU_HFEXPPRESC_PRESC_MASK)
                        | (presc << _CMU_HFEXPPRESC_PRESC_SHIFT);
      break;

    case CMU_HFCLKLEPRESC_REG:
      presc = CMU_DivToLog2(presc);
      CMU->HFPRESC = (CMU->HFPRESC & ~_CMU_HFPRESC_HFCLKLEPRESC_MASK)
                     | (presc << _CMU_HFPRESC_HFCLKLEPRESC_SHIFT);
      break;

    case CMU_HFPERPRESC_REG:
      EFM_ASSERT(presc < 512U);
      CMU->HFPERPRESC = (CMU->HFPERPRESC & ~_CMU_HFPERPRESC_PRESC_MASK)
                        | (presc << _CMU_HFPERPRESC_PRESC_SHIFT);
      break;

#if defined(_CMU_HFPERPRESCB_MASK)
    case CMU_HFPERPRESCB_REG:
      EFM_ASSERT(presc < 512U);
      CMU->HFPERPRESCB = (CMU->HFPERPRESCB & ~_CMU_HFPERPRESCB_PRESC_MASK)
                         | (presc << _CMU_HFPERPRESCB_PRESC_SHIFT);
      break;
#endif

#if defined(_CMU_HFPERPRESCC_MASK)
    case CMU_HFPERPRESCC_REG:
      EFM_ASSERT(presc < 512U);
      CMU->HFPERPRESCC = (CMU->HFPERPRESCC & ~_CMU_HFPERPRESCC_PRESC_MASK)
                         | (presc << _CMU_HFPERPRESCC_PRESC_SHIFT);
      break;
#endif

    case CMU_HFCOREPRESC_REG:
      EFM_ASSERT(presc < 512U);

      /* Configure worst case wait-states for flash. */
      flashWaitStateMax();

      CMU->HFCOREPRESC = (CMU->HFCOREPRESC & ~_CMU_HFCOREPRESC_PRESC_MASK)
                         | (presc << _CMU_HFCOREPRESC_PRESC_SHIFT);

      /* Update the CMSIS core clock variable (this function updates the global variable).
         Optimize flash and HFLE wait states. */
      freq = SystemCoreClockGet();
      CMU_UpdateWaitStates(freq, VSCALE_DEFAULT);
      break;

    case CMU_LFAPRESC0_REG:
      switch (clock) {
#if defined(RTC_PRESENT)
        case cmuClock_RTC:
          EFM_ASSERT(presc <= 32768U);

          /* Convert the prescaler value to a DIV exponent scale. */
          presc = CMU_PrescToLog2(presc);

          /* LF register about to be modified requires sync. Busy check. */
          syncReg(CMU_SYNCBUSY_LFAPRESC0);

          CMU->LFAPRESC0 = (CMU->LFAPRESC0 & ~_CMU_LFAPRESC0_RTC_MASK)
                           | (presc << _CMU_LFAPRESC0_RTC_SHIFT);
          break;
#endif

#if defined(RTCC_PRESENT)
        case cmuClock_RTCC:
#if defined(_CMU_LFEPRESC0_RTCC_MASK)
#if defined(_CMU_LFEPRESC0_RTCC_DIV4)
          EFM_ASSERT(presc <= _CMU_LFEPRESC0_RTCC_DIV4);
#elif defined(_CMU_LFEPRESC0_RTCC_DIV2)
          EFM_ASSERT(presc <= _CMU_LFEPRESC0_RTCC_DIV2);
#else
          EFM_ASSERT(presc <= 0U);
#endif

          /* LF register about to be modified requires sync. Busy check. */
          syncReg(CMU_SYNCBUSY_LFEPRESC0);

          CMU->LFEPRESC0 = (CMU->LFEPRESC0 & ~_CMU_LFEPRESC0_RTCC_MASK)
                           | (presc << _CMU_LFEPRESC0_RTCC_SHIFT);
#else
          EFM_ASSERT(presc <= 32768U);

          /* Convert the prescaler value to a DIV exponent scale. */
          presc = CMU_PrescToLog2(presc);

          /* LF register about to be modified requires sync. Busy check. */
          syncReg(CMU_SYNCBUSY_LFAPRESC0);

          CMU->LFAPRESC0 = (CMU->LFAPRESC0 & ~_CMU_LFAPRESC0_RTCC_MASK)
                           | (presc << _CMU_LFAPRESC0_RTCC_SHIFT);
#endif
          break;
#endif

#if defined(_CMU_LFAPRESC0_LETIMER0_MASK)
        case cmuClock_LETIMER0:
          EFM_ASSERT(presc <= 32768U);

          /* Convert the prescaler value to a DIV exponent scale. */
          presc = CMU_PrescToLog2(presc);

          /* LF register about to be modified requires sync. Busy check. */
          syncReg(CMU_SYNCBUSY_LFAPRESC0);

          CMU->LFAPRESC0 = (CMU->LFAPRESC0 & ~_CMU_LFAPRESC0_LETIMER0_MASK)
                           | (presc << _CMU_LFAPRESC0_LETIMER0_SHIFT);
          break;
#endif

#if defined(_CMU_LFAPRESC0_LETIMER1_MASK)
        case cmuClock_LETIMER1:
          EFM_ASSERT(presc <= 32768U);

          /* Convert the prescaler value to a DIV exponent scale. */
          presc = CMU_PrescToLog2(presc);

          /* LF register about to be modified requires sync. Busy check. */
          syncReg(CMU_SYNCBUSY_LFAPRESC0);

          CMU->LFAPRESC0 = (CMU->LFAPRESC0 & ~_CMU_LFAPRESC0_LETIMER1_MASK)
                           | (presc << _CMU_LFAPRESC0_LETIMER1_SHIFT);
          break;
#endif

#if defined(_CMU_LFAPRESC0_LESENSE_MASK)
        case cmuClock_LESENSE:
          EFM_ASSERT(presc <= 8U);

          /* Convert the prescaler value to a DIV exponent scale. */
          presc = CMU_PrescToLog2(presc);

          /* LF register about to be modified requires sync. Busy check. */
          syncReg(CMU_SYNCBUSY_LFAPRESC0);

          CMU->LFAPRESC0 = (CMU->LFAPRESC0 & ~_CMU_LFAPRESC0_LESENSE_MASK)
                           | (presc << _CMU_LFAPRESC0_LESENSE_SHIFT);
          break;
#endif

#if defined(_CMU_LFAPRESC0_LCD_MASK)
        case cmuClock_LCDpre:
        case cmuClock_LCD:
          EFM_ASSERT(presc <= 32768U);

          /* Convert the prescaler value to a DIV exponent scale. */
          presc = CMU_PrescToLog2(presc);

          /* LF register about to be modified requires sync. Busy check. */
          syncReg(CMU_SYNCBUSY_LFAPRESC0);

          CMU->LFAPRESC0 = (CMU->LFAPRESC0 & ~_CMU_LFAPRESC0_LCD_MASK)
                           | (presc << _CMU_LFAPRESC0_LCD_SHIFT);
          break;
#endif

        default:
          EFM_ASSERT(false);
          break;
      }
      break;

    case CMU_LFBPRESC0_REG:
      switch (clock) {
#if defined(_CMU_LFBPRESC0_LEUART0_MASK)
        case cmuClock_LEUART0:
          EFM_ASSERT(presc <= 8U);

          /* Convert the prescaler value to a DIV exponent scale. */
          presc = CMU_PrescToLog2(presc);

          /* LF register about to be modified requires sync. Busy check. */
          syncReg(CMU_SYNCBUSY_LFBPRESC0);

          CMU->LFBPRESC0 = (CMU->LFBPRESC0 & ~_CMU_LFBPRESC0_LEUART0_MASK)
                           | (presc << _CMU_LFBPRESC0_LEUART0_SHIFT);
          break;
#endif

#if defined(_CMU_LFBPRESC0_LEUART1_MASK)
        case cmuClock_LEUART1:
          EFM_ASSERT(presc <= 8U);

          /* Convert the prescaler value to a DIV exponent scale. */
          presc = CMU_PrescToLog2(presc);

          /* LF register about to be modified requires sync. Busy check. */
          syncReg(CMU_SYNCBUSY_LFBPRESC0);

          CMU->LFBPRESC0 = (CMU->LFBPRESC0 & ~_CMU_LFBPRESC0_LEUART1_MASK)
                           | (presc << _CMU_LFBPRESC0_LEUART1_SHIFT);
          break;
#endif

#if defined(_CMU_LFBPRESC0_CSEN_MASK)
        case cmuClock_CSEN_LF:
          EFM_ASSERT((presc <= 127U) && (presc >= 15U));

          /* Convert the prescaler value to a DIV exponent scale.
           * DIV16 is the lowest supported prescaler. */
          presc = CMU_PrescToLog2(presc) - 4U;

          /* LF register about to be modified requires sync. Busy check. */
          syncReg(CMU_SYNCBUSY_LFBPRESC0);

          CMU->LFBPRESC0 = (CMU->LFBPRESC0 & ~_CMU_LFBPRESC0_CSEN_MASK)
                           | (presc << _CMU_LFBPRESC0_CSEN_SHIFT);
          break;
#endif

        default:
          EFM_ASSERT(false);
          break;
      }
      break;

    case CMU_LFEPRESC0_REG:
      switch (clock) {
#if defined(_CMU_LFEPRESC0_RTCC_MASK)
        case cmuClock_RTCC:
#if defined(_CMU_LFEPRESC0_RTCC_DIV4)
          EFM_ASSERT(presc <= _CMU_LFEPRESC0_RTCC_DIV4);
#elif defined(_CMU_LFEPRESC0_RTCC_DIV2)
          EFM_ASSERT(presc <= _CMU_LFEPRESC0_RTCC_DIV2);
#else
          EFM_ASSERT(presc <= 0U);
#endif

          /* LF register about to be modified requires sync. Busy check. */
          syncReg(CMU_SYNCBUSY_LFEPRESC0);

          CMU->LFEPRESC0 = (CMU->LFEPRESC0 & ~_CMU_LFEPRESC0_RTCC_MASK)
                           | (presc << _CMU_LFEPRESC0_RTCC_SHIFT);
          break;
#endif

        default:
          EFM_ASSERT(false);
          break;
      }
      break;

#if defined(_CMU_ADCCTRL_ADC0CLKDIV_MASK) \
      ||  defined(_CMU_ADCCTRL_ADC1CLKDIV_MASK)
    case CMU_ADCASYNCDIV_REG:
      switch (clock) {
#if defined(_CMU_ADCCTRL_ADC0CLKDIV_MASK)
        case cmuClock_ADC0ASYNC:
          EFM_ASSERT(presc <= 3);
          CMU->ADCCTRL = (CMU->ADCCTRL & ~_CMU_ADCCTRL_ADC0CLKDIV_MASK)
                         | (presc << _CMU_ADCCTRL_ADC0CLKDIV_SHIFT);
          break;
#endif

#if defined(_CMU_ADCCTRL_ADC1CLKDIV_MASK)
        case cmuClock_ADC1ASYNC:
          EFM_ASSERT(presc <= 3);
          CMU->ADCCTRL = (CMU->ADCCTRL & ~_CMU_ADCCTRL_ADC1CLKDIV_MASK)
                         | (presc << _CMU_ADCCTRL_ADC1CLKDIV_SHIFT);
          break;
#endif
        default:
          EFM_ASSERT(false);
          break;
      }
      break;
#endif

#if defined(_CMU_HFBUSPRESC_MASK)
    case CMU_HFBUSPRESC_REG:
      EFM_ASSERT(presc <= _CMU_HFBUSPRESC_MASK >> _CMU_HFBUSPRESC_PRESC_SHIFT);
      CMU->HFBUSPRESC = (CMU->HFBUSPRESC & ~_CMU_HFBUSPRESC_MASK)
                        | (presc << _CMU_HFBUSPRESC_PRESC_SHIFT);
      break;
#endif

    default:
      EFM_ASSERT(false);
      break;
  }
}
#endif

/***************************************************************************//**
 * @brief
 *   Get the currently selected reference clock used for a clock branch.
 *
 * @param[in] clock
 *   Clock branch to fetch selected ref. clock for. One of:
 *   @li #cmuClock_HF
 *   @li #cmuClock_LFA
 *   @li #cmuClock_LFB @if _CMU_LFCLKSEL_LFAE_ULFRCO
 *   @li #cmuClock_LFC
 *   @endif            @if _SILICON_LABS_32B_SERIES_1
 *   @li #cmuClock_LFE
 *   @endif
 *   @li #cmuClock_DBG @if DOXYDOC_USB_PRESENT
 *   @li #cmuClock_USBC
 *   @endif
 *
 * @return
 *   The reference clock used for clocking the selected branch, #cmuSelect_Error if
 *   invalid @p clock provided.
 ******************************************************************************/
CMU_Select_TypeDef CMU_ClockSelectGet(CMU_Clock_TypeDef clock)
{
  CMU_Select_TypeDef ret = cmuSelect_Disabled;
  uint32_t selReg;

  selReg = ((unsigned)clock >> CMU_SEL_REG_POS) & CMU_SEL_REG_MASK;

  switch (selReg) {
    case CMU_HFCLKSEL_REG:
#if defined(_CMU_HFCLKSTATUS_MASK)
      switch (CMU->HFCLKSTATUS & _CMU_HFCLKSTATUS_SELECTED_MASK) {
        case CMU_HFCLKSTATUS_SELECTED_LFXO:
          ret = cmuSelect_LFXO;
          break;

        case CMU_HFCLKSTATUS_SELECTED_LFRCO:
          ret = cmuSelect_LFRCO;
          break;

        case CMU_HFCLKSTATUS_SELECTED_HFXO:
          ret = cmuSelect_HFXO;
          break;

#if defined(CMU_HFCLKSTATUS_SELECTED_HFRCODIV2)
        case CMU_HFCLKSTATUS_SELECTED_HFRCODIV2:
          ret = cmuSelect_HFRCODIV2;
          break;
#endif

#if defined(CMU_HFCLKSTATUS_SELECTED_CLKIN0)
        case CMU_HFCLKSTATUS_SELECTED_CLKIN0:
          ret = cmuSelect_CLKIN0;
          break;
#endif

#if defined(CMU_HFCLKSTATUS_SELECTED_USHFRCO)
        case CMU_HFCLKSTATUS_SELECTED_USHFRCO:
          ret = cmuSelect_USHFRCO;
          break;
#endif

        default:
          ret = cmuSelect_HFRCO;
          break;
      }
#else
      switch (CMU->STATUS
              & (CMU_STATUS_HFRCOSEL
                 | CMU_STATUS_HFXOSEL
                 | CMU_STATUS_LFRCOSEL
#if defined(CMU_STATUS_USHFRCODIV2SEL)
                 | CMU_STATUS_USHFRCODIV2SEL
#endif
                 | CMU_STATUS_LFXOSEL)) {
        case CMU_STATUS_LFXOSEL:
          ret = cmuSelect_LFXO;
          break;

        case CMU_STATUS_LFRCOSEL:
          ret = cmuSelect_LFRCO;
          break;

        case CMU_STATUS_HFXOSEL:
          ret = cmuSelect_HFXO;
          break;

#if defined(CMU_STATUS_USHFRCODIV2SEL)
        case CMU_STATUS_USHFRCODIV2SEL:
          ret = cmuSelect_USHFRCODIV2;
          break;
#endif

        default:
          ret = cmuSelect_HFRCO;
          break;
      }
#endif
      break;

#if defined(_CMU_LFCLKSEL_MASK) || defined(_CMU_LFACLKSEL_MASK)
    case CMU_LFACLKSEL_REG:
#if defined(_CMU_LFCLKSEL_MASK)
      switch (CMU->LFCLKSEL & _CMU_LFCLKSEL_LFA_MASK) {
        case CMU_LFCLKSEL_LFA_LFRCO:
          ret = cmuSelect_LFRCO;
          break;

        case CMU_LFCLKSEL_LFA_LFXO:
          ret = cmuSelect_LFXO;
          break;

#if defined(CMU_LFCLKSEL_LFA_HFCORECLKLEDIV2)
        case CMU_LFCLKSEL_LFA_HFCORECLKLEDIV2:
          ret = cmuSelect_HFCLKLE;
          break;
#endif

        default:
#if defined(CMU_LFCLKSEL_LFAE)
          if (CMU->LFCLKSEL & _CMU_LFCLKSEL_LFAE_MASK) {
            ret = cmuSelect_ULFRCO;
            break;
          }
#else
          ret = cmuSelect_Disabled;
#endif
          break;
      }

#elif defined(_CMU_LFACLKSEL_MASK)
      switch (CMU->LFACLKSEL & _CMU_LFACLKSEL_LFA_MASK) {
        case CMU_LFACLKSEL_LFA_LFRCO:
          ret = cmuSelect_LFRCO;
          break;

        case CMU_LFACLKSEL_LFA_LFXO:
          ret = cmuSelect_LFXO;
          break;

        case CMU_LFACLKSEL_LFA_ULFRCO:
          ret = cmuSelect_ULFRCO;
          break;

#if defined(PLFRCO_PRESENT)
        case CMU_LFACLKSEL_LFA_PLFRCO:
          ret = cmuSelect_PLFRCO;
          break;
#endif

        default:
          ret = cmuSelect_Disabled;
          break;
      }
#endif
      break;
#endif /* _CMU_LFCLKSEL_MASK || _CMU_LFACLKSEL_MASK */

#if defined(_CMU_LFCLKSEL_MASK) || defined(_CMU_LFBCLKSEL_MASK)
    case CMU_LFBCLKSEL_REG:
#if defined(_CMU_LFCLKSEL_MASK)
      switch (CMU->LFCLKSEL & _CMU_LFCLKSEL_LFB_MASK) {
        case CMU_LFCLKSEL_LFB_LFRCO:
          ret = cmuSelect_LFRCO;
          break;

        case CMU_LFCLKSEL_LFB_LFXO:
          ret = cmuSelect_LFXO;
          break;

#if defined(CMU_LFCLKSEL_LFB_HFCORECLKLEDIV2)
        case CMU_LFCLKSEL_LFB_HFCORECLKLEDIV2:
          ret = cmuSelect_HFCLKLE;
          break;
#endif

#if defined(CMU_LFCLKSEL_LFB_HFCLKLE)
        case CMU_LFCLKSEL_LFB_HFCLKLE:
          ret = cmuSelect_HFCLKLE;
          break;
#endif

        default:
#if defined(CMU_LFCLKSEL_LFBE)
          if (CMU->LFCLKSEL & _CMU_LFCLKSEL_LFBE_MASK) {
            ret = cmuSelect_ULFRCO;
            break;
          }
#else
          ret = cmuSelect_Disabled;
#endif
          break;
      }

#elif defined(_CMU_LFBCLKSEL_MASK)
      switch (CMU->LFBCLKSEL & _CMU_LFBCLKSEL_LFB_MASK) {
        case CMU_LFBCLKSEL_LFB_LFRCO:
          ret = cmuSelect_LFRCO;
          break;

        case CMU_LFBCLKSEL_LFB_LFXO:
          ret = cmuSelect_LFXO;
          break;

        case CMU_LFBCLKSEL_LFB_ULFRCO:
          ret = cmuSelect_ULFRCO;
          break;

        case CMU_LFBCLKSEL_LFB_HFCLKLE:
          ret = cmuSelect_HFCLKLE;
          break;

#if defined(PLFRCO_PRESENT)
        case CMU_LFBCLKSEL_LFB_PLFRCO:
          ret = cmuSelect_PLFRCO;
          break;
#endif

        default:
          ret = cmuSelect_Disabled;
          break;
      }
#endif
      break;
#endif /* _CMU_LFCLKSEL_MASK || _CMU_LFBCLKSEL_MASK */

#if defined(_CMU_LFCLKSEL_LFC_MASK)
    case CMU_LFCCLKSEL_REG:
      switch (CMU->LFCLKSEL & _CMU_LFCLKSEL_LFC_MASK) {
        case CMU_LFCLKSEL_LFC_LFRCO:
          ret = cmuSelect_LFRCO;
          break;

        case CMU_LFCLKSEL_LFC_LFXO:
          ret = cmuSelect_LFXO;
          break;

        default:
          ret = cmuSelect_Disabled;
          break;
      }
      break;
#endif

#if defined(_CMU_LFECLKSEL_LFE_MASK)
    case CMU_LFECLKSEL_REG:
      switch (CMU->LFECLKSEL & _CMU_LFECLKSEL_LFE_MASK) {
        case CMU_LFECLKSEL_LFE_LFRCO:
          ret = cmuSelect_LFRCO;
          break;

        case CMU_LFECLKSEL_LFE_LFXO:
          ret = cmuSelect_LFXO;
          break;

        case CMU_LFECLKSEL_LFE_ULFRCO:
          ret = cmuSelect_ULFRCO;
          break;

#if defined(PLFRCO_PRESENT)
        case CMU_LFECLKSEL_LFE_PLFRCO:
          ret = cmuSelect_PLFRCO;
          break;
#endif

        default:
          ret = cmuSelect_Disabled;
          break;
      }
      break;
#endif /* CMU_LFECLKSEL_REG */

#if defined(_CMU_LFCCLKSEL_LFC_MASK)
    case CMU_LFCCLKSEL_REG:
      switch (CMU->LFCCLKSEL & _CMU_LFCCLKSEL_LFC_MASK) {
        case CMU_LFCCLKSEL_LFC_LFRCO:
          ret = cmuSelect_LFRCO;
          break;

        case CMU_LFCCLKSEL_LFC_LFXO:
          ret = cmuSelect_LFXO;
          break;

        case CMU_LFCCLKSEL_LFC_ULFRCO:
          ret = cmuSelect_ULFRCO;
          break;

        default:
          ret = cmuSelect_Disabled;
          break;
      }
      break;
#endif /* CMU_LFCCLKSEL_REG */

    case CMU_DBGCLKSEL_REG:
#if defined(_CMU_DBGCLKSEL_DBG_MASK)
      switch (CMU->DBGCLKSEL & _CMU_DBGCLKSEL_DBG_MASK) {
        case CMU_DBGCLKSEL_DBG_HFCLK:
          ret = cmuSelect_HFCLK;
          break;

        case CMU_DBGCLKSEL_DBG_AUXHFRCO:
          ret = cmuSelect_AUXHFRCO;
          break;

        default:
          ret = cmuSelect_Disabled;
          break;
      }

#elif defined(_CMU_CTRL_DBGCLK_MASK)
      switch (CMU->CTRL & _CMU_CTRL_DBGCLK_MASK) {
        case CMU_CTRL_DBGCLK_AUXHFRCO:
          ret = cmuSelect_AUXHFRCO;
          break;

        case CMU_CTRL_DBGCLK_HFCLK:
          ret = cmuSelect_HFCLK;
          break;
      }
#else
      ret = cmuSelect_AUXHFRCO;
#endif
      break;

#if defined(USBC_CLOCK_PRESENT)
    case CMU_USBCCLKSEL_REG:
      switch (CMU->STATUS
              & (CMU_STATUS_USBCLFXOSEL
#if defined(_CMU_STATUS_USBCHFCLKSEL_MASK)
                 | CMU_STATUS_USBCHFCLKSEL
#endif
#if defined(_CMU_STATUS_USBCUSHFRCOSEL_MASK)
                 | CMU_STATUS_USBCUSHFRCOSEL
#endif
                 | CMU_STATUS_USBCLFRCOSEL)) {
#if defined(_CMU_STATUS_USBCHFCLKSEL_MASK)
        case CMU_STATUS_USBCHFCLKSEL:
          ret = cmuSelect_HFCLK;
          break;
#endif

#if defined(_CMU_STATUS_USBCUSHFRCOSEL_MASK)
        case CMU_STATUS_USBCUSHFRCOSEL:
          ret = cmuSelect_USHFRCO;
          break;
#endif

        case CMU_STATUS_USBCLFXOSEL:
          ret = cmuSelect_LFXO;
          break;

        case CMU_STATUS_USBCLFRCOSEL:
          ret = cmuSelect_LFRCO;
          break;

        default:
          ret = cmuSelect_Disabled;
          break;
      }
      break;
#endif

#if defined(_CMU_ADCCTRL_ADC0CLKSEL_MASK)
    case CMU_ADC0ASYNCSEL_REG:
      switch (CMU->ADCCTRL & _CMU_ADCCTRL_ADC0CLKSEL_MASK) {
        case CMU_ADCCTRL_ADC0CLKSEL_DISABLED:
          ret = cmuSelect_Disabled;
          break;

        case CMU_ADCCTRL_ADC0CLKSEL_AUXHFRCO:
          ret = cmuSelect_AUXHFRCO;
          break;

        case CMU_ADCCTRL_ADC0CLKSEL_HFXO:
          ret = cmuSelect_HFXO;
          break;

        case CMU_ADCCTRL_ADC0CLKSEL_HFSRCCLK:
          ret = cmuSelect_HFSRCCLK;
          break;

        default:
          ret = cmuSelect_Disabled;
          break;
      }
      break;
#endif

#if defined(_CMU_ADCCTRL_ADC1CLKSEL_MASK)
    case CMU_ADC1ASYNCSEL_REG:
      switch (CMU->ADCCTRL & _CMU_ADCCTRL_ADC1CLKSEL_MASK) {
        case CMU_ADCCTRL_ADC1CLKSEL_DISABLED:
          ret = cmuSelect_Disabled;
          break;

        case CMU_ADCCTRL_ADC1CLKSEL_AUXHFRCO:
          ret = cmuSelect_AUXHFRCO;
          break;

        case CMU_ADCCTRL_ADC1CLKSEL_HFXO:
          ret = cmuSelect_HFXO;
          break;

        case CMU_ADCCTRL_ADC1CLKSEL_HFSRCCLK:
          ret = cmuSelect_HFSRCCLK;
          break;
      }
      break;
#endif

#if defined(_CMU_SDIOCTRL_SDIOCLKSEL_MASK)
    case CMU_SDIOREFSEL_REG:
      switch (CMU->SDIOCTRL & _CMU_SDIOCTRL_SDIOCLKSEL_MASK) {
        case CMU_SDIOCTRL_SDIOCLKSEL_HFRCO:
          ret = cmuSelect_HFRCO;
          break;

        case CMU_SDIOCTRL_SDIOCLKSEL_HFXO:
          ret = cmuSelect_HFXO;
          break;

        case CMU_SDIOCTRL_SDIOCLKSEL_AUXHFRCO:
          ret = cmuSelect_AUXHFRCO;
          break;

        case CMU_SDIOCTRL_SDIOCLKSEL_USHFRCO:
          ret = cmuSelect_USHFRCO;
          break;
      }
      break;
#endif

#if defined(_CMU_QSPICTRL_QSPI0CLKSEL_MASK)
    case CMU_QSPI0REFSEL_REG:
      switch (CMU->QSPICTRL & _CMU_QSPICTRL_QSPI0CLKSEL_MASK) {
        case CMU_QSPICTRL_QSPI0CLKSEL_HFRCO:
          ret = cmuSelect_HFRCO;
          break;

        case CMU_QSPICTRL_QSPI0CLKSEL_HFXO:
          ret = cmuSelect_HFXO;
          break;

        case CMU_QSPICTRL_QSPI0CLKSEL_AUXHFRCO:
          ret = cmuSelect_AUXHFRCO;
          break;

        case CMU_QSPICTRL_QSPI0CLKSEL_USHFRCO:
          ret = cmuSelect_USHFRCO;
          break;
      }
      break;
#endif

#if defined(_CMU_USBCTRL_USBCLKSEL_MASK)
    case CMU_USBRCLKSEL_REG:
      switch (CMU->USBCTRL & _CMU_USBCTRL_USBCLKSEL_MASK) {
        case CMU_USBCTRL_USBCLKSEL_USHFRCO:
          ret = cmuSelect_USHFRCO;
          break;

        case CMU_USBCTRL_USBCLKSEL_HFXO:
          ret = cmuSelect_HFXO;
          break;

        case CMU_USBCTRL_USBCLKSEL_HFXOX2:
          ret = cmuSelect_HFXOX2;
          break;

        case CMU_USBCTRL_USBCLKSEL_HFRCO:
          ret = cmuSelect_HFRCO;
          break;

        case CMU_USBCTRL_USBCLKSEL_LFXO:
          ret = cmuSelect_LFXO;
          break;

        case CMU_USBCTRL_USBCLKSEL_LFRCO:
          ret = cmuSelect_LFRCO;
          break;
      }
      break;
#endif

#if defined(_CMU_PDMCTRL_PDMCLKSEL_MASK)
    case CMU_PDMREFSEL_REG:
      switch (CMU->PDMCTRL & _CMU_PDMCTRL_PDMCLKSEL_MASK) {
        case CMU_PDMCTRL_PDMCLKSEL_USHFRCO:
          ret = cmuSelect_USHFRCO;
          break;

        case CMU_PDMCTRL_PDMCLKSEL_HFXO:
          ret = cmuSelect_HFXO;
          break;

        case CMU_PDMCTRL_PDMCLKSEL_HFRCO:
          ret = cmuSelect_HFRCO;
          break;
      }
      break;
#endif

    default:
      ret = cmuSelect_Error;
      EFM_ASSERT(false);
      break;
  }

  return ret;
}

/***************************************************************************//**
 * @brief
 *   Select the reference clock/oscillator used for a clock branch.
 *
 * @details
 *   Notice that if a selected reference is not enabled prior to selecting its
 *   use, it will be enabled and this function will wait for the selected
 *   oscillator to be stable. It will however NOT be disabled if another
 *   reference clock is selected later.
 *
 *   This feature is particularly important if selecting a new reference
 *   clock for the clock branch clocking the core. Otherwise, the system
 *   may halt.
 *
 * @note
 *   HFCLKLE prescaler is automatically modified when peripherals with clock
 *   domain HFBUSCLK is chosen based on the maximum HFLE frequency allowed.
 *
 * @param[in] clock
 *   A clock branch to select reference clock for. One of:
 *   @li #cmuClock_HF
 *   @li #cmuClock_LFA
 *   @li #cmuClock_LFB
 *   @if _CMU_LFCCLKEN0_MASK
 *   @li #cmuClock_LFC
 *   @endif
 *   @if _CMU_LFECLKEN0_MASK
 *   @li #cmuClock_LFE
 *   @endif
 *   @li #cmuClock_DBG
 *   @if _CMU_CMD_USBCLKSEL_MASK
 *   @li #cmuClock_USBC
 *   @endif
 *   @if _CMU_USBCTRL_MASK
 *   @li #cmuClock_USBR
 *   @endif
 *
 * @param[in] ref
 *   A reference selected for clocking. See the reference manual
 *   for details about references available for a specific clock branch.
 *   @li #cmuSelect_HFRCO
 *   @li #cmuSelect_LFRCO
 *   @li #cmuSelect_HFXO
 *   @if _CMU_HFXOCTRL_HFXOX2EN_MASK
 *   @li #cmuSelect_HFXOX2
 *   @endif
 *   @li #cmuSelect_LFXO
 *   @li #cmuSelect_HFCLKLE
 *   @li #cmuSelect_AUXHFRCO
 *   @if _CMU_USHFRCOCTRL_MASK
 *   @li #cmuSelect_USHFRCO
 *   @endif
 *   @li #cmuSelect_HFCLK
 *   @ifnot DOXYDOC_EFM32_GECKO_FAMILY
 *   @li #cmuSelect_ULFRCO
 *   @endif
 *   @if CMU_OSCENCMD_PLFRCOEN
 *   @li #cmuSelect_PLFRCO
 *   @endif
 ******************************************************************************/
void CMU_ClockSelectSet(CMU_Clock_TypeDef clock, CMU_Select_TypeDef ref)
{
  uint32_t              select = (uint32_t)cmuOsc_HFRCO;
  CMU_Osc_TypeDef       osc    = cmuOsc_HFRCO;
  uint32_t              freq;
  uint32_t              tmp;
  uint32_t              selRegId;
#if defined(_SILICON_LABS_32B_SERIES_1)
  volatile uint32_t     *selReg = NULL;
#endif
#if defined(CMU_LFCLKSEL_LFAE_ULFRCO)
  uint32_t              lfExtended = 0;
#endif

#if defined(_EMU_CMD_EM01VSCALE0_MASK)
  uint32_t              vScaleFrequency = 0; /* Use default. */

  /* Start voltage upscaling before the clock is set. */
  if (clock == cmuClock_HF) {
    if (ref == cmuSelect_HFXO) {
      vScaleFrequency = SystemHFXOClockGet();
    } else if ((ref == cmuSelect_HFRCO)
               && ((uint32_t)CMU_HFRCOBandGet()
                   > CMU_VSCALEEM01_LOWPOWER_VOLTAGE_CLOCK_MAX)) {
      vScaleFrequency = (uint32_t)CMU_HFRCOBandGet();
    } else {
      /* Use the default frequency. */
    }
    if (vScaleFrequency != 0UL) {
      EMU_VScaleEM01ByClock(vScaleFrequency, false);
    }
  }
#endif

  selRegId = ((unsigned)clock >> CMU_SEL_REG_POS) & CMU_SEL_REG_MASK;

  switch (selRegId) {
    case CMU_HFCLKSEL_REG:
      switch (ref) {
        case cmuSelect_LFXO:
#if defined(_SILICON_LABS_32B_SERIES_1)
          select = CMU_HFCLKSEL_HF_LFXO;
#elif defined(_SILICON_LABS_32B_SERIES_0)
          select = CMU_CMD_HFCLKSEL_LFXO;
#endif
          osc = cmuOsc_LFXO;
          break;

        case cmuSelect_LFRCO:
#if defined(_SILICON_LABS_32B_SERIES_1)
          select = CMU_HFCLKSEL_HF_LFRCO;
#elif defined(_SILICON_LABS_32B_SERIES_0)
          select = CMU_CMD_HFCLKSEL_LFRCO;
#endif
          osc = cmuOsc_LFRCO;
          break;

        case cmuSelect_HFXO:
#if defined(CMU_HFCLKSEL_HF_HFXO)
          select = CMU_HFCLKSEL_HF_HFXO;
#elif defined(CMU_CMD_HFCLKSEL_HFXO)
          select = CMU_CMD_HFCLKSEL_HFXO;
#endif
          osc = cmuOsc_HFXO;
#if defined(CMU_MAX_FREQ_HFLE)
          /* Set 1 HFLE wait-state until the new HFCLKLE frequency is known.
             This is known after 'select' is written below. */
          setHfLeConfig(CMU_MAX_FREQ_HFLE + 1UL);
#endif
#if defined(CMU_CTRL_HFXOBUFCUR_BOOSTABOVE32MHZ)
          /* Adjust HFXO buffer current for frequencies above 32 MHz. */
          if (SystemHFXOClockGet() > 32000000) {
            CMU->CTRL = (CMU->CTRL & ~_CMU_CTRL_HFXOBUFCUR_MASK)
                        | CMU_CTRL_HFXOBUFCUR_BOOSTABOVE32MHZ;
          } else {
            CMU->CTRL = (CMU->CTRL & ~_CMU_CTRL_HFXOBUFCUR_MASK)
                        | CMU_CTRL_HFXOBUFCUR_BOOSTUPTO32MHZ;
          }
#endif
          break;

        case cmuSelect_HFRCO:
#if defined(_SILICON_LABS_32B_SERIES_1)
          select = CMU_HFCLKSEL_HF_HFRCO;
#elif defined(_SILICON_LABS_32B_SERIES_0)
          select = CMU_CMD_HFCLKSEL_HFRCO;
#endif
          osc = cmuOsc_HFRCO;
#if defined(CMU_MAX_FREQ_HFLE)
          /* Set 1 HFLE wait-state until the new HFCLKLE frequency is known.
             This is known after 'select' is written below. */
          setHfLeConfig(CMU_MAX_FREQ_HFLE + 1UL);
#endif
          break;

#if defined(CMU_CMD_HFCLKSEL_USHFRCODIV2)
        case cmuSelect_USHFRCODIV2:
          select = CMU_CMD_HFCLKSEL_USHFRCODIV2;
          osc = cmuOsc_USHFRCO;
          break;
#endif

#if defined(CMU_HFCLKSTATUS_SELECTED_HFRCODIV2)
        case cmuSelect_HFRCODIV2:
          select = CMU_HFCLKSEL_HF_HFRCODIV2;
          osc = cmuOsc_HFRCO;
          break;
#endif

#if defined(CMU_HFCLKSTATUS_SELECTED_CLKIN0)
        case cmuSelect_CLKIN0:
          select = CMU_HFCLKSEL_HF_CLKIN0;
          osc = cmuOsc_CLKIN0;
          break;
#endif

#if defined(CMU_HFCLKSTATUS_SELECTED_USHFRCO)
        case cmuSelect_USHFRCO:
          select = CMU_HFCLKSEL_HF_USHFRCO;
          osc = cmuOsc_USHFRCO;
          break;
#endif

#if defined(CMU_LFCLKSEL_LFAE_ULFRCO) || defined(CMU_LFACLKSEL_LFA_ULFRCO)
        case cmuSelect_ULFRCO:
          /* ULFRCO cannot be used as HFCLK.  */
          EFM_ASSERT(false);
          return;
#endif

        default:
          EFM_ASSERT(false);
          return;
      }

      /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
#if defined(CMU_HFCLKSTATUS_SELECTED_CLKIN0)
      if (osc != cmuOsc_CLKIN0) {
        CMU_OscillatorEnable(osc, true, true);
      }
#else
      CMU_OscillatorEnable(osc, true, true);
#endif

      /* Configure worst case wait-states for flash and set safe HFPER
         clock-tree prescalers. */
      flashWaitStateMax();
      hfperClkSafePrescaler();

#if defined(_EMU_CMD_EM01VSCALE0_MASK)
      /* Wait for voltage upscaling to complete before the clock is set. */
      if (vScaleFrequency != 0UL) {
        EMU_VScaleWait();
      }
#endif

      /* Switch to the selected oscillator. */
#if defined(_CMU_HFCLKSEL_MASK)
      CMU->HFCLKSEL = select;
#else
      CMU->CMD = select;
#endif
      /* Update the CMSIS core clock variable. */
      /* (The function will update the global variable). */
      freq = SystemCoreClockGet();

#if defined(CMU_MAX_FREQ_HFLE)
      /* Update the HFLE configuration after 'select' is set.
         Note that the HFCLKLE clock is connected differently on platforms 1 and 2. */
#if defined(_SILICON_LABS_32B_SERIES_0)
      setHfLeConfig(freq);
#else
      setHfLeConfig(SystemHFClockGet());
#endif
#endif

      /* Optimize flash access wait state setting for the currently selected core clk. */
      CMU_UpdateWaitStates(freq, VSCALE_DEFAULT);

#if defined(_EMU_CMD_EM01VSCALE0_MASK)
      /* Keep EMU module informed on the source HF clock frequency. This will apply voltage
         downscaling after clock is set if downscaling is configured. */
      if (vScaleFrequency == 0UL) {
        EMU_VScaleEM01ByClock(0, true);
      }
#endif
      /* Set optimized HFPER clock-tree prescalers. */
      hfperClkOptimizedPrescaler();
      break;

#if defined(_SILICON_LABS_32B_SERIES_1)
    case CMU_LFACLKSEL_REG:
      selReg = &CMU->LFACLKSEL;
      /* HFCLKCLE can't be used as LFACLK. */
      EFM_ASSERT(ref != cmuSelect_HFCLKLE);
      SL_FALLTHROUGH
      /* Fall through and select the clock source. */

#if defined(_CMU_LFCCLKSEL_MASK)
    case CMU_LFCCLKSEL_REG:
      selReg = (selReg == NULL) ? &CMU->LFCCLKSEL : selReg;
      /* HFCLKCLE can't be used as LFCCLK. */
      EFM_ASSERT(ref != cmuSelect_HFCLKLE);
      SL_FALLTHROUGH
#endif
    /* Fall through and select the clock source. */

    case CMU_LFECLKSEL_REG:
      selReg = (selReg == NULL) ? &CMU->LFECLKSEL : selReg;
      /* HFCLKCLE can't be used as LFECLK. */
      EFM_ASSERT(ref != cmuSelect_HFCLKLE);
      SL_FALLTHROUGH
    /* Fall through and select the clock source. */

    case CMU_LFBCLKSEL_REG:
      selReg = (selReg == NULL) ? &CMU->LFBCLKSEL : selReg;
      switch (ref) {
        case cmuSelect_Disabled:
          tmp = _CMU_LFACLKSEL_LFA_DISABLED;
          break;

        case cmuSelect_LFXO:
          /* Ensure that thes elected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
          tmp = _CMU_LFACLKSEL_LFA_LFXO;
          break;

        case cmuSelect_LFRCO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_LFRCO, true, true);
          tmp = _CMU_LFACLKSEL_LFA_LFRCO;
          break;

        case cmuSelect_HFCLKLE:
          /* Ensure the correct HFLE wait-states and enable HFCLK to LE.*/
          setHfLeConfig(SystemHFClockGet());
          BUS_RegBitWrite(&CMU->HFBUSCLKEN0, _CMU_HFBUSCLKEN0_LE_SHIFT, 1);
          tmp = _CMU_LFBCLKSEL_LFB_HFCLKLE;
          break;

        case cmuSelect_ULFRCO:
          /* ULFRCO is always on, there is no need to enable it. */
          tmp = _CMU_LFACLKSEL_LFA_ULFRCO;
          break;

#if defined(PLFRCO_PRESENT)
        case cmuSelect_PLFRCO:
          CMU_OscillatorEnable(cmuOsc_PLFRCO, true, true);
          tmp = _CMU_LFACLKSEL_LFA_PLFRCO;
          break;
#endif

        default:
          EFM_ASSERT(false);
          return;
      }
      *selReg = tmp;
      break;

#elif defined(_SILICON_LABS_32B_SERIES_0)
    case CMU_LFACLKSEL_REG:
    case CMU_LFBCLKSEL_REG:
      switch (ref) {
        case cmuSelect_Disabled:
          tmp = _CMU_LFCLKSEL_LFA_DISABLED;
          break;

        case cmuSelect_LFXO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
          tmp = _CMU_LFCLKSEL_LFA_LFXO;
          break;

        case cmuSelect_LFRCO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_LFRCO, true, true);
          tmp = _CMU_LFCLKSEL_LFA_LFRCO;
          break;

        case cmuSelect_HFCLKLE:
#if defined(CMU_MAX_FREQ_HFLE)
          /* Set the HFLE wait-state and divider. */
          freq = SystemCoreClockGet();
          setHfLeConfig(freq);
#endif
          /* Ensure HFCORE to LE clocking is enabled. */
          BUS_RegBitWrite(&CMU->HFCORECLKEN0, _CMU_HFCORECLKEN0_LE_SHIFT, 1);
          tmp = _CMU_LFCLKSEL_LFA_HFCORECLKLEDIV2;
          break;

#if defined(CMU_LFCLKSEL_LFAE_ULFRCO)
        case cmuSelect_ULFRCO:
          /* ULFRCO is always enabled. */
          tmp = _CMU_LFCLKSEL_LFA_DISABLED;
          lfExtended = 1;
          break;
#endif

        default:
          /* An illegal clock source for LFA/LFB selected. */
          EFM_ASSERT(false);
          return;
      }

      /* Apply select. */
      if (selRegId == CMU_LFACLKSEL_REG) {
#if defined(_CMU_LFCLKSEL_LFAE_MASK)
        CMU->LFCLKSEL = (CMU->LFCLKSEL
                         & ~(_CMU_LFCLKSEL_LFA_MASK | _CMU_LFCLKSEL_LFAE_MASK))
                        | (tmp << _CMU_LFCLKSEL_LFA_SHIFT)
                        | (lfExtended << _CMU_LFCLKSEL_LFAE_SHIFT);
#else
        CMU->LFCLKSEL = (CMU->LFCLKSEL & ~_CMU_LFCLKSEL_LFA_MASK)
                        | (tmp << _CMU_LFCLKSEL_LFA_SHIFT);
#endif
      } else {
#if defined(_CMU_LFCLKSEL_LFBE_MASK)
        CMU->LFCLKSEL = (CMU->LFCLKSEL
                         & ~(_CMU_LFCLKSEL_LFB_MASK | _CMU_LFCLKSEL_LFBE_MASK))
                        | (tmp << _CMU_LFCLKSEL_LFB_SHIFT)
                        | (lfExtended << _CMU_LFCLKSEL_LFBE_SHIFT);
#else
        CMU->LFCLKSEL = (CMU->LFCLKSEL & ~_CMU_LFCLKSEL_LFB_MASK)
                        | (tmp << _CMU_LFCLKSEL_LFB_SHIFT);
#endif
      }
      break;

#if defined(_CMU_LFCLKSEL_LFC_MASK)
    case CMU_LFCCLKSEL_REG:
      switch (ref) {
        case cmuSelect_Disabled:
          tmp = _CMU_LFCLKSEL_LFA_DISABLED;
          break;

        case cmuSelect_LFXO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
          tmp = _CMU_LFCLKSEL_LFC_LFXO;
          break;

        case cmuSelect_LFRCO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_LFRCO, true, true);
          tmp = _CMU_LFCLKSEL_LFC_LFRCO;
          break;

        default:
          /* An illegal clock source for LFC selected. */
          EFM_ASSERT(false);
          return;
      }

      /* Apply select. */
      CMU->LFCLKSEL = (CMU->LFCLKSEL & ~_CMU_LFCLKSEL_LFC_MASK)
                      | (tmp << _CMU_LFCLKSEL_LFC_SHIFT);
      break;
#endif
#endif

#if defined(_CMU_DBGCLKSEL_DBG_MASK) || defined(CMU_CTRL_DBGCLK)
    case CMU_DBGCLKSEL_REG:
      switch (ref) {
#if defined(_CMU_DBGCLKSEL_DBG_MASK)
        case cmuSelect_AUXHFRCO:
          /* Select AUXHFRCO as a debug clock. */
          CMU->DBGCLKSEL = CMU_DBGCLKSEL_DBG_AUXHFRCO;
          break;

        case cmuSelect_HFCLK:
          /* Select divided HFCLK as a debug clock. */
          CMU->DBGCLKSEL = CMU_DBGCLKSEL_DBG_HFCLK;
          break;
#endif

#if defined(CMU_CTRL_DBGCLK)
        case cmuSelect_AUXHFRCO:
          /* Select AUXHFRCO as a debug clock. */
          CMU->CTRL = (CMU->CTRL & ~(_CMU_CTRL_DBGCLK_MASK))
                      | CMU_CTRL_DBGCLK_AUXHFRCO;
          break;

        case cmuSelect_HFCLK:
          /* Select divided HFCLK as a debug clock. */
          CMU->CTRL = (CMU->CTRL & ~(_CMU_CTRL_DBGCLK_MASK))
                      | CMU_CTRL_DBGCLK_HFCLK;
          break;
#endif

        default:
          /* An illegal clock source for debug selected. */
          EFM_ASSERT(false);
          return;
      }
      break;
#endif

#if defined(USBC_CLOCK_PRESENT)
    case CMU_USBCCLKSEL_REG:
      switch (ref) {
        case cmuSelect_LFXO:
          /* Select LFXO as a clock source for USB. It can only be used in sleep mode. */
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_LFXO, true, true);

          /* Switch the oscillator. */
          CMU->CMD = CMU_CMD_USBCCLKSEL_LFXO;

          /* Wait until the clock is activated. */
          while ((CMU->STATUS & CMU_STATUS_USBCLFXOSEL) == 0) {
          }
          break;

        case cmuSelect_LFRCO:
          /* Select LFRCO as a clock source for USB. It can only be used in sleep mode. */
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_LFRCO, true, true);

          /* Switch the oscillator. */
          CMU->CMD = CMU_CMD_USBCCLKSEL_LFRCO;

          /* Wait until the clock is activated. */
          while ((CMU->STATUS & CMU_STATUS_USBCLFRCOSEL) == 0) {
          }
          break;

#if defined(CMU_STATUS_USBCHFCLKSEL)
        case cmuSelect_HFCLK:
          /* Select undivided HFCLK as a clock source for USB. */
          /* The oscillator must already be enabled to avoid a core lockup. */
          CMU->CMD = CMU_CMD_USBCCLKSEL_HFCLKNODIV;
          /* Wait until the clock is activated. */
          while ((CMU->STATUS & CMU_STATUS_USBCHFCLKSEL) == 0) {
          }
          break;
#endif

#if defined(CMU_CMD_USBCCLKSEL_USHFRCO)
        case cmuSelect_USHFRCO:
          /* Select USHFRCO as a clock source for USB. */
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_USHFRCO, true, true);

          /* Switch the oscillator. */
          CMU->CMD = CMU_CMD_USBCCLKSEL_USHFRCO;

          /* Wait until the clock is activated. */
          while ((CMU->STATUS & CMU_STATUS_USBCUSHFRCOSEL) == 0) {
          }
          break;
#endif

        default:
          /* An illegal clock source for USB. */
          EFM_ASSERT(false);
          return;
      }
      break;
#endif

#if defined(_CMU_ADCCTRL_ADC0CLKSEL_MASK)
    case CMU_ADC0ASYNCSEL_REG:
      switch (ref) {
        case cmuSelect_Disabled:
          tmp = _CMU_ADCCTRL_ADC0CLKSEL_DISABLED;
          break;

        case cmuSelect_AUXHFRCO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_AUXHFRCO, true, true);
          tmp = _CMU_ADCCTRL_ADC0CLKSEL_AUXHFRCO;
          break;

        case cmuSelect_HFXO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
          tmp = _CMU_ADCCTRL_ADC0CLKSEL_HFXO;
          break;

        case cmuSelect_HFSRCCLK:
          tmp = _CMU_ADCCTRL_ADC0CLKSEL_HFSRCCLK;
          break;

        default:
          /* An illegal clock source for ADC0ASYNC selected. */
          EFM_ASSERT(false);
          return;
      }

      /* Apply select. */
      CMU->ADCCTRL = (CMU->ADCCTRL & ~_CMU_ADCCTRL_ADC0CLKSEL_MASK)
                     | (tmp << _CMU_ADCCTRL_ADC0CLKSEL_SHIFT);
      break;
#endif

#if defined(_CMU_ADCCTRL_ADC1CLKSEL_MASK)
    case CMU_ADC1ASYNCSEL_REG:
      switch (ref) {
        case cmuSelect_Disabled:
          tmp = _CMU_ADCCTRL_ADC1CLKSEL_DISABLED;
          break;

        case cmuSelect_AUXHFRCO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_AUXHFRCO, true, true);
          tmp = _CMU_ADCCTRL_ADC1CLKSEL_AUXHFRCO;
          break;

        case cmuSelect_HFXO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
          tmp = _CMU_ADCCTRL_ADC1CLKSEL_HFXO;
          break;

        case cmuSelect_HFSRCCLK:
          tmp = _CMU_ADCCTRL_ADC1CLKSEL_HFSRCCLK;
          break;

        default:
          /* An illegal clock source for ADC1ASYNC selected. */
          EFM_ASSERT(false);
          return;
      }

      /* Apply select. */
      CMU->ADCCTRL = (CMU->ADCCTRL & ~_CMU_ADCCTRL_ADC1CLKSEL_MASK)
                     | (tmp << _CMU_ADCCTRL_ADC1CLKSEL_SHIFT);
      break;
#endif

#if defined(_CMU_SDIOCTRL_SDIOCLKSEL_MASK)
    case CMU_SDIOREFSEL_REG:
      switch (ref) {
        case cmuSelect_HFRCO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_HFRCO, true, true);
          tmp = _CMU_SDIOCTRL_SDIOCLKSEL_HFRCO;
          break;

        case cmuSelect_HFXO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
          tmp = _CMU_SDIOCTRL_SDIOCLKSEL_HFXO;
          break;

        case cmuSelect_AUXHFRCO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_AUXHFRCO, true, true);
          tmp = _CMU_SDIOCTRL_SDIOCLKSEL_AUXHFRCO;
          break;

        case cmuSelect_USHFRCO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_USHFRCO, true, true);
          tmp = _CMU_SDIOCTRL_SDIOCLKSEL_USHFRCO;
          break;

        default:
          /* An illegal clock source for SDIOREF selected. */
          EFM_ASSERT(false);
          return;
      }

      /* Apply select. */
      CMU->SDIOCTRL = (CMU->SDIOCTRL & ~_CMU_SDIOCTRL_SDIOCLKSEL_MASK)
                      | (tmp << _CMU_SDIOCTRL_SDIOCLKSEL_SHIFT);
      break;
#endif

#if defined(_CMU_QSPICTRL_QSPI0CLKSEL_MASK)
    case CMU_QSPI0REFSEL_REG:
      switch (ref) {
        case cmuSelect_HFRCO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_HFRCO, true, true);
          tmp = _CMU_QSPICTRL_QSPI0CLKSEL_HFRCO;
          break;

        case cmuSelect_HFXO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
          tmp = _CMU_QSPICTRL_QSPI0CLKSEL_HFXO;
          break;

        case cmuSelect_AUXHFRCO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_AUXHFRCO, true, true);
          tmp = _CMU_QSPICTRL_QSPI0CLKSEL_AUXHFRCO;
          break;

        case cmuSelect_USHFRCO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_USHFRCO, true, true);
          tmp = _CMU_QSPICTRL_QSPI0CLKSEL_USHFRCO;
          break;

        default:
          /* An illegal clock source for QSPI0REF selected. */
          EFM_ASSERT(false);
          return;
      }

      /* Apply select. */
      CMU->QSPICTRL = (CMU->QSPICTRL & ~_CMU_QSPICTRL_QSPI0CLKSEL_MASK)
                      | (tmp << _CMU_QSPICTRL_QSPI0CLKSEL_SHIFT);
      break;
#endif

#if defined(_CMU_USBCTRL_USBCLKSEL_MASK)
    case CMU_USBRCLKSEL_REG:
      switch (ref) {
        case cmuSelect_USHFRCO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_USHFRCO, true, true);
          tmp = _CMU_USBCTRL_USBCLKSEL_USHFRCO;
          break;

        case cmuSelect_HFXO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
          tmp = _CMU_USBCTRL_USBCLKSEL_HFXO;
          break;

        case cmuSelect_HFXOX2:
          /* Only allowed for HFXO frequencies up to 25 MHz. */
          EFM_ASSERT(SystemHFXOClockGet() <= 25000000u);

          /* Enable HFXO X2. */
          CMU->HFXOCTRL |= CMU_HFXOCTRL_HFXOX2EN;

          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_HFXO, true, true);

          tmp = _CMU_USBCTRL_USBCLKSEL_HFXOX2;
          break;

        case cmuSelect_HFRCO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_HFRCO, true, true);
          tmp = _CMU_USBCTRL_USBCLKSEL_HFRCO;
          break;

        case cmuSelect_LFXO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
          tmp = _CMU_USBCTRL_USBCLKSEL_LFXO;
          break;

        case cmuSelect_LFRCO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_LFRCO, true, true);
          tmp = _CMU_USBCTRL_USBCLKSEL_LFRCO;
          break;

        default:
          /* An illegal clock source for USBR selected. */
          EFM_ASSERT(false);
          return;
      }

      /* Apply select. */
      CMU->USBCTRL = (CMU->USBCTRL & ~_CMU_USBCTRL_USBCLKSEL_MASK)
                     | (tmp << _CMU_USBCTRL_USBCLKSEL_SHIFT);
      break;
#endif

#if defined(_CMU_PDMCTRL_PDMCLKSEL_MASK)
    case CMU_PDMREFSEL_REG:
      switch (ref) {
        case cmuSelect_USHFRCO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_USHFRCO, true, true);
          tmp = _CMU_PDMCTRL_PDMCLKSEL_USHFRCO;
          break;

        case cmuSelect_HFXO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
          tmp = _CMU_PDMCTRL_PDMCLKSEL_HFXO;
          break;

        case cmuSelect_HFRCO:
          /* Ensure that the selected oscillator is enabled, waiting for it to stabilize. */
          CMU_OscillatorEnable(cmuOsc_HFRCO, true, true);
          tmp = _CMU_PDMCTRL_PDMCLKSEL_HFRCO;
          break;

        default:
          /* An illegal clock source for PDMREF selected. */
          EFM_ASSERT(false);
          return;
      }

      /* Apply select. */
      CMU->PDMCTRL = (CMU->PDMCTRL & ~_CMU_PDMCTRL_PDMCLKSEL_MASK)
                     | (tmp << _CMU_PDMCTRL_PDMCLKSEL_SHIFT);
      break;
#endif

    default:
      EFM_ASSERT(false);
      break;
  }
}

/***************************************************************************//**
 * @brief
 *   Gets the precision (in PPM) of the specified low frequency clock branch.
 *
 * @param[in] clock
 *   Clock branch.
 *
 * @return
 *   Precision, in PPM, of the specified clock branch.
 *
 * @note
 *   This function is only for internal usage.
 *
 * @note
 *   The current implementation of this function is used to determine if the
 *   clock has a precision <= 500 ppm or not (which is the minimum required
 *   for BLE). Future version of this function should provide more accurate
 *   precision numbers to allow for further optimizations from the stacks.
 ******************************************************************************/
uint16_t CMU_LF_ClockPrecisionGet(CMU_Clock_TypeDef clock)
{
  CMU_Select_TypeDef src = CMU_ClockSelectGet(clock);
  uint16_t precision;

  switch (src) {
    case cmuSelect_LFXO:
      precision = lfxo_precision;
      break;

#if defined(PLFRCO_PRESENT)
#if defined(_SILICON_LABS_32B_SERIES_1)
    case cmuSelect_PLFRCO:
      precision = 500;
      break;
#endif

#if defined(LFRCO_CFG_HIGHPRECEN)
    case cmuSelect_LFRCO:
      CMU->CLKEN0_SET = CMU_CLKEN0_LFRCO;

      if (LFRCO->CFG & _LFRCO_CFG_HIGHPRECEN_MASK) {
        precision = 500;
      } else {
        precision = 0xFFFF;
      }
      break;
#endif
#endif

    default:
      precision = 0xFFFF;
      break;
  }

  return precision;
}

#if defined(CMU_OSCENCMD_DPLLEN)
/**************************************************************************//**
 * @brief
 *   Lock the DPLL to a given frequency.
 *
 *   The frequency is given by: Fout = Fref * (N+1) / (M+1).
 *
 * @note
 *   This function does not check if the given N & M values will actually
 *   produce the desired target frequency. @n
 *   N & M limitations: @n
 *     300 < N <= 4095 @n
 *     0 <= M <= 4095 @n
 *   Any peripheral running off HFRCO should be switched to HFRCODIV2 prior to
 *   calling this function to avoid over-clocking.
 *
 *   HFCLKLE prescaler is automatically modified before updating HFRCO
 *   based on the maximum HFLE frequency allowed.
 *
 * @param[in] init
 *    DPLL setup parameters.
 *
 * @return
 *   Returns false on invalid target frequency or DPLL locking error.
 *****************************************************************************/
bool CMU_DPLLLock(const CMU_DPLLInit_TypeDef *init)
{
  int index = 0;
  unsigned int i;
  bool hfrcoDiv2override = false;
  uint32_t hfrcoCtrlVal, lockStatus, sysFreq;

  EFM_ASSERT(init->frequency >= hfrcoCtrlTable[0].minFreq);
  EFM_ASSERT(init->frequency
             <= hfrcoCtrlTable[HFRCOCTRLTABLE_ENTRIES - 1U].maxFreq);
  EFM_ASSERT(init->n > 300U);
  EFM_ASSERT(init->n <= (_CMU_DPLLCTRL1_N_MASK >> _CMU_DPLLCTRL1_N_SHIFT));
  EFM_ASSERT(init->m <= (_CMU_DPLLCTRL1_M_MASK >> _CMU_DPLLCTRL1_M_SHIFT));
  EFM_ASSERT(init->ssInterval  <= (_CMU_HFRCOSS_SSINV_MASK
                                   >> _CMU_HFRCOSS_SSINV_SHIFT));
  EFM_ASSERT(init->ssAmplitude <= (_CMU_HFRCOSS_SSAMP_MASK
                                   >> _CMU_HFRCOSS_SSAMP_SHIFT));

#if defined(_EMU_STATUS_VSCALE_MASK)
  if ((EMU_VScaleGet() == emuVScaleEM01_LowPower)
      && (init->frequency > CMU_VSCALEEM01_LOWPOWER_VOLTAGE_CLOCK_MAX)) {
    EFM_ASSERT(false);
    return false;
  }
#endif

  // Find the correct HFRCO band and retrieve a HFRCOCTRL value.
  for (i = 0; i < HFRCOCTRLTABLE_ENTRIES; i++) {
    if ((init->frequency    >= hfrcoCtrlTable[i].minFreq)
        && (init->frequency <= hfrcoCtrlTable[i].maxFreq)) {
      index = (int)i;                       // Correct band found
      break;
    }
  }
  if ((uint32_t)index == HFRCOCTRLTABLE_ENTRIES) {
    EFM_ASSERT(false);
    return false;                           // Target frequency out of spec.
  }
  hfrcoCtrlVal = hfrcoCtrlTable[index].value;

  // Check if a calibrated HFRCOCTRL.TUNING value is in device DI page.
  if (hfrcoCtrlTable[index].band != (CMU_HFRCOFreq_TypeDef)0) {
    uint32_t tuning;

    tuning = (CMU_HFRCODevinfoGet(hfrcoCtrlTable[index].band)
              & _CMU_HFRCOCTRL_TUNING_MASK)
             >> _CMU_HFRCOCTRL_TUNING_SHIFT;

    // When HFRCOCTRL.FINETUNINGEN is enabled, the center frequency
    // of the band shifts down by 5.8%. 9 is subtracted to compensate.
    if (tuning > 9UL) {
      tuning -= 9UL;
    } else {
      tuning = 0UL;
    }

    hfrcoCtrlVal |= tuning << _CMU_HFRCOCTRL_TUNING_SHIFT;
  }

  // Update the CMSIS frequency SystemHfrcoFreq value.
  SystemHfrcoFreq = init->frequency;

  // Set maximum wait-states while changing the core clock.
  if (CMU_ClockSelectGet(cmuClock_HF) == cmuSelect_HFRCO) {
    flashWaitStateMax();
  }

  // Update the HFLE configuration before updating HFRCO, use new DPLL frequency.
  if (CMU_ClockSelectGet(cmuClock_HF) == cmuSelect_HFRCO) {
    setHfLeConfig(init->frequency);

    // Switch to HFRCO/2 before setting DPLL to avoid over-clocking.
    hfrcoDiv2override = true;
    CMU->HFCLKSEL = CMU_HFCLKSEL_HF_HFRCODIV2;
  }

  CMU->OSCENCMD  = CMU_OSCENCMD_DPLLDIS;
  while ((CMU->STATUS & (CMU_STATUS_DPLLENS | CMU_STATUS_DPLLRDY)) != 0UL) {
  }
  CMU->IFC       = CMU_IFC_DPLLRDY | CMU_IFC_DPLLLOCKFAILLOW
                   | CMU_IFC_DPLLLOCKFAILHIGH;
  CMU->DPLLCTRL1 = ((uint32_t)init->n   << _CMU_DPLLCTRL1_N_SHIFT)
                   | ((uint32_t)init->m << _CMU_DPLLCTRL1_M_SHIFT);
  CMU->HFRCOCTRL = hfrcoCtrlVal;
  CMU->DPLLCTRL  = ((uint32_t)init->refClk << _CMU_DPLLCTRL_REFSEL_SHIFT)
                   | ((init->autoRecover ? 1UL : 0UL)
                      << _CMU_DPLLCTRL_AUTORECOVER_SHIFT)
                   | ((uint32_t)init->edgeSel << _CMU_DPLLCTRL_EDGESEL_SHIFT)
                   | ((uint32_t)init->lockMode << _CMU_DPLLCTRL_MODE_SHIFT);
  CMU->OSCENCMD  = CMU_OSCENCMD_DPLLEN;
  while ((lockStatus = (CMU->IF & (CMU_IF_DPLLRDY
                                   | CMU_IF_DPLLLOCKFAILLOW
                                   | CMU_IF_DPLLLOCKFAILHIGH))) == 0UL) {
  }

  // Restore to HFRCO
  if ((CMU_ClockSelectGet(cmuClock_HF) == cmuSelect_HFRCODIV2)
      && (hfrcoDiv2override == true)) {
    CMU->HFCLKSEL = CMU_HFCLKSEL_HF_HFRCO;
  }

  // If HFRCO is selected as an HF clock, optimize the flash access wait-state
  // configuration for this frequency and update the CMSIS core clock variable.
  if (CMU_ClockSelectGet(cmuClock_HF) == cmuSelect_HFRCO) {
    // Call @ref SystemCoreClockGet() to update the CMSIS core clock variable.
    sysFreq = SystemCoreClockGet();
    EFM_ASSERT(sysFreq <= init->frequency);
    EFM_ASSERT(sysFreq <= SystemHfrcoFreq);
    EFM_ASSERT(init->frequency == SystemHfrcoFreq);
    CMU_UpdateWaitStates(sysFreq, VSCALE_DEFAULT);
  }

  // Reduce HFLE frequency if possible.
  setHfLeConfig(SystemHFClockGet());

#if defined(_EMU_CMD_EM01VSCALE0_MASK)
  // Update voltage scaling.
  EMU_VScaleEM01ByClock(0, true);
#endif

  if (lockStatus == CMU_IF_DPLLRDY) {
    return true;
  }
  return false;
}
#endif // CMU_OSCENCMD_DPLLEN

/**************************************************************************//**
 * @brief
 *   CMU low frequency register synchronization freeze control.
 *
 * @details
 *   Some CMU registers require synchronization into the low-frequency (LF)
 *   domain. The freeze feature allows for several such registers to be
 *   modified before passing them to the LF domain simultaneously (which
 *   takes place when the freeze mode is disabled).
 *
 *   Another use case for this feature is using an API (such
 *   as the CMU API) for modifying several bit fields consecutively in the
 *   same register. If freeze mode is enabled during this sequence, stalling
 *   can be avoided.
 *
 * @note
 *   When enabling freeze mode, this function will wait for all current
 *   ongoing CMU synchronization to LF domain to complete (normally
 *   synchronization will not be in progress.) However, for this reason, when
 *   using freeze mode, modifications of registers requiring LF synchronization
 *   should be done within one freeze enable/disable block to avoid unnecessary
 *   stalling.
 *
 * @param[in] enable
 *   @li true - enable freeze, modified registers are not propagated to the
 *       LF domain
 *   @li false - disable freeze, modified registers are propagated to the LF
 *       domain
 *****************************************************************************/
void CMU_FreezeEnable(bool enable)
{
  if (enable) {
    /* Wait for any ongoing LF synchronizations to complete. This */
    /* protects against the rare case when a user                            */
    /* - modifies a register requiring LF sync                              */
    /* - then enables freeze before LF sync completed                       */
    /* - then modifies the same register again                              */
    /* since modifying a register while it is in sync progress should be    */
    /* avoided.                                                             */
    while (CMU->SYNCBUSY != 0UL) {
    }

    CMU->FREEZE = CMU_FREEZE_REGFREEZE;
  } else {
    CMU->FREEZE = 0;
  }
}

#if defined(_CMU_HFRCOCTRL_BAND_MASK)
/***************************************************************************//**
 * @brief
 *   Get HFRCO band in use.
 *
 * @return
 *   HFRCO band in use.
 ******************************************************************************/
CMU_HFRCOBand_TypeDef CMU_HFRCOBandGet(void)
{
  return (CMU_HFRCOBand_TypeDef)((CMU->HFRCOCTRL & _CMU_HFRCOCTRL_BAND_MASK)
                                 >> _CMU_HFRCOCTRL_BAND_SHIFT);
}
#endif /* _CMU_HFRCOCTRL_BAND_MASK */

#if defined(_CMU_HFRCOCTRL_BAND_MASK)
/***************************************************************************//**
 * @brief
 *   Set HFRCO band and the tuning value based on the value in the calibration
 *   table made during production.
 *
 * @note
 *   HFCLKLE prescaler is automatically modified based on the maximum
 *   HFLE frequency allowed.
 *
 * @param[in] band
 *   HFRCO band to activate.
 ******************************************************************************/
void CMU_HFRCOBandSet(CMU_HFRCOBand_TypeDef band)
{
  uint32_t           tuning;
  uint32_t           freq;
  CMU_Select_TypeDef osc;

  /* Read the tuning value from the calibration table. */
  switch (band) {
    case cmuHFRCOBand_1MHz:
      tuning = (DEVINFO->HFRCOCAL0 & _DEVINFO_HFRCOCAL0_BAND1_MASK)
               >> _DEVINFO_HFRCOCAL0_BAND1_SHIFT;
      break;

    case cmuHFRCOBand_7MHz:
      tuning = (DEVINFO->HFRCOCAL0 & _DEVINFO_HFRCOCAL0_BAND7_MASK)
               >> _DEVINFO_HFRCOCAL0_BAND7_SHIFT;
      break;

    case cmuHFRCOBand_11MHz:
      tuning = (DEVINFO->HFRCOCAL0 & _DEVINFO_HFRCOCAL0_BAND11_MASK)
               >> _DEVINFO_HFRCOCAL0_BAND11_SHIFT;
      break;

    case cmuHFRCOBand_14MHz:
      tuning = (DEVINFO->HFRCOCAL0 & _DEVINFO_HFRCOCAL0_BAND14_MASK)
               >> _DEVINFO_HFRCOCAL0_BAND14_SHIFT;
      break;

    case cmuHFRCOBand_21MHz:
      tuning = (DEVINFO->HFRCOCAL1 & _DEVINFO_HFRCOCAL1_BAND21_MASK)
               >> _DEVINFO_HFRCOCAL1_BAND21_SHIFT;
      break;

#if defined(_CMU_HFRCOCTRL_BAND_28MHZ)
    case cmuHFRCOBand_28MHz:
      tuning = (DEVINFO->HFRCOCAL1 & _DEVINFO_HFRCOCAL1_BAND28_MASK)
               >> _DEVINFO_HFRCOCAL1_BAND28_SHIFT;
      break;
#endif

    default:
      EFM_ASSERT(false);
      return;
  }

  /* If HFRCO is used for the core clock, flash access WS has to be considered. */
  osc = CMU_ClockSelectGet(cmuClock_HF);
  if (osc == cmuSelect_HFRCO) {
    /* Configure worst case wait states for flash access before setting the divider. */
    flashWaitStateMax();
  }

  /* Set band/tuning. */
  CMU->HFRCOCTRL = (CMU->HFRCOCTRL
                    & ~(_CMU_HFRCOCTRL_BAND_MASK | _CMU_HFRCOCTRL_TUNING_MASK))
                   | (band << _CMU_HFRCOCTRL_BAND_SHIFT)
                   | (tuning << _CMU_HFRCOCTRL_TUNING_SHIFT);

  /* If HFRCO is used for the core clock, optimize flash WS. */
  if (osc == cmuSelect_HFRCO) {
    /* Call @ref SystemCoreClockGet() to update the CMSIS core clock variable. */
    freq = SystemCoreClockGet();
    CMU_UpdateWaitStates(freq, VSCALE_DEFAULT);
  }

#if defined(CMU_MAX_FREQ_HFLE)
  /* Reduce HFLE frequency if possible. */
  setHfLeConfig(SystemCoreClockGet());
#endif
}
#endif /* _CMU_HFRCOCTRL_BAND_MASK */

#if defined(_CMU_HFRCOCTRL_FREQRANGE_MASK)
/**************************************************************************//**
 * @brief
 *   Get the HFRCO frequency calibration word in DEVINFO.
 *
 * @param[in] freq
 *   Frequency in Hz.
 *
 * @return
 *   HFRCO calibration word for a given frequency.
 *****************************************************************************/
static uint32_t CMU_HFRCODevinfoGet(CMU_HFRCOFreq_TypeDef freq)
{
  switch (freq) {
    /* 1, 2, and 4 MHz share the same calibration word. */
    case cmuHFRCOFreq_1M0Hz:
    case cmuHFRCOFreq_2M0Hz:
    case cmuHFRCOFreq_4M0Hz:
      return DEVINFO->HFRCOCAL0;

    case cmuHFRCOFreq_7M0Hz:
      return DEVINFO->HFRCOCAL3;

    case cmuHFRCOFreq_13M0Hz:
      return DEVINFO->HFRCOCAL6;

    case cmuHFRCOFreq_16M0Hz:
      return DEVINFO->HFRCOCAL7;

    case cmuHFRCOFreq_19M0Hz:
      return DEVINFO->HFRCOCAL8;

    case cmuHFRCOFreq_26M0Hz:
      return DEVINFO->HFRCOCAL10;

    case cmuHFRCOFreq_32M0Hz:
      return DEVINFO->HFRCOCAL11;

    case cmuHFRCOFreq_38M0Hz:
      return DEVINFO->HFRCOCAL12;

#if defined(_DEVINFO_HFRCOCAL13_MASK)
    case cmuHFRCOFreq_48M0Hz:
      return DEVINFO->HFRCOCAL13;
#endif

#if defined(_DEVINFO_HFRCOCAL14_MASK)
    case cmuHFRCOFreq_56M0Hz:
      return DEVINFO->HFRCOCAL14;
#endif

#if defined(_DEVINFO_HFRCOCAL15_MASK)
    case cmuHFRCOFreq_64M0Hz:
      return DEVINFO->HFRCOCAL15;
#endif

#if defined(_DEVINFO_HFRCOCAL16_MASK)
    case cmuHFRCOFreq_72M0Hz:
      return DEVINFO->HFRCOCAL16;
#endif

    default: /* cmuHFRCOFreq_UserDefined */
      return 0;
  }
}

/***************************************************************************//**
 * @brief
 *   Get the current HFRCO frequency.
 *
 * @return
 *   HFRCO frequency.
 ******************************************************************************/
CMU_HFRCOFreq_TypeDef CMU_HFRCOBandGet(void)
{
  return (CMU_HFRCOFreq_TypeDef)SystemHfrcoFreq;
}

/***************************************************************************//**
 * @brief
 *   Set the HFRCO calibration for the selected target frequency.
 *
 * @note
 *   HFCLKLE prescaler is automatically modified based on the maximum
 *   HFLE frequency allowed.
 *
 * @param[in] setFreq
 *   HFRCO frequency to set.
 ******************************************************************************/
void CMU_HFRCOBandSet(CMU_HFRCOFreq_TypeDef setFreq)
{
  uint32_t freqCal;
  uint32_t sysFreq;
  uint32_t prevFreq;

  /* Get the DEVINFO index and set the CMSIS frequency SystemHfrcoFreq. */
  freqCal = CMU_HFRCODevinfoGet(setFreq);
  EFM_ASSERT((freqCal != 0UL) && (freqCal != UINT_MAX));
  prevFreq = SystemHfrcoFreq;
  SystemHfrcoFreq = (uint32_t)setFreq;

  /* Set maximum wait-states and set safe HFPER clock-tree prescalers while
     changing the core clock. */
  if (CMU_ClockSelectGet(cmuClock_HF) == cmuSelect_HFRCO) {
    flashWaitStateMax();
    hfperClkSafePrescaler();
  }

  /* Wait for any previous sync to complete and set calibration data
     for the selected frequency.  */
  while (BUS_RegBitRead(&CMU->SYNCBUSY, _CMU_SYNCBUSY_HFRCOBSY_SHIFT) != 0UL) {
  }

  /* Check for valid calibration data. */
  EFM_ASSERT(freqCal != UINT_MAX);

  /* Set divider in HFRCOCTRL for 1, 2, and 4 MHz. */
  switch (setFreq) {
    case cmuHFRCOFreq_1M0Hz:
      freqCal = (freqCal & ~_CMU_HFRCOCTRL_CLKDIV_MASK)
                | CMU_HFRCOCTRL_CLKDIV_DIV4;
      break;

    case cmuHFRCOFreq_2M0Hz:
      freqCal = (freqCal & ~_CMU_HFRCOCTRL_CLKDIV_MASK)
                | CMU_HFRCOCTRL_CLKDIV_DIV2;
      break;

    case cmuHFRCOFreq_4M0Hz:
      freqCal = (freqCal & ~_CMU_HFRCOCTRL_CLKDIV_MASK)
                | CMU_HFRCOCTRL_CLKDIV_DIV1;
      break;

    default:
      break;
  }

  /* Update HFLE configuration before updating HFRCO.
     Use the new set frequency. */
  if (CMU_ClockSelectGet(cmuClock_HF) == cmuSelect_HFRCO) {
    /* setFreq is worst-case as dividers may reduce the HFLE frequency. */
    setHfLeConfig((uint32_t)setFreq);
  }

  if ((uint32_t)setFreq > prevFreq) {
#if defined(_EMU_CMD_EM01VSCALE0_MASK)
    /* When increasing frequency voltage scale must be done before the change. */
    EMU_VScaleEM01ByClock((uint32_t)setFreq, true);
#endif
  }

  CMU->HFRCOCTRL = freqCal;

  /* If HFRCO is selected as an HF clock, optimize the flash access wait-state configuration
     for this frequency and update the CMSIS core clock variable. */
  if (CMU_ClockSelectGet(cmuClock_HF) == cmuSelect_HFRCO) {
    /* Call @ref SystemCoreClockGet() to update the CMSIS core clock variable. */
    sysFreq = SystemCoreClockGet();
    EFM_ASSERT(sysFreq <= (uint32_t)setFreq);
    EFM_ASSERT(sysFreq <= SystemHfrcoFreq);
    EFM_ASSERT((uint32_t)setFreq == SystemHfrcoFreq);
    CMU_UpdateWaitStates(sysFreq, VSCALE_DEFAULT);
  }

  /* Reduce HFLE frequency if possible. */
  setHfLeConfig(SystemHFClockGet());

  if ((uint32_t)setFreq <= prevFreq) {
#if defined(_EMU_CMD_EM01VSCALE0_MASK)
    /* When decreasing frequency voltage scale must be done after the change */
    EMU_VScaleEM01ByClock(0, true);
#endif
  }
  if (CMU_ClockSelectGet(cmuClock_HF) == cmuSelect_HFRCO) {
    /* Set optimized HFPER clock-tree prescalers. */
    hfperClkOptimizedPrescaler();
  }
}
#endif /* _CMU_HFRCOCTRL_FREQRANGE_MASK */

#if defined(_CMU_HFRCOCTRL_SUDELAY_MASK)
/***************************************************************************//**
 * @brief
 *   Get the HFRCO startup delay.
 *
 * @details
 *   See the reference manual for more details.
 *
 * @return
 *   The startup delay in use.
 ******************************************************************************/
uint32_t CMU_HFRCOStartupDelayGet(void)
{
  return (CMU->HFRCOCTRL & _CMU_HFRCOCTRL_SUDELAY_MASK)
         >> _CMU_HFRCOCTRL_SUDELAY_SHIFT;
}

/***************************************************************************//**
 * @brief
 *   Set the HFRCO startup delay.
 *
 * @details
 *   See the reference manual for more details.
 *
 * @param[in] delay
 *   The startup delay to set (<= 31).
 ******************************************************************************/
void CMU_HFRCOStartupDelaySet(uint32_t delay)
{
  EFM_ASSERT(delay <= 31);

  delay &= _CMU_HFRCOCTRL_SUDELAY_MASK >> _CMU_HFRCOCTRL_SUDELAY_SHIFT;
  CMU->HFRCOCTRL = (CMU->HFRCOCTRL & ~(_CMU_HFRCOCTRL_SUDELAY_MASK))
                   | (delay << _CMU_HFRCOCTRL_SUDELAY_SHIFT);
}
#endif

#if defined(_CMU_USHFRCOCTRL_FREQRANGE_MASK)
/**************************************************************************//**
 * @brief
 *   Get the USHFRCO frequency calibration word in DEVINFO.
 *
 * @param[in] freq
 *   Frequency in Hz.
 *
 * @return
 *   USHFRCO calibration word for a given frequency.
 *****************************************************************************/
static uint32_t CMU_USHFRCODevinfoGet(CMU_USHFRCOFreq_TypeDef freq)
{
  switch (freq) {
    case cmuUSHFRCOFreq_16M0Hz:
      return DEVINFO->USHFRCOCAL7;

    case cmuUSHFRCOFreq_32M0Hz:
      return DEVINFO->USHFRCOCAL11;

    case cmuUSHFRCOFreq_48M0Hz:
      return DEVINFO->USHFRCOCAL13;

    case cmuUSHFRCOFreq_50M0Hz:
      return DEVINFO->USHFRCOCAL14;

    default: /* cmuUSHFRCOFreq_UserDefined */
      return 0;
  }
}

/***************************************************************************//**
 * @brief
 *   Get the current USHFRCO frequency.
 *
 * @return
 *   HFRCO frequency.
 ******************************************************************************/
CMU_USHFRCOFreq_TypeDef CMU_USHFRCOBandGet(void)
{
  return (CMU_USHFRCOFreq_TypeDef) ushfrcoFreq;
}

/***************************************************************************//**
 * @brief
 *   Get USHFRCO frequency.
 *
 * @return
 *   USHFRCO frequency.
 ******************************************************************************/
uint32_t CMU_USHFRCOFreqGet(void)
{
  return ushfrcoFreq;
}

/***************************************************************************//**
 * @brief
 *   Set the USHFRCO calibration for the selected target frequency.
 *
 * @param[in] setFreq
 *   USHFRCO frequency to set.
 ******************************************************************************/
void CMU_USHFRCOBandSet(CMU_USHFRCOFreq_TypeDef setFreq)
{
  uint32_t freqCal;

  /* Get DEVINFO calibration values. */
  freqCal = CMU_USHFRCODevinfoGet(setFreq);
  EFM_ASSERT((freqCal != 0) && (freqCal != UINT_MAX));
  ushfrcoFreq = (uint32_t)setFreq;

  /* Wait for any previous sync to complete and set calibration data
     for the selected frequency.  */
  while (BUS_RegBitRead(&CMU->SYNCBUSY, _CMU_SYNCBUSY_USHFRCOBSY_SHIFT)) ;

  CMU->USHFRCOCTRL = freqCal;
}
#endif /* _CMU_USHFRCOCTRL_FREQRANGE_MASK  */

#if defined(_CMU_HFXOCTRL_AUTOSTARTEM0EM1_MASK)
/***************************************************************************//**
 * @brief
 *   Enable or disable HFXO autostart.
 *
 * @param[in] userSel
 *   Additional user specified enable bit.
 *
 * @param[in] enEM0EM1Start
 *   If true, HFXO is automatically started upon entering EM0/EM1 entry from
 *   EM2/EM3. HFXO selection has to be handled by the user.
 *   If false, HFXO is not started automatically when entering EM0/EM1.
 *
 * @param[in] enEM0EM1StartSel
 *   If true, HFXO is automatically started and immediately selected upon
 *   entering EM0/EM1 entry from EM2/EM3. Note that this option stalls the use of
 *   HFSRCCLK until HFXO becomes ready. HFCLKLE prescaler is also automatically
 *   modified if userSel is specified.
 *   If false, HFXO is not started or selected automatically when entering
 *   EM0/EM1.
 ******************************************************************************/
void CMU_HFXOAutostartEnable(uint32_t userSel,
                             bool enEM0EM1Start,
                             bool enEM0EM1StartSel)
{
  uint32_t hfxoFreq;
  uint32_t hfxoCtrl;

#if defined(_EMU_CTRL_EM23VSCALE_MASK)
  if (enEM0EM1StartSel) {
    /* Voltage scaling is not compatible with HFXO auto start and select. */
    EFM_ASSERT((EMU->CTRL & _EMU_CTRL_EM23VSCALE_MASK) == EMU_CTRL_EM23VSCALE_VSCALE2);
  }
#endif

  /* Mask supported enable bits. */
#if defined(_CMU_HFXOCTRL_AUTOSTARTRDYSELRAC_MASK)
  userSel &= _CMU_HFXOCTRL_AUTOSTARTRDYSELRAC_MASK;
#else
  userSel = 0;
#endif

  hfxoFreq = SystemHFXOClockGet();
#if defined(_EMU_CMD_EM01VSCALE0_MASK)
  // Update voltage scaling.
  EMU_VScaleEM01ByClock(hfxoFreq, true);
#endif
  /* Set wait-states for HFXO if automatic start and select is configured. */
  if ((userSel > 0UL) || enEM0EM1StartSel) {
    CMU_UpdateWaitStates(hfxoFreq, VSCALE_DEFAULT);
    setHfLeConfig(hfxoFreq / CMU_ClockDivGet(cmuClock_HF));
  }

  if (enEM0EM1Start || enEM0EM1StartSel) {
    /* Enable the HFXO once in order to finish first time calibrations. */
    CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
  }

  /* Since call to CMU_OscillatorEnable() can change the CMU->HFXOCTRL register,
   * it's important to read the CMU->HFXOCTRL register after the call to CMU_OscillatorEnable(). */
  hfxoCtrl = CMU->HFXOCTRL & ~(userSel
                               | _CMU_HFXOCTRL_AUTOSTARTEM0EM1_MASK
                               | _CMU_HFXOCTRL_AUTOSTARTSELEM0EM1_MASK);

  hfxoCtrl |= userSel
              | (enEM0EM1Start ? CMU_HFXOCTRL_AUTOSTARTEM0EM1 : 0UL)
              | (enEM0EM1StartSel ? CMU_HFXOCTRL_AUTOSTARTSELEM0EM1 : 0UL);

  /* Update HFXOCTRL after wait-states are updated as HF may automatically switch
     to HFXO when automatic select is enabled . */
  CMU->HFXOCTRL = hfxoCtrl;
}
#endif

/**************************************************************************//**
 * @brief
 *   Set HFXO control registers.
 *
 * @note
 *   HFXO configuration should be obtained from a configuration tool,
 *   app note, or crystal data sheet. This function disables the HFXO to
 *   ensure a valid state before update.
 *
 * @param[in] hfxoInit
 *    HFXO setup parameters.
 *****************************************************************************/
void CMU_HFXOInit(const CMU_HFXOInit_TypeDef *hfxoInit)
{
  /* Do not disable HFXO if it is currently selected as the HF/Core clock. */
  EFM_ASSERT(CMU_ClockSelectGet(cmuClock_HF) != cmuSelect_HFXO);

  /* HFXO must be disabled before reconfiguration. */
  CMU_OscillatorEnable(cmuOsc_HFXO, false, true);

#if defined(_SILICON_LABS_32B_SERIES_1) \
  && (_SILICON_LABS_GECKO_INTERNAL_SDID >= 100)
  uint32_t tmp = CMU_HFXOCTRL_MODE_XTAL;

  switch (hfxoInit->mode) {
    case cmuOscMode_Crystal:
      tmp = CMU_HFXOCTRL_MODE_XTAL;
      break;
    case cmuOscMode_External:
      tmp = CMU_HFXOCTRL_MODE_DIGEXTCLK;
      break;
    case cmuOscMode_AcCoupled:
      tmp = CMU_HFXOCTRL_MODE_ACBUFEXTCLK;
      break;
    default:
      EFM_ASSERT(false); /* Unsupported configuration */
      break;
  }
  CMU->HFXOCTRL = (CMU->HFXOCTRL & ~_CMU_HFXOCTRL_MODE_MASK) | tmp;

#if defined(CMU_HFXOCTRL_HFXOX2EN)
  /* HFXO Doubler can only be enabled on crystals up to max 25 MHz. */
  tmp = 0;
  if (SystemHFXOClockGet() <= 25000000) {
    tmp |= CMU_HFXOCTRL_HFXOX2EN;
  }

  CMU->HFXOCTRL = (CMU->HFXOCTRL & ~_CMU_HFXOCTRL_HFXOX2EN_MASK) | tmp;
#endif

  /* Set tuning for startup and steady state. */
  CMU->HFXOSTARTUPCTRL = (hfxoInit->ctuneStartup
                          << _CMU_HFXOSTARTUPCTRL_CTUNE_SHIFT)
                         | (hfxoInit->xoCoreBiasTrimStartup
                            << _CMU_HFXOSTARTUPCTRL_IBTRIMXOCORE_SHIFT);

  CMU->HFXOSTEADYSTATECTRL = (CMU->HFXOSTEADYSTATECTRL
                              & ~(_CMU_HFXOSTEADYSTATECTRL_CTUNE_MASK
                                  | _CMU_HFXOSTEADYSTATECTRL_IBTRIMXOCORE_MASK))
                             | (hfxoInit->ctuneSteadyState
                                << _CMU_HFXOSTEADYSTATECTRL_CTUNE_SHIFT)
                             | (hfxoInit->xoCoreBiasTrimSteadyState
                                << _CMU_HFXOSTEADYSTATECTRL_IBTRIMXOCORE_SHIFT);

  /* Set timeouts */
  CMU->HFXOTIMEOUTCTRL = (hfxoInit->timeoutPeakDetect
                          << _CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_SHIFT)
                         | (hfxoInit->timeoutSteady
                            << _CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_SHIFT)
                         | (hfxoInit->timeoutStartup
                            << _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_SHIFT);

#elif defined(_CMU_HFXOCTRL_MASK)
  /* Verify that the deprecated autostart fields are not used,
   * @ref CMU_HFXOAutostartEnable must be used instead. */
  EFM_ASSERT(!(hfxoInit->autoStartEm01
               || hfxoInit->autoSelEm01
               || hfxoInit->autoStartSelOnRacWakeup));

  uint32_t tmp = CMU_HFXOCTRL_MODE_XTAL;

  /* AC coupled external clock not supported. */
  EFM_ASSERT(hfxoInit->mode != cmuOscMode_AcCoupled);
  if (hfxoInit->mode == cmuOscMode_External) {
    tmp = CMU_HFXOCTRL_MODE_DIGEXTCLK;
  }

  /* Apply control settings. */
  CMU->HFXOCTRL = (CMU->HFXOCTRL & ~_CMU_HFXOCTRL_MODE_MASK)
                  | tmp;
  BUS_RegBitWrite(&CMU->HFXOCTRL,
                  _CMU_HFXOCTRL_LOWPOWER_SHIFT,
                  (unsigned)hfxoInit->lowPowerMode);

  /* Set XTAL tuning parameters. */

#if defined(_CMU_HFXOCTRL1_PEAKDETTHR_MASK)
  /* Set peak detection threshold. */
  CMU->HFXOCTRL1 = (CMU->HFXOCTRL1 & ~_CMU_HFXOCTRL1_PEAKDETTHR_MASK)
                   | (hfxoInit->thresholdPeakDetect
                      << _CMU_HFXOCTRL1_PEAKDETTHR_SHIFT);
#endif
  /* Set tuning for startup and steady state. */
  CMU->HFXOSTARTUPCTRL = ((uint32_t)hfxoInit->ctuneStartup
                          << _CMU_HFXOSTARTUPCTRL_CTUNE_SHIFT)
                         | ((uint32_t)hfxoInit->xoCoreBiasTrimStartup
                            << _CMU_HFXOSTARTUPCTRL_IBTRIMXOCORE_SHIFT);

  CMU->HFXOSTEADYSTATECTRL = (CMU->HFXOSTEADYSTATECTRL
                              & ~(_CMU_HFXOSTEADYSTATECTRL_CTUNE_MASK
                                  | _CMU_HFXOSTEADYSTATECTRL_IBTRIMXOCORE_MASK
                                  | _CMU_HFXOSTEADYSTATECTRL_REGISH_MASK
                                  | _CMU_HFXOSTEADYSTATECTRL_REGISHUPPER_MASK))
                             | ((uint32_t)hfxoInit->ctuneSteadyState
                                << _CMU_HFXOSTEADYSTATECTRL_CTUNE_SHIFT)
                             | ((uint32_t)hfxoInit->xoCoreBiasTrimSteadyState
                                << _CMU_HFXOSTEADYSTATECTRL_IBTRIMXOCORE_SHIFT)
                             | ((uint32_t)hfxoInit->regIshSteadyState
                                << _CMU_HFXOSTEADYSTATECTRL_REGISH_SHIFT)
                             | getRegIshUpperVal(hfxoInit->regIshSteadyState);

  /* Set timeouts. */
  CMU->HFXOTIMEOUTCTRL = ((uint32_t)hfxoInit->timeoutPeakDetect
                          << _CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_SHIFT)
                         | ((uint32_t)hfxoInit->timeoutSteady
                            << _CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_SHIFT)
                         | ((uint32_t)hfxoInit->timeoutStartup
                            << _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_SHIFT)
                         | ((uint32_t)hfxoInit->timeoutShuntOptimization
                            << _CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_SHIFT);

#else
  CMU->CTRL = (CMU->CTRL & ~(_CMU_CTRL_HFXOTIMEOUT_MASK
                             | _CMU_CTRL_HFXOBOOST_MASK
                             | _CMU_CTRL_HFXOMODE_MASK
                             | _CMU_CTRL_HFXOGLITCHDETEN_MASK))
              | (hfxoInit->timeout << _CMU_CTRL_HFXOTIMEOUT_SHIFT)
              | (hfxoInit->boost << _CMU_CTRL_HFXOBOOST_SHIFT)
              | (hfxoInit->mode << _CMU_CTRL_HFXOMODE_SHIFT)
              | (hfxoInit->glitchDetector ? CMU_CTRL_HFXOGLITCHDETEN : 0);
#endif
}

/***************************************************************************//**
 * @brief
 *   Get the LCD framerate divisor (FDIV) setting.
 *
 * @return
 *   The LCD framerate divisor.
 ******************************************************************************/
uint32_t CMU_LCDClkFDIVGet(void)
{
#if defined(LCD_PRESENT) && defined(_CMU_LCDCTRL_MASK)
  return (CMU->LCDCTRL & _CMU_LCDCTRL_FDIV_MASK) >> _CMU_LCDCTRL_FDIV_SHIFT;
#else
  return 0;
#endif /* defined(LCD_PRESENT) */
}

/***************************************************************************//**
 * @brief
 *   Set the LCD framerate divisor (FDIV) setting.
 *
 * @note
 *   The FDIV field (CMU LCDCTRL register) should only be modified while the
 *   LCD module is clock disabled (CMU LFACLKEN0.LCD bit is 0). This function
 *   will NOT modify FDIV if the LCD module clock is enabled. See
 *   @ref CMU_ClockEnable() for disabling/enabling LCD clock.
 *
 * @param[in] div
 *   The FDIV setting to use.
 ******************************************************************************/
void CMU_LCDClkFDIVSet(uint32_t div)
{
#if defined(LCD_PRESENT) && defined(_CMU_LCDCTRL_MASK)
  EFM_ASSERT(div <= cmuClkDiv_128);

  /* Do not allow modification if LCD clock enabled. */
  if (CMU->LFACLKEN0 & CMU_LFACLKEN0_LCD) {
    return;
  }

  div        <<= _CMU_LCDCTRL_FDIV_SHIFT;
  div         &= _CMU_LCDCTRL_FDIV_MASK;
  CMU->LCDCTRL = (CMU->LCDCTRL & ~_CMU_LCDCTRL_FDIV_MASK) | div;
#else
  (void)div;  /* Unused parameter. */
#endif /* defined(LCD_PRESENT) */
}

/**************************************************************************//**
 * @brief
 *   Set LFXO control registers.
 *
 * @note
 *   LFXO configuration should be obtained from a configuration tool,
 *   app note, or crystal data sheet. This function disables the LFXO when
 *   necessary to ensure a valid state before update.
 *
 * @param[in] lfxoInit
 *    LFXO setup parameters.
 *****************************************************************************/
void CMU_LFXOInit(const CMU_LFXOInit_TypeDef *lfxoInit)
{
  /* Do not disable LFXO if it is currently selected as the HF/Core clock. */
  EFM_ASSERT(CMU_ClockSelectGet(cmuClock_HF) != cmuSelect_LFXO);

#if defined(_SILICON_LABS_32B_SERIES_1)
  uint32_t reg = (CMU->LFXOCTRL & ~(_CMU_LFXOCTRL_TUNING_MASK
                                    | _CMU_LFXOCTRL_GAIN_MASK
                                    | _CMU_LFXOCTRL_TIMEOUT_MASK
                                    | _CMU_LFXOCTRL_MODE_MASK))
                 | ((uint32_t)lfxoInit->ctune << _CMU_LFXOCTRL_TUNING_SHIFT)
                 | ((uint32_t)lfxoInit->gain << _CMU_LFXOCTRL_GAIN_SHIFT)
                 | ((uint32_t)lfxoInit->timeout << _CMU_LFXOCTRL_TIMEOUT_SHIFT)
                 | ((uint32_t)lfxoInit->mode << _CMU_LFXOCTRL_MODE_SHIFT);

  /* If LFXO already contains the correct configuration then there is no need
   * to stop the oscillator and apply new settings. The LFXO can be running out
   * of reset, in which case there is no need to disable it unless necessary. */
  if (reg != CMU->LFXOCTRL) {
    CMU_OscillatorEnable(cmuOsc_LFXO, false, true);
    CMU->LFXOCTRL = reg;
  }
#elif defined(_SILICON_LABS_32B_SERIES_0)
  /* LFXO must be disabled before reconfiguration. */
  CMU_OscillatorEnable(cmuOsc_LFXO, false, true);

  bool cmuBoost  = (lfxoInit->boost & 0x2);
  BUS_RegMaskedWrite(&CMU->CTRL,
                     _CMU_CTRL_LFXOTIMEOUT_MASK
                     | _CMU_CTRL_LFXOBOOST_MASK
                     | _CMU_CTRL_LFXOMODE_MASK,
                     ((uint32_t)lfxoInit->timeout
                      << _CMU_CTRL_LFXOTIMEOUT_SHIFT)
                     | ((cmuBoost ? 1 : 0) << _CMU_CTRL_LFXOBOOST_SHIFT)
                     | ((uint32_t)lfxoInit->mode << _CMU_CTRL_LFXOMODE_SHIFT));
#if defined(_EMU_AUXCTRL_REDLFXOBOOST_MASK)
  /* EFM32GG has a "reduce startup boost" field in the EMU */
  bool emuReduce = (lfxoInit->boost & 0x1);
  BUS_RegBitWrite(&EMU->AUXCTRL, _EMU_AUXCTRL_REDLFXOBOOST_SHIFT, emuReduce ? 1 : 0);
#endif
#endif
}

/**************************************************************************//**
 * @brief
 *   Sets LFXO's crystal precision, in PPM.
 *
 * @note
 *   LFXO precision should be obtained from a crystal datasheet.
 *
 * @param[in] precision
 *    LFXO's crystal precision, in PPM.
 *****************************************************************************/
void CMU_LFXOPrecisionSet(uint16_t precision)
{
  lfxo_precision = precision;
}

/***************************************************************************//**
 * @brief
 *   Enable/disable oscillator.
 *
 * @note
 *   WARNING: When this function is called to disable either cmuOsc_LFXO or
 *   cmuOsc_HFXO, the LFXOMODE or HFXOMODE fields of the CMU_CTRL register
 *   are reset to the reset value. In other words, if external clock sources are selected
 *   in either LFXOMODE or HFXOMODE fields, the configuration will be cleared
 *   and needs to be reconfigured if needed later.
 *
 * @param[in] osc
 *   The oscillator to enable/disable.
 *
 * @param[in] enable
 *   @li true - enable specified oscillator.
 *   @li false - disable specified oscillator.
 *
 * @param[in] wait
 *   Only used if @p enable is true.
 *   @li true - wait for oscillator start-up time to timeout before returning.
 *   @li false - do not wait for oscillator start-up time to timeout before
 *     returning.
 ******************************************************************************/
void CMU_OscillatorEnable(CMU_Osc_TypeDef osc, bool enable, bool wait)
{
  uint32_t rdyBitPos;
#if defined(_SILICON_LABS_32B_SERIES_1)
  uint32_t ensBitPos;
#endif
#if defined(_CMU_STATUS_HFXOPEAKDETRDY_MASK)
  uint32_t hfxoTrimStatus;
#endif

  uint32_t enBit;
  uint32_t disBit;

  switch (osc) {
    case cmuOsc_HFRCO:
      enBit  = CMU_OSCENCMD_HFRCOEN;
      disBit = CMU_OSCENCMD_HFRCODIS;
      rdyBitPos = _CMU_STATUS_HFRCORDY_SHIFT;
#if defined(_SILICON_LABS_32B_SERIES_1)
      ensBitPos = _CMU_STATUS_HFRCOENS_SHIFT;
#endif
      break;

    case cmuOsc_HFXO:
      enBit  = CMU_OSCENCMD_HFXOEN;
      disBit = CMU_OSCENCMD_HFXODIS;
      rdyBitPos = _CMU_STATUS_HFXORDY_SHIFT;
#if defined(_SILICON_LABS_32B_SERIES_1)
      ensBitPos = _CMU_STATUS_HFXOENS_SHIFT;
#endif
      break;

    case cmuOsc_AUXHFRCO:
      enBit  = CMU_OSCENCMD_AUXHFRCOEN;
      disBit = CMU_OSCENCMD_AUXHFRCODIS;
      rdyBitPos = _CMU_STATUS_AUXHFRCORDY_SHIFT;
#if defined(_SILICON_LABS_32B_SERIES_1)
      ensBitPos = _CMU_STATUS_AUXHFRCOENS_SHIFT;
#endif
      break;

    case cmuOsc_LFRCO:
      enBit  = CMU_OSCENCMD_LFRCOEN;
      disBit = CMU_OSCENCMD_LFRCODIS;
      rdyBitPos = _CMU_STATUS_LFRCORDY_SHIFT;
#if defined(_SILICON_LABS_32B_SERIES_1)
      ensBitPos = _CMU_STATUS_LFRCOENS_SHIFT;
#endif
      break;

    case cmuOsc_LFXO:
      enBit  = CMU_OSCENCMD_LFXOEN;
      disBit = CMU_OSCENCMD_LFXODIS;
      rdyBitPos = _CMU_STATUS_LFXORDY_SHIFT;
#if defined(_SILICON_LABS_32B_SERIES_1)
      ensBitPos = _CMU_STATUS_LFXOENS_SHIFT;
#endif
      break;

#if defined(_CMU_STATUS_USHFRCOENS_MASK)
    case cmuOsc_USHFRCO:
      enBit  = CMU_OSCENCMD_USHFRCOEN;
      disBit = CMU_OSCENCMD_USHFRCODIS;
      rdyBitPos = _CMU_STATUS_USHFRCORDY_SHIFT;
#if defined(_SILICON_LABS_32B_SERIES_1)
      ensBitPos = _CMU_STATUS_USHFRCOENS_SHIFT;
#endif
      break;
#endif

#if defined(PLFRCO_PRESENT)
    case cmuOsc_PLFRCO:
      if (!deviceHasPlfrco()) {
        while (true) { // PLFRCO is not available
          EFM_ASSERT(false);
        }
      }
      enBit = CMU_OSCENCMD_PLFRCOEN;
      disBit = CMU_OSCENCMD_PLFRCODIS;
      rdyBitPos = _CMU_STATUS_PLFRCORDY_SHIFT;
      ensBitPos = _CMU_STATUS_PLFRCOENS_SHIFT;
      break;
#endif

    default:
      /* Undefined clock source, cmuOsc_CLKIN0 or cmuOsc_ULFRCO. ULFRCO is always enabled
         and cannot be disabled. In other words,the definition of cmuOsc_ULFRCO is primarily
         intended for information: the ULFRCO is always on.  */
      EFM_ASSERT(false);
      return;
  }

  if (enable) {
 #if defined(_CMU_HFXOCTRL_MASK)
    bool firstHfxoEnable = false;

    /* Enabling the HFXO for the first time requires special handling.
     * PEAKDETSHUTOPTMODE field of the HFXOCTRL register is used to see if this is the
     * first time the HFXO is enabled. */
    if (osc == cmuOsc_HFXO) {
      if (getHfxoTuningMode() == HFXO_TUNING_MODE_AUTO) {
        /* REGPWRSEL must be set to DVDD before the HFXO can be enabled. */
#if defined(_EMU_PWRCTRL_REGPWRSEL_MASK)
        EFM_ASSERT((EMU->PWRCTRL & EMU_PWRCTRL_REGPWRSEL_DVDD) != 0UL);
#endif

        firstHfxoEnable = true;
        /* The first time that an external clock is enabled, switch to CMD mode to make sure that
         * only SCO and not PDA tuning is performed. */
        if ((CMU->HFXOCTRL & (_CMU_HFXOCTRL_MODE_MASK)) == CMU_HFXOCTRL_MODE_DIGEXTCLK) {
          setHfxoTuningMode(HFXO_TUNING_MODE_CMD);
        }
      }
    }
#endif
    CMU->OSCENCMD = enBit;

#if defined(_SILICON_LABS_32B_SERIES_1)
    /* Always wait for ENS to go high. */
    while (BUS_RegBitRead(&CMU->STATUS, ensBitPos) == 0UL) {
    }
#endif

    /* Wait for the clock to become ready after enable. */
    if (wait) {
      while (BUS_RegBitRead(&CMU->STATUS, rdyBitPos) == 0UL) {
      }
#if defined(_SILICON_LABS_32B_SERIES_1)
      if ((osc == cmuOsc_HFXO) && firstHfxoEnable) {
        if ((CMU->HFXOCTRL & _CMU_HFXOCTRL_MODE_MASK)
            == CMU_HFXOCTRL_MODE_DIGEXTCLK) {
#if defined(CMU_CMD_HFXOSHUNTOPTSTART)
          /* External clock mode should only do shunt current optimization. */
          (void)CMU_OscillatorTuningOptimize(cmuOsc_HFXO,
                                             cmuHFXOTuningMode_ShuntCommand,
                                             true);
#endif
        } else {
          /* Wait for the peak detection and shunt current optimization
             to complete. */
          (void)CMU_OscillatorTuningWait(cmuOsc_HFXO, cmuHFXOTuningMode_Auto);
        }

        /* Disable the HFXO again to apply the trims. Apply trim from
           HFXOTRIMSTATUS when disabled. */
        hfxoTrimStatus = CMU_OscillatorTuningGet(cmuOsc_HFXO);
        CMU_OscillatorEnable(cmuOsc_HFXO, false, true);
        CMU_OscillatorTuningSet(cmuOsc_HFXO, hfxoTrimStatus);

        /* Restart in CMD mode. */
        CMU->OSCENCMD = enBit;
        while (BUS_RegBitRead(&CMU->STATUS, rdyBitPos) == 0UL) {
        }
      }
#endif
    }
  } else {
    CMU->OSCENCMD = disBit;

#if defined(_SILICON_LABS_32B_SERIES_1)
    /* Always wait for ENS to go low. */
    while ((CMU->STATUS & (0x1 << ensBitPos)) != 0U) {
    }

    if (wait) {
      /* Wait for RDY to go low as well. */
      while ((CMU->STATUS & (0x1 << rdyBitPos)) != 0U) {
      }
    }
#endif
  }
}

/***************************************************************************//**
 * @brief
 *   Get the oscillator frequency tuning setting.
 *
 * @param[in] osc
 *   An oscillator to get tuning value for, one of the following:
 *   @li #cmuOsc_LFRCO
 *   @li #cmuOsc_HFRCO @if _CMU_USHFRCOCTRL_TUNING_MASK
 *   @li #cmuOsc_USHFRCO
 *   @endif
 *   @li #cmuOsc_AUXHFRCO
 *   @li #cmuOsc_HFXO if CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE is defined
 *
 * @return
 *   The oscillator frequency tuning setting in use.
 ******************************************************************************/
uint32_t CMU_OscillatorTuningGet(CMU_Osc_TypeDef osc)
{
  uint32_t ret;

  switch (osc) {
    case cmuOsc_LFRCO:
      ret = (CMU->LFRCOCTRL & _CMU_LFRCOCTRL_TUNING_MASK)
            >> _CMU_LFRCOCTRL_TUNING_SHIFT;
      break;

    case cmuOsc_HFRCO:
      ret = (CMU->HFRCOCTRL & _CMU_HFRCOCTRL_TUNING_MASK)
            >> _CMU_HFRCOCTRL_TUNING_SHIFT;
      break;

#if defined (_CMU_USHFRCOCTRL_TUNING_MASK)
    case cmuOsc_USHFRCO:
      ret = (CMU->USHFRCOCTRL & _CMU_USHFRCOCTRL_TUNING_MASK)
            >> _CMU_USHFRCOCTRL_TUNING_SHIFT;
      break;
#endif

    case cmuOsc_AUXHFRCO:
      ret = (CMU->AUXHFRCOCTRL & _CMU_AUXHFRCOCTRL_TUNING_MASK)
            >> _CMU_AUXHFRCOCTRL_TUNING_SHIFT;
      break;

#if defined(_CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_MASK)
    case cmuOsc_HFXO:
      ret = CMU->HFXOTRIMSTATUS & (_CMU_HFXOTRIMSTATUS_IBTRIMXOCORE_MASK
#if defined(_CMU_HFXOTRIMSTATUS_REGISH_MASK)
                                   | _CMU_HFXOTRIMSTATUS_REGISH_MASK
#endif
                                   );
      break;
#endif

    default:
      ret = 0;
      EFM_ASSERT(false);
      break;
  }

  return ret;
}

/***************************************************************************//**
 * @brief
 *   Set the oscillator frequency tuning control.
 *
 * @note
 *   Oscillator tuning is done during production and the tuning value is
 *   automatically loaded after reset. Changing the tuning value from the
 *   calibrated value is for more advanced use. Certain oscillators also have
 *   build-in tuning optimization.
 *
 * @param[in] osc
 *   An oscillator to set tuning value for, one of the following:
 *   @li #cmuOsc_LFRCO
 *   @li #cmuOsc_HFRCO @if _CMU_USHFRCOCTRL_TUNING_MASK
 *   @li #cmuOsc_USHFRCO
 *   @endif
 *   @li #cmuOsc_AUXHFRCO
 *   @li #cmuOsc_HFXO if PEAKDETSHUNTOPTMODE is available. Note that CMD mode is set.
 *
 * @param[in] val
 *   The oscillator frequency tuning setting to use.
 ******************************************************************************/
void CMU_OscillatorTuningSet(CMU_Osc_TypeDef osc, uint32_t val)
{
#if defined(_CMU_HFXOSTEADYSTATECTRL_REGISH_MASK)
  uint32_t regIshUpper;
#endif

  switch (osc) {
    case cmuOsc_LFRCO:
      EFM_ASSERT(val <= (_CMU_LFRCOCTRL_TUNING_MASK
                         >> _CMU_LFRCOCTRL_TUNING_SHIFT));
      val &= (_CMU_LFRCOCTRL_TUNING_MASK >> _CMU_LFRCOCTRL_TUNING_SHIFT);
#if defined(_SILICON_LABS_32B_SERIES_1)
      while (BUS_RegBitRead(&CMU->SYNCBUSY,
                            _CMU_SYNCBUSY_LFRCOBSY_SHIFT) != 0UL) {
      }
#endif
      CMU->LFRCOCTRL = (CMU->LFRCOCTRL & ~(_CMU_LFRCOCTRL_TUNING_MASK))
                       | (val << _CMU_LFRCOCTRL_TUNING_SHIFT);
      break;

    case cmuOsc_HFRCO:
      EFM_ASSERT(val <= (_CMU_HFRCOCTRL_TUNING_MASK
                         >> _CMU_HFRCOCTRL_TUNING_SHIFT));
      val &= (_CMU_HFRCOCTRL_TUNING_MASK >> _CMU_HFRCOCTRL_TUNING_SHIFT);
#if defined(_SILICON_LABS_32B_SERIES_1)
      while (BUS_RegBitRead(&CMU->SYNCBUSY,
                            _CMU_SYNCBUSY_HFRCOBSY_SHIFT) != 0UL) {
      }
#endif
      CMU->HFRCOCTRL = (CMU->HFRCOCTRL & ~(_CMU_HFRCOCTRL_TUNING_MASK))
                       | (val << _CMU_HFRCOCTRL_TUNING_SHIFT);
      break;

#if defined (_CMU_USHFRCOCTRL_TUNING_MASK)
    case cmuOsc_USHFRCO:
      EFM_ASSERT(val <= (_CMU_USHFRCOCTRL_TUNING_MASK
                         >> _CMU_USHFRCOCTRL_TUNING_SHIFT));
      val &= (_CMU_USHFRCOCTRL_TUNING_MASK >> _CMU_USHFRCOCTRL_TUNING_SHIFT);
#if defined(_SILICON_LABS_32B_SERIES_1)
      while (BUS_RegBitRead(&CMU->SYNCBUSY, _CMU_SYNCBUSY_USHFRCOBSY_SHIFT)) {
      }
#endif
      CMU->USHFRCOCTRL = (CMU->USHFRCOCTRL & ~(_CMU_USHFRCOCTRL_TUNING_MASK))
                         | (val << _CMU_USHFRCOCTRL_TUNING_SHIFT);
      break;
#endif

    case cmuOsc_AUXHFRCO:
      EFM_ASSERT(val <= (_CMU_AUXHFRCOCTRL_TUNING_MASK
                         >> _CMU_AUXHFRCOCTRL_TUNING_SHIFT));
      val &= (_CMU_AUXHFRCOCTRL_TUNING_MASK >> _CMU_AUXHFRCOCTRL_TUNING_SHIFT);
#if defined(_SILICON_LABS_32B_SERIES_1)
      while (BUS_RegBitRead(&CMU->SYNCBUSY,
                            _CMU_SYNCBUSY_AUXHFRCOBSY_SHIFT) != 0UL) {
      }
#endif
      CMU->AUXHFRCOCTRL = (CMU->AUXHFRCOCTRL & ~(_CMU_AUXHFRCOCTRL_TUNING_MASK))
                          | (val << _CMU_AUXHFRCOCTRL_TUNING_SHIFT);
      break;

#if defined(_CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_MASK)
    case cmuOsc_HFXO:

      /* Do set PEAKDETSHUNTOPTMODE or HFXOSTEADYSTATECTRL if HFXO is enabled. */
      EFM_ASSERT((CMU->STATUS & CMU_STATUS_HFXOENS) == 0UL);

      /* Switch to command mode. Automatic SCO and PDA calibration is not done
         at the next enable. Set user REGISH, REGISHUPPER, and IBTRIMXOCORE. */
      CMU->HFXOCTRL = (CMU->HFXOCTRL & ~_CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_MASK)
                      | CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_CMD;

#if defined(_CMU_HFXOSTEADYSTATECTRL_REGISH_MASK)
      regIshUpper = getRegIshUpperVal((val & _CMU_HFXOSTEADYSTATECTRL_REGISH_MASK)
                                      >> _CMU_HFXOSTEADYSTATECTRL_REGISH_SHIFT);
      CMU->HFXOSTEADYSTATECTRL = (CMU->HFXOSTEADYSTATECTRL
                                  & ~(_CMU_HFXOSTEADYSTATECTRL_IBTRIMXOCORE_MASK
                                      | _CMU_HFXOSTEADYSTATECTRL_REGISH_MASK
                                      | _CMU_HFXOSTEADYSTATECTRL_REGISHUPPER_MASK))
                                 | val
                                 | regIshUpper;
#else
      CMU->HFXOSTEADYSTATECTRL = (CMU->HFXOSTEADYSTATECTRL
                                  & ~_CMU_HFXOSTEADYSTATECTRL_IBTRIMXOCORE_MASK)
                                 | val;
#endif

      break;
#endif

    default:
      EFM_ASSERT(false);
      break;
  }
}

#if defined(_CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_MASK) || defined(_CMU_HFXOCTRL_PEAKDETMODE_MASK)
/***************************************************************************//**
 * @brief
 *   Wait for the oscillator tuning optimization.
 *
 * @param[in] osc
 *   An oscillator to set tuning value for, one of the following:
 *   @li #cmuOsc_HFXO
 *
 * @param[in] mode
 *   Tuning optimization mode.
 *
 * @return
 *   Returns false on invalid parameters or oscillator error status.
 ******************************************************************************/
bool CMU_OscillatorTuningWait(CMU_Osc_TypeDef osc,
                              CMU_HFXOTuningMode_TypeDef mode)
{
  uint32_t waitFlags;
  EFM_ASSERT(osc == cmuOsc_HFXO);

  /* Currently implemented for HFXO with PEAKDETSHUNTOPTMODE only. */
  (void)osc;

  if (getHfxoTuningMode() == HFXO_TUNING_MODE_AUTO) {
    waitFlags = HFXO_TUNING_READY_FLAGS;
  } else {
    /* Set wait flags for each command and wait. */
    switch (mode) {
#if defined(_CMU_STATUS_HFXOSHUNTOPTRDY_MASK)
      case cmuHFXOTuningMode_ShuntCommand:
        waitFlags = CMU_STATUS_HFXOSHUNTOPTRDY;
        break;
#endif
      case cmuHFXOTuningMode_Auto:
        waitFlags = HFXO_TUNING_READY_FLAGS;
        break;

#if defined(CMU_CMD_HFXOSHUNTOPTSTART)
      case cmuHFXOTuningMode_PeakShuntCommand:
        waitFlags = HFXO_TUNING_READY_FLAGS;
        break;
#endif

      default:
        waitFlags = _CMU_STATUS_MASK;
        EFM_ASSERT(false);
        break;
    }
  }
  while ((CMU->STATUS & waitFlags) != waitFlags) {
  }

#if defined(CMU_IF_HFXOPEAKDETERR)
  /* Check error flags. */
  if ((waitFlags & CMU_STATUS_HFXOPEAKDETRDY) != 0UL) {
    return (CMU->IF & CMU_IF_HFXOPEAKDETERR) != 0UL ? true : false;
  }
#endif
  return true;
}

/***************************************************************************//**
 * @brief
 *   Start and optionally wait for the oscillator tuning optimization.
 *
 * @param[in] osc
 *   An oscillator to set tuning value for, one of the following:
 *   @li #cmuOsc_HFXO
 *
 * @param[in] mode
 *   Tuning optimization mode.
 *
 * @param[in] wait
 *   Wait for tuning optimization to complete.
 *   true - wait for tuning optimization to complete.
 *   false - return without waiting.
 *
 * @return
 *   Returns false on invalid parameters or oscillator error status.
 ******************************************************************************/
bool CMU_OscillatorTuningOptimize(CMU_Osc_TypeDef osc,
                                  CMU_HFXOTuningMode_TypeDef mode,
                                  bool wait)
{
  switch (osc) {
    case cmuOsc_HFXO:
      if ((unsigned)mode != 0U) {
#if defined(CMU_IF_HFXOPEAKDETERR)
        /* Clear the error flag before command write. */
        CMU->IFC = CMU_IFC_HFXOPEAKDETERR;
#endif
        CMU->CMD = (uint32_t)mode;
      }
      if (wait) {
        return CMU_OscillatorTuningWait(osc, mode);
      }
      break;

    default:
      EFM_ASSERT(false);
      break;
  }
  return true;
}
#endif

/**************************************************************************//**
 * @brief
 *   Determine if the currently selected PCNTn clock used is external or LFBCLK.
 *
 * @param[in] instance
 *   PCNT instance number to get currently selected clock source for.
 *
 * @return
 *   @li true - selected clock is external clock.
 *   @li false - selected clock is LFBCLK.
 *****************************************************************************/
bool CMU_PCNTClockExternalGet(unsigned int instance)
{
  uint32_t setting;

  switch (instance) {
#if defined(_CMU_PCNTCTRL_PCNT0CLKEN_MASK)
    case 0:
      setting = CMU->PCNTCTRL & CMU_PCNTCTRL_PCNT0CLKSEL_PCNT0S0;
      break;

#if defined(_CMU_PCNTCTRL_PCNT1CLKEN_MASK)
    case 1:
      setting = CMU->PCNTCTRL & CMU_PCNTCTRL_PCNT1CLKSEL_PCNT1S0;
      break;

#if defined(_CMU_PCNTCTRL_PCNT2CLKEN_MASK)
    case 2:
      setting = CMU->PCNTCTRL & CMU_PCNTCTRL_PCNT2CLKSEL_PCNT2S0;
      break;
#endif
#endif
#endif

    default:
      setting = 0;
      break;
  }
  return setting > 0UL ? true : false;
}

/**************************************************************************//**
 * @brief
 *   Select the PCNTn clock.
 *
 * @param[in] instance
 *   PCNT instance number to set selected clock source for.
 *
 * @param[in] external
 *   Set to true to select the external clock, false to select LFBCLK.
 *****************************************************************************/
void CMU_PCNTClockExternalSet(unsigned int instance, bool external)
{
#if defined(PCNT_PRESENT)
  uint32_t setting = 0;

  EFM_ASSERT(instance < (unsigned)PCNT_COUNT);

  if (external) {
    setting = 1;
  }

  BUS_RegBitWrite(&(CMU->PCNTCTRL), (instance * 2U) + 1U, setting);

#else
  (void)instance;  /* An unused parameter */
  (void)external;  /* An unused parameter */
#endif
}

#if defined(_CMU_USHFRCOCONF_BAND_MASK)
/***************************************************************************//**
 * @brief
 *   Get USHFRCO band in use.
 *
 * @return
 *   USHFRCO band in use.
 ******************************************************************************/
CMU_USHFRCOBand_TypeDef CMU_USHFRCOBandGet(void)
{
  return (CMU_USHFRCOBand_TypeDef)((CMU->USHFRCOCONF
                                    & _CMU_USHFRCOCONF_BAND_MASK)
                                   >> _CMU_USHFRCOCONF_BAND_SHIFT);
}

/***************************************************************************//**
 * @brief
 *   Get USHFRCO frequency.
 *
 * @return
 *   USHFRCO frequency.
 ******************************************************************************/
uint32_t CMU_USHFRCOFreqGet(void)
{
  return ushfrcoFreq;
}
#endif

#if defined(_CMU_USHFRCOCONF_BAND_MASK)
/***************************************************************************//**
 * @brief
 *   Set the USHFRCO band to use.
 *
 * @param[in] band
 *   USHFRCO band to activate.
 ******************************************************************************/
void CMU_USHFRCOBandSet(CMU_USHFRCOBand_TypeDef band)
{
  uint32_t           tuning;
  uint32_t           fineTuning;

  /* Cannot switch band if USHFRCO is already selected as HF clock. */
  EFM_ASSERT(CMU_ClockSelectGet(cmuClock_HF) != cmuSelect_USHFRCODIV2);

  /* Read tuning value from calibration table. */
  switch (band) {
    case cmuUSHFRCOBand_24MHz:
      tuning = (DEVINFO->USHFRCOCAL0 & _DEVINFO_USHFRCOCAL0_BAND24_TUNING_MASK)
               >> _DEVINFO_USHFRCOCAL0_BAND24_TUNING_SHIFT;
      fineTuning = (DEVINFO->USHFRCOCAL0
                    & _DEVINFO_USHFRCOCAL0_BAND24_FINETUNING_MASK)
                   >> _DEVINFO_USHFRCOCAL0_BAND24_FINETUNING_SHIFT;
      ushfrcoFreq = 24000000UL;
      break;

    case cmuUSHFRCOBand_48MHz:
      tuning = (DEVINFO->USHFRCOCAL0 & _DEVINFO_USHFRCOCAL0_BAND48_TUNING_MASK)
               >> _DEVINFO_USHFRCOCAL0_BAND48_TUNING_SHIFT;
      fineTuning = (DEVINFO->USHFRCOCAL0
                    & _DEVINFO_USHFRCOCAL0_BAND48_FINETUNING_MASK)
                   >> _DEVINFO_USHFRCOCAL0_BAND48_FINETUNING_SHIFT;
      /* Enable the clock divider before switching the band from 24 to 48 MHz */
      BUS_RegBitWrite(&CMU->USHFRCOCONF, _CMU_USHFRCOCONF_USHFRCODIV2DIS_SHIFT, 0);
      ushfrcoFreq = 48000000UL;
      break;

    default:
      EFM_ASSERT(false);
      return;
  }

  /* Set band and tuning. */
  CMU->USHFRCOCONF = (CMU->USHFRCOCONF & ~_CMU_USHFRCOCONF_BAND_MASK)
                     | (band << _CMU_USHFRCOCONF_BAND_SHIFT);
  CMU->USHFRCOCTRL = (CMU->USHFRCOCTRL & ~_CMU_USHFRCOCTRL_TUNING_MASK)
                     | (tuning << _CMU_USHFRCOCTRL_TUNING_SHIFT);
  CMU->USHFRCOTUNE = (CMU->USHFRCOTUNE & ~_CMU_USHFRCOTUNE_FINETUNING_MASK)
                     | (fineTuning << _CMU_USHFRCOTUNE_FINETUNING_SHIFT);

  /* Disable the clock divider after switching the band from 48 to 24 MHz. */
  if (band == cmuUSHFRCOBand_24MHz) {
    BUS_RegBitWrite(&CMU->USHFRCOCONF, _CMU_USHFRCOCONF_USHFRCODIV2DIS_SHIFT, 1);
  }
}
#endif

#endif // defined(_SILICON_LABS_32B_SERIES_2)

/** @} (end addtogroup cmu) */
#endif /* defined(CMU_PRESENT) */
