/***************************************************************************//**
 * @file
 * @brief This file contains the type definitions for efr32xg2x chip specific
 *   aspects of RAIL.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef __RAIL_CHIP_SPECIFIC_H_
#define __RAIL_CHIP_SPECIFIC_H_

// Include standard type headers to help define structures
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "em_gpio.h"

#include "rail_types.h"
#include "rail_features.h"

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Multiprotocol
// -----------------------------------------------------------------------------

/**
 * @def TRANSITION_TIME_US
 * @brief Time it takes to take care of protocol switching.
 */
#define TRANSITION_TIME_US 510

/**
 * @def EFR32XG21_RAIL_SCHEDULER_STATE_UINT32_BUFFER_SIZE
 * @brief The size in 32-bit words of RAIL_SchedulerStateBuffer_t to store
 *   RAIL multiprotocol internal state.
 */
#define EFR32XG21_RAIL_SCHEDULER_STATE_UINT32_BUFFER_SIZE 26

/**
 * @def EFR32XG22_RAIL_SCHEDULER_STATE_UINT32_BUFFER_SIZE
 * @brief The size in 32-bit words of RAIL_SchedulerStateBuffer_t to store
 *   RAIL multiprotocol internal state.
 */
#define EFR32XG22_RAIL_SCHEDULER_STATE_UINT32_BUFFER_SIZE 26

/**
 * @def EFR32XG23_RAIL_SCHEDULER_STATE_UINT32_BUFFER_SIZE
 * @brief The size in 32-bit words of RAIL_SchedulerStateBuffer_t to store
 *   RAIL multiprotocol internal state.
 */
#define EFR32XG23_RAIL_SCHEDULER_STATE_UINT32_BUFFER_SIZE 25

#if (_SILICON_LABS_32B_SERIES_2_CONFIG == 1)
#define RAIL_SCHEDULER_STATE_UINT32_BUFFER_SIZE EFR32XG21_RAIL_SCHEDULER_STATE_UINT32_BUFFER_SIZE
#elif (_SILICON_LABS_32B_SERIES_2_CONFIG == 2)
#define RAIL_SCHEDULER_STATE_UINT32_BUFFER_SIZE EFR32XG22_RAIL_SCHEDULER_STATE_UINT32_BUFFER_SIZE
#elif (_SILICON_LABS_32B_SERIES_2_CONFIG == 3)
#define RAIL_SCHEDULER_STATE_UINT32_BUFFER_SIZE EFR32XG23_RAIL_SCHEDULER_STATE_UINT32_BUFFER_SIZE
#else
#error "Unsupported platform!"
#endif //_SILICON_LABS_32B_SERIES_2_CONFIG

/**
 * @typedef RAIL_SchedulerStateBuffer_t
 * @brief A buffer used to store multiprotocol scheduler internal state.
 *
 * This buffer must be allocated in application global read-write memory
 * that persists for the duration of RAIL usage. It cannot be allocated
 * in read-only memory or on the call stack.
 */
typedef uint32_t RAIL_SchedulerStateBuffer_t[RAIL_SCHEDULER_STATE_UINT32_BUFFER_SIZE];

/**
 * @struct RAILSched_Config_t
 * @brief A multiprotocol scheduler configuration and internal state.
 *
 * This buffer must be allocated in application global read-write memory
 * that persists for the duration of RAIL usage. It cannot be allocated
 * in read-only memory or on the call stack.
 */
typedef struct RAILSched_Config {
  RAIL_SchedulerStateBuffer_t buffer; /**< An internal state buffer. */
} RAILSched_Config_t;

/**
 * @def EFR32XG21_RAIL_STATE_UINT32_BUFFER_SIZE
 * @brief The size, in 32-bit words, of RAIL_StateBuffer_t to store RAIL
 *   internal state for the EFR32XG21 series.
 */
#define EFR32XG21_RAIL_STATE_UINT32_BUFFER_SIZE 102

/**
 * @def EFR32XG22_RAIL_STATE_UINT32_BUFFER_SIZE
 * @brief The size, in 32-bit words, of RAIL_StateBuffer_t to store RAIL
 *   internal state for the EFR32XG22 series.
 */
#define EFR32XG22_RAIL_STATE_UINT32_BUFFER_SIZE 104

/**
 * @def EFR32XG23_RAIL_STATE_UINT32_BUFFER_SIZE
 * @brief The size, in 32-bit words, of RAIL_StateBuffer_t to store RAIL
 *   internal state for the EFR32XG23 series.
 */
#define EFR32XG23_RAIL_STATE_UINT32_BUFFER_SIZE 82

