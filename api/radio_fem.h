/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file radio_fem.h
 *
 * Application library for radio power and front end module control.
 *
 * Library services are accessed  via @ref app_lib_radio_fem_t "lib_fem"
 * handle.
 *
 * @note This is experimental feature. If you are interested using this, please
 * contact <a href="https://support.wirepas.com/">Wirepas support</a>.
 *
 * @note This library is currently available for Nordic nRF52 only.
 */
#ifndef APP_LIB_RADIO_FEM_H_
#define APP_LIB_RADIO_FEM_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "app/app.h"

/** @brief Library symbolic name */
#define APP_LIB_RADIO_FEM_NAME 0x6e081e55 //!< "RADFEM"

/** @brief Maximum supported library version */
#define APP_LIB_RADIO_FEM_VERSION 0x200

/** @brief Firmware utilizes (APP_LIB_RADIO_FEM_POWER_MAX + 1) power levels */
#define APP_LIB_RADIO_FEM_POWER_MAX 7

/**
 * @brief  FEM control command from firmware to application
 *
 * Used as parameter for @ref app_lib_radio_fem_femcmd_cb_f
 *
 * @note FEM driver may not need to to implement these literally. It is
 * often possible to use lazy strategies.
 *
 * Example: If power consumption is irrelevant, FEM may be kept in RX
 * state by default, and in TX state only when APP_LIB_RADIO_FEM_TX_ON
 * is asked.
 *
 * Example: FEM may not have true power off / power on states but
 * only some kind of low-current standby (non-active) state.
 */
typedef enum
{
    /** FEM receiver on */
    APP_LIB_RADIO_FEM_RX_ON,
    /** FEM transmitter on */
    APP_LIB_RADIO_FEM_TX_ON,
    /** FEM low power sleep mode */
    APP_LIB_RADIO_FEM_STANDBY,
    /** FEM power on */
    APP_LIB_RADIO_FEM_PWR_ON,
    /** FEM power off */
    APP_LIB_RADIO_FEM_PWR_OFF,
} app_lib_radio_fem_femcmd_e;


/**
 * @brief   Callback function to set radio TX power.
 *
 * This is called by firmware to tell the next TX power level.
 * Never called when the TX is already active.
 * Must not activate TX state.
 * Sets power level of FEM PA (immediately or when TX actually starts).
 * Can be empty function if nothing to do (no PA or fixed PA).
 * It is used here @ref app_lib_radio_fem_config_t.setPower "setPower"
 *
 * @note    Firmware sets the radio power level. Rationale: In some systems it
 *          would be impossible to access the internal radio from here.
 *
 * @param   Power level 0...APP_LIB_RADIO_FEM_POWER_MAX.
 *          Used as index for power configuration structures.
 */
typedef void
    (*app_lib_radio_fem_set_power_cb_f)(uint8_t power);

/**
 * @brief   Callback function to control FEM state.
 *
 * This should be fast and not use other services.
 * In some cases may be called from interrupt context.
 * Customers are responsible for actual control strategy.
 * Can be empty function if nothing to do.
 * It is used here @ref app_lib_radio_fem_config_t.femCmd "femCmd"
 *
 * @param   femcmd
 *          Command to set FEM state.
 *          For future compatibility, do nothing if command is not recognized.
 */
typedef void
    (*app_lib_radio_fem_femcmd_cb_f)(app_lib_radio_fem_femcmd_e femcmd);

/**
 * @brief  Radio FEM configuration structure from application to firmware.
 *
 * Address given as a parameter to setupFunc.
 * All tables are indexed by power level.
 * All fields must be filled realistically.
 */
typedef struct
{
    /** Callback from firmware to application */
    app_lib_radio_fem_set_power_cb_f   setPower;
    /** Callback from firmware to application */
    app_lib_radio_fem_femcmd_cb_f      femCmd;

    uint16_t pa_voltage_mv;   // Radio PA voltage [mV]
    uint16_t rx_current;      // RX state current, unit [mA x 10]

    /** TX power configuration */
    /** Nominal output power , nearest integer dBm */
    int8_t   dbm_tx_output[APP_LIB_RADIO_FEM_POWER_MAX+1];
    /** Raw power setting to radio register. Used by firmware. */
    uint8_t  dbm_radio_raw[APP_LIB_RADIO_FEM_POWER_MAX+1];
    /** Transmit current, unit [mA x 10] */
    uint16_t tx_current[APP_LIB_RADIO_FEM_POWER_MAX+1];

    int8_t   dBm_fem_rx_gain; // RX LNA gain or 0 [dB]
} app_lib_radio_fem_config_t;

/**
 * @brief   Setup and start FEM control.
 * @param   config_ptr
 *          Pointer to configuration (defined in application)
 * @return  Result code, \ref APP_RES_OK if successful
 */
typedef app_res_e
    (*app_lib_radio_fem_setup)(app_lib_radio_fem_config_t * config_ptr);

/**
 * @brief   List of library functions
 */
typedef struct
{
    app_lib_radio_fem_setup      setupFunc;
} app_lib_radio_fem_t;

#endif /* APP_LIB_RADIO_FEM_H_ */
