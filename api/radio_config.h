/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file radio_config.h
 *
 * Application library for radio power and front end module control.
 *
 * Library services are accessed  via @ref app_lib_radio_cfg_t "lib_radiocfg"
 * handle.
 *
 * @note This is experimental feature. If you are interested using this, please
 * contact <a href="https://support.wirepas.com/">Wirepas support</a>.
 *
 * @note This library is currently available for Nordic nRF52 only.
 */

#ifndef APP_LIB_RADIO_CONFIG_H_
#define APP_LIB_RADIO_CONFIG_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "app/app.h"

/** @brief Library symbolic name */
#define APP_LIB_RADIO_CFG_NAME              0x6e080bb7 //!< "RADCFG"

/** @brief Maximum supported library version */
#define APP_LIB_RADIO_CFG_VERSION           0x200

/** @brief Maximum amount of configurable power levels */
#define APP_LIB_RADIO_CFG_POWER_MAX_CNT     10

/**
 * @brief  FEM control command from firmware to application
 *
 * Used as parameter for @ref app_lib_radio_cfg_fem_cmd_cb_f
 *
 * @note FEM driver may not need to to implement these literally. It is
 * often possible to use lazy strategies.
 *
 * Example: If power consumption is irrelevant, FEM may be kept in RX
 * state by default, and in TX state only when @ref APP_LIB_RADIO_FEM_TX_ON
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
    /** FEM low power sleep mode */
    APP_LIB_RADIO_CFG_FEM_STANDBY,
    /** FEM power on */
    APP_LIB_RADIO_CFG_FEM_PWR_ON,
    /** FEM power off */
    APP_LIB_RADIO_CFG_FEM_PWR_OFF,
} app_lib_radio_cfg_femcmd_e;

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
 * Address given as a parameter to femSetup.
 * All tables are indexed by power level.
 * All fields must be filled realistically.
 */
typedef struct
{
    /** Radio PA voltage [mV] */
    uint16_t                        pa_voltage_mv;
    /** RX state current, unit [mA x 10] */
    uint16_t                        rx_current;
    /** RX LNA gain or 0 [dB] */
    int8_t                          rx_gain_dbm;
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
 * @brief   List of library functions
 */
typedef struct
{
    app_lib_radio_cfg_fem_setup_f   femSetup;
    app_lib_radio_cfg_power_setup_f powerSetup;
} app_lib_radio_cfg_t;

#endif /* APP_LIB_RADIO_CONFIG_H_ */