#if (_SILICON_LABS_32B_SERIES_2_CONFIG == 1)
#define RAIL_STATE_UINT32_BUFFER_SIZE EFR32XG21_RAIL_STATE_UINT32_BUFFER_SIZE
#elif (_SILICON_LABS_32B_SERIES_2_CONFIG == 2)
#define RAIL_STATE_UINT32_BUFFER_SIZE EFR32XG22_RAIL_STATE_UINT32_BUFFER_SIZE
#elif (_SILICON_LABS_32B_SERIES_2_CONFIG == 3)
#define RAIL_STATE_UINT32_BUFFER_SIZE EFR32XG23_RAIL_STATE_UINT32_BUFFER_SIZE
#else
#error "Unsupported platform!"
#endif //_SILICON_LABS_32B_SERIES_2_CONFIG

/**
 * @typedef RAIL_StateBuffer_t
 * @brief A buffer to store RAIL internal state.
 */
typedef uint32_t RAIL_StateBuffer_t[RAIL_STATE_UINT32_BUFFER_SIZE];

/**
 * @struct RAIL_Config_t
 * @brief RAIL configuration and internal state structure.
 *
 * This structure must be allocated in application global read-write memory
 * that persists for the duration of RAIL usage. It cannot be allocated
 * in read-only memory or on the call stack.
 */
typedef struct RAIL_Config {
  /**
   * A pointer to a function, which is called whenever a RAIL event occurs.
   *
   * @param[in] railHandle A handle for a RAIL instance.
   * @param[in] events A bit mask of RAIL events.
   * @return void.
   *
   * See the \ref RAIL_Events_t documentation for the list of RAIL events.
   */
  void (*eventsCallback)(RAIL_Handle_t railHandle, RAIL_Events_t events);
  /**
   * Pointer to a structure to hold state information required by the \ref
   * Protocol_Specific APIs. If needed, this structure must be allocated in
   * global read-write memory and initialized to all zeros.
   *
   * Currently, this is only required when using the \ref BLE APIs and should be
   * set to point to a \ref RAIL_BLE_State_t structure. When using \ref
   * IEEE802_15_4 or \ref Z_Wave this should be set to NULL.
   */
  void *protocol;
  /**
   * A pointer to a RAIL scheduler state object allocated in global read-write
   * memory and initialized to all zeros. When not using a multiprotocol
   * scheduler, it should be NULL.
   */
  RAILSched_Config_t *scheduler;
  /**
   * A structure for RAIL to maintain its internal state, which must be
   * initialized to all zeros.
   */
  RAIL_StateBuffer_t buffer;
} RAIL_Config_t;

/**
 * @addtogroup Multiprotocol_EFR32 EFR32
 * @{
 * @brief EFR32-specific multiprotocol support defines
 * @ingroup Multiprotocol
 */

/**
 * A placeholder for a chip-specific RAIL handle. Using NULL as a RAIL handle is
 * not recommended. As a result, another value that can't be de-referenced is used.
 */
#define RAIL_EFR32_HANDLE ((RAIL_Handle_t)0xFFFFFFFFUL)

/** @} */ // end of group Multiprotocol_EFR32

// -----------------------------------------------------------------------------
// Calibration
// -----------------------------------------------------------------------------
/**
 * @addtogroup Calibration_EFR32XG2X EFR32XG2X
 * @{
 * @brief EFR32XG2X-specific Calibrations
 * @ingroup Calibration
 *
 * The EFR32 supports the Image Rejection (IR)
 * calibration and a temperature-dependent calibration. The IR calibration
 * can be computed once and stored off or computed each time at
 * startup. Because it is PHY-specific and provides sensitivity improvements,
 * it is highly recommended. The IR calibration should only be run when the
 * radio is IDLE.
 *
 * The temperature-dependent calibrations are used to recalibrate the synth if
 * the temperature crosses 0C or the temperature delta since the last
 * calibration exceeds 70C while in receive. RAIL will run the VCO calibration
 * automatically upon entering receive or transmit states, so the application
 * can omit this calibration if the stack re-enters receive or transmit with
 * enough frequency to avoid reaching the temperature delta. If the application
 * does not calibrate for temperature, it's possible to miss receive packets due
 * to a drift in the carrier frequency.
 */

/** EFR32-specific temperature calibration bit */
#define RAIL_CAL_TEMP_VCO         (0x00000001U)
/** EFR32-specific IR calibration bit */
#define RAIL_CAL_ONETIME_IRCAL    (0x00010000U)

