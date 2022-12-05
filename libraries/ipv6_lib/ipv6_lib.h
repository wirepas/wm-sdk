#ifndef _IPV6_LIB_H_
#define _IPV6_LIB_H_

#include <api.h>
#include "ipv6.h"

#define IPV6_LIB_MAXIMUM_PACKET_SIZE 1500

/**
 * \brief   List of return code
 */
typedef enum
{
    /** Operation is successful */
    IPV6_LIB_RES_OK = 0,
    /** Message cannot be sent */
    IPV6_LIB_RES_CANNOT_SEND = 1,
} ipv6_lib_res_e;

/**
 * \brief   Initialize ipv6 lib
 * \return  Return code of the operation
 */
ipv6_lib_res_e Ipv6_lib_init(void);

/**
 * \brief   Set IPv6 data callback that bypasses the protocols processing at the library
 * \return  Return code of the operation
 */
ipv6_lib_res_e Ipv6_lib_setIpv6DataCb(app_lib_data_data_received_cb_f cb);

/**
 * \brief   Send and IPv6 packet
 * \param   ipv6_packet
 *          IPv6 packet to be sent
 * \param   length
 *          Length of the packet
 * \return  Return code of the operation
 */
ipv6_lib_res_e Ipv6_lib_sendData(const uint8_t * ipv6_packet, const size_t);

/**
 * \brief   Get the large buffer for IPv6 Packets
 * \return  Pointer to the large buffer
 */
uint8_t * Ipv6_lib_getIpv6PacketBuffer(void);

#endif