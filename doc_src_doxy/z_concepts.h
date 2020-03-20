
#ifndef _CONCEPTS_H_
#define _CONCEPTS_H_

/**

@page concepts Wirepas Mesh Concepts

This page describes various <I>concepts</I> used in Wirepas Mesh networks. The
concept itself is described. The link to the services are also part of the
concept description.

@section appconfig Application Configuration Data

The application configuration service (commonly referred as <I>app config</I>)
can be used for two things:
-# Configure application-specific parameters (application configuration data) to
   the application running in the nodes (via the network)
-# Configure transmission interval of the stack diagnostic data


The application configuration data is persistent global data for the whole
network. The data format can be decided by the application. Application
configuration data can be set by the sinks' application after which it is
disseminated to the whole network and stored at every node. It can include e.g.
application parameters, such as measurement interval. The service makes it
possible to set the data, after which every new node joining the network
receives the data from its neighbors without the need for end-to-end polling.
Furthermore, new configurations can be set and updated to the network on the
run.

The Single-MCU Api contains following services for using the app config:
-# <code>@ref app_lib_data_write_app_config_f "lib_data->writeAppConfig()"</code>
   for updating the app config.
-# <code>@ref app_lib_data_read_app_config_f "lib_data->readAppConfig()"</code> for
   reading the app config.
-# <code>@ref app_lib_data_set_new_app_config_cb_f "lib_data->setNewAppConfigCb()"
   </code> for setting the callback that is called when app config changes.
-# <code>@ref app_lib_data_get_app_config_num_bytes_f "lib_data->getAppConfigNumBytes()"
   </code> for querying the amount of configuration data allowed.

See also example application @ref source/appconfig_app "appconfig_app" on how to
use application configuration.

\note Writing of app config can only be used in sink role.
\note In a network including multiple sinks, the same configuration data should
      be set to all sinks so that it can be guaranteed to disseminate to every
      node.
\note Application configuration data is stored in permanent memory similarly to
      the persistent attributes. To avoid memory wearing, do not write new
      values too often (e.g. more often than once per 30 minutes).
\note Compared to sending data packets from sink to nodes, application
      configuration is way lighter mechanism and guaranteed to be disseminated
      to every node that appear in the network.

@section addressing Node addressing

Node addressing is done on type @ref app_addr_t. When allocating addresses,
some special addresses have been reserved and nodes are <B>not</B> allowed to
use these special addresses as their own address.The addresses are summarized as
following:

<table>
<tr><th>Address type</th><th>Address space</th><th>Description</th>
<th>Which nodes can transmit to?</tr>

<tr><td>Invalid address</td><td>0x0000 0000</td><td>Invalid address, cannot be
used in any service that use @ref app_addr_t.</td><td>none</td></tr>

<tr><td>Unicast</td><td>0x0000 0001 - 0x7FFF FFFF and 0x8100 0000-0xFFFF FFFD
</td><td>Valid unicast
addresses. Each node on the network must have one of these addresses set as its
address. Two or more devices with identical addresses should never be present
on a network.

To set these address, use function @ref
app_lib_settings_set_node_address_f "lib_settings->setNodeAddress".</td><td>By a
sink and any device in CSMA-CA network.

Any node can send message to itself by using own address as a destination.
</td></tr>

<tr><td>AnySink</td><td>@ref APP_ADDR_ANYSINK</td><td>Address which identifies
that the source or the destination of a packet is an unspecified sink on the
network.</td><td>Any device</td></tr>

<tr><td>Broadcast</td><td>@ref APP_ADDR_BROADCAST</td><td>Broadcast address
with which a packet is delivered to all nodes on the network.

@note When transmitting from a sink, note that a broadcast will only be
transmitted to the nodes directly under the sink's routing tree.
To reach all nodes on the network, it is necessary to send the
broadcast from all sinks.</td><td>By a sink
and any device in CSMA-CA network.</td></tr>

<tr><td>Multicast</td><td>0x8000 0000 - 0x80FF FFFF

(@ref APP_ADDR_MULTICAST bit is set)</td><td>Packet is delivered to the
group of nodes. Group may contain 0 or more nodes. Each node may belong to 0 or
more groups.

The lowest 24 bits contain the actual group address and highest bit is an
indication that message is sent to that group.

For example, address definition of:
@code
APP_ADDR_MULTICAST | 5
@endcode
sends data to a multicast group address 5.</td><td>By a sink and any device in
CSMA-CA network</td></tr>

<tr><td>Reserved addresses</td><td>0x0100 0000 - 0x8000 0000

0x80FF FFFE - 0xFFFF FFFF</td><td>Reserved for future use</td><td>none</td></tr>

</table>

Following picture shows the addressing mechanism differences when sink (black)
sends data to nodes (green and yellow):
@image html addressing.png

</table>

@section endpoint Source and Destination Endpoints

There are two endpoints, <I>source</I> and <I>destination</I> endpoints. When
transmitting data (service @ref app_lib_data_send_data_f "lib_data->sendData"),
they can be used to distinguish different application channels. E.g. if the
device has multiple sensors it could use different endpoint numbers for data
packets containing data from different sensors or different endpoint numbers for
applications with different functionality.

\note Endpoints 240 - 255 are reserved for Wirepas Mesh stack internal use.

@section direction Communication Directions

There are three _directions_ that can be used for transmitting data.

<table>
<tr><th>Direction</th><th>Description</th><th>Which devices can execute</th><th>
Example</th></tr>
<tr><td>Uplink</td><td>Hop-by-hop transmissions towards sink</td><td>Any device</td>
<td>Transmission is done by selecting @ref APP_ADDR_ANYSINK as target address
for @ref app_lib_data_send_data_f "lib_data->sendData"</td></tr>

<tr><td>Downlink</td><td>Transmission from sink towards nodes.

@note Only transmitted to the nodes directly under the sink's routing tree.
To reach all nodes on the network, it is necessary to send the
broadcast from all sinks.

@note Compared to other communication directions, this is the heaviest
communication direction and the throughput of this is way less than
other communication directions.To overcome this problem, the mechanism for
Unacknowledged CSMA-CA transmission is available. This can be done by using
flag @ref APP_LIB_DATA_SEND_FLAG_UNACK_CSMA_CA as an argument for @ref
app_lib_data_send_data_f "lib_data->sendData"</td>
<td>Only sink</td><td>When sink transmit the data, using @ref addressing
"unicast, multicast and broadcast addresses" as target address for @ref
app_lib_data_send_data_f "lib_data->sendData"</td></tr>

<tr><td>Node-2-node communication</td><td>Transmission 'without direction' in
the network.

Transmission from node to other node(s)</td><td>Only CSMA-CA devices, excluding
sinks</td><td>When device transmit the data, using any address except @ref
APP_ADDR_ANYSINK as target address for @ref app_lib_data_send_data_f
"lib_data->sendData"</td></tr>
</table>

@image html communication_directions.png

*/

#endif /* API_DOC_CONCEPTS_H_ */