/** A mask to run temperature-dependent calibrations */
#define RAIL_CAL_TEMP             (RAIL_CAL_TEMP_VCO)
/** A mask to run one-time calibrations */
#define RAIL_CAL_ONETIME          (RAIL_CAL_ONETIME_IRCAL)
/** A mask to run optional performance calibrations */
#define RAIL_CAL_PERF             (0)
/** A mask for calibrations that require the radio to be off */
#define RAIL_CAL_OFFLINE          (RAIL_CAL_ONETIME_IRCAL)
/** A mask to run all possible calibrations for this chip */
#define RAIL_CAL_ALL              (RAIL_CAL_TEMP | RAIL_CAL_ONETIME)
/** A mask to run all pending calibrations */
#define RAIL_CAL_ALL_PENDING      (0x00000000U)
/** An invalid calibration value */
#define RAIL_CAL_INVALID_VALUE    (0xFFFFFFFFU)

/**
 * Applies a given image rejection calibration value.
 *
 * @param[in] railHandle A RAIL instance handle.
 * @param[in] imageRejection The image rejection value to apply.
 * @return A status code indicating success of the function call.
 *
 * Take an image rejection calibration value and apply it. This value should be
 * determined from a previous run of \ref RAIL_CalibrateIr on the same
 * physical device with the same radio configuration. The imageRejection value
 * will also be stored to the \ref RAIL_ChannelConfigEntry_t::attr, if possible.
 *
 * If multiple protocols are used, this function will return
 * \ref RAIL_STATUS_INVALID_STATE if it is called and the given railHandle is
 * not active. In that case, the caller must attempt to re-call this function later.
 */
RAIL_Status_t RAIL_ApplyIrCalibration(RAIL_Handle_t railHandle,
                                      uint32_t imageRejection);

/**
 * Runs the image rejection calibration.
 *
 * @param[in] railHandle A RAIL instance handle.
 * @param[out] imageRejection The result of the image rejection calibration.
 * @return A status code indicating success of the function call.
 *
 * Run the image rejection calibration and apply the resulting value. If the
 * imageRejection parameter is not NULL, store the value at that
 * location. The imageRejection value will also be stored to the
 * \ref RAIL_ChannelConfigEntry_t::attr, if possible. This is a long-running
 * calibration that adds significant code space when run and can be run with a
 * separate firmware image on each device to save code space in the
 * final image.
 *
 * If multiple protocols are used, this function will return
 * \ref RAIL_STATUS_INVALID_STATE if it is called and the given railHandle is
 * not active. In that case, the caller must attempt to re-call this function later.
 */
RAIL_Status_t RAIL_CalibrateIr(RAIL_Handle_t railHandle,
                               uint32_t *imageRejection);

/**
 * Calibrates image rejection for IEEE 802.15.4 2.4 GHz
 *
 * @param[in] railHandle A RAIL instance handle.
 * @param[out] imageRejection The result of the image rejection calibration.
 * @return A status code indicating success of the function call.
 *
 * Some chips have protocol-specific image rejection calibrations programmed
 * into their flash. This function will either get the value from flash and
 * apply it, or run the image rejection algorithm to find the value.
 */
RAIL_Status_t RAIL_IEEE802154_CalibrateIr2p4Ghz(RAIL_Handle_t railHandle,
                                                uint32_t *imageRejection);

/**
 * Calibrates image rejection for IEEE 802.15.4 915 MHz and 868 MHz
 *
 * @param[in] railHandle A RAIL instance handle.
 * @param[out] imageRejection The result of the image rejection calibration.
 * @return A status code indicating success of the function call.
 *
 * Some chips have protocol-specific image rejection calibrations programmed
 * into their flash. This function will either get the value from flash and
 * apply it, or run the image rejection algorithm to find the value.
 */
RAIL_Status_t RAIL_IEEE802154_CalibrateIrSubGhz(RAIL_Handle_t railHandle,
                                                uint32_t *imageRejection);

/**
 * Calibrates image rejection for Bluetooth Low Energy
 *
 * @param[in] railHandle A RAIL instance handle.
 * @param[out] imageRejection The result of the image rejection calibration.
 * @return A status code indicating success of the function call.
 *
 * Some chips have protocol-specific image rejection calibrations programmed
 * into their flash. This function will either get the value from flash and
 * apply it, or run the image rejection algorithm to find the value.
 */
RAIL_Status_t RAIL_BLE_CalibrateIr(RAIL_Handle_t railHandle,
                                   uint32_t *imageRejection);

