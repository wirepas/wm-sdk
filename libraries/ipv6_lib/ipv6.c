#include "ipv6.h"

#include "api.h"
#include <string.h>

#define DEBUG_LOG_MODULE_NAME "IPV6"
#define DEBUG_LOG_MAX_LEVEL LVL_DEBUG
#include "debug_log.h"

/* Device's IPv6 address */
static uint8_t m_ipv6_address[16];
static bool m_ipv6_address_set;


static uint32_t get_bits(const uint32_t field, const uint32_t mask, const uint_fast8_t offset)
{
	return (field & mask) >> offset;
}

static void set_bits(uint32_t * field, const uint32_t value, const uint32_t mask, const uint_fast8_t offset)
{
	*field &= ~mask;
	*field |= (value << offset) & mask;
}

void Ipv6_unpackIpv6Header(const uint8_t * input, ipv6_header_t * ipv6_header)
{
	/* Fields come in Big Endian, converting to Little Endian */
	uint32_t version_tc_flow_label = __builtin_bswap32(((uint32_t *)input)[0]);
	uint32_t pl_length_nxt_hdr_hop_lmt = __builtin_bswap32(((uint32_t *)input)[1]);

	ipv6_header->version = get_bits(version_tc_flow_label, IPV6_VERSION_MASK, IPV6_VERSION_OFFSET);
	ipv6_header->traffic_class = get_bits(version_tc_flow_label, IPV6_TRAFFIC_CLASS_MASK, IPV6_TRAFFIC_CLASS_OFFSET);
	ipv6_header->flow_label = get_bits(version_tc_flow_label, IPV6_FLOW_LABEL_MASK, IPV6_FLOW_LABEL_OFFSET);

	ipv6_header->payload_length = get_bits(pl_length_nxt_hdr_hop_lmt, IPV6_PAYLOAD_LENGTH_MASK, IPV6_PAYLOAD_LENGTH_OFFSET);
	ipv6_header->next_header = get_bits(pl_length_nxt_hdr_hop_lmt, IPV6_NEXT_HEADER_MASK, IPV6_NEXT_HEADER_OFFSET);
	ipv6_header->hop_limit = get_bits(pl_length_nxt_hdr_hop_lmt, IPV6_HOP_LIMIT_MASK, IPV6_HOP_LIMIT_OFFSET);

	memcpy(ipv6_header->src_address, &((uint32_t *)input)[2], 16);
	memcpy(ipv6_header->dst_address, &((uint32_t *)input)[6], 16);
}

void Ipv6_packIpv6Header(uint32_t * output, ipv6_header_t * ipv6_header)
{
	uint32_t version_tc_flow_label = 0;
	uint32_t pl_length_nxt_hdr_hop_lmt = 0;

	set_bits(&version_tc_flow_label, ipv6_header->version, IPV6_VERSION_MASK, IPV6_VERSION_OFFSET);
	set_bits(&version_tc_flow_label, ipv6_header->traffic_class, IPV6_TRAFFIC_CLASS_MASK, IPV6_TRAFFIC_CLASS_OFFSET);
	set_bits(&version_tc_flow_label, ipv6_header->flow_label, IPV6_FLOW_LABEL_MASK, IPV6_FLOW_LABEL_OFFSET);

	/* Fields stored in Little Endian, converting to Big Endian */
	version_tc_flow_label = __builtin_bswap32(version_tc_flow_label);
	output[0] = version_tc_flow_label;

	set_bits(&pl_length_nxt_hdr_hop_lmt, ipv6_header->payload_length, IPV6_PAYLOAD_LENGTH_MASK, IPV6_PAYLOAD_LENGTH_OFFSET);
	set_bits(&pl_length_nxt_hdr_hop_lmt, ipv6_header->next_header, IPV6_NEXT_HEADER_MASK, IPV6_NEXT_HEADER_OFFSET);
	set_bits(&pl_length_nxt_hdr_hop_lmt, ipv6_header->hop_limit, IPV6_HOP_LIMIT_MASK, IPV6_HOP_LIMIT_OFFSET);
	/* Fields stored in Little Endian, converting to Big Endian */
	pl_length_nxt_hdr_hop_lmt = __builtin_bswap32(pl_length_nxt_hdr_hop_lmt);
	output[1] = pl_length_nxt_hdr_hop_lmt;

	memcpy(output + 2, ipv6_header->src_address, 16);
	memcpy(output + 6, ipv6_header->dst_address, 16);
}

void Ipv6_inititalizeHeader(ipv6_header_t * ipv6_header)
{
	ipv6_header->flow_label = 0;
	ipv6_header->hop_limit = 255;
	ipv6_header->traffic_class = 0;
	ipv6_header->version = 0b0110;
}

void Ipv6_getPseudoHeader(ipv6_header_t *ipv6_header, ipv6_pseudo_header_t *ipv6_pseudo_header)
{
	memcpy(ipv6_pseudo_header->src_address, ipv6_header->src_address, 16);
	memcpy(ipv6_pseudo_header->dst_address, ipv6_header->dst_address, 16);
	ipv6_pseudo_header->payload_length = __builtin_bswap32(ipv6_header->payload_length);
	memset(ipv6_pseudo_header->zeros, 0, 3);
	ipv6_pseudo_header->next_header = ipv6_header->next_header;
}

uint16_t Ipv6_checksumFinish(uint32_t sum)
{
	while (sum >> 16)
	{
		sum = (sum & 0xFFFF) + (sum >> 16);
	}
	return ~((uint16_t) sum);
}

uint32_t Ipv6_checksumAdd(const uint8_t * addr, size_t length)
{
	uint32_t sum = 0;

	while (length > 1)
	{
		sum += *addr++ << 8;
		sum += *addr++;
		length -= 2;
	}

	/*  Add left-over byte, if any */
	if (length)
	{
		sum += *addr << 8;
	}

	return sum;
}

bool Ipv6_isGlobalUnicastAddressSet()
{
	return m_ipv6_address_set;
}

bool Ipv6_isLinkLocalAddr(const uint8_t * address)
{
  uint8_t link_local_prefix[8] = {0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  return memcmp(address, link_local_prefix, 8) == 0;
}

void Ipv6_setGlobalUnicastAddress(const uint8_t * address)
{
	m_ipv6_address_set = true;
	memcpy(m_ipv6_address, address, 16);
}

void Ipv6_getGlobalUnicastAddress(uint8_t * address)
{
	memcpy(address, m_ipv6_address, 16);
}

void Ipv6_getAddrFromDstAddr(uint8_t * address, const uint8_t * dst_address)
{
	if (Ipv6_isLinkLocalAddr(dst_address))
	{
		uint8_t link_local_prefix[12] = {0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		app_addr_t node_addr;
		lib_settings->getNodeAddress(&node_addr);
		memcpy(address, link_local_prefix, 12);
		memcpy(address + 12, &node_addr, 4);
	}
	else
	{
		Ipv6_getGlobalUnicastAddress(address);
	}
}
