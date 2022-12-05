#include "ipv6_config.h"
#include "ipv6.h"
#include "udp.h"
#include "shared_appconfig.h"
#include "cbor.h"
#include "stack_state.h"

#include <string.h>

#define DEBUG_LOG_MODULE_NAME "IPV6_CONFIG"
#define DEBUG_LOG_MAX_LEVEL LVL_ERROR
#include "debug_log.h"

// UDP port for IPV6 address signaling
#define UPDATE_ADDR_DATAGRAM_EP 0xF0FB
#define UPDATE_ADDR_DATAGRAM_LENGTH 48

// Type used in Wirepas App config TLV
#define IPV6_CONF_TLV_TYPE 0xC5

#define NODE_UID_FIELD_ID  0
#define NODE_ADDR_FIELD_ID 1
#define NODE_PREVIOUS_IPV6_ADDR_FIELD_ID 2
#define NODE_IPV6_ADDR_FIELD_ID 3

#define IPV6_CONF_S_MASK 0b10000000
#define IPV6_CONF_S_OFFSET 7

#define IPV6_CONF_C_MASK 0b01000000
#define IPV6_CONF_C_OFFSET 6

#define IPV6_CONF_CC_MASK 0b00100000
#define IPV6_CONF_CC_OFFSET 5

#define IPV6_CONF_RFU_MASK 0b00010000
#define IPV6_CONF_RFU_OFFSET 4

#define IPV6_CONF_CID_MASK 0b00001111
#define IPV6_CONF_CID_OFFSET 0

/* Previous IPv6 address of the node */
static uint8_t m_previous_ipv6_addr[16];
/* Off-mesh service IPv6 address */
static uint8_t m_offmesh_ipv6_addr[16];
/* Prefix's sequence number in the App Config to know if a response is needed */
static uint8_t m_prefix_seq_num;
/* Filter id registered */
static uint16_t m_filter_id;

/* Sink address used to generate the IPv6 address */
static app_addr_t m_sink_address;
/* Prefix acquired by AppConfig */
static uint8_t m_prefix[8];

/* Flag if the sink address has been set */
static bool m_sink_address_set;
/* Flag if the prefix has been set */
static bool m_prefix_set;
/* Flag if the offmesh ipv6 addr has been set */
static bool m_offmesh_ipv6_set;

static void update_ipv6_address()
{
	if (!m_sink_address_set)
	{
		// Sink address is not known yet
		return;
	}

	if (!m_prefix_set)
	{
		// Prefix is not known yet
		return;
	}

	uint8_t new_ipv6_addr[16];
	app_addr_t node_addr;
	lib_settings->getNodeAddress(&node_addr);

	uint32_t sink_address_be = __builtin_bswap32((uint32_t) m_sink_address);
	uint32_t node_addr_be = __builtin_bswap32((uint32_t) node_addr);

	memcpy(new_ipv6_addr, m_prefix, 8);
	memcpy(new_ipv6_addr + 8, &sink_address_be, 4);
	memcpy(new_ipv6_addr + 12, &node_addr_be, 4);

	Ipv6_getGlobalUnicastAddress(m_previous_ipv6_addr);
	Ipv6_setGlobalUnicastAddress(new_ipv6_addr);
}