/**
 * Runs the temperature calibration.
 *
 * @param[in] railHandle A RAIL instance handle.
 * @return A status code indicating success of the function call.
 *
 * Run the temperature calibration, which needs to recalibrate the synth if
 * the temperature crosses 0C or the temperature delta since the last
 * calibration exceeds 70C while in receive. RAIL will run the VCO calibration
 * automatically upon entering receive or transmit states, so the application
 * can omit this calibration if the stack re-enters receive or transmit with
 * enough frequency to avoid reaching the temperature delta. If the application
 * does not calibrate for temperature, it's possible to miss receive packets due
 * to a drift in the carrier frequency.
 *
 * If multiple protocols are used, this function will return
 * \ref RAIL_STATUS_INVALID_STATE if it is called and the given railHandle is
 * not active. In that case, the calibration will be automatically performed
 * next time the radio enters receive.
 */
RAIL_Status_t RAIL_CalibrateTemp(RAIL_Handle_t railHandle);

/**
 * @struct RAIL_CalValues_t
 * @brief A calibration value structure.
 *
 * This structure contains the set of persistent calibration values for
 * EFR32. You can set these beforehand and apply them at startup to save the
 * time required to compute them. Any of these values may be set to
 * RAIL_CAL_INVALID_VALUE to force the code to compute that calibration value.
 */
typedef struct RAIL_CalValues {
  uint32_t imageRejection; /**< An Image Rejection (IR) calibration value */
} RAIL_CalValues_t;

/**
 * A define to set all RAIL_CalValues_t values to uninitialized.
 *
 * This define can be used when you have no data to pass to the calibration
 * routines but wish to compute and save all possible calibrations.
 */
#define RAIL_CALVALUES_UNINIT (RAIL_CalValues_t){ \
    RAIL_CAL_INVALID_VALUE,                       \
}

/** @} */ // end of group Calibration_EFR32

// -----------------------------------------------------------------------------
// Diagnostic
// -----------------------------------------------------------------------------
/**
 * @addtogroup Diagnostic_EFR32 EFR32
 * @{
 * @brief Types specific to the EFR32 for the diagnostic routines.
 * @ingroup Diagnostic
 */

/**
 * @typedef RAIL_FrequencyOffset_t
 * @brief Chip-specific type that represents the number of Frequency Offset
 *   units. It is used with \ref RAIL_GetRxFreqOffset() and
 *   \ref RAIL_SetFreqOffset().
 *
 * The units on this chip are radio synthesizer resolution steps (synthTicks).
 * On EFR32 (at least for now), the frequency offset is limited to 15 bits
 * (size of SYNTH_CALOFFSET). A value of \ref RAIL_FREQUENCY_OFFSET_INVALID
 * means that this value is invalid.
 */
typedef int16_t RAIL_FrequencyOffset_t;

/**
 * The maximum frequency offset value supported by this radio.
 */
#define RAIL_FREQUENCY_OFFSET_MAX ((RAIL_FrequencyOffset_t) 0x3FFF)

/**
 * The minimum frequency offset value supported by this radio.
 */
#define RAIL_FREQUENCY_OFFSET_MIN ((RAIL_FrequencyOffset_t) -RAIL_FREQUENCY_OFFSET_MAX)

/**
 * Specifies an invalid frequency offset value. This will be returned if you
 * call \ref RAIL_GetRxFreqOffset() at an invalid time.
 */
#define RAIL_FREQUENCY_OFFSET_INVALID ((RAIL_FrequencyOffset_t) 0x8000)

/** @} */ // end of group Diagnostic_EFR32

// -----------------------------------------------------------------------------
// Radio Configuration
// -----------------------------------------------------------------------------
/**
 * @addtogroup Radio_Configuration_EFR32 EFR32
 * @{
 * @ingroup Radio_Configuration
 * @brief Types specific to the EFR32 for radio configuration.
 */

/**
 * @brief The radio configuration structure.
 *
 * The radio configuration properly configures the
 * radio for operation on a protocol. These configurations should not be
 * created or edited by hand.
 */
typedef const uint32_t *RAIL_RadioConfig_t;

/** @} */ // end of group Radio_Configuration_EFR32

// -----------------------------------------------------------------------------
// Transmit
// -----------------------------------------------------------------------------
/**
 * @addtogroup PA_EFR32XG2X EFR32XG2X
 * @{
 * @ingroup PA
 * @brief Types specific to the EFR32 for dealing with the on-chip PAs.
 */

