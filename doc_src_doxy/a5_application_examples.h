
#ifndef _APPLICATION_EXAMPLES_H_
#define _APPLICATION_EXAMPLES_H_

/**

@page application_examples  Application Examples and Tools

@section source_folder Application examples

Applications exist in source/ subfolders, named after application name. Note that not
all applications exist for every processor architecture.

Applications are categorized into two categories:
- @ref example_applications "Example applications" which can be used as a
  template for own application development. They are simple applications
  targeted to specific use case. They also include unitary apps.
- @ref production_apps "Demos and production apps". They are applications that
  are dedicated to specific purpose. Compared to example applications, they are
  relatively complex and mainly to be used as such.

@anchor example_applications

Example and unitary applications are:

<table>
<tr><th>Application name</th><th>Description</th><th>Notes</th></tr>

<tr><td>@ref battery_voltage_read_app/app.c "battery_voltage_read_app"</td>
<td>Battery voltage reading demo app </td><td></td></tr>

<tr><td>@ref ble_scanner/app.c "ble_scanner"</td>
<td>How to receive BLE beacons</td><td>Only 2.4GHz devices</td></tr>

<tr><td>@ref control_node/app.c "control_node"</td>
<td>To be used with control_router, switch in lighting example</td><td></td></tr>

<tr><td>@ref control_router/app.c "control_router"</td>
<td>To be used with control_node, lighting fixture</td><td></td></tr>

<tr><td>@ref custom_app/app.c "custom_app"</td>
<td>Simple data transmission and reception</td><td></td></tr>

<tr><td>@ref evaluation_app/app.c "evaluation_app"</td>
<td>Wirepas Massive discovery application</td><td></td></tr>

<tr><td>@ref inventory_app_router/app.c "inventory_app_router"</td>
<td>Inventory application using directed-advertiser for headnodes</td>
<td></td></tr>

<tr><td>@ref inventory_app_tag/app.c "inventory_app_tag"</td>
<td>Inventory application using directed-advertiser for advertisers</td>
<td></td></tr>

<tr><td>@ref low_latency_app/app.c "low_latency_app"</td>
<td>Low-latency mode demonstration app</td>
<td></td></tr>

<tr><td>@ref minimal_app/app.c "minimal_app"</td>
<td>Minimal app that just starts the stack</td>
<td></td></tr>

<tr><td>@ref ruuvi_evk/app.c "ruuvi_evk"</td>
<td>Send sensor data </td>
<td>Only Ruuvitag</td></tr>

<tr><td>@ref aes/app.c "aes"</td>
<td>Test software AES library</td><td></td></tr>

<tr><td>@ref app_persistent/app.c "app_persistent"</td>
<td>App persistent feature demo</td><td></td></tr>

<tr><td>@ref appconfig/app.c "appconfig"</td>
<td>Receiving application configuration</td><td></td></tr>

<tr><td>@ref basic_interrupt/app.c "basic_interrupt"</td>
<td>How to use interrupt service</td><td>Only Nordic nRF52xx</td></tr>

<tr><td>@ref ble_tx/app.c "ble_tx"</td>
<td>How to transmit BLE beacons</td><td>Only 2.4GHz devices</td></tr>

<tr><td>@ref blink/app.c "blink"</td>
<td>Very simple blink/hello world application</td><td></td></tr>

<tr><td>@ref diradv/app.c "diradv"</td>
<td>Direct Advertiser demo app</td><td></td></tr>

<tr><td>@ref local_provisioning/app.c "local_provisioning"</td>
<td>Local provisioning demo app</td><td></td></tr>

<tr><td>@ref nfc/app.c "nfc"</td>
<td>NFC peripheral usage</td><td>Only Nordic nRF52xx</td></tr>

<tr><td>@ref provisioning_joining_node/app.c "provisioning_joining_node"</td>
<td>Using provisioning for a joining node</td><td></td></tr>

<tr><td>@ref provisioning_proxy/app.c "provisioning_proxy"</td>
<td>Using provisioning for a proxy node</td><td></td></tr>

<tr><td>@ref scheduler/app.c "scheduler"</td>
<td>How to use @ref app_scheduler.h "Application scheduler"</td><td></td></tr>

<tr><td>@ref shared_data/app.c "shared_data"</td>
<td>How to use @ref shared_data.h</td><td></td></tr>

<tr><td>@ref tinycbor/app.c "tinycbor"</td>
<td>use of the tiny cbor library</td><td></td></tr>

</table>

@anchor production_apps

Production apps are following:

<table>
<tr><th>Application name</th><th>Description</th><th>Notes</th></tr>

<tr><td>@ref dualmcu_app/app.c "dualmcu_app"</td>
<td>Implementation of dual-MCU API interface, used most commonly in sinks</td>
<td></td></tr>

<tr><td>@ref positioning_app/app.c "positioning_app"</td>
<td>Application to acquire network data for the positioning use case</td>
<td>Only Nordic nRF52xx</td></tr>


</table>

This page contains following sections:
- @subpage source_app_c
- @subpage source_config_mk
- @subpage source_makefile

@subsection source_app_c app.c

This file is present in all applications. Application init function and
its application callback functions should be implemented here as defined
in the <code>app.h</code> file.

@subsection source_config_mk config.mk

<code>config.mk</code> file is used to configure application compilation. Here,
the contents of that file have been described. Only fields that are common to
all applications are described here! Some applications may have own,
application-specific fields which are out of the scope of this documentation.
For example, content of the file could be following:

@code
# Define a specific application area_id
app_specific_area_id=0x8CEC79

# App version
app_major=1
app_minor=0
app_maintenance=0
app_development=0
@endcode

This page contains following sections:
- @subpage app_specific_area_id
- @subpage app_version
- @subpage app_target_boards

@subsection app_specific_area_id app_specific_area_id

Wirepas network supports <I>Over The Air</I> (OTAP) updates of devices on the
network.

A network can contain heterogenous devices (different types of sensors,
for example), so various different kinds of applications can coexist
simultaneously in a network.

Even when upgrading a subset of nodes in the network (just nodes with a
specific type of sensor, for example), all nodes will receive the update
image. This is to ensure that all nodes on the network receive the
update image, due to the multi-hop nature of the network. Consequently,
it is crucial for a node to know if the received image, called a
<I>scratchpad</I>, contains any updates for the node in question. This is the
purpose of the application area ID.

More information about <I>Over The Air Protocol</I> can be found in \ref
relmat1 "[1]".

Each new application <B>must</B> define its own
<code>app_specific_area_id</code> in its <code>config.mk</code> file. It is a
random 24-bit field that must have its most significant bit set to 1 (MSB to 0
is reserved for Wirepas):

@code
    # Define a specific application area_id
    app_specific_area_id=0x83744C
@endcode

This specific area ID will be used in two different places:

- The <I>bootloader</I>, which contains a list of area IDs that it
  accepts, when a new scratchpad is processed. All the nodes that will
  be flashed with the image generated from an
  application build (containing the bootloader) will *only* accept
  updates of the application matching the specific area ID defined in
  <code>config.mk</code>.

- In the <I>scratchpad generation tool</I> (@ref genscratchpad.py),
  where it identifies the application scratchpad image. All the
  <code>*.otap</code> images generated from this application build will be
  only accepted by nodes flashed with a bootloader matching this area
  ID.

The <code>app_specific_area_id</code> is only part of the scratchpad area id
existing on the device. The whole scratchpad area id is composed on 4 bytes,
as following:

<table>
<tr><th>byte #</th><th>content</th></tr>
<tr><td>MSB byte 0</td><td>app_specific_area_id MSB byte 0</td></tr>
<tr><td>byte 1</td><td>app_specific area id byte 1</td></tr>
<tr><td>byte 2</td><td>app_specific area id LSB byte 2</td></tr>
<tr><td>byte 3 LSB</td>
<td>hardware magic (see @ref app_lib_system_hardware_magic_e)</td></tr>
</table>

For example, if <code>app_specific_area_id</code> has value of <code>0x83744C
</code> and used hardware is Nordic nRF52 (i.e. @ref
app_lib_system_hardware_magic_e is @ref
APP_LIB_SYSTEM_HARDWARE_MAGIC_NRF52832_SP_2), the resulted area id stored in the
devices is:
@code
(0x83744C << 8) | 3 = 0x83744C03
@endcode

To summarize, in order to have unique scratchpad image on the device, the image
*must have*
- Unique <code>app_specific_area_id</code> OR
- Unique processor architecture.

If, however, same application is built on different board (but having same
processor architecture), it will result same area id and otap mechanism cannot
disinguish the images from others.

@note It is up to each customer to maintain
its own set of <code>app_area_id</code> to avoid mismatch between its
applications.

@subsection app_version Application version

Following definitions:

@code
# App version
app_major=1
app_minor=0
app_maintenance=0
app_development=0
@endcode

define the version of the application software. It is different from stack
software and is application- specific. This information can be used to pinpoint
which version of the application is running on the device.

@subsection app_target_boards TARGET_BOARDS

This configuration defines the @ref board_folder "boards" which application
supports. Configuration is optional.

@note If configuration is not present, it equals that it is supported on _every_
board available. Usually that is not the case so it is better to ensure that
only boards supported are listed here. Then, if incorrect board is tried to be
compiled, it is ensured that compilation fails.

Example:
@code
TARGET_BOARDS := pca10056 pca10059 pca10040 wirepas_brd4254a silabs_brd4254a tbsense2 ublox_b204 promistel_rpi_hat
@endcode

@subsection source_makefile makefile

Application specific makefile contains application-specific build recipes. Here,
the common features are documented:

This page contains following sections:
- @subpage source_makefile_app_printing
- @subpage source_makefile_app_scheduler
- @subpage source_makefile_cflags
- @subpage source_makefile_hal_gpio
- @subpage source_makefile_hal_button
- @subpage source_makefile_hal_hw_delay
- @subpage source_makefile_hal_i2c
- @subpage source_makefile_hal_led
- @subpage source_makefile_hal_persistent_memory
- @subpage source_makefile_hal_spi
- @subpage source_makefile_hal_uart
- @subpage source_makefile_includes
- @subpage source_makefile_ldflags
- @subpage source_makefile_libs
- @subpage source_makefile_shared_data
- @subpage source_makefile_provisioning
- @subpage source_makefile_provisioning_proxy
- @subpage source_makefile_srcs
- @subpage source_makefile_sw_aes

@subsection source_makefile_app_printing APP_PRINTING

Enabling of @ref uart_print.h app debug prints can be done with this
configuration.

Example:
@code
# Enable application debug prints
APP_PRINTING=yes
@endcode

@subsection source_makefile_app_scheduler APP_SCHEDULER

Using of @ref app_scheduler.h "app scheduler" can be done by using this flag.

Example:
@code
# Use App Scheduler. Declare 4 tasks
APP_SCHEDULER=yes
APP_SCHEDULER_TASKS=4
@endcode

@subsection source_makefile_cflags CFLAGS

Introducing custom compilation flags for compilation can be done by extending
this list.

Example:
@code
CFLAGS += -DOWN_FLAG
@endcode

@note Commonly this is used in many applications to propagate network address
and network channel defined in @ref config.mk "config.mk" file to compilation
flags:
@code
# Define default network settings
CFLAGS += -DNETWORK_ADDRESS=$(default_network_address)
CFLAGS += -DNETWORK_CHANNEL=$(default_network_channel)
@endcode

@subsection source_makefile_hal_gpio HAL_GPIO

Using of @ref gpio.h "HAL for GPIOs" can be done by this flag.

Example:
@code
# This application use HAL for GPIOs
HAL_GPIO=yes
@endcode

@note: in order for application to be able to drive GPIOs, they must be
defined in specific @ref board_folder "board".

@subsection source_makefile_hal_button HAL_BUTTON

Using of @ref button.h "HAL for buttons" can be done by this flag.

Example:
@code
# This application use HAL for buttons
HAL_BUTTON=yes
@endcode

@note: in order for application to be able to drive buttons, they must be
defined in specific @ref board_folder "board".

@subsection source_makefile_hal_hw_delay HAL_HW_DELAY

Using of @ref hw_delay.h "HAL for hardware delay" can be done by this flag.

Example:
@code
HAL_HW_DELAY=yes
@endcode

@subsection source_makefile_hal_i2c HAL_I2C

Using of @ref i2c.h "HAL for I2C interface" can be done by this flag.

Example:
@code
HAL_I2C=yes
@endcode

@note This option is supported only on nRF52 architectures.

@subsection source_makefile_hal_led HAL_LED

Using of @ref led.h "HAL for led driving" can be done by this flag.

Example:
@code
# This application use HAL for leds
HAL_LED=yes
@endcode

@note: in order for application to be able to drive LEDs, they must be defined
in specific @ref board_folder "board".

@subsection source_makefile_hal_persistent_memory HAL_PERSISTENT_MEMORY

Using of @ref persistent.h "HAL for persistent memory" can be done by this flag.

@note You need to define <code>-DUSE_PERSISTENT_MEMORY</code> by using @ref
source_makefile_cflags "CLAGS" option.

Example:
@code
# Use persistent memeory
HAL_PERSISTENT_MEMORY=yes
CFLAGS += -DUSE_PERSISTENT_MEMORY
@endcode

@subsection source_makefile_hal_spi HAL_SPI

Using of @ref spi.h "HAL for SPI interface" can be done by this flag.

Example:
@code
HAL_SPI=yes
@endcode

@note This option is supported only on nRF52 architectures.

@subsection source_makefile_hal_uart HAL_UART

Using of @ref usart.h "HAL for UART interface" can be done by this flag.

Example:
@code
HAL_UART=yes
@endcode

@note: in order for application to be able to UART, they must be defined
in specific @ref board_folder "board".

There is also related flag <code>UART_USE_DMA</code> that can be used to enable
DMA functionality.

Example:
@code
HAL_UART=yes
UART_USE_DMA=yes
@endcode

@subsection source_makefile_includes INCLUDES

Extending to to include folders for include search paths can be done by
extending this list.

Example:
@code
INCLUDES +=
@endcode

@subsection source_makefile_ldflags LDFLAGS

Introducing custom linker flags for ld can be done by extending this list.

Example:
@code
#Link standard C math library
LDFLAGS += -lm
@endcode

@subsection source_makefile_libs LIBS

Introducing of precompiled libraries (.a files) for compilation can be done
by extending this list.

Example:
@code
LIBS +=
@endcode

@subsection source_makefile_provisioning PROVISIONING

Using of @ref provisioning.h "provisioning library" can be done by using
this flag.

Example:
@code
# Use Provisioning
PROVISIONING=yes
@endcode

@subsection source_makefile_provisioning_proxy PROVISIONING_PROXY

Using of @ref provisioning.h "provisioning library (for the existing node)"
can be done by using this flag.

Example:
@code
# Use Provisioning Proxy
PROVISIONING_PROXY=yes
@endcode

@subsection source_makefile_shared_data SHARED_DATA

Using of @ref shared_data.h "shared data library" can be done by using
this flag.

Example:
@code
# Use Shared Data
SHARED_DATA=yes
@endcode

@subsection source_makefile_srcs SRCS

Introducing of new source files can be done by extending this list.

Example:
@code
SRCS +=
@endcode

@subsection source_makefile_sw_aes SW_AES

@ref aes.h "Software AES library" can be enabled by using this configuration.

Example:
@code
# Enable software AES
SW_AES=yes
@endcode

@section tools_folder Tools

This folder contains various tools, mainly implemented in Python, used during
build process of the application.

This page contains following sections:
- @subpage genscratchpad_py
- @subpage config_mk_ini_file

@subsection genscratchpad_py genscratchpad.py

@ref genscratchpad.py "This tool", used by the @ref makefile "Makefile", allows
the scratchpad binary generation for the OTAP update. It uses the
<code>ini</code> file in the same directory

@subsection config_mk_ini_file INI_FILE

All scratchpad images received <I>Over The Air</I> are compressed and
encrypted.

The bootloader authenticates the image and decrypts it, so
authentication and encryption keys used to generate a scratchpad must
match the ones stored in the bootloader.

The list of keys stored in bootloader can be configured at the end of
<code>tools/scratchpad_\<mcu\>.ini</code> file. The bootloader can contain several
keys and will decrypt a received scratchpad, if it can be authenticated
with any of the listed keys.

By default, each application uses the same <code>.ini</code> file stored in
<code>tools/scratchpad_\<mcu\>.ini</code> but it can be copied to the application
folder and modified as needed. The new <code>.ini</code> file can be specified
from the application as following:

@code
    # Define a specific scratchpad ini
    INI_FILE = $(APP_SRCS_PATH)/scratchpad_custom.ini
@endcode

The scratchpad generated from an application build use the key named
<code>default</code> from the <code>.ini</code> file. It can be changed by
adding the <code>--keyname</code> option to the invocation of
<code>genscratchpad.py</code> script from makefile.


 */

/** @example aes/app.c
 */

/** @example debug/app.c
 */

/** @example appconfig/app.c
 */

/** @example basic_interrupt/app.c
 */

/** @example ble_scanner/app.c
 */

/** @example ble_tx/app.c
 */

/** @example blink/app.c
 */

/** @example custom_app/app.c
 */

/** @example dualmcu_app/app.c
 */

/** @example inventory_app_router/app.c
 */

/** @example inventory_app_tag/app.c
 */

/** @example minimal_app/app.c
 */

/** @example nfc/app.c
 */

/** @example positioning_app/app.c
 */

/** @example provisioning_joining_node/app.c
 */

/** @example provisioning_proxy/app.c
 */

/** @example ruuvi_evk/app.c
 */

/** @example scheduler/app.c
 */

/** @example shared_data/app.c
 */

/** @example tinycbor/app.c
 */

/** @example board/efr32_template/bootloader/early_init_efr32.c
 */
#endif /* API_DOC_APPLICATION_EXAMPLES_H_ */
