/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifndef EXTERNAL_FLASH_MX25R6435F_H_
#define EXTERNAL_FLASH_MX25R6435F_H_

// MX25R6435F chip identification.
#define MX25R6435F_ID_1 0xC2
#define MX25R6435F_ID_2 0x28
#define MX25R6435F_ID_3 0x17

// MX25R6435F Status register.
typedef enum
{
    MX25R6435F_STATUS_WIP = 0x01, // Write In Progress = Busy
    MX25R6435F_STATUS_WEL = 0x02, // Write Enable Latch
} ext_flash_status_t;

// MX25R6435F SECURITY register.
typedef enum
{
    MX25R6435F_SECURITY_P_FAIL = 0x20, // Programming failed
    MX25R6435F_SECURITY_E_FAIL = 0x40, // Erase failed
} ext_flash_security_t;

// MX25R6435F Maximum transfer size.
// Limited by Nordic SPI master to 255 bytes per SPI DMA transaction.
#define MX25R6435F_MAX_TRANSFER_SIZE 0xff

// SPI commands
enum {
    MX25R6435F_CMD_WRITE_STATUS        = 0x01,
    MX25R6435F_CMD_PROGRAM_PAGE        = 0x02,
    MX25R6435F_CMD_READ_ARRAY          = 0x03,
    MX25R6435F_CMD_WRITE_DISABLE       = 0x04,
    MX25R6435F_CMD_READ_STATUS         = 0x05,
    MX25R6435F_CMD_WRITE_ENABLE        = 0x06,
    MX25R6435F_CMD_SECTOR_ERASE        = 0x20,
    MX25R6435F_CMD_READ_SECURITY       = 0x2B,
    MX25R6435F_CMD_BLOCK_ERASE_32K     = 0x52,
    MX25R6435F_CMD_READ_IDENTIFICATION = 0x9F,
} ext_flash_cmd_t;

#endif // EXTERNAL_FLASH_MX25R6435F_H_