/**
 * Raw power levels used directly by the RAIL_Get/SetTxPower API where a higher
 * numerical value corresponds to a higher output power. These are referred to
 * as 'raw (values/units)'. On EFR32, they can range from one of \ref
 * RAIL_TX_POWER_LEVEL_LP_MIN, \ref RAIL_TX_POWER_LEVEL_HP_MIN, or
 * \ref RAIL_TX_POWER_LEVEL_SUBGIG_MIN to one of \ref
 * RAIL_TX_POWER_LEVEL_LP_MAX, \ref RAIL_TX_POWER_LEVEL_HP_MAX, and \ref
 * RAIL_TX_POWER_LEVEL_SUBGIG_MAX, respectively, depending on the selected \ref
 * RAIL_TxPowerMode_t.
 */
typedef uint8_t RAIL_TxPowerLevel_t;

#if _SILICON_LABS_32B_SERIES_2_CONFIG == 1
/**
 * The maximum valid value for the \ref RAIL_TxPowerLevel_t when in \ref
 * RAIL_TX_POWER_MODE_2P4_HP mode.
 */
#define RAIL_TX_POWER_LEVEL_HP_MAX     (180U)
/**
 * The minimum valid value for the \ref RAIL_TxPowerLevel_t when in \ref
 * RAIL_TX_POWER_MODE_2P4_HP mode.
 */
#define RAIL_TX_POWER_LEVEL_HP_MIN     (1U)
/**
 * The maximum valid value for the \ref RAIL_TxPowerLevel_t when in \ref
 * RAIL_TX_POWER_MODE_2P4_MP mode.
 */
#define RAIL_TX_POWER_LEVEL_MP_MAX     (90U)
/**
 * The maximum valid value for the \ref RAIL_TxPowerLevel_t when in \ref
 * RAIL_TX_POWER_MODE_2P4_LP mode.
 */
#define RAIL_TX_POWER_LEVEL_LP_MAX     (64U)
/**
 * The minimum valid value for the \ref RAIL_TxPowerLevel_t when in \ref
 * RAIL_TX_POWER_MODE_2P4_MP mode.
 */
#define RAIL_TX_POWER_LEVEL_MP_MIN     (1U)
/**
 * The minimum valid value for the \ref RAIL_TxPowerLevel_t when in \ref
 * RAIL_TX_POWER_MODE_2P4_LP mode.
 */
#define RAIL_TX_POWER_LEVEL_LP_MIN     (1U)
#else
/**
 * The maximum valid value for the \ref RAIL_TxPowerLevel_t when in \ref
 * RAIL_TX_POWER_MODE_2P4_HP mode.
 */
#define RAIL_TX_POWER_LEVEL_HP_MAX     (128U)
/**
 * The minimum valid value for the \ref RAIL_TxPowerLevel_t when in \ref
 * RAIL_TX_POWER_MODE_2P4_HP mode.
 */
#define RAIL_TX_POWER_LEVEL_HP_MIN     (0U)
/**
 * The maximum valid value for the \ref RAIL_TxPowerLevel_t when in \ref
 * RAIL_TX_POWER_MODE_2P4_LP mode.
 */
#define RAIL_TX_POWER_LEVEL_LP_MAX     (15U)
/**
 * The minimum valid value for the \ref RAIL_TxPowerLevel_t when in \ref
 * RAIL_TX_POWER_MODE_2P4_LP mode.
 */
#define RAIL_TX_POWER_LEVEL_LP_MIN     (0U)
#endif

#if RAIL_FEAT_SUBGIG_RADIO
/**
 * The maximum valid value for the \ref RAIL_TxPowerLevel_t when in \ref
 * RAIL_TX_POWER_MODE_SUBGIG mode.
 */
#define RAIL_TX_POWER_LEVEL_SUBGIG_MAX (248U)
#endif

/**
 * Invalid RAIL_TxPowerLevel_t value returned when an error occurs
 * with RAIL_GetTxPower.
 */
#define RAIL_TX_POWER_LEVEL_INVALID (255U)
/**
 * Sentinel value that can be passed to RAIL_SetTxPower to set
 * the highest power level available on the current PA, regardless
 * of which one is selected.
 */
#define RAIL_TX_POWER_LEVEL_MAX (254U)

/**
 * @enum RAIL_TxPowerMode_t
 * @brief An enumeration of the EFR32 power modes.
 *
 * The power modes on the EFR32 correspond to the different on-chip PAs that
 * are available. For more information about the power and performance
 * characteristics of a given amplifier, see the data sheet.
 */
