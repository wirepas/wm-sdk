#ifndef _ICMPV6_H_
#define _ICMPV6_H_

#include "ipv6.h"
#include <stdint.h>

#define ICMPV6_HEADER_LENGTH 4

#define ICMPV6_ECHO_REQUEST_TYPE 128
#define ICMPV6_ECHO_REPLY_TYPE 129

#define ICMPV6_ECHO_REQUEST_CODE 0
#define ICMPV6_ECHO_REPLY_CODE 0

typedef enum
{
    ICMPV6_RES_OK,
    ICMPV6_RES_SRC_ADDRESS_NOT_KNOWN,
} icmpv6_res_e;

typedef struct __attribute__((packed))
{
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
} icmpv6_header_t;

typedef void
    (*icmpv6_echo_reply_cb_f)(const uint8_t * icmpv6_payload, uint16_t length);

/**
 * \brief   Callback to process ICMPv6 traffic. Only called from IPv6_lib
 */
void Icmpv6_trafficCb(ipv6_header_t * ipv6_pseudo_header, const uint8_t * ipv6_payload);

/**
 * \brief   Set the callback for ECHO REPLY commands
 * \param   cb
 *          Callback function
 */
void Icmpv6_setEchoReplyCb(icmpv6_echo_reply_cb_f cb);

/**
 * \brief   Send an ICMPv6 ECHO REQUEST command
 * \param   ipv6_dest_addr
 *          IPv6 destination address
 * \param   icmpv6_payload
 *          ICMPv6 payload
 * \param   length
 *          Length of the ICMPv6 payload
 */
icmpv6_res_e Icmpv6_sendEchoRequest(uint8_t * ipv6_dest_addr, uint8_t * icmpv6_payload, uint16_t length);

/**
 * \brief   Generate an ICMPv6 packet
 * \param   output
 *          Output buffer where to write the packet
 * \param   src_addr
 *          IPv6 source address
 * \param   dst_addr
 *          IPv6 destination address
 * \param   icmpv6_header
 *          ICMPv6 header metadata
 * \param   icmpv6_payload
 *          ICMPv6 payload
 * \param   length
 *          Length of the ICMPv6 payload
 * \return  Number of bytes written in the output buffer
 */
uint16_t Icmpv6_genPacket(uint8_t * output, uint8_t * src_addr, uint8_t * dst_addr, icmpv6_header_t * icmpv6_header, const uint8_t * icmpv6_payload, uint16_t length);

#endif //_ICMPV6_H_