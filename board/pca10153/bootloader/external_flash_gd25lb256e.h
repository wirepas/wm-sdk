/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifndef EXTERNAL_FLASH_GD25LB256E_H_
#define EXTERNAL_FLASH_GD25LB256E_H_

//GD25LB256E rev 1.9


// GD25LB256E chip identificatio (page 19)
#define GD25LB256E_ID_1 0xC8 //M7-M0
#define GD25LB256E_ID_2 0x67 //ID23-ID16
#define GD25LB256E_ID_3 0x19 //ID15-ID8
#define GD25LB256E_ID_4 0xff //ID7-ID0
// GD25LB256E Status register.
typedef enum
{
    GD25LB256E_STATUS_WIP = 0x01, // Write In Progress = Busy
    GD25LB256E_STATUS_WEL = 0x02, // Write Enable Latch
} ext_flash_status_t;

// GD25LB256E Flag Status register.
typedef enum
{
    GD25LB256E_FLAGSTATUS_ADS  = 0x01, // Current Address Mode
    GD25LB256E_FLAGSTATUS_PTE  = 0x02, // Protection Error bit
    GD25LB256E_FLAGSTATUS_SUS2 = 0x04, // Program Suspend
    GD25LB256E_FLAGSTATUS_PE   = 0x10, // Program Error bit
    GD25LB256E_FLAGSTATUS_EE   = 0x20, // Erase Error bit
    GD25LB256E_FLAGSTATUS_SUS1 = 0x40, // Erase Suspend
    GD25LB256E_FLAGSTATUS_BY   = 0x80, // Busy
} ext_flash_security_t;

// GD25LB256E Maximum transfer size.
// Limited by Nordic SPI master to 255 bytes per SPI DMA transaction.
#define GD25LB256E_MAX_TRANSFER_SIZE 0xff

// SPI commands
enum {
    GD25LB256E_CMD_WRITE_STATUS        = 0x01,
    GD25LB256E_CMD_PROGRAM_PAGE        = 0x02,
    GD25LB256E_CMD_READ_ARRAY          = 0x03,
    GD25LB256E_CMD_WRITE_DISABLE       = 0x04,
    GD25LB256E_CMD_READ_STATUS         = 0x05,
    GD25LB256E_CMD_WRITE_ENABLE        = 0x06,
    GD25LB256E_CMD_SECTOR_ERASE        = 0x20,
    GD25LB256E_CMD_BLOCK_ERASE_32K     = 0x52,
    GD25LB256E_CMD_READ_FLAG_STATUS    = 0x70,
    GD25LB256E_CMD_READ_IDENTIFICATION = 0x9F,
} ext_flash_cmd_t;

#endif // EXTERNAL_FLASH_GD25LB256E_H_
