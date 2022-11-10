/* Copyright 2022 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file wireshark.h
 *
 * Helper file to print wireshark frame to be displayed in Wireshark UI
 */
#ifndef WIRESHARK_H
#define WIRESHARK_H

#include <stdint.h>
#include <stdarg.h>
#include "api.h"

#ifndef WIRESHARK_UART_BAUDRATE
#define WIRESHARK_UART_BAUDRATE 115200
#endif

void Wireshark_print(
        uint32_t src,
        uint32_t dest,
        uint8_t qos,
        uint8_t src_ep,
        uint8_t dst_ep,
        int8_t rssi,
        uint32_t delay,
        const uint8_t * data,
        size_t len);

void Wireshark_init(void);

#endif // WIRESHARK_H
