#include "ipv6_lib.h"
#include "ipv6_config.h"
#include "ipv6.h"
#include "icmpv6.h"
#include "udp.h"

#include "shared_data.h"
#include "api.h"
#include "node_configuration.h"

#include <string.h>

#define DEBUG_LOG_MODULE_NAME "IPV6_LIB"
#define DEBUG_LOG_MAX_LEVEL LVL_DEBUG
#include "debug_log.h"

/* Enpoints use as source and destination for IPV6 without header compression */
#define IPV6_SRC_EP 128
#define IPV6_DST_EP 2

/* IPv6 packet buffer */
static uint8_t m_ipv6_packet[IPV6_LIB_MAXIMUM_PACKET_SIZE];
/* Callback for raw IPv6 packets */
static app_lib_data_data_received_cb_f m_ipv6_data_cb;

ipv6_lib_res_e Ipv6_lib_setIpv6DataCb(app_lib_data_data_received_cb_f cb)
{
	m_ipv6_data_cb = cb;
	return IPV6_LIB_RES_OK;
}

/**
 * \brief   Data reception callback
 * \param   data
 *          Received data, \ref app_lib_data_received_t
 * \return  Result code, \ref app_lib_data_receive_res_e
 */
static app_lib_data_receive_res_e ipv6TrafficCb(
	const shared_data_item_t *item,
	const app_lib_data_received_t *data)
{
	(void)(item);
	
	if (m_ipv6_data_cb != NULL)
	{
		app_lib_data_receive_res_e res;
		res = m_ipv6_data_cb(data);
		return res;
	}

	ipv6_header_t ipv6_header;
	const uint8_t * ipv6_payload;

	Ipv6_unpackIpv6Header(data->bytes, &ipv6_header);

	LOG(LVL_DEBUG, "Received an IPv6 packet");

	ipv6_payload = data->bytes + IPV6_HEADER_LENGTH;

	switch (ipv6_header.next_header)
	{
	case IPV6_NEXT_HEADER_ICMP_V6:
		Icmpv6_trafficCb(&ipv6_header, ipv6_payload);
		break;
	case IPV6_NEXT_HEADER_UPD:
		Udp_trafficCb(&ipv6_header, ipv6_payload);
		break;
	default:
		/* Next header unknown */
		return APP_LIB_DATA_RECEIVE_RES_HANDLED;
	}

	// Data handled successfully
	return APP_LIB_DATA_RECEIVE_RES_HANDLED;
}


/* Unicast messages filter */
static shared_data_item_t ipv6_packets_filter =
{
	.cb = ipv6TrafficCb,
	.filter = {
		.mode = SHARED_DATA_NET_MODE_ALL,
		/* Filtering by source endpoint. */
		.src_endpoint = IPV6_SRC_EP,
		/* Filtering by destination endpoint. */
		.dest_endpoint = IPV6_DST_EP,
		.multicast_cb = NULL,
	}
};

ipv6_lib_res_e Ipv6_lib_init(void)
{
	LOG(LVL_DEBUG, "Init IPv6 lib");

	// Set callback for all ipv6 traffic
	Shared_Data_addDataReceivedCb(&ipv6_packets_filter);
	// Init configuration
	Ipv6_initConfig();

	return IPV6_LIB_RES_OK;
}

ipv6_lib_res_e Ipv6_lib_sendData(const uint8_t * ipv6_packet, const size_t length)
{
	LOG(LVL_DEBUG, "Sending an IPv6 packet");
	
	/* For now only uplink and downlink ipv6 supported (no node to node)
	   So node sends to ANYSINK */

	/* Create a data packet to send */
	app_lib_data_to_send_t data_to_send =
		{
			.bytes = ipv6_packet,
			.num_bytes = length,
			.dest_address = APP_ADDR_ANYSINK,
			.src_endpoint = IPV6_SRC_EP,
			.dest_endpoint = IPV6_DST_EP,
			.qos = APP_LIB_DATA_QOS_HIGH,
		};

	/* Send the data packet */
	if (Shared_Data_sendData(&data_to_send, NULL) != APP_LIB_DATA_SEND_RES_SUCCESS)
	{
		return IPV6_LIB_RES_CANNOT_SEND;
	}

	return IPV6_LIB_RES_OK;
}

uint8_t * Ipv6_lib_getIpv6PacketBuffer()
{
	return m_ipv6_packet;
}

