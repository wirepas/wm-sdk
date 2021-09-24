/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * Reference FEM driver for nRF21540 FEM.
 *
 * radio FEM == Front End Module (aka AFE == Analog Front End).
 * Can be used as template for custom implementation.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include <stdlib.h>
#include <string.h>
#include "mcu.h"
#include "api.h"
#include "board.h"

// --------------- Radio configuration -----------------------------------------
// This example sets up the FEM with +10dB PA gain, +13 dB LNA gain, for use
// with the 2.4GHz ISM band in Europe and areas where +10dBm maximum output
// power limit applies. The resulting maximum output power (ERP) is ~~ +10dBm
// with this configuration
// Note: The PA output power gain has a few dB of output power hysteresis, and
// this configuration can result in an illegal output power level. See the
// nRF21540 Product Specification v1.0, chapter 8.1.4 CONFREG3, which states
// that the default TX gain of +10dB has an error margin of +/- 1.5dB.

// For RADIO_TX_POWER_XXX
#include "mcu/nrf/common/vendor/mdk/nrf52840_bitfields.h"

// +10 dBm power table for nrf52840, with 7 power levels, PA gain +10dB
// With +10dB gain power consumption increases by 38mA
const app_lib_radio_cfg_power_t m_power_table =
{
    .rx_current     = 46 + 29,  // 4.6 mA RX current (+2.9 mA LNA current)
    .rx_gain_dbm    = 13,       // 13 dBm RX gain (from LNA)
    .power_count    = 7,        // 7 power levels
    .powers =
    {
        {RADIO_TXPOWER_TXPOWER_Neg40dBm, -30, 1, 23 + 380},
        {RADIO_TXPOWER_TXPOWER_Neg20dBm, -10, 1, 27 + 380},
        {RADIO_TXPOWER_TXPOWER_Neg16dBm, -6,  1, 28 + 380},
        {RADIO_TXPOWER_TXPOWER_Neg12dBm, -2,  1, 30 + 380},
        {RADIO_TXPOWER_TXPOWER_Neg8dBm,   2,  1, 33 + 380},
        {RADIO_TXPOWER_TXPOWER_Neg4dBm,   6,  1, 37 + 380},
        {RADIO_TXPOWER_TXPOWER_0dBm,      10, 1, 48 + 380},
    },
};

// Maximum power level
#define POWER_LEVEL_MAX     (m_power_table.power_count - 1)

//-----------------------------------------------------------------------------

/** Latest asked TX power level from FW. Used as index to power tables.
 *  NOTE: This example does not utilize this value for anything, as the FEM PA
 *  gain does not change */
static uint8_t m_tx_power_index;

/** Current FEM state */
static volatile uint32_t m_fem_state;

// Code:

/**
 * \brief   Callback function to control FEM state.
 *          This should be fast and not use other services.
 *          In some cases may be called from interrupt context.
 *          Customers are responsible for actual control strategy.
 *          Can be empty function if nothing to do.
 * \param   femcmd
 *          Command to set FEM state.
 *          For future compatibility, do nothing if command is not recognized.
 */
static void fem_cmd_cb(app_lib_radio_cfg_femcmd_e femcmd)
{
    if (m_fem_state == (uint32_t)femcmd)
    {
        // State does not change
        return;
    }

    // Record new state
    m_fem_state = femcmd;

    switch (femcmd)
    {
        case APP_LIB_RADIO_CFG_FEM_STANDBY:
            // Keep the FEM in RX mode, when in "standby", see why this is done
            // below in case APP_LIB_RADIO_CFG_FEM_STANDBY:
        case APP_LIB_RADIO_CFG_FEM_RX_ON:
            // According to datasheet, CSN must be driven low in RX
            nrf_gpio_pin_clear(BOARD_FEM_CS_PIN);
            // go to rx state with LNA on:
            nrf_gpio_pin_clear(BOARD_FEM_TXEN_PIN);
            nrf_gpio_pin_set(BOARD_FEM_RXEN_PIN);
            break;
        case APP_LIB_RADIO_CFG_FEM_TX_ON:
            nrf_gpio_pin_clear(BOARD_FEM_CS_PIN);
            nrf_gpio_pin_clear(BOARD_FEM_RXEN_PIN);
            nrf_gpio_pin_set(BOARD_FEM_TXEN_PIN);
            break;
#if 0 // Entering standby / PG mode messes up the TX/RX timings, might be a chip
      // or chip revision related issue
        case APP_LIB_RADIO_CFG_FEM_STANDBY:
            nrf_gpio_pin_set(BOARD_FEM_CS_PIN);
            // Remain in program state
            nrf_gpio_pin_clear(BOARD_FEM_RXEN_PIN);
            nrf_gpio_pin_clear(BOARD_FEM_TXEN_PIN);
            nrf_gpio_pin_set(BOARD_FEM_PDN_PIN);
            break;
#endif
        case APP_LIB_RADIO_CFG_FEM_PWR_ON:
            nrf_gpio_pin_set(BOARD_FEM_CS_PIN);
            // Remain in program state
            nrf_gpio_pin_clear(BOARD_FEM_RXEN_PIN);
            nrf_gpio_pin_clear(BOARD_FEM_TXEN_PIN);
            nrf_gpio_pin_set(BOARD_FEM_PDN_PIN);
            break;
        case APP_LIB_RADIO_CFG_FEM_PWR_OFF:
            nrf_gpio_pin_set(BOARD_FEM_CS_PIN);
            nrf_gpio_pin_clear(BOARD_FEM_RXEN_PIN);
            nrf_gpio_pin_clear(BOARD_FEM_TXEN_PIN);
            nrf_gpio_pin_clear(BOARD_FEM_PDN_PIN);
            break;
        default:
            // Do nothing if command not identified:
            break;
    }
}