RAIL_ENUM(RAIL_TxPowerMode_t) {
  /**
   *  High-power 2.4 GHz amplifier
   *  EFR32XG21: up to 20 dBm, raw values: 1-180
   *  EFR32XG22: up to 6 dBm, raw values: 1-128
   */
  RAIL_TX_POWER_MODE_2P4GIG_HP,
  /** Deprecated enum equivalent to \ref RAIL_TX_POWER_MODE_2P4GIG_HP */
  RAIL_TX_POWER_MODE_2P4_HP = RAIL_TX_POWER_MODE_2P4GIG_HP,
#if _SILICON_LABS_32B_SERIES_2_CONFIG != 2
  /**
   *  Mid-power 2.4 GHz amplifier
   *  EFR32XG21: up to 10 dBm, raw values: 1-90
   *  EFR32XG22: N/A
   */
  RAIL_TX_POWER_MODE_2P4GIG_MP,
  /** Deprecated enum equivalent to \ref RAIL_TX_POWER_MODE_2P4GIG_MP */
  RAIL_TX_POWER_MODE_2P4_MP = RAIL_TX_POWER_MODE_2P4GIG_MP,
#endif
  /**
   *  Low-power 2.4 GHz amplifier
   *  EFR32XG21: up to 0 dBm, raw values: 1-64
   *  EFR32XG22: up to 0 dBm, raw values: 1-16
   */
  RAIL_TX_POWER_MODE_2P4GIG_LP,
  /** Deprecated enum equivalent to \ref RAIL_TX_POWER_MODE_2P4GIG_LP */
  RAIL_TX_POWER_MODE_2P4_LP = RAIL_TX_POWER_MODE_2P4GIG_LP,
  /** Select the highest power PA available on the current chip. */
  RAIL_TX_POWER_MODE_2P4GIG_HIGHEST,
  /** Deprecated enum equivalent to \ref RAIL_TX_POWER_MODE_2P4GIG_HIGHEST */
  RAIL_TX_POWER_MODE_2P4_HIGHEST = RAIL_TX_POWER_MODE_2P4GIG_HIGHEST,
#if RAIL_FEAT_SUBGIG_RADIO
  /** High-power amplifier, up to 20 dBm, raw values: 0-180 */
  RAIL_TX_POWER_MODE_SUBGIG_HP,
  /** Mid-power amplifier, up to 10 dBm, raw values: 0-90 */
  RAIL_TX_POWER_MODE_SUBGIG_MP,
  /** Low-power amplifier, up to 0 dBm, raw values: 1-7 */
  RAIL_TX_POWER_MODE_SUBGIG_LP,
  /** Select the highest power PA available on the current chip. */
  RAIL_TX_POWER_MODE_SUBGIG_HIGHEST,
#endif
  /** Invalid amplifier Selection */
  RAIL_TX_POWER_MODE_NONE,
};

/**
 * The number of PA's on this chip.
 */
#if _SILICON_LABS_32B_SERIES_2_CONFIG == 2
#define RAIL_NUM_PA (2U)
#else
#define RAIL_NUM_PA (3U)
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// Self-referencing defines minimize compiler complaints when using RAIL_ENUM
#define RAIL_TX_POWER_MODE_2P4GIG_HP ((RAIL_TxPowerMode_t) RAIL_TX_POWER_MODE_2P4GIG_HP)
#define RAIL_TX_POWER_MODE_2P4_HP ((RAIL_TxPowerMode_t) RAIL_TX_POWER_MODE_2P4_HP)
#if _SILICON_LABS_32B_SERIES_2_CONFIG != 2
#define RAIL_TX_POWER_MODE_2P4GIG_MP ((RAIL_TxPowerMode_t) RAIL_TX_POWER_MODE_2P4GIG_MP)
#define RAIL_TX_POWER_MODE_2P4_MP ((RAIL_TxPowerMode_t) RAIL_TX_POWER_MODE_2P4_MP)
#endif
#define RAIL_TX_POWER_MODE_2P4GIG_LP ((RAIL_TxPowerMode_t) RAIL_TX_POWER_MODE_2P4GIG_LP)
#define RAIL_TX_POWER_MODE_2P4_LP ((RAIL_TxPowerMode_t) RAIL_TX_POWER_MODE_2P4_LP)
#define RAIL_TX_POWER_MODE_2P4GIG_HIGHEST ((RAIL_TxPowerMode_t) RAIL_TX_POWER_MODE_2P4GIG_HIGHEST)
#define RAIL_TX_POWER_MODE_2P4_HIGHEST ((RAIL_TxPowerMode_t) RAIL_TX_POWER_MODE_2P4_HIGHEST)
#if RAIL_FEAT_SUBGIG_RADIO
#define RAIL_TX_POWER_MODE_SUBGIG ((RAIL_TxPowerMode_t) RAIL_TX_POWER_MODE_SUBGIG)
#endif
#define RAIL_TX_POWER_MODE_NONE   ((RAIL_TxPowerMode_t) RAIL_TX_POWER_MODE_NONE)
#endif//DOXYGEN_SHOULD_SKIP_THIS

