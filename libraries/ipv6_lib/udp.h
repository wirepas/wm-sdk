#ifndef _UDP_H_
#define _UDP_H_

#include "ipv6.h"

#define UDP_HEADER_LENGTH 8

typedef enum
{
    UDP_RES_OK,
    UDP_RES_SRC_ADDRESS_NOT_KNOWN,
    UDP_RES_CANNOT_SEND,
} udp_res_e;

typedef struct __attribute__((packed))
{
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} udp_header_t;

typedef struct
{
    const uint8_t * data;
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint8_t ipv6_dst_address[16];
} udp_data_to_send_t;

typedef struct
{
    const uint8_t * data;
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint8_t ipv6_src_address[16];
} udp_received_data_t;

typedef void
    (*udp_received_data_cb_f)(const udp_received_data_t * udp_received_data);

/**
 * \brief   Callback to process UDP traffic. Only called from IPv6 lib
 * \param   ipv6_header
 *          IPv6 header
 * \param   ipv6_payload
 *          IPv6 packet's payload
 */
void Udp_trafficCb(ipv6_header_t * ipv6_header, const uint8_t * ipv6_payload);

/**
 * \brief   Send an UDP packet
 * \param   udp_data_to_send
 *          UDP data and metadata to send
 */
udp_res_e Udp_sendData(const udp_data_to_send_t * udp_data_to_send);

/**
 * \brief   Set the application callback for UDP packets
 * \param   cb
 *          Application callback for UDP packets
 */
void Udp_setReceivedDataCb(udp_received_data_cb_f cb);

/**
 * \brief   Get the pointer for a large buffer to store UDP data
 * \return Pointer to the buffer
 */
uint8_t * Udp_getDataPtr(void);

/**
 * \brief   Generate an UDP packet
 * \param   output
 *          Output buffer to write the packet
 * \param   udp_data_to_send
 *          UDP metadata and payload to send
 */
uint16_t Udp_genPacket(uint8_t * output, const udp_data_to_send_t * udp_data_to_send);

#endif