/**
 * \brief   Callback function to set radio TX power.
 *          This is called by firmware to tell the next TX power level.
 *          Never called when the TX is already active.
 *          Must not activate TX state.
 *          Sets power level of FEM PA (immediately or when TX actually starts).
 *          Can be empty function if nothing to do (no PA or fixed PA).
 * \note    Firmware sets the radio power level. Rationale: In some systems it
 *          would be impossible to access the internal radio from here.
 *
 * \param   Power level 0...POWER_LEVEL_MAX.
 *          Used as index for power configuration structures.
 */
static void fem_set_power_cb(uint8_t power)
{
    if (power > POWER_LEVEL_MAX)
    {
        power = POWER_LEVEL_MAX;
    }

    // Store for later use (see APP_LIB_RADIO_CFG_FEM_TX_ON).
    m_tx_power_index = power;
}

/**
 *  \brief  Public interface. Initialize and register FEM driver.
 *          Call exactly once, from App_init().
 */
void Fem_init(void)
{
    m_tx_power_index = 0;  // (index 0 is the lowest possible power value)
    m_fem_state = APP_LIB_RADIO_CFG_FEM_PWR_OFF;

    // Initialize FEM control pins (nrf21540).
    // Safe initial state:
    nrf_gpio_cfg_output(BOARD_FEM_PDN_PIN);
    nrf_gpio_pin_clear(BOARD_FEM_PDN_PIN);      // FEM disabled

    nrf_gpio_cfg_output(BOARD_FEM_RXEN_PIN);
    nrf_gpio_pin_clear(BOARD_FEM_RXEN_PIN);     // RX off

    nrf_gpio_cfg_output(BOARD_FEM_TXEN_PIN);
    nrf_gpio_pin_clear(BOARD_FEM_TXEN_PIN);     // TX off

    nrf_gpio_cfg_output(BOARD_FEM_ANTSEL_PIN);
    nrf_gpio_pin_clear(BOARD_FEM_ANTSEL_PIN);   // Antenna 1

    // DO _NOT_ touch MODE pin afterwards, otherwise the FEM will load a
    // "default" value from POUTX_UICR or POUTX_UICR
    nrf_gpio_cfg_output(BOARD_FEM_MODE_PIN);    // Load default value once
    nrf_gpio_pin_set(BOARD_FEM_MODE_PIN);       // 0: +20dB 1: +10dB

    // Put all pins of spi to low values
    nrf_gpio_cfg_output(BOARD_FEM_SPI_MOSI_PIN);
    nrf_gpio_pin_clear(BOARD_FEM_SPI_MOSI_PIN);
    nrf_gpio_cfg_output(BOARD_FEM_SPI_CLK_PIN);
    nrf_gpio_pin_clear(BOARD_FEM_SPI_CLK_PIN);
    nrf_gpio_cfg_output(BOARD_FEM_SPI_MISO_PIN);
    nrf_gpio_pin_clear(BOARD_FEM_SPI_MISO_PIN);
    // Slave select has to be de-asserted
    nrf_gpio_cfg_output(BOARD_FEM_CS_PIN);
    nrf_gpio_pin_set(BOARD_FEM_CS_PIN);

    // Prepare the FEM configuration to the femcfg structure.
    // Set callbacks:
    app_lib_radio_cfg_fem_t femcfg =
    {
        .setPower = fem_set_power_cb,
        .femCmd = fem_cmd_cb,
        // Delays from nRF21540 PS1.0 Table 6
        .femTimings =
        {
            .pd_to_sby = 18, // 17.5
            .sby_to_tx = 11, // 10.5
            .sby_to_rx = 11, // 10.5
            .delay_values_set = true,
        },
    };

    app_res_e status = lib_radio_cfg->femSetup(&femcfg);
    if (status != APP_RES_OK)
    {
        // This will crash the SW but it is not possible to continue.
        while (1);
    }

    // Write radio configuration
    status = lib_radio_cfg->powerSetup(&m_power_table);
    if (status != APP_RES_OK)
    {
        // This will crash the SW but it is not possible to continue.
        while (1);
    }

    // Otherwise, we are done
}
