/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

// Modified by Wirepas to use Wirepas I2C driver

#include <stddef.h>
#include "board.h"

#include "i2c.h"

// I2C address of power management chip
#define PM1300_I2C_ADDRESS          0x6B

// I2C address of wifi power management chip
#define PM6001_I2C_ADDRESS          0x70

#define CHECKERR if (err) { return err;}

static i2c_conf_t m_i2c_conf =
{
    .clock = 100000,
    .pullup = BOARD_I2C_PIN_PULLUP
};

static i2c_res_e pmic_write_reg(uint16_t address, uint8_t value)
{
    uint8_t buf[] = {
        address >> 8,
        address & 0xFF,
        value
    };

    i2c_res_e res = I2C_init(&m_i2c_conf);
    if (res == I2C_RES_OK || res == I2C_RES_ALREADY_INITIALIZED)
    {
        i2c_xfer_t xfer_tx = {
            .address = PM1300_I2C_ADDRESS,
            .write_ptr = buf,
            .write_size = sizeof(buf),
            .read_ptr = NULL,
            .read_size = 0,
            .custom = 0
        };

        res = I2C_transfer(&xfer_tx, NULL);
    }


    I2C_close();

    return res;
}

static i2c_res_e pmic_read_reg(uint16_t address, uint8_t *value)
{
    uint8_t buf[] = {
        address >> 8,
        address & 0xFF,
    };

    i2c_res_e res = I2C_init(&m_i2c_conf);

    if (res == I2C_RES_OK || res == I2C_RES_ALREADY_INITIALIZED)
    {
        i2c_xfer_t xfer_rx = {
            .address = PM1300_I2C_ADDRESS,
            .write_ptr = buf,
            .write_size = sizeof(buf),
            .read_ptr = value,
            .read_size  = 1,
            .custom = 0
        };

        res = I2C_transfer(&xfer_rx, NULL);
    }

    I2C_close();

    return res;
}

static int power_mgmt_init(void)
{
    i2c_res_e err = 0;
    uint8_t reg = 0;

    // disable charger for config
    err = pmic_write_reg(0x0305, 0x03); CHECKERR;

    // set VBUS current limit 500mA
    err = pmic_write_reg(0x0201, 0x00); CHECKERR;
    err = pmic_write_reg(0x0202, 0x00); CHECKERR;
    err = pmic_write_reg(0x0200, 0x01); CHECKERR;

    // set RF switch to BLE by default
    err = pmic_write_reg(0x0601, 0x08); CHECKERR;

    // enable VDD_SENS:
    err = pmic_write_reg(0x0802, 0x01); CHECKERR;

    // let BUCK2 be controlled by GPIO2
    err = pmic_write_reg(0x0602, 0x00); CHECKERR;
    err = pmic_write_reg(0x040C, 0x18); CHECKERR;

    // set bias resistor for 10k NTC
    err = pmic_write_reg(0x050A, 0x01); CHECKERR;
    // set COLD threshold to 0C
    err = pmic_write_reg(0x0310, 0xbb); CHECKERR;
    err = pmic_write_reg(0x0311, 0x01); CHECKERR;
    // set COOL threshold to 10C
    err = pmic_write_reg(0x0312, 0xa4); CHECKERR;
    err = pmic_write_reg(0x0313, 0x02); CHECKERR;
    // set WARM threshold to 45C
    err = pmic_write_reg(0x0314, 0x54); CHECKERR;
    err = pmic_write_reg(0x0315, 0x01); CHECKERR;
    // set HOT threshold to 45C
    err = pmic_write_reg(0x0316, 0x54); CHECKERR;
    err = pmic_write_reg(0x0317, 0x01); CHECKERR;

    // set charging current to 800mA
    err = pmic_write_reg(0x0308, 0xc8); CHECKERR;
    err = pmic_write_reg(0x0309, 0x00); CHECKERR;
    // set charging termination voltage 4.2V
    err = pmic_write_reg(0x030C, 0x08); CHECKERR;
    // enable charger
    err = pmic_write_reg(0x0304, 0x03); CHECKERR;

    err = pmic_read_reg(0x0410, &reg); CHECKERR;

    err = pmic_read_reg(0x0411, &reg); CHECKERR;

    return err;
}

int thingy91v2_board_init(void)
{
    int err;

    err = power_mgmt_init();
    if (err) {
        return err;
    }

    return 0;
}