static CborError encode_update_address_payload(uint8_t * buffer, uint8_t  * len, bool previous_ipv6_addr_flag)
{
  CborError err;
  CborEncoder encoder, mapEncoder;

	app_addr_t node_addr;
	lib_settings->getNodeAddress(&node_addr);

  uint8_t length = 3 ? previous_ipv6_addr_flag : 2;

  uint8_t current_ipv6_addr[16];
  Ipv6_getGlobalUnicastAddress(current_ipv6_addr);

  cbor_encoder_init(&encoder, buffer, *len, 0);
  err = cbor_encoder_create_map(&encoder, &mapEncoder, length);
  if (err != CborNoError)
  {
    return err;
  }

  err = cbor_encode_uint(&mapEncoder, NODE_ADDR_FIELD_ID);
  if (err != CborNoError)
  {
    return err;
  }
  err = cbor_encode_uint(&mapEncoder, node_addr);
  if (err != CborNoError)
  {
    return err;
  }

  if (previous_ipv6_addr_flag)
  {
    // The previous IPv6 address has to be included
    err = cbor_encode_uint(&mapEncoder, NODE_PREVIOUS_IPV6_ADDR_FIELD_ID);
    if (err != CborNoError)
    {
      return err;
    }
    err = cbor_encode_byte_string(&mapEncoder,
                                  m_previous_ipv6_addr,
                                  16);
    if (err != CborNoError)
    {
        return err;
    }
  }

  err = cbor_encode_uint(&mapEncoder, NODE_IPV6_ADDR_FIELD_ID);
  if (err != CborNoError)
  {
    return err;
  }
  err = cbor_encode_byte_string(&mapEncoder,
                                current_ipv6_addr,
                                16);
  if (err != CborNoError)
  {
      return err;
  }

  return CborNoError;
}

static void send_update_address_datagram(bool previous_ipv6_addr_flag)
{
  uint8_t payload[UPDATE_ADDR_DATAGRAM_LENGTH];
  uint8_t length = UPDATE_ADDR_DATAGRAM_LENGTH;

  // CBOR encoding if the IPv6 address update signalling
  encode_update_address_payload(payload, &length, previous_ipv6_addr_flag);

	udp_data_to_send_t udp_data_to_send = {
		.dst_port = UPDATE_ADDR_DATAGRAM_EP,
		.src_port = UPDATE_ADDR_DATAGRAM_EP,
		.length = length,
		.data = payload,
	};

  if (m_offmesh_ipv6_set)
  {
    memcpy(udp_data_to_send.ipv6_dst_address, m_offmesh_ipv6_addr, 16);
  }
  else if (m_prefix_set)
  {
    // No off_mesh_ipv6 address set yet, send the packet to gateway
    // TODO, make this address available directly
    memcpy(udp_data_to_send.ipv6_dst_address, m_prefix, 8);
    memcpy(udp_data_to_send.ipv6_dst_address + 8, &m_sink_address, 4);
    memset(udp_data_to_send.ipv6_dst_address + 12, 0, 4);
  }
  else
  {
    // No one to send the message
    return;
  }

	Udp_sendData(&udp_data_to_send);
}

static void route_cb(app_lib_stack_event_e event, void * param)
{
	app_lib_state_route_info_t * route_info;
	switch (event)
	{
	case APP_LIB_STATE_STACK_EVENT_ROUTE_CHANGED:
		route_info = (app_lib_state_route_info_t *) param;

		if (m_sink_address != route_info->sink)
		{
			m_sink_address_set = true;
			m_sink_address = route_info->sink;
			if (m_prefix_set)
			{
        // Update the IPv6 address used by the library
				update_ipv6_address();
				// The device received a prefix and the route changed, inform the gateway
				send_update_address_datagram(true);
			}
		}
		break;
	default:
		break;
	}
}

