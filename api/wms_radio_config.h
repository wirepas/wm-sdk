/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file wms_radio_config.h
 *
 * Application library for radio power and front end module control.
 *
 * Library services are accessed  via @ref app_lib_radio_cfg_t "lib_radio_cfg"
 * handle.
 *
 */

#ifndef APP_LIB_RADIO_CONFIG_H_
#define APP_LIB_RADIO_CONFIG_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "wms_app.h"

/** @brief Library symbolic name */
#define APP_LIB_RADIO_CFG_NAME              0x6e080bb7 //!< "RADCFG"

/** @brief Maximum supported library version */
#define APP_LIB_RADIO_CFG_VERSION           0x200

/** @brief Maximum amount of configurable power levels */
#define APP_LIB_RADIO_CFG_POWER_MAX_CNT     10

/** @brief Minimum configured radio current (10 x mA) */
#define APP_LIB_RADIO_CFG_CURRENT_MIN       1

/**
 * @brief Maximum configured radio current (10 x mA) 
 *
 * FCC allow maximum power for 2.4GHz to be roughly 600mA, so to be sure that
 * we don't block the users from doing what they want, but also have some
 * reasonable limit to root out obviously invalid values, we set the upper
 * limit for current to be 10A.
 */
#define APP_LIB_RADIO_CFG_CURRENT_MAX       1000

/**
 * @brief  FEM control command from firmware to application
 *
 * Used as parameter for @ref app_lib_radio_cfg_fem_cmd_cb_f
 *
 * @note FEM driver may not need to to implement these literally. It is
 * often possible to use lazy strategies.
 *
 * Example: If power consumption is irrelevant, FEM may be kept in RX
 * state by default, and in TX state only when @ref APP_LIB_RADIO_CFG_FEM_TX_ON
 * is asked.
 *
 * Example: FEM may not have true power off / power on states but
 * only some kind of low-current standby (non-active) state.
 */
typedef enum
{
    /** FEM receiver on */
    APP_LIB_RADIO_CFG_FEM_RX_ON,
    /** FEM transmitter on */
    APP_LIB_RADIO_CFG_FEM_TX_ON,
    /** FEM low power sleep mode (STANDBY / SBY) */
    APP_LIB_RADIO_CFG_FEM_STANDBY,
    /** FEM power on, this state is mostly intermediate between OFF and SBY */
    APP_LIB_RADIO_CFG_FEM_PWR_ON,
    /** FEM power off */
    APP_LIB_RADIO_CFG_FEM_PWR_OFF,
} app_lib_radio_cfg_femcmd_e;

/**
 * @brief  FEM state transition delays
 *
 * Discrete delays between between different FEM states. The stack needs to know
 * these values in order to command the FEM to correct state so it is ready and
 * stabilized when a TX/RX operation begins.
 *
 * Typically these values can all be set to 0, if the FEM stabilization can be
 * assumed instantaneous (under a few microseconds).
 *
 * However, if the FEM PA/LNA takes a significant amount of time to stabilize
 * (in the order of tens of microseconds) then the application _must_ provide
 * the delay values here, otherwise the FEM PA/LNA will not be stable when TX/RX
 * begins, and the RF performance will suffer.
 */
typedef struct
{
    /** Delay when moving from @ref APP_LIB_RADIO_CFG_FEM_PWR_OFF state to
     *  @ref APP_LIB_RADIO_CFG_FEM_STANDBY state. The meaning of the value is
     *  to estimate how long it takes to move from the lowest power FEM state
     *  to a state where the FEM is ready to enter TX or RX state. The stack
     *  uses @ref APP_LIB_RADIO_CFG_FEM_PWR_ON only as an intermediate state. */
    uint32_t pd_to_sby;
    /** Delay when moving from @ref APP_LIB_RADIO_CFG_FEM_STANDBY to
     *  @ref APP_LIB_RADIO_CFG_FEM_TX_ON, that is how much time the FEM PA
     *  power output needs to stabilize. The stack will command the FEM to enter
     *  TX state at least this amount of time prior to commanding the radio to
     *  TX state */
    uint32_t sby_to_tx;
    /** Delay when moving from @ref APP_LIB_RADIO_CFG_FEM_STANDBY to
     *  @ref APP_LIB_RADIO_CFG_FEM_RX_ON, that is how much time the FEM LNA
     *  power output needs to stabilize. The stack will command the FEM to enter
     *  RX state at least this amount of time prior to commanding the radio to
     *  RX state */
    uint32_t sby_to_rx;
    /** Set this to true, if values are valid. If this is false, stack will
     *  assume FEM is fast enough to not care about state transition delays */
    bool delay_values_set;
} app_lib_radio_cfg_fem_timings_t;

/**
 * @brief   Callback function to set radio TX power.
 *
 * This is called by firmware to tell the next TX power level.
 * Never called when the TX is already active.
 * Must not activate TX state.
 * Sets power level of FEM PA (immediately or when TX actually starts).
 * Can be empty function if nothing to do (no PA or fixed PA).
 * It is set via @ref app_lib_radio_cfg_t "lib_radiocfg->femSetup()" in
 * @ref app_lib_radio_cfg_fem_t.setPower "setPower" argument
 *
 * @note    Firmware sets the radio power level. Rationale: In some systems it
 *          would be impossible to access the internal radio from here.
 *
 * @param   Power level 0...@ref APP_LIB_RADIO_CFG_POWER_MAX_CNT.
 *          Used as index for power configuration structures.
 */
