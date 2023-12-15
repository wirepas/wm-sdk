/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#ifndef EXTERNAL_FLASH_MX25R8035F_H_
#define EXTERNAL_FLASH_MX25R8035F_H_

// MX25R8035F chip identification.
#define MX25_MAN_ID                     0xC2
#define MX25R8035F_MEM_TYPE             0x28
#define MX25R8035F_MEM_DENSITY          0x14

// MX25R8035F Status register
// Write In Progress = Busy
#define MX25_STATUS_WIP                 0x01
// Write Enable Latch
#define MX25_STATUS_WEL                 0x02

// MX25 commands
#define MX25_CMD_WRITE_STATUS           0x01
#define MX25_CMD_PROGRAM_PAGE           0x02
#define MX25_CMD_READ_ARRAY             0x03
#define MX25_CMD_WRITE_DISABLE          0x04
#define MX25_CMD_READ_STATUS            0x05
#define MX25_CMD_WRITE_ENABLE           0x06
#define MX25_CMD_SECTOR_ERASE           0x20
#define MX25_CMD_READ_SECURITY          0x2B
#define MX25_CMD_BLOCK_ERASE_32K        0x52
#define MX25_CMD_READ_IDENTIFICATION    0x9F
#define MX25_CMD_RES                    0xAB

#endif // EXTERNAL_FLASH_MX25R8035F_H_
