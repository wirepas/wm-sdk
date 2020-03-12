/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * Reference FEM driver.
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
#include "fem_driver.h"
#include "board.h"

//-------------------------- DEBUG ONLY ---------------------------------------
//#define DEBUG_FEM_DRIVER
#if defined(DEBUG_FEM_DRIVER)
#define FEM_GREEN_LED     17
#define FEM_BLUE_LED      19
#define FEM_RED_LED       25
#define SET_RED_LED       nrf_gpio_pin_clear(FEM_RED_LED)
#define CLEAR_RED_LED     nrf_gpio_pin_set(FEM_RED_LED)
#define SET_GREEN_LED     nrf_gpio_pin_clear(FEM_GREEN_LED)
#define CLEAR_GREEN_LED   nrf_gpio_pin_set(FEM_GREEN_LED)
#define SET_BLUE_LED      nrf_gpio_pin_clear(FEM_BLUE_LED)
#define CLEAR_BLUE_LED    nrf_gpio_pin_set(FEM_BLUE_LED)
#else
#define SET_RED_LED
#define CLEAR_RED_LED
#define SET_GREEN_LED
#define CLEAR_GREEN_LED
#define SET_BLUE_LED
#define CLEAR_BLUE_LED
#endif
//-----------------------------------------------------------------------------

// FEM pin definitions. Ideally should be located in board.h
// (SKY66112-11)
// CSD:
#define FEM_PIN_ENA     28
// CPS:
#define FEM_PIN_BYPASS  31
// CRX:
#define FEM_PIN_RX      27
// CTX:
#define FEM_PIN_TX      30
// CHL:
#define FEM_PIN_HIPO    29
// ANT2:
#define FEM_PIN_ANT2    26

typedef enum
{
    FEM_TX_LOWPOWER,
    FEM_TX_HIPOWER,
    FEM_TX_BYPASS,
} fem_tx_power_state_e;

// --------------- FEM configuration ------------------------------------------
// Note: all tables are indexed by power level value (0 is lowest).

// Nominal TX output power, nearest integer [dBm]. Info for firmware.
static const int8_t   dbm_tx_output[APP_LIB_RADIO_FEM_POWER_MAX+1] =
{
    -18, -10, -2, +2, +7, +11, +15, +19
/*    0    1   2   3   4    5    6    7 */
};

// Radio power setting register values. Set by firmware:
static const int8_t  dbm_radio_raw[APP_LIB_RADIO_FEM_POWER_MAX+1] =
{
    RADIO_TXPOWER_TXPOWER_Neg16dBm,  /* 0 */
    RADIO_TXPOWER_TXPOWER_Neg8dBm,   /* 1 */
    RADIO_TXPOWER_TXPOWER_0dBm,      /* 2 */
    RADIO_TXPOWER_TXPOWER_Pos4dBm,   /* 3 */
    RADIO_TXPOWER_TXPOWER_Neg12dBm,  /* 4 */
    RADIO_TXPOWER_TXPOWER_Neg8dBm,   /* 5 */
    RADIO_TXPOWER_TXPOWER_Neg4dBm,   /* 6 */
    RADIO_TXPOWER_TXPOWER_0dBm       /* 7 */
};

// FEM TX PA mode. Set by this driver.
// This info is internal for this driver, not part of firmware configuration.
static const fem_tx_power_state_e fem_tx_pa[APP_LIB_RADIO_FEM_POWER_MAX+1] =
{
        FEM_TX_BYPASS,   /* 0 */
        FEM_TX_BYPASS,   /* 1 */
        FEM_TX_BYPASS,   /* 2 */
        FEM_TX_BYPASS,   /* 3 */
        FEM_TX_HIPOWER,  /* 4 */
        FEM_TX_HIPOWER,  /* 5 */
        FEM_TX_HIPOWER,  /* 6 */
        FEM_TX_HIPOWER,  /* 7 */
};

// Node current consumption during TX burst [mA * 10]. Needed by firmware.
// Please use realistic values. May have effect to node behavior.
static const uint16_t tx_current[APP_LIB_RADIO_FEM_POWER_MAX+1] =
{
    96, 107, 141, 192, 287, 379, 641, 1210
/*   0   1    2    3    4    5    6    7 */
};

// RX LNA gain estimate. Signed value [dB]. Needed by firmware.
// System only supports fixed gain LNA (always on during RX).
static const int8_t dBm_fem_rx_gain = 11;

// Node current consumption estimate for RX state [mA * 10]. Needed by firmware.
// Please use realistic value. May have effect to node behavior.
static const int8_t rx_current = 95;
//-----------------------------------------------------------------------------

/** Configuration structure from driver to firmware. Mandatory. */
static app_lib_radio_fem_config_t m_femcfg;

/** Latest asked TX power level from FW. Used as index to power tables. */
static uint8_t m_fem_power;

