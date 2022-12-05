#ifndef _IPV6_H_
#define _IPV6_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


#define IPV6_HEADER_LENGTH 40

#define IPV6_NEXT_HEADER_ICMP_V6 58
#define IPV6_NEXT_HEADER_UPD 17

#define IPV6_VERSION_OFFSET 28
#define IPV6_TRAFFIC_CLASS_OFFSET 20
#define IPV6_FLOW_LABEL_OFFSET 0

#define IPV6_VERSION_MASK           0xF0000000
#define IPV6_TRAFFIC_CLASS_MASK     0x0FF00000
#define IPV6_FLOW_LABEL_MASK        0x000FFFFF

#define IPV6_PAYLOAD_LENGTH_OFFSET 16
#define IPV6_NEXT_HEADER_OFFSET 8
#define IPV6_HOP_LIMIT_OFFSET 0

#define IPV6_PAYLOAD_LENGTH_MASK    0xFFFF0000
#define IPV6_NEXT_HEADER_MASK       0x0000FF00
#define IPV6_HOP_LIMIT_MASK         0x000000FF

/*
 * Pseudo header needed for checksum
 * Cf checksum chapter from https://en.wikipedia.org/wiki/Internet_Control_Message_Protocol_for_IPv6
 */
typedef struct __attribute__((packed))
{
    uint8_t src_address[16];
    uint8_t dst_address[16];
    uint32_t payload_length;
    uint8_t zeros[3];
    uint8_t next_header;
} ipv6_pseudo_header_t;

typedef struct
{
    uint32_t flow_label;
    uint16_t payload_length;
    uint8_t version;
    uint8_t traffic_class;
    uint8_t next_header;
    uint8_t hop_limit;
    uint8_t src_address[16];
    uint8_t dst_address[16];
} ipv6_header_t;

/**
 * \brief   Initialise an IPv6 header with default values
 * \param   ipv6_header
 *          Pointer to the IPv6 header struct
 */
void Ipv6_inititalizeHeader(ipv6_header_t * ipv6_header);

/**
 * \brief   Unpack the IPv6 header from the received packet
 * \param   input
 *          Received packet
 * \param   ipv6_header
 *          Pointer to the IPv6 header struct
 */
void Ipv6_unpackIpv6Header(const uint8_t * input, ipv6_header_t * ipv6_header);

/**
 * \brief   Pack the IPv6 header to the packet to be sent
 * \param   output
 *          Packet to be sent
 * \param   ipv6_header
 *          Pointer to the IPv6 header struct
 */
void Ipv6_packIpv6Header(uint32_t * output, ipv6_header_t * ipv6_header);

/**
 * \brief   Generate the IPv6 pseudo header
 * \param   ipv6_header
 *          Pointer to the IPv6 header struct
 * \param   ipv6_pseudo_header
 *          Pointer to the IPv6 pseudo header struct
 */
void Ipv6_getPseudoHeader(ipv6_header_t * ipv6_header, ipv6_pseudo_header_t * ipv6_pseudo_header);

/**
 * \brief   Add a data chunk to the checksum computation
 * \param   addr
 *          Start address
 * \param   length
 *          Length of the data chunk
 * \return  Chunk's checksum
 */
uint32_t Ipv6_checksumAdd(const uint8_t * addr, size_t length);

/**
 * \brief   Finish the checksum computation
 * \param   sum
 *          On-going checksum
 * \return  Final checksum
 */
uint16_t Ipv6_checksumFinish(uint32_t sum);

/**
 * \brief   Checks if the GUA has been set in this device
 * \return  Flag
 */
bool Ipv6_isGlobalUnicastAddressSet();

/**
 * \brief   Set the Global Unicast Address of the device
 * \param   address
 *          IPv6 GUA address
 */
void Ipv6_setGlobalUnicastAddress(const uint8_t * address);

/**
 * \brief   Get the Global Unicast Address of the device
 * \param   address
 *          IPv6 GUA address
 */
void Ipv6_getGlobalUnicastAddress(uint8_t * address);

/**
 * \brief   Check if the IPv6 address is Link Local
 * \param   address
 *          IPv6 address
 */
bool Ipv6_isLinkLocalAddr(const uint8_t * address);

/**
 * \brief   Get the IPv6 of the device based on the destination address, Link-Local adddress or GUA.
 * \param   address
 *          Device IPv6 address
 * \param   dst_address
 *          Destination IPv6 address
 */
void Ipv6_getAddrFromDstAddr(uint8_t * address, const uint8_t * dst_address);

#endif