static uint8_t * parse_ipv6_ie(uint8_t * payload, uint8_t * length)
{
  uint8_t conf_flags = *payload;
  bool offmesh_service_flag = (conf_flags & IPV6_CONF_S_MASK) >> IPV6_CONF_S_OFFSET;
  // Unused for now
  // bool context_flag = (conf_flags & IPV6_CONF_C_MASK) >> IPV6_CONF_C_OFFSET;
  // bool context_compression_flag = (conf_flags & IPV6_CONF_CC_MASK) >> IPV6_CONF_CC_OFFSET;
  // uint8_t cid = (conf_flags & IPV6_CONF_CID_MASK) >> IPV6_CONF_CID_OFFSET;
  payload++;
  (*length)--;

  if (*length == 0)
  {
	  LOG(LVL_ERROR, "Error parsing IPv6 IE");
    return payload;
  }

  LOG(LVL_DEBUG, "Parsing ipv6 ie: length = %d, %d", *length, offmesh_service_flag);
  if (offmesh_service_flag)
  {
    if (false) // Unused for now, will be replaced by if (context_flag)
    {
      uint8_t sid = *payload;
      payload++;
      (*length)--;
      (void) sid;
    }

    // The next 16 bytes contain the off-mesh IPv6 address
    if (*length < 16)
    {
      LOG(LVL_ERROR, "Error parsing IPv6 IE, not enough bytes for offmesh_service_flag");
      // No point to parse more, as we have to way to find next entry
      *length = 0;
      return payload + *length;
    }

    memcpy(m_offmesh_ipv6_addr, payload, 16);
    m_offmesh_ipv6_set = true;
    payload += 16;
    (*length) -= 16;
  }
  else
  {
    if (*length < 8)
    {
      LOG(LVL_ERROR, "Error parsing IPv6 IE, not enough bytes for m_prefix");
      // No point to parse more, , as we have to way to find next entry
      *length = 0;
      return payload + *length;
    }

    // The next 8 bytes contain the prefix
    memcpy(m_prefix, payload, 8);
    m_prefix_set = true;
    payload += 8;
    (*length) -= 8;
  }

  return payload;
}

static void parse_app_config_ipv6(uint8_t * payload, uint8_t length, uint8_t * prefix_seq_num)
{
	uint8_t conf_header = *payload;
  // uint8_t version = 0xf0 & conf_header;
  *prefix_seq_num = 0x0f & conf_header;
  LOG(LVL_DEBUG, "Nonce is %d", *prefix_seq_num);
  payload++;
  length--;

  while (length)
  {
    // Parse the configuration information element containing the mesh network IPv6 prefix
    // It should contain at least a network prefix and maybe a off_mesh_service
    // If more than one, the latest will be in use (no support yet for multiple prefix or off_mesh services)
    payload = parse_ipv6_ie(payload, &length);
  }
}

static void app_config_cb(uint16_t type, uint8_t length, uint8_t * value_p)
{
	(void)type;
	(void)length;
	uint8_t prefix_seq_num;
	bool was_prefix_set = m_prefix_set;

  // Reset offmesh address in case it is not set anymore
  m_offmesh_ipv6_set = false;
  // Parse the content of the app config
  parse_app_config_ipv6(value_p, length, &prefix_seq_num);

	// Update the device's IPv6 address
	update_ipv6_address();

	if (!was_prefix_set && 	m_prefix_set)
	{
		// First time receiving the prefix, inform the gateway
		send_update_address_datagram(false);
	}
	else if (m_prefix_seq_num != prefix_seq_num)
	{
		// The gateway is requesting a response
		send_update_address_datagram(false);
	}

	m_prefix_seq_num = prefix_seq_num;
}

/* AppConfig filter */
static shared_app_config_filter_t m_app_config_filter = 
{
	.cb = app_config_cb,
	.type = IPV6_CONF_TLV_TYPE,
	.call_cb_always = true,
};

void Ipv6_initConfig(void)
{
	// Set callback for Route change event
	Stack_State_addEventCb(route_cb, 1 << APP_LIB_STATE_STACK_EVENT_ROUTE_CHANGED);
	// Set callback for App Config carrying the IPv6 prefix
	Shared_Appconfig_addFilter(&m_app_config_filter, &m_filter_id);
}

bool Ipv6_config_getOffMeshIpv6Address(uint8_t ipv6_offmesh_address[16])
{
  if (!m_offmesh_ipv6_set)
  {
    return false;
  }

  memcpy(ipv6_offmesh_address, m_offmesh_ipv6_addr, 16);
  return true;
}