/**
 * @def RAIL_TX_POWER_MODE_NAMES
 * @brief The names of the TX power modes
 *
 * A list of the names for the TX power modes on the EFR32 series 2 parts. This
 * macro is useful for test applications and debugging output.
 */
#if _SILICON_LABS_32B_SERIES_2_CONFIG == 2
#define RAIL_TX_POWER_MODE_NAMES {       \
    "RAIL_TX_POWER_MODE_2P4GIG_HP",      \
    "RAIL_TX_POWER_MODE_2P4GIG_LP",      \
    "RAIL_TX_POWER_MODE_2P4GIG_HIGHEST", \
    "RAIL_TX_POWER_MODE_NONE"            \
}
#else
#define RAIL_TX_POWER_MODE_NAMES {       \
    "RAIL_TX_POWER_MODE_2P4GIG_HP",      \
    "RAIL_TX_POWER_MODE_2P4GIG_MP",      \
    "RAIL_TX_POWER_MODE_2P4GIG_LP",      \
    "RAIL_TX_POWER_MODE_2P4GIG_HIGHEST", \
    "RAIL_TX_POWER_MODE_NONE"            \
}
#endif

/**
 * @struct RAIL_TxPowerConfig_t
 *
 * @brief A structure containing values used to initialize the power amplifiers.
 */
typedef struct RAIL_TxPowerConfig {
  /** TX power mode */
  RAIL_TxPowerMode_t mode;
  /** Power amplifier supply voltage in mV, generally:
   *  DCDC supply ~ 1800 mV (1.8 V)
   *  Battery supply ~ 3300 mV (3.3 V)
   */
  uint16_t voltage;
  /** The amount of time to spend ramping for TX in uS. */
  uint16_t rampTime;
} RAIL_TxPowerConfig_t;

/** @} */ // end of group PA_EFR32

// -----------------------------------------------------------------------------
// PTI
// -----------------------------------------------------------------------------
/**
 * @addtogroup PTI_EFR32 EFR32
 * @{
 * @brief EFR32 PTI functionality
 * @ingroup PTI
 *
 * These enumerations and structures are used with RAIL PTI API. EFR32 supports
 * SPI and UART PTI and is configurable in terms of baud rates and pin PTI
 * pin locations.
 */

/** A channel type enumeration. */
RAIL_ENUM(RAIL_PtiMode_t) {
  /** Turn PTI off entirely. */
  RAIL_PTI_MODE_DISABLED,
  /** SPI mode. */
  RAIL_PTI_MODE_SPI,
  /** UART mode. */
  RAIL_PTI_MODE_UART,
  /** 9-bit UART mode. */
  RAIL_PTI_MODE_UART_ONEWIRE,
};

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// Self-referencing defines minimize compiler complaints when using RAIL_ENUM
#define RAIL_PTI_MODE_DISABLED     ((RAIL_PtiMode_t) RAIL_PTI_MODE_DISABLED)
#define RAIL_PTI_MODE_SPI          ((RAIL_PtiMode_t) RAIL_PTI_MODE_SPI)
#define RAIL_PTI_MODE_UART         ((RAIL_PtiMode_t) RAIL_PTI_MODE_UART)
#define RAIL_PTI_MODE_UART_ONEWIRE ((RAIL_PtiMode_t) RAIL_PTI_MODE_UART_ONEWIRE)
#endif//DOXYGEN_SHOULD_SKIP_THIS

/**
 * @struct RAIL_PtiConfig_t
 * @brief A configuration for PTI.
 */
typedef struct RAIL_PtiConfig {
  /** Packet Trace mode (UART or SPI) */
  RAIL_PtiMode_t mode;
  /** Output baudrate for PTI in Hz */
  uint32_t baud;
  /** Data output (DOUT) location for pin/port */
  uint8_t doutLoc;
  /** Data output (DOUT) GPIO port */
  uint8_t doutPort;
  /** Data output (DOUT) GPIO pin */
  uint8_t doutPin;
  /** Data clock (DCLK) location for pin/port. Only used in SPI mode */
  uint8_t dclkLoc;
  /** Data clock (DCLK) GPIO port. Only used in SPI mode */
  uint8_t dclkPort;
  /** Data clock (DCLK) GPIO pin. Only used in SPI mode */
  uint8_t dclkPin;
  /** Data frame (DFRAME) location for pin/port */
  uint8_t dframeLoc;
  /** Data frame (DFRAME) GPIO port */
  uint8_t dframePort;
  /** Data frame (DFRAME) GPIO pin */
  uint8_t dframePin;
} RAIL_PtiConfig_t;