typedef void (*app_lib_radio_cfg_fem_set_power_cb_f)(uint8_t power_index);

/**
 * @brief   Callback function to control FEM state.
 *
 * This should be fast and not use other services.
 * In some cases may be called from interrupt context.
 * Customers are responsible for actual control strategy.
 * Can be empty function if nothing to do.
 * It is set via @ref app_lib_radio_cfg_t "lib_radiocfg->femSetup()" in
 * @ref app_lib_radio_cfg_fem_t.femCmd "femCmd" argument
 *
 * @param   femcmd
 *          Command to set FEM state, @ref app_lib_radio_cfg_femcmd_e
 *          For future compatibility, do nothing if command is not recognized.
 */
typedef void (*app_lib_radio_cfg_fem_cmd_cb_f)(uint8_t femcmd);

/**
 * @brief  Radio FEM configuration structure from application to firmware.
 *
 * Address given as a parameter to femSetup.
 */
typedef struct
{
    /** Callback from firmware to application */
    app_lib_radio_cfg_fem_set_power_cb_f    setPower;
    /** Callback from firmware to application */
    app_lib_radio_cfg_fem_cmd_cb_f          femCmd;
    /** FEM timings for stack, @ref app_lib_radio_cfg_fem_timings_t */
    app_lib_radio_cfg_fem_timings_t         femTimings;
} app_lib_radio_cfg_fem_t;

/**
 * @brief  Definition for single TX power level
 *
 * Information needed by firmware to correctly configure TX a power level on
 * the radio, and calculate e.g. relative energy usage from TX power level
 * power consumption.
 *
 * Used in @ref app_lib_radio_cfg_power_t.powers[]-table.
 */
typedef struct
{
    /** Raw power setting to radio register. Used by firmware. */
    uint32_t    tx_power_raw;
    /** Nominal output power , nearest integer dBm */
    int8_t      tx_output_dbm;
    /** 1: value set for index, 0: index empty */
    uint8_t     value_set;
    /** Transmit current, unit [mA x 10] */
    uint16_t    tx_current;
} app_lib_radio_cfg_tx_pwr_lvl_t;

/**
 * @brief  Radio FEM configuration structure from application to firmware.
 *
 * This is given as a parameter for
 * @ref app_lib_radio_cfg_t "lib_radiocfg->powerSetup()" in
 * @ref app_lib_radio_cfg_power_setup_f "power_cfg" argument.
 * All tables are indexed by power level.
 * All fields must be filled realistically.
 */
typedef struct
{
    /** RX state current, unit [mA x 10] */
    uint16_t                        rx_current;
    /** RX LNA gain or 0 [dB] */
    int8_t                          rx_gain_db;
    /** Amount of power levels configured */
    uint8_t                         power_count;
    /** TX power level configuration / table.
     *  Requirements:
     *  - At least 1 power level must be set, recommendation is to use at least
     *    8 power levels
     *  - Power levels MUST be in ascending order
     */
    app_lib_radio_cfg_tx_pwr_lvl_t  powers[APP_LIB_RADIO_CFG_POWER_MAX_CNT];
} app_lib_radio_cfg_power_t;

/**
 * @brief   Setup FEM control.
 * @param   fem_cfg
 *          Pointer to configuration (defined in application)
 * @return  Result code, @ref APP_RES_OK if successful
 */
typedef app_res_e (*app_lib_radio_cfg_fem_setup_f)
    (const app_lib_radio_cfg_fem_t * fem_cfg);

/**
 * @brief   Setup radio configuration / power level settings.
 * @param   power_cfg
 *          Pointer to configuration (defined in application)
 * @return  Result code, @ref APP_RES_OK if successful
 */
typedef app_res_e (*app_lib_radio_cfg_power_setup_f)
    (const app_lib_radio_cfg_power_t * power_cfg);

/**
 * @brief   Setup radio PA configuration, relevant only for platforms that
 *          require PA configuration (e.g. SiLabs)
 * @param   pa_cfg
 *          Pointer to configuration defined by Vendor SDK. Refer to the Vendor
 *          SDK for type declarations. The stack assumes the format is what
 *          the silicon vendor uses to configure PA.
 *          As an example, for SiLabs the type is: RAIL_TxPowerConfig_t
 * @return  Result code, @ref APP_RES_OK if successful
 */
typedef app_res_e (*app_lib_radio_cfg_pa_setup_f) (const void * pa_cfg);

/**
 * @brief   List of library functions
 */
typedef struct
{
    /** Setup FEM control, works only when stack is in
     *  @ref APP_LIB_STATE_STOPPED state*/
    app_lib_radio_cfg_fem_setup_f   femSetup;
    /** Setup custom radio power table, works only when stack is in
     *  @ref APP_LIB_STATE_STOPPED state */
    app_lib_radio_cfg_power_setup_f powerSetup;
    /** Setup radio PA configuration, works only when stack is in
     *  @ref APP_LIB_STATE_STOPPED state. This is relevant only on platforms
     *  that require explicit radio PA configuration (e.g. SiLabs) */
    app_lib_radio_cfg_pa_setup_f    paSetup;
} app_lib_radio_cfg_t;

#endif /* APP_LIB_RADIO_CONFIG_H_ */
