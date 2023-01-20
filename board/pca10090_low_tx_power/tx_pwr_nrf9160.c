/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * TX power adjustments for nrf9160
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include <stdlib.h>
#include <string.h>
#include "mcu.h"
#include "api.h"
#include "board.h"

// ETSI TS 103 636-4
#define RADIO_TXPOWER_Neg40dBm    0b0000
#define RADIO_TXPOWER_Neg30dBm    0b0001
#define RADIO_TXPOWER_Neg20dBm	  0b0010
#define RADIO_TXPOWER_Neg13dBm    0b0011
#define RADIO_TXPOWER_Neg6dBm     0b0100
#define RADIO_TXPOWER_Neg3dBm     0b0101
#define RADIO_TXPOWER_0dBm        0b0110
#define RADIO_TXPOWER_3dBm        0b0111
#define RADIO_TXPOWER_6dBm        0b1000


const app_lib_radio_cfg_power_t m_power_table =
{
    .rx_current     = 450,  // 45 mA RX current
    .rx_gain_db     = 0,    // 0 dB RX gain
    .power_count    = 8,    // 8 power levels
    .powers =
    {
        {RADIO_TXPOWER_Neg40dBm, -40,  1, 500},
        {RADIO_TXPOWER_Neg30dBm, -30,  1, 525},
        {RADIO_TXPOWER_Neg20dBm, -20,  1, 550},
        {RADIO_TXPOWER_Neg13dBm, -13,  1, 575},
        {RADIO_TXPOWER_Neg6dBm,   -6,  1, 600},
        {RADIO_TXPOWER_Neg3dBm,   -3,  1, 625},
        {RADIO_TXPOWER_0dBm,       0,  1, 650},
        {RADIO_TXPOWER_3dBm,       3,  1, 700},
    },
};

// Maximum power level
#define POWER_LEVEL_MAX     (m_power_table.power_count - 1)

//-----------------------------------------------------------------------------

/** Latest asked TX power level from FW. Used as index to power tables.
 */
static uint8_t m_tx_power_index;


// Code:

/**
 * \brief   Callback function to set radio TX power.
 *          This is called by firmware to tell the next TX power level.
 * \param   Power level 0...POWER_LEVEL_MAX.
 *          Used as index for power configuration structures.
 */
static void set_power_cb(uint8_t power)
{
    if (power > POWER_LEVEL_MAX)
    {
        power = POWER_LEVEL_MAX;
    }

    m_tx_power_index = power;

}

/**
 *  \brief  Initialize/overwrite stack's default power levels.
 */
void tx_pwr_init(void)
{
    m_tx_power_index = 0;  // (index 0 is the lowest possible power value)

    // femcfg defined to set callback (set_power_cb) for power level changes
    // even the board do not have external FEM.
    app_lib_radio_cfg_fem_t femcfg =
    {
        .setPower = set_power_cb,
        .femCmd = NULL,
        .femTimings =
        {
            .delay_values_set = false,
        },
    };

    app_res_e status = lib_radio_cfg->femSetup(&femcfg);
    if (status != APP_RES_OK)
    {
        // This will crash the SW but it is not possible to continue.
        while (1);
    }

    status = lib_radio_cfg->powerSetup(&m_power_table);
    if (status != APP_RES_OK)
    {
        // This will crash the SW but it is not possible to continue.
        while (1);
    }
}