/** @} */ // end of group PTI_EFR32

// -----------------------------------------------------------------------------
// Antenna Control
// -----------------------------------------------------------------------------
/**
 * @addtogroup Antenna_Control_EFR32XG2X EFR32XG2X
 * @{
 * @brief EFR32 Antenna Control Functionality
 * @ingroup Antenna_Control
 *
 * These enumerations and structures are used with RAIL Antenna Control API. EFR32 supports
 * up to two antennas with configurable pin locations.
 */

/** Antenna path Selection enumeration. */
RAIL_ENUM(RAIL_AntennaSel_t) {
  /** Enum for antenna path 0. */
  RAIL_ANTENNA_0 = 0,
  /** Enum for antenna path 1. */
  RAIL_ANTENNA_1 = 1,
  /** Enum for antenna path auto. */
  RAIL_ANTENNA_AUTO = 255,
};

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// Self-referencing defines minimize compiler complaints when using RAIL_ENUM
#define RAIL_ANTENNA_0    ((RAIL_AntennaSel_t) RAIL_ANTENNA_0)
#define RAIL_ANTENNA_1    ((RAIL_AntennaSel_t) RAIL_ANTENNA_1)
#define RAIL_ANTENNA_AUTO ((RAIL_AntennaSel_t) RAIL_ANTENNA_AUTO)
#endif//DOXYGEN_SHOULD_SKIP_THIS

/**
 * @struct RAIL_AntennaConfig_t
 * @brief A configuration for antenna selection.
 */
typedef struct RAIL_AntennaConfig {
  /** Antenna 0 Pin Enable */
  bool ant0PinEn;
  /** Antenna 1 Pin Enable */
  bool ant1PinEn;
  /** Antenna 0 internal RF Path to use */
  RAIL_AntennaSel_t ant0Loc;
  /** Map internal default path to ant0Loc */
  #define defaultPath ant0Loc
  /** Antenna 0 output GPIO port */
  uint8_t ant0Port;
  /** Antenna 0 output GPIO pin */
  uint8_t ant0Pin;
  /** Antenna 1 internal RF Path to use */
  RAIL_AntennaSel_t ant1Loc;
  /** Antenna 1 output GPIO port */
  uint8_t ant1Port;
  /** Antenna 1 output GPIO pin */
  uint8_t ant1Pin;
} RAIL_AntennaConfig_t;

/** @} */ // end of group Antenna_Control_EFR32

/******************************************************************************
 * Calibration Structures
 *****************************************************************************/
/**
 * @addtogroup Calibration
 * @{
 */

/// Use this value with either TX or RX values in RAIL_SetPaCTune
/// to use whatever value is already set and do no update. This
/// value is provided to provide consistency across EFR32 chips,
/// but technically speaking, all PA capacitance tuning values are
/// invalid on EFR32XG21 parts, as RAIL_SetPaCTune is not supported
/// on those parts.
#define RAIL_PACTUNE_IGNORE (255U)

/** @} */ // end of group Calibration

/******************************************************************************
 * RX Channel Hopping
 *****************************************************************************/
/**
 * @addtogroup Rx_Channel_Hopping RX Channel Hopping
 * @{
 */

/// The static amount of memory needed per channel for channel
/// hopping, regardless of the size of radio configuration structures.
#define RAIL_CHANNEL_HOPPING_BUFFER_SIZE_PER_CHANNEL (23U)

/** @} */  // end of group Rx_Channel_Hopping

/// Fixed-width type indicating the needed alignment for RX and TX FIFOs. Note
/// that docs.silabs.com will incorrectly indicate that this is always a
/// uint8_t, but it does vary across RAIL platforms.
#if _SILICON_LABS_32B_SERIES_2_CONFIG >= 2
#define RAIL_FIFO_ALIGNMENT_TYPE uint32_t
#else
#define RAIL_FIFO_ALIGNMENT_TYPE uint8_t
#endif

/// Alignment that is needed for the RX and TX FIFOs.
#define RAIL_FIFO_ALIGNMENT (sizeof(RAIL_FIFO_ALIGNMENT_TYPE))

#ifdef __cplusplus
}
#endif

#endif
