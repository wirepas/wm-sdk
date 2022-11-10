#include "ipv6_lib.h"
#include "icmpv6.h"
#include "ipv6.h"

#include <stdbool.h>
#include <string.h>

#define DEBUG_LOG_MODULE_NAME "ICMPV6"
#define DEBUG_LOG_MAX_LEVEL LVL_DEBUG
#include "debug_log.h"


static icmpv6_echo_reply_cb_f m_icmpv6_echo_reply_cb;

void Icmpv6_setEchoReplyCb(icmpv6_echo_reply_cb_f cb)
{
	m_icmpv6_echo_reply_cb = cb;
}

static uint16_t compute_checksum(const ipv6_pseudo_header_t * ipv6_pseudo_header, const icmpv6_header_t * icmpv6_header, const uint8_t * icmpv6_payload)
{
	uint32_t checksum = 0;
	icmpv6_header_t icmpv6_header_checksum;

	/* RFC 4443, 2.3 Message Checksum Calculation
	 * For computing the checksum, the checksum field is first set to zero.
	 */
	memcpy(&icmpv6_header_checksum, icmpv6_header, sizeof(icmpv6_header_t));
	icmpv6_header_checksum.checksum = 0;

	checksum += Ipv6_checksumAdd((uint8_t *)ipv6_pseudo_header, sizeof(ipv6_pseudo_header_t));
	checksum += Ipv6_checksumAdd((uint8_t *)&icmpv6_header_checksum, sizeof(icmpv6_header_t));
	checksum += Ipv6_checksumAdd(icmpv6_payload, __builtin_bswap32(ipv6_pseudo_header->payload_length) - ICMPV6_HEADER_LENGTH);
	checksum = Ipv6_checksumFinish(checksum);

	return (uint16_t) checksum;
}

static bool verify_checksum(const ipv6_pseudo_header_t * ipv6_pseudo_header, const icmpv6_header_t * icmpv6_header, const uint8_t * icmpv6_payload)
{
	uint16_t checksum = compute_checksum(ipv6_pseudo_header, icmpv6_header, icmpv6_payload);
	return (checksum == icmpv6_header->checksum);
}

void unpack_icmpv6_header(const uint8_t * input, icmpv6_header_t * icmpv6_header)
{
	memcpy(icmpv6_header, input, sizeof(icmpv6_header_t));
	icmpv6_header->checksum = __builtin_bswap16(icmpv6_header->checksum);
}

void pack_icmpv6_header(uint8_t * output, icmpv6_header_t * icmpv6_header)
{
	memcpy(output, icmpv6_header, sizeof(icmpv6_header_t));
	icmpv6_header_t * output_icmpv6_header = (icmpv6_header_t *)output;
	output_icmpv6_header->checksum = __builtin_bswap16(icmpv6_header->checksum);
}

static void send_echo_reply(ipv6_header_t * ipv6_header_request, const uint8_t * icmpv6_payload)
{
	LOG(LVL_DEBUG, "Sending an ICMPv6 ECHO REPLY");

	uint8_t own_global_address[16];

	icmpv6_header_t icmpv6_header = {
		.type = ICMPV6_ECHO_REPLY_TYPE,
		.code = ICMPV6_ECHO_REPLY_CODE
	};

	uint8_t * ipv6_packet = Ipv6_lib_getIpv6PacketBuffer();
	uint16_t icmpv6_pl_length = ipv6_header_request->payload_length - ICMPV6_HEADER_LENGTH;

	/* Always use our own global address for ping response (do not swap src/dst in case destination is broadcast) */
	if (!Ipv6_isGlobalUnicastAddressSet())
	{
		/* No global address set, cannot answer */
		return;
	}

	Ipv6_getGlobalUnicastAddress(own_global_address);
	Icmpv6_genPacket(ipv6_packet, own_global_address, ipv6_header_request->src_address, &icmpv6_header, icmpv6_payload, icmpv6_pl_length);
	Ipv6_lib_sendData(ipv6_packet, ipv6_header_request->payload_length + IPV6_HEADER_LENGTH);
}

