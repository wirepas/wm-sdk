#ifndef DOC_SDK_FILE_STRUCTURE_H_
#define DOC_SDK_FILE_STRUCTURE_H_

/**

@page sdk_file_structure SDK file structure

This page describes the file structure of the SDK. Here, you can find the
purpose of directories and files.

This page contains following sections:
- @subpage api_folder
- @subpage board_folder
- @subpage bootloader_folder
- @subpage debug_folder
- @subpage doc_folder
- @subpage image_folder
- @subpage libraries_folder
- @subpage makefile
- @subpage mcu_folder
- @subpage project_folder
- @subpage source_folder
- @subpage tools_folder
- @subpage util_folder

@page api_folder api/ Libraries offered by stack

The Wirepas Mesh stack (hereafter referred to as the *stack*) runs on ARM Cortex
M based microcontrollers. Hence, the stack and the application both follow the
Procedure Call Standard for the ARM Architecture. The stack and the application
communicate with each other by calling functions on each other.

The protocol stack is in control of program execution. Application can request
to be called periodically, or when certain events happen. Applications can also
directly access hardware peripherals that are not used by the stack.
Applications can have interrupt handlers, but the service routines must be kept
short so they do not interfere with the timing of the Wirepas Mesh protocol.

To keep features in logically separate units, stack functions are collected into
<I>libraries</I>. The application opens each library it needs and calls
functions in the library via function pointers. A global list of functions is
given to the application when it is first called, so that it can open libraries
and find out details about the environment in which it is running.

The following table contains following information on libraries:
- Name of the library
- Library handle set by \ref API_Open "API_Open()" function. Application can use
  this handle to access the services.
- Short description of the library

<table>
<tr><th>Header file</th><th>Handle</th><th>Description</th></tr>
<tr><td>@ref advertiser.h</td><td>@ref app_lib_advertiser_t "lib_advertiser"
</td><td>Application library for direct advertiser functionality</td></tr>
<tr><td>@ref app.h</td><td>NA</td><td>The global macros, types and functions
available to applications can be found in the app.h header</td></tr>
<tr><td>@ref beacon_rx.h</td><td>@ref app_lib_beacon_rx_t "lib_beacon_rx"</td>
<td>Application library for Bluetooth LE beacon RX</td></tr>
<tr><td>@ref beacon_tx.h</td><td>@ref app_lib_beacon_tx_t "lib_beacon_tx"</td>
<td>Transmission of Bluetooth LE compatible beacons</td></tr>
<tr><td>@ref data.h</td><td>@ref app_lib_data_t "lib_data"</td><td>Sending and
receiving data packets</td></tr>
<tr><td>@ref hardware.h</td><td>@ref app_lib_hardware_t "lib_hw"</td><td>Sharing
of hardware peripherals between stack and application</td></tr>
<tr><td>@ref memory_area.h</td><td>@ref app_lib_memory_area_t "lib_memory_area"
</td><td>Access to non-volatile memory areas</td></tr>
<tr><td>@ref otap.h</td><td>@ref app_lib_otap_t "lib_otap"</td><td>The
Over-The-Air-Programming (OTAP) library</td></tr>
<tr><td>@ref radio_fem.h</td><td>@ref app_lib_radio_fem_t "lib_radio_fem"</td>
<td>Application library for radio power and front end module control</td></tr>
<tr><td>@ref settings.h</td><td>@ref app_lib_settings_t "lib_settings"</td><td>
Access to node settings, which are stored in nonvolatile memory</td></tr>
<tr><td>@ref sleep.h</td><td>@ref app_lib_sleep_t "lib_sleep"</td><td>Sleep
Wirepas Mesh stack for time periods</td></tr>
<tr><td>@ref state.h</td><td>@ref app_lib_state_t "lib_state"</td><td>Viewing
and controlling stack runtime state</td></tr>
<tr><td>@ref storage.h</td><td>@ref app_lib_storage_t "lib_storage"</td><td>
Small Non-volatile storage area to use for application</td></tr>
<tr><td>@ref system.h</td><td>@ref app_lib_system_t "lib_system"</td><td>
Low-level functions such as application scheduling, interrupt handling, critical
sections and power management</td></tr>
<tr><td>@ref test.h</td><td>@ref app_lib_test_t "lib_test"</td><td>Library for
radio testing services</td></tr>
<tr><td>@ref time.h</td><td>@ref app_lib_time_t "lib_time"</td><td>Keeping track
of time and comparing timestamps</td></tr>
</table>
<b>NOTE</b>: Not all of the services are available in every platform!

<!-- The following subsubsection is intentionally violating 80 character limit
due to Doxygen inability to have multiline titles. Do not fix it! -->

This page contains following sections:
- @subpage application_and_library_versioning

@page application_and_library_versioning Application and Library Versioning

To keep features in logically separate units and allow each unit to be updated
in a *backward- and forward-compatible* manner, stack functions are collected
into *libraries*. When an application requests a specific version of a library
and no exact version is found, the stack can do one of three things:

1. *Return a newer, compatible library*: This works if the newer library version
   has new features that have been added in such a way that the old API still
   works as expected. This is ideally the way new firmware releases add features
   to the libraries.

2. *Emulate the old library*: If the new firmware has the library, but it has an
   incompatible API and application calls <code>@ref app_open_library_f
   "openLibrary()"</code> with an old version number. Then, stack returns an
   emulated version of the old library, which just calls functions in the new
   library.

3. *Fail to open the library*: Old, obsoleted features can be phased out by
   simply failing to open a library that is too old. Stack firmware release
   notes will list the obsoleted libraries for each release. Then, the
   associated library handle will be NULL.

In addition of library versioning, applications also have an API version number,
@ref APP_API_VERSION, placed in their header. The SDK places the version number
there to indicate which version of the low-level application API the application
supports. If there is a mismatch between the low-level API versions of the stack
and the application, the stack may choose to not run the application.

@page board_folder board/ Board definitions

This folder contains definitions for various boards. Board approach allows
executing same application in different radio boards. Mainly, board definition
define GPIO pins for various purposes, such as serial port pins, leds and
buttons.

Each subdirectory name defines the name of the board used in the build process
(see @ref app_target_boards "here" for how to enable application to use specific
board).

Especially, there are two _template_ boards that contain the documentation of
the board definitions, according to processor architecture:

<table>
<tr><th>Processor architecture</th><th>File</th><th>File description</th></tr>

<tr><td>EFR32</td><td>@ref board/efr32_template/board.h
"board/efr32_template/board.h"</td><td>Board definition</td></tr>

<tr><td>EFR32</td><td>@ref board/efr32_template/config.mk
"board/efr32_template/config.mk"</td><td>Board Configuration</td></tr>

<tr><td>nRF52</td><td>@ref board/nrf52_template/board.h
"board/nrf52_template/board.h"</td><td>Board definition</td></tr>

<tr><td>nRF52</td><td>@ref board/nrf52_template/config.mk
"board/nrf52_template/config.mk"</td><td>Board Configuration</td></tr>
</table>

This page contains following sections:
- @subpage DCDC_converter
- @subpage crystal_32k
- @subpage platform

@page DCDC_converter DCDC converter configuration

DCDC converter configuration is very important topic, especially if low energy
consumption is desired. DCDC converter is automatically enabled from the
bootloader @ref early_init.h "early_init() function".

To enable the DCDC converter, board_hw_dcdc must be set to yes in
@ref board/nrf52_template/config.mk "board config.mk" file.

@page crystal_32k 32kHz crystal configuration

For most applications the 32kHz crystal must be mounted on the board. But for
some specific application like lighting, it is possible to omit this crystal on
board running in Low Latency. Note that the crystal is mandatory for battery
operated nodes in Low Energy.

To inform the stack that the board doesn't have 32kHz crystal,
board_hw_crystal_32k must be set to no in
@ref board/nrf52_template/config.mk "board config.mk" file.

@page platform platform specific hardware configuration

Nordic nRF52 platform does not have platform specific hardware descriptions
that could be configured.

Silabs EFR32 platform requires descriptions of lfxo and hfxo crystals.
There are three values that are specific to each board design due selected
crystal model and parasitic capacitance caused by wiring of the crystal
on the board: board_hw_hfxo_ctune, board_hw_lfxo_ctune and
board_hw_lfxo_gain. As the parasitic capacitance plays significant role
in selection of these values, it is recommended to individually
tune tens of devices to find average value to be used for the
board. For more information about crystal oscillators
see Silabs application notes an0016.1 and an0016.2.
@ref board/efr32_template/config.mk "board config.mk" file.

@page bootloader_folder bootloader/ Bootloader configuration

This folder contains configuration mechanisms for bootloader operations. By
default, they are weak symbols implementing stub functions. THey can be
overridden in the application, if desired.

Following files are present:

<table>
<tr><th>Name</th><th>Description</th></tr>
<tr><td>@ref early_init.h "early_init.h"</td>
<td>Hooks called early during boot process</td></tr>
<tr><td>@ref external_flash.h "external_flash.h"</td>
<td>External flash operation</td></tr>
<tr><td>@ref bl_hardware.h "bl_hardware.h"</td>
<td>Board hardware capabilities description</td></tr>
</table>

@page debug_folder debug/ Debug printing

This folder contains implementation of debugs.

@page doc_folder doc/ Doxygen documentation

This folder contains files for generating doxygen documentation not present
in source codes itself, such as this page.

@page image_folder image/ Firmware images folder

This folder contains Wirepas firmware images. Originally folder does not contain
any images but when you have @ref licensing "licensed" Wirepas Mesh, you will
get an access to firmware images which should be unpacked here.

@page libraries_folder libraries/ SDK Internal API

There are numerous services that are given as a source code as part of the SDK.
They are for various purposes, like easier access for low level hardware (HAL)
routines. They are only present in the generated binaries if they are used.

First group of services is Wirepas wrapper services. They are abstractions of
single mcu api to ease the implementation. They are located in @ref libraries
"libraries" folder.

Following table, summarize these services and files by:
- Name and path of the header file
- Short description

<table>
<tr><th>Name</th><th>Description</th></tr>
<tr><td>@ref app_scheduler.h "app_scheduler.h"</td><td>Scheduling of
multiple application tasks</td></tr>
<tr><td>@ref control_node.h "control_node.h"</td><td>Control node using
Directed Advertiser</td></tr>
<tr><td>@ref shared_data.h "shared_data.h"</td><td>Handling of data
packets</td></tr>
<tr><td>@ref uart_print.h "uart_print.h"</td><td>UART print module
</td></tr>
<tr><td>@ref libraries/provisioning/provisioning.h "provisioning/provisioning.h"
</td><td>Provisioning</td></tr>
</table>

@page makefile makefile

The makefile is used to build an application and produce images to be
flashed or used with OTAP mechanism.

All built files for a given app and target_board are stored under
<code>build/\<target_board\>/\<app_name\></code>:

-  <code>final_image_\<app_name\>.hex</code>: this image can be flashed on a
   blank chip. It contains everything (Bootloader, Wirepas Mesh Stack
   and the application).

-  <code>\<app_name\>.otap</code>: this image contains only the application and
   can be used to update the application with the OTAP mechanism.

-  <code>\<app_name\>_wc_stack.otap</code>: this image contains the application
   and the Wirepas Mesh stack and can be used to update the application
   and the stack at the same time with the OTAP mechanism.

@page mcu_folder mcu/ Low level hardware services (HAL)

This folder contain hardware-specific services.

This page contains following sections:
- @subpage mcu_common
- @subpage mcu_hal_api
- @subpage mcu_specific_files
- @subpage linker_file

@page mcu_common mcu/common Common MCU files

This folder contains common files for all hardware platforms.

This page contains following sections:
- @subpage mcu_common_start_c

@page mcu_common_start_c start.c

@ref start.c "This file" is present in all applications. It positions the application
entry point at the correct place in memory and do basic initialization:
it loads the initialized data from flash to RAM, sets the bss area to 0
in RAM, and calls the application initialization function defined in
<code>app.c</code>.

It also manages compatibility with the stack to avoid issues when
running an application built for an old stack to a newer stack. Running
an application built with a SDK newer than the stack version is not
allowed.

@page mcu_hal_api mcu/hal_api Low level hardware API

Second group of services is low level (HAL) hardware services. They contain the
implementations of various hardware peripherals for various hardware platforms
and boards.

Relevant services are located in @ref mcu/hal_api "mcu/hal_api" folder. Services
are following:

@note Not all of the services are available in every platform!

<table>
<tr><th>Name (related to mcu/hal_api folder)</th><th>Description</th></tr>
<tr><td>@ref button.h "button.h"</td><td>Button functions</td></tr>
<tr><td>@ref ds.h "ds.h"</td><td>Deep sleep control module</td></tr>
<tr><td>@ref fem_driver.h "fem_driver.h"</td><td>Radio FEM (front-end module)
</td></tr>
<tr><td>@ref hal_api.h "hal_api.h"</td><td>Initialization of HAL services</td>
</tr>
<tr><td>@ref hw_delay.h "hw_delay.h"</td><td>Hardware delay module</td></tr>
<tr><td>@ref i2c.h "i2c.h"</td><td>Simple minimal I2C master driver</td></tr>
<tr><td>@ref led.h "led.h"</td><td>LED functions</td></tr>
<tr><td>@ref persistent.h "persistent.h"</td><td>Hardware-specific persistant
memory area</td></tr>
<tr><td>@ref power.h "power.h"</td><td>Enabling of DCDC converter</td></tr>
<tr><td>@ref spi.h "spi.h"</td><td>Simple minimal SPI master driver</td></tr>
<tr><td>@ref usart.h "usart.h"</td><td>USART block handling</td>
</tr>
</table>

@page mcu_specific_files mcu/<hardware>

@ref mcu "These folders" contain mcu specific files to ease and factorize
application development. Header files (<code>.h</code>) from this folder can be
included in applications directly.

@page linker_file Linker file

Linker files are located in <code>mcu/\<processor\>/linker</code> folder. Linker
file is linker script. It ensures that the application is loadable in its
dedicated area in Flash. Particularly, it sets the application entry point at
the beginning of the area.

@note Linker files named <code>gcc_bl_*.ld</code> are for bootloader and they
should never be modified!

This page contains following sections:
- @subpage flash_memory
- @subpage ram_memory

@page flash_memory Flash Memory

To be correctly detected by the Wirepas Mesh stack, the entry point of
the application code must be position at address <code>0x40000</code> (256KB
after the beginning of flash). This is valid for all hardware profiles.

As described with more details in WP-RM-108 - OTAP Reference Manual, the
application shares its flash memory area with the scratchpad area. There
is no strict limit for the size of the application but if the
application is too big, it will prevent the OTAP to store its scratchpad
in the remaining free space.

The recommended maximum size for an application is 40kB.

Example:
@code
MEMORY
{
  FLASH (rx)      : ORIGIN = 0x00040000, LENGTH = 40K
  ...
}
@endcode

@page ram_memory RAM memory

RAM memory is configured in linker file. Value is fixed and should not be
modified.

@page project_folder projects/ Doxygen files

This folder contains necessary files to recreate this documentation.

@page source_folder source/ Application examples

This folder contains applications.

Applications exist in subfolders, named after application name. Note that not
all applications exist for every processor architecture.

Applications are categorized into two categories:
- @ref example_applications "Example applications" which can be used as a
  template for own application development. They are simple applications
  targeted to specific use case.
- @ref production_apps "Demos and production apps". They are applications that
  are dedicated to specific purpose. Compared to example applications, they are
  relatively complex and mainly to be used as such.

@anchor example_applications

Example applications are following:

<table>
<tr><th>Application name</th><th>Description</th><th>Notes</th></tr>

<tr><td>@ref aes_example/app.c "aes_example"</td>
<td>Test software AES library</td><td></td></tr>

<tr><td>@ref app_debug/app.c "app_debug"</td>
<td>Using debug prints</td><td></td></tr>

<tr><td>@ref appconfig_app/app.c "appconfig_app"</td>
<td>Receiving application configuration</td><td></td></tr>

<tr><td>@ref basic_interrupt/app.c "basic_interrupt"</td>
<td>How to use interrupt service</td><td>Only Nordic nRF52xx</td></tr>

<tr><td>@ref ble_app/app.c "ble_app"</td><td>How to receive BLE beacons</td>
<td>Only 2.4GHz devices</td></tr>

<tr><td>@ref ble_beacon/app.c "ble_beacon"</td>
<td>How to transmit BLE beacons</td><td>Only 2.4GHz devices</td></tr>

<tr><td>@ref blink/app.c "blink"</td>
<td>Very simple blink/hello world application</td><td></td></tr>

<tr><td>@ref bme280_example/app.c "bme280_example"</td>
<td>How to use <a href="https://www.bosch-sensortec.com/bst/products/all_products/bme280">Bosch Sensortec BME280 environmental sensor</a> </td>
<td>Only Ruuvitag</td></tr>

<tr><td>@ref control_node/app.c "control_node"</td>
<td>Control node example app using Directed Advertiser</td><td></td></tr>

<tr><td>@ref control_router/app.c "control_router"</td>
<td>Control Router app to be used with control node</td><td></td></tr>

<tr><td>@ref custom_app/app.c "custom_app"</td>
<td>Simple data transmission and reception</td><td></td></tr>

<tr><td>@ref diradv_example/app.c "diradv_example"</td>
<td>How to use directed advertiser- functionality</td><td></td></tr>

<tr><td>@ref inventory_app_router/app.c "inventory_app_router"</td>
<td>Inventory application using directed-advertiser for headnodes</td>
<td></td></tr>

<tr><td>@ref inventory_app_tag/app.c "inventory_app_tag"</td>
<td>Inventory application using directed-advertiser for advertisers</td>
<td></td></tr>

<tr><td>@ref minimal_app/app.c "minimal_app"</td>
<td>Minimal app that just starts the stack</td><td></td></tr>

<tr><td>@ref nfc/app.c "nfc"</td>
<td>NFC peripheral usage</td><td>Only Nordic nRF52xx</td></tr>

<tr><td>@ref provisioning_joining_node/app.c "provisioning_joining_node"</td>
<td>Using provisioning for a joining node</td><td></td></tr>

<tr><td>@ref provisioning_proxy/app.c "provisioning_proxy"</td>
<td>Using provisioning for a proxy node</td><td></td></tr>

<tr><td>@ref pwm_driver/app.c "pwm_driver"</td><td>Create PWM signal</td>
<td>Only Nordic nRF52xx</td></tr>

<tr><td>@ref scheduler_example/app.c "scheduler_example"</td>
<td>How to use @ref app_scheduler.h "Application scheduler"</td><td></td></tr>

<tr><td>@ref shared_data_example/app.c "shared_data_example"</td>
<td>How to use @ref shared_data.h</td><td></td></tr>

<tr><td>@ref test_app/app.c "test_app"</td>
<td>using @ref api/test.h "test library" services</td><td></td></tr>

<tr><td>@ref tinycbor_example/app.c "tinycbor_example"</td>
<td>use of the tiny cbor library</td><td></td></tr>

</table>

@anchor production_apps

Demos and production apps are following:

<table>
<tr><th>Application name</th><th>Description</th><th>Notes</th></tr>

<tr><td>@ref dualmcu_app/app.c "dualmcu_app"</td>
<td>Implementation of dual-MCU API interface, used most commonly in sinks</td>
<td></td></tr>

<tr><td>@ref positioning_app/app.c "positioning_app"</td>
<td>Application to acquire network data for the positioning use case</td>
<td>Only Nordic nRF52xx</td></tr>

<tr><td>@ref ruuvi_evk/app.c "ruuvi_evk"</td>
<td>Send sensor data to backend, control how to send, used in evaluation kits
</td>
<td>Only Ruuvitag</td></tr>

</table>

This page contains following sections:
- @subpage source_app_c
- @subpage source_config_mk
- @subpage source_makefile

@page source_app_c app.c

This file is present in all applications. Application init function and
its application callback functions should be implemented here as defined
in the <code>app.h</code> file.

@page source_config_mk config.mk

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

@page app_specific_area_id app_specific_area_id

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

@page app_version Application version

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

@page app_target_boards TARGET_BOARDS

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

@page source_makefile makefile

Application specific makefile contains application-specific build recipes. Here,
the common features are documented:

This page contains following sections:
- @subpage source_makefile_app_printing
- @subpage source_makefile_app_scheduler
- @subpage source_makefile_cflags
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

@page source_makefile_app_printing APP_PRINTING

Enabling of @ref uart_print.h app debug prints can be done with this
configuration.

Example:
@code
# Enable application debug prints
APP_PRINTING=yes
@endcode

@page source_makefile_app_scheduler APP_SCHEDULER

Using of @ref app_scheduler.h "app scheduler" can be done by using this flag.

@note You also need to enable two compilation flags by using @ref
source_makefile_cflags "CFLAGS" option

Example:
@code
# Use App Scheduler. Declare 4 tasks with maximum execution time of 100 us each
APP_SCHEDULER=yes
CFLAGS += -DAPP_SCHEDULER_MAX_TASKS=4
CFLAGS += -DAPP_SCHEDULER_MAX_EXEC_TIME_US=100
@endcode

@page source_makefile_cflags CFLAGS

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

@page source_makefile_hal_button HAL_BUTTON

Using of @ref button.h "HAL for buttons" can be done by this flag.

Example:
@code
# This application use HAL for buttons
HAL_BUTTON=yes
@endcode

@note: in order for application to be able to drive buttons, they must be
defined in specific @ref board_folder "board".
@note This option is supported only on nRF52 architectures.

@page source_makefile_hal_hw_delay HAL_HW_DELAY

Using of @ref hw_delay.h "HAL for hardware delay" can be done by this flag.

Example:
@code
HAL_HW_DELAY=yes
@endcode

@page source_makefile_hal_i2c HAL_I2C

Using of @ref i2c.h "HAL for I2C interface" can be done by this flag.

Example:
@code
HAL_I2C=yes
@endcode

@note This option is supported only on nRF52 architectures.

@page source_makefile_hal_led HAL_LED

Using of @ref led.h "HAL for led driving" can be done by this flag.

Example:
@code
# This application use HAL for leds
HAL_LED=yes
@endcode

@note: in order for application to be able to drive LEDs, they must be defined
in specific @ref board_folder "board".

@page source_makefile_hal_persistent_memory HAL_PERSISTENT_MEMORY

Using of @ref persistent.h "HAL for persistent memory" can be done by this flag.

@note You need to define <code>-DUSE_PERSISTENT_MEMORY</code> by using @ref
source_makefile_cflags "CLAGS" option.

Example:
@code
# Use persistent memeory
HAL_PERSISTENT_MEMORY=yes
CFLAGS += -DUSE_PERSISTENT_MEMORY
@endcode

@page source_makefile_hal_spi HAL_SPI

Using of @ref spi.h "HAL for SPI interface" can be done by this flag.

Example:
@code
HAL_SPI=yes
@endcode

@note This option is supported only on nRF52 architectures.

@page source_makefile_hal_uart HAL_UART

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

@page source_makefile_includes INCLUDES

Extending to to include folders for include search paths can be done by
extending this list.

Example:
@code
INCLUDES +=
@endcode

@page source_makefile_ldflags LDFLAGS

Introducing custom linker flags for ld can be done by extending this list.

Example:
@code
#Link standard C math library
LDFLAGS += -lm
@endcode

@page source_makefile_libs LIBS

Introducing of precompiled libraries (.a files) for compilation can be done
by extending this list.

Example:
@code
LIBS +=
@endcode

@page source_makefile_provisioning PROVISIONING

Using of @ref provisioning.h "provisioning library" can be done by using
this flag.

Example:
@code
# Use Provisioning
PROVISIONING=yes
@endcode

@page source_makefile_provisioning_proxy PROVISIONING_PROXY

Using of @ref provisioning.h "provisioning library (for the existing node)"
can be done by using this flag.

Example:
@code
# Use Provisioning Proxy
PROVISIONING_PROXY=yes
@endcode

@page source_makefile_shared_data SHARED_DATA

Using of @ref shared_data.h "shared data library" can be done by using
this flag.

Example:
@code
# Use Shared Data
SHARED_DATA=yes
@endcode

@page source_makefile_srcs SRCS

Introducing of new source files can be done by extending this list.

Example:
@code
SRCS +=
@endcode

@page source_makefile_sw_aes SW_AES

@ref aes.h "Software AES library" can be enabled by using this configuration.

Example:
@code
# Enable software AES
SW_AES=yes
@endcode

@page tools_folder tools/ Various tools used in build process

This folder contains various tools, mainly implemented in Python, used during
build process of the application.

This page contains following sections:
- @subpage genscratchpad_py
- @subpage config_mk_ini_file

@page genscratchpad_py genscratchpad.py

@ref genscratchpad.py "This tool", used by the @ref makefile "Makefile", allows
the scratchpad binary generation for the OTAP update. It uses the
<code>ini</code> file in the same directory

@page config_mk_ini_file INI_FILE

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

@note All released SDK have the same default keys (known by all users), so these
keys cannot be considered secure.

@note It is highly recommended to modify these keys before any deployment, to
reduce the risk of malicious network access.

@page util_folder utils/ Utility and helper services

Thirs group of services is utility and helper functions. They are various,
hardware-independent, functions solely to ease the implementation. They are
located in @ref util "util" folder.

<table>
<tr><th>Name</th><th>Description</th></tr>
<tr><td>@ref api.h "api.h"</td><td>Global API for @ref stack_libraries
"Wirepas library services"</td></tr>
<tr><td>@ref crc.h "crc.h"</td><td>Implementation of <A
HREF="http://srecord.sourceforge.net/crc16-ccitt.html">"bad CRC-16 CCITT"</A>
algorithm</td></tr>
<tr><td>@ref doublebuffer.h "doublebuffer.h"</td><td>Managing of double
buffering</td></tr>
<tr><td>@ref node_configuration.h "node_configuration.h"</td><td>Helper function
to initially setup a node if not already configured. This configuration can be
modified later with remote API.</td></tr>
<tr><td>@ref pack.h "pack.h"</td><td>Little Endian bytes to native integers
packing and unpacking</td></tr>
<tr><td>@ref random.h "random.h"</td><td>Random number generator</td></tr>
<tr><td>@ref ringbuffer.h "ringbuffer.h"</td><td>Ring buffers</td></tr>
<tr><td>@ref sl_list.h "sl_list.h"</td><td>Single Linked List</td></tr>
<tr><td>@ref tlv.h "tlv.h"</td><td>Encoding/decoding of TLV (Type Length Value)
format</td></tr>
<tr><td>@ref util.h "util.h"</td><td>Miscellaneous helper functions</td></tr>
</table>

 */

#endif /* DOC_SDK_FILE_STRUCTURE_H_ */