/** Latest command from FW. This is stored mainly for debugging. */
static app_lib_radio_fem_femcmd_e m_femcmd;

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
static void fem_cmd_cb(app_lib_radio_fem_femcmd_e femcmd)
{
    m_femcmd = femcmd;
    switch(femcmd)
    {
        case APP_LIB_RADIO_FEM_RX_ON:
            SET_GREEN_LED;
            // go to rx state with LNA on:
            nrf_gpio_pin_clear(FEM_PIN_TX);
            nrf_gpio_pin_clear(FEM_PIN_BYPASS);
            nrf_gpio_pin_set(FEM_PIN_RX);
            nrf_gpio_pin_set(FEM_PIN_ENA);
            break;
        case APP_LIB_RADIO_FEM_TX_ON:
            SET_RED_LED;
            // go to tx mode:
            nrf_gpio_pin_clear(FEM_PIN_RX);
            switch (fem_tx_pa[m_fem_power])
            {
                case FEM_TX_LOWPOWER:
                    nrf_gpio_pin_clear(FEM_PIN_BYPASS);
                    nrf_gpio_pin_clear(FEM_PIN_HIPO);
                    break;
                case FEM_TX_HIPOWER:
                    nrf_gpio_pin_clear(FEM_PIN_BYPASS);
                    nrf_gpio_pin_set(FEM_PIN_HIPO);
                    break;
                case FEM_TX_BYPASS:
                default:
                    nrf_gpio_pin_clear(FEM_PIN_HIPO);
                    nrf_gpio_pin_set(FEM_PIN_BYPASS);
                    break;
            }
            nrf_gpio_pin_set(FEM_PIN_TX);
            nrf_gpio_pin_set(FEM_PIN_ENA);
            break;
        case APP_LIB_RADIO_FEM_STANDBY:
            CLEAR_RED_LED;
            CLEAR_GREEN_LED;
            // selected strategy: lowest power, sleep when possible.
            nrf_gpio_pin_set(FEM_PIN_RX);
            nrf_gpio_pin_clear(FEM_PIN_ENA);
            break;
        case APP_LIB_RADIO_FEM_PWR_ON:
            CLEAR_BLUE_LED;
            // set RX but do not enable FEM yet:
            nrf_gpio_pin_clear(FEM_PIN_TX);
            nrf_gpio_pin_clear(FEM_PIN_BYPASS);
            nrf_gpio_pin_set(FEM_PIN_RX);

            break;
        case APP_LIB_RADIO_FEM_PWR_OFF:
            CLEAR_RED_LED;
            CLEAR_GREEN_LED;
            SET_BLUE_LED;
            // put FEM to low energy state:
            nrf_gpio_pin_clear(FEM_PIN_TX);
            nrf_gpio_pin_clear(FEM_PIN_RX);
            nrf_gpio_pin_clear(FEM_PIN_ENA);
            break;
        default:
            // Do nothing if command not identified:
            break;
    }
    // Wait for FEM settling here, max 10...15us.
    // Only needed if firmware wait 2us is not enough.
    return;
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
 * \param   Power level 0...APP_LIB_RADIO_FEM_POWER_MAX.
 *          Used as index for power configuration structures.
 */
static void fem_set_power_cb(uint8_t power)
{
    if (power > APP_LIB_RADIO_FEM_POWER_MAX)
    {
        power = APP_LIB_RADIO_FEM_POWER_MAX;
    }
    m_fem_power = power; // Store for later use (see APP_LIB_RADIO_FEM_TX_ON).

    // (Could do FEM power setting here if it could be done in advance.)

    return;
}

/**
 *  \brief  Public interface. Initialize and register FEM driver.
 *          Call exactly once, from App_init().
 */
void Fem_init()
{
    app_res_e fem_status;
    m_fem_power = 0;  // (index 0 is the lowest possible power value)

#if defined(DEBUG_FEM_DRIVER)
    nrf_gpio_cfg_output(FEM_RED_LED);
    nrf_gpio_cfg_output(FEM_GREEN_LED);
    nrf_gpio_cfg_output(FEM_BLUE_LED);
    CLEAR_RED_LED;
    CLEAR_GREEN_LED;
    CLEAR_BLUE_LED;
#endif

    // Initialize FEM control pins (SKY66112-11).
    // Safe initial state:
    nrf_gpio_cfg_output(FEM_PIN_ENA);
    nrf_gpio_pin_clear(FEM_PIN_ENA);      // FEM not enabled

    nrf_gpio_cfg_output(FEM_PIN_BYPASS);
    nrf_gpio_pin_clear(FEM_PIN_BYPASS);   // no bypass

    nrf_gpio_cfg_output(FEM_PIN_RX);
    nrf_gpio_pin_clear(FEM_PIN_RX);       // rx off

    nrf_gpio_cfg_output(FEM_PIN_TX);
    nrf_gpio_pin_clear(FEM_PIN_TX);       // tx off

    nrf_gpio_cfg_output(FEM_PIN_HIPO);
    nrf_gpio_pin_clear(FEM_PIN_HIPO);     // no high-power

    nrf_gpio_cfg_output(FEM_PIN_ANT2);
    //nrf_gpio_pin_clear(FEM_PIN_ANT2);     // Antenna 1
    nrf_gpio_pin_set(FEM_PIN_ANT2);       // Antenna 2


    // Prepare the FEM configuration to the m_femcfg structure.
    // Set callbacks:
    m_femcfg.setPower = fem_set_power_cb;
    m_femcfg.femCmd = fem_cmd_cb;
    // Set data:
    memcpy(&m_femcfg.dbm_tx_output, &dbm_tx_output, sizeof(dbm_tx_output));
    memcpy(&m_femcfg.dbm_radio_raw, &dbm_radio_raw, sizeof(dbm_radio_raw));
    memcpy(&m_femcfg.tx_current, &tx_current, sizeof(tx_current));
    m_femcfg.dBm_fem_rx_gain = dBm_fem_rx_gain;
    m_femcfg.rx_current = rx_current;
    m_femcfg.pa_voltage_mv = 3300; // (relevant for some platforms)

    // Assume that lib_radio_fem is already opened ok (or open here).
    // Setup the firmware to use this implementation of FEM control:
    fem_status = lib_radio_fem->setupFunc(&m_femcfg);
    if (fem_status != APP_RES_OK)
    {
        // This will crash the SW but it is not possible to continue.
        while (1);
    }
}
