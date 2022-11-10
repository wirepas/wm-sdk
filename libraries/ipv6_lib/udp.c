#include "udp.h"
#include "ipv6_lib.h"

#include <string.h>

#define DEBUG_LOG_MODULE_NAME "UDP"
#define DEBUG_LOG_MAX_LEVEL LVL_DEBUG
#include "debug_log.h"


udp_received_data_cb_f m_udp_received_data_cb;

void Udp_setReceivedDataCb(udp_received_data_cb_f cb)
{
	m_udp_received_data_cb = cb;
}

uint8_t *Udp_getDataPtr()
{
	uint8_t *ipv6_packet_buffer = Ipv6_lib_getIpv6PacketBuffer();
	/* The full buffer minus 40 bytes for the IPv6 header and 8 bytes for the UDP header */
	return &ipv6_packet_buffer[IPV6_HEADER_LENGTH + UDP_HEADER_LENGTH];
}

void pack_udp_header(uint16_t * output, const udp_header_t * udp_header)
{
	output[0] = __builtin_bswap16(udp_header->src_port);
	output[1] = __builtin_bswap16(udp_header->dst_port);
	output[2] = __builtin_bswap16(udp_header->length);
	output[3] = __builtin_bswap16(udp_header->checksum);
}

void unpack_udp_header(const uint16_t * input, udp_header_t * udp_header)
{
	udp_header->src_port = __builtin_bswap16(input[0]);
	udp_header->dst_port = __builtin_bswap16(input[1]);
	udp_header->length   = __builtin_bswap16(input[2]);
	udp_header->checksum = __builtin_bswap16(input[3]);
}

static uint16_t compute_checksum(const ipv6_pseudo_header_t * ipv6_pseudo_header, const udp_header_t * udp_header, const uint8_t * udp_payload)
{
	uint32_t checksum = 0;
	udp_header_t udp_header_checksum;

	/* Fields need to be in Big Endian for checksum computation */
	memcpy(&udp_header_checksum, udp_header, sizeof(udp_header_t));
	udp_header_checksum.dst_port = __builtin_bswap16(udp_header_checksum.dst_port);
	udp_header_checksum.src_port = __builtin_bswap16(udp_header_checksum.src_port);
	udp_header_checksum.length = __builtin_bswap16(udp_header_checksum.length);
	udp_header_checksum.checksum = 0;

	checksum += Ipv6_checksumAdd((uint8_t *)ipv6_pseudo_header, sizeof(ipv6_pseudo_header_t));
	checksum += Ipv6_checksumAdd((uint8_t *)&udp_header_checksum, sizeof(udp_header_t));
	checksum += Ipv6_checksumAdd(udp_payload, udp_header->length - UDP_HEADER_LENGTH);
	checksum = Ipv6_checksumFinish(checksum);

	return checksum;
}

static bool verify_checksum(const ipv6_pseudo_header_t * ipv6_pseudo_header, const udp_header_t * udp_header, const uint8_t * udp_payload)
{
	uint16_t checksum = compute_checksum(ipv6_pseudo_header, udp_header, udp_payload);
	return (checksum == udp_header->checksum);
}

uint16_t Udp_genPacket(uint8_t * output, const udp_data_to_send_t * udp_data_to_send)
{
	ipv6_pseudo_header_t ipv6_pseudo_header;
	ipv6_header_t ipv6_header = {
		.next_header = IPV6_NEXT_HEADER_UPD,
		.payload_length = UDP_HEADER_LENGTH + udp_data_to_send->length,
	};
	uint8_t src_address[16];

	Ipv6_getAddrFromDstAddr(src_address, udp_data_to_send->ipv6_dst_address);
	Ipv6_inititalizeHeader(&ipv6_header);

	memcpy(ipv6_header.dst_address, udp_data_to_send->ipv6_dst_address, 16);
	memcpy(ipv6_header.src_address, src_address, 16);

	Ipv6_packIpv6Header((uint32_t *)output, &ipv6_header);
	Ipv6_getPseudoHeader(&ipv6_header, &ipv6_pseudo_header);

	output += IPV6_HEADER_LENGTH;

	udp_header_t udp_header = {
		.dst_port = udp_data_to_send->dst_port,
		.src_port = udp_data_to_send->src_port,
		.length = udp_data_to_send->length + UDP_HEADER_LENGTH
	};

	udp_header.checksum = compute_checksum(&ipv6_pseudo_header, &udp_header, udp_data_to_send->data);

	/* Copy the UDP header */
	pack_udp_header((uint16_t *)output, &udp_header);
	output += UDP_HEADER_LENGTH;
	
	/* If the application did not use the large buffer, copy the payload */
	if (udp_data_to_send->data != output)
	{
		/* Copy the UDP payload */
		memcpy(output, udp_data_to_send->data, udp_data_to_send->length);
	}

	return IPV6_HEADER_LENGTH + UDP_HEADER_LENGTH + udp_data_to_send->length;
}

void Udp_trafficCb(ipv6_header_t * ipv6_header, const uint8_t * ipv6_payload)
{
	bool is_checksum_valid;
	ipv6_pseudo_header_t ipv6_pseudo_header;
	udp_header_t udp_header;
	const uint8_t * udp_payload = ipv6_payload + UDP_HEADER_LENGTH;

	Ipv6_getPseudoHeader(ipv6_header, &ipv6_pseudo_header);
	
	unpack_udp_header((uint16_t *)ipv6_payload, &udp_header);

	udp_received_data_t udp_received_data = {
		.data = udp_payload,
		.length = udp_header.length - UDP_HEADER_LENGTH,
		.dst_port = udp_header.dst_port,
		.src_port = udp_header.src_port
	};
	
	memcpy(udp_received_data.ipv6_src_address, ipv6_header->src_address, 16);

	is_checksum_valid = verify_checksum(&ipv6_pseudo_header, &udp_header, udp_payload);

	if (!is_checksum_valid)
	{
		/* Something went wrong. TODO: Handle this */
		LOG(LVL_DEBUG, "UDP checksum failed");
	}
	else
	{
		LOG(LVL_DEBUG, "UDP checksum is correct");
	}

	if (m_udp_received_data_cb != NULL)
	{
		m_udp_received_data_cb(&udp_received_data);
	}
}

udp_res_e Udp_sendData(const udp_data_to_send_t * udp_data_to_send)
{
	if (!Ipv6_isLinkLocalAddr(udp_data_to_send->ipv6_dst_address) 
				&& !Ipv6_isGlobalUnicastAddressSet())
	{
		return UDP_RES_SRC_ADDRESS_NOT_KNOWN;
	}

	uint8_t * ipv6_packet = Ipv6_lib_getIpv6PacketBuffer();
	uint16_t length = Udp_genPacket(ipv6_packet, udp_data_to_send);

	if (Ipv6_lib_sendData(ipv6_packet, length) != IPV6_LIB_RES_OK)
	{
		return UDP_RES_CANNOT_SEND;
	}

	return UDP_RES_OK;
}