uint16_t Icmpv6_genPacket(uint8_t * output, uint8_t * src_addr, uint8_t * dst_addr, icmpv6_header_t * icmpv6_header, const uint8_t * icmpv6_payload, uint16_t length)
{
	ipv6_header_t ipv6_header = {
		.next_header = IPV6_NEXT_HEADER_ICMP_V6,
		.payload_length = ICMPV6_HEADER_LENGTH + length,
	};

	Ipv6_inititalizeHeader(&ipv6_header);

	ipv6_pseudo_header_t ipv6_pseudo_header;
	
	memcpy(ipv6_header.dst_address, dst_addr, 16);
	memcpy(ipv6_header.src_address, src_addr, 16);

	/* Copy the IPv6 header */
	Ipv6_packIpv6Header((uint32_t *)output, &ipv6_header);
	Ipv6_getPseudoHeader(&ipv6_header, &ipv6_pseudo_header);
	output += IPV6_HEADER_LENGTH;

	/* Generate the checksum */
	icmpv6_header->checksum = compute_checksum(&ipv6_pseudo_header, icmpv6_header, icmpv6_payload);

	/* Copy the ICMPv6 header */
	pack_icmpv6_header(output, icmpv6_header);
	output += ICMPV6_HEADER_LENGTH;

	/* Copy the ICMPv6 payload */
	memcpy(output, icmpv6_payload, length);

	return length + ICMPV6_HEADER_LENGTH + IPV6_HEADER_LENGTH;
}

icmpv6_res_e Icmpv6_sendEchoRequest(uint8_t * ipv6_dest_addr, uint8_t * icmpv6_payload, uint16_t length)
{
	if (!Ipv6_isLinkLocalAddr(ipv6_dest_addr) 
				&& !Ipv6_isGlobalUnicastAddressSet())
	{
		return ICMPV6_RES_SRC_ADDRESS_NOT_KNOWN;
	}

	LOG(LVL_DEBUG, "Sending an ICMPv6 ECHO REQUEST");
	uint8_t * ipv6_packet = Ipv6_lib_getIpv6PacketBuffer();
	uint8_t ipv6_src_addr[16];
	Ipv6_getAddrFromDstAddr(ipv6_src_addr, ipv6_dest_addr);

	icmpv6_header_t icmpv6_header = {
		.type = ICMPV6_ECHO_REQUEST_TYPE,
		.code = ICMPV6_ECHO_REQUEST_CODE
	};

	Icmpv6_genPacket(ipv6_packet, ipv6_src_addr, ipv6_dest_addr, &icmpv6_header, icmpv6_payload, length);
	Ipv6_lib_sendData(ipv6_packet, length + IPV6_HEADER_LENGTH + ICMPV6_HEADER_LENGTH);

	return ICMPV6_RES_OK;
}

void Icmpv6_trafficCb(ipv6_header_t * ipv6_header, const uint8_t * ipv6_payload)
{
	icmpv6_header_t icmpv6_header;
	ipv6_pseudo_header_t ipv6_pseudo_header;
	bool is_checksum_valid;
	const uint8_t * icmpv6_payload = ipv6_payload + ICMPV6_HEADER_LENGTH;

	Ipv6_getPseudoHeader(ipv6_header, &ipv6_pseudo_header);
	unpack_icmpv6_header(ipv6_payload, &icmpv6_header);
	//LOG(LVL_DEBUG, "Type %d, Code=%d, checksum = 0x%x", icmpv6_header.type, icmpv6_header.code, icmpv6_header.checksum);

	is_checksum_valid = verify_checksum(&ipv6_pseudo_header, &icmpv6_header, icmpv6_payload);

	if (!is_checksum_valid)
	{
		/* Something went wrong. TODO: Handle this */
		LOG(LVL_DEBUG, "ICMPv6 checksum failed");
	}
	else
	{
		LOG(LVL_DEBUG, "ICMPv6 checksum is correct");
	}

	switch (icmpv6_header.type)
	{
	case ICMPV6_ECHO_REQUEST_TYPE:
		LOG(LVL_DEBUG, "Received an ICMPv6 ECHO REQUEST");
		send_echo_reply(ipv6_header, icmpv6_payload);
		return;
	case ICMPV6_ECHO_REPLY_TYPE:
		/* Reply to our request */
		LOG(LVL_DEBUG, "Received an ICMPv6 ECHO REPLY");
		if (m_icmpv6_echo_reply_cb != NULL)
		{
			m_icmpv6_echo_reply_cb(icmpv6_payload, ipv6_header->payload_length - ICMPV6_HEADER_LENGTH);
		}
		return;
	default:
		/* Unknown type */
		LOG(LVL_DEBUG, "Received an ICMPv6 unknown type: %d\n", icmpv6_header.type);
		break;
	}
}
