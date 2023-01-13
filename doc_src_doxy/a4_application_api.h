
#ifndef _APPLICATION_API_H_
#define _APPLICATION_API_H_

/**

@page programming_interface API

API is split into following areas:

- @subpage stack_api "Stack Libraries". These are
  implemented in stack and header files are offered to access those.
- @subpage libraries_api "SDK Libraries". These are given
  as source code in SDK under libraries folder.
- @subpage board_api "Board definitions", i.e. pinout mappings
  for various boards
- @subpage bootloader_api "Bootloader configuration", i.e.
  early initialization functionality
- @subpage mcu_api "Low level hardware services (HAL)", i.e.
  low level routines, such as access to peripherals
- @subpage util_api "Utility and helper services", i.e.
  various utility type functions, not related to stack operations as such, such
  as random number generation.

@section stack_api Stack Libraries

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
<tr><td>@ref wms_advertiser.h</td><td>@ref app_lib_advertiser_t "lib_advertiser"
</td><td>Application library for direct advertiser functionality</td></tr>
<tr><td>@ref wms_app.h</td><td>NA</td><td>The global macros, types and functions
available to applications can be found in the wms_app.h header</td></tr>
<tr><td>@ref wms_beacon_rx.h</td><td>@ref app_lib_beacon_rx_t "lib_beacon_rx"</td>
<td>Application library for Bluetooth LE beacon RX</td></tr>
<tr><td>@ref wms_beacon_tx.h</td><td>@ref app_lib_beacon_tx_t "lib_beacon_tx"</td>
<td>Transmission of Bluetooth LE compatible beacons</td></tr>
<tr><td>@ref wms_data.h</td><td>@ref app_lib_data_t "lib_data"</td><td>Sending and
receiving data packets</td></tr>
<tr><td>@ref wms_hardware.h</td><td>@ref app_lib_hardware_t "lib_hw"</td><td>Sharing
of hardware peripherals between stack and application</td></tr>
<tr><td>@ref wms_joining.h</td><td>@ref app_lib_joining_t "lib_joining"</td><td>Wirepas
Open Joining protocol</td></tr>
<tr><td>@ref wms_memory_area.h</td><td>@ref app_lib_memory_area_t "lib_memory_area"
</td><td>Access to non-volatile memory areas</td></tr>
<tr><td>@ref wms_otap.h</td><td>@ref app_lib_otap_t "lib_otap"</td><td>The
Over-The-Air-Programming (OTAP) library</td></tr>
<tr><td>@ref wms_radio_config.h</td><td>@ref app_lib_radio_cfg_t "lib_radio_config"</td>
<td>Application library for radio power and front end module control</td></tr>
<tr><td>@ref wms_settings.h</td><td>@ref app_lib_settings_t "lib_settings"</td><td>
Access to node settings, which are stored in nonvolatile memory</td></tr>
<tr><td>@ref wms_sleep.h</td><td>@ref app_lib_sleep_t "lib_sleep"</td><td>Sleep
Wirepas Mesh stack for time periods</td></tr>
<tr><td>@ref wms_state.h</td><td>@ref app_lib_state_t "lib_state"</td><td>Viewing
and controlling stack runtime state</td></tr>
<tr><td>@ref wms_storage.h</td><td>@ref app_lib_storage_t "lib_storage"</td><td>
Small Non-volatile storage area to use for application</td></tr>
<tr><td>@ref wms_system.h</td><td>@ref app_lib_system_t "lib_system"</td><td>
Low-level functions such as application scheduling, interrupt handling, critical
sections and power management</td></tr>
<tr><td>@ref wms_time.h</td><td>@ref app_lib_time_t "lib_time"</td><td>Keeping track
of time and comparing timestamps</td></tr>
</table>
@note Not all of the services are available in every platform!

<!-- The following subsubsection is intentionally violating 80 character limit
due to Doxygen inability to have multiline titles. Do not fix it! -->

@subsection application_and_library_versioning Application and Library Versioning

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

@section libraries_api SDK Libraries

There are numerous services that are given as a source code as part of the SDK.
They are for various purposes, like higher API level or application services
implemented by Wirepas. They are only present in the generated binaries if they are used.

They are abstractions of single mcu api to ease the implementation.
They are located in @ref libraries "libraries" folder.

@note When a library is enabled, some single mcu api cannot be used anymore directly from
application. It would result in undefined behavior. Those api are listed in each library
init function description.

Following table, summarize these libraries:

<table>
<tr><th>Name</th><th>Description</th></tr>
<tr><td>@ref app_persistent.h "app_persistent"</td><td>Managing persistent data area</td></tr>
<tr><td>@ref dualmcu_lib.h "dualmcu"</td><td>Scheduling of multiple application tasks</td></tr>
<tr><td>@ref local_provisioning.h "local_provisioning"</td><td>Local provisioning feature built on top of more generic provisioning library</td></tr>
<tr><td>@ref poslib.h "positioning"</td><td>Positioning related logic for anchor and tags</td></tr>
<tr><td>@ref libraries/provisioning/provisioning.h "provisioning"</td><td>Provisioning</td></tr>
<tr><td>@ref app_scheduler.h "scheduler"</td><td>Scheduling of multiple application tasks</td></tr>
<tr><td>@ref shared_data.h "shared_data"</td><td>Handling of data packets between different modules</td></tr>
<tr><td>@ref shared_appconfig.h "shared_appconfig"</td><td>Register for app config following Wirepas TLV format</td></tr>
<tr><td>@ref shared_offline.h "shared_offline"</td><td>Manages the NRLS setting between different modules</td></tr>
<tr><td>@ref stack_state.h "stack_state"</td><td>Allows module to start/stop stack and be notified when such event happens</td></tr>
</table>

@section board_api Board definitions

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

<tr><td>EFR32</td><td>@ref board/efr32_template/bootloader/early_init_efr32.c
"board/efr32_template/bootloader/early_init_efr32.c"</td>
<td>Board DCDC configuration</td></tr>

<tr><td>nRF52</td><td>@ref board/nrf52_template/board.h
"board/nrf52_template/board.h"</td><td>Board definition</td></tr>

<tr><td>nRF52</td><td>@ref board/nrf52_template/config.mk
"board/nrf52_template/config.mk"</td><td>Board Configuration</td></tr>
</table>

@subsection DCDC_converter DCDC converter configuration

DCDC converter configuration is very important topic, especially if low energy
consumption is desired. In following table, the configuration is summarized for
all processor architectures.

<table>
<tr><th>Processor architecture</th><th>DCDC enabled</th><th>DCDC disabled</th></tr>

<tr><td>EFR32</td><td>Leave following code line commented in <code>
early_init_efr32.c</code>:
@code
//#define MCU_NO_DCDC
@endcode
</td><td>Uncomment the line in <code>early_init_efr32.c</code>
@code
#define MCU_NO_DCDC
@endcode
</td></tr>

<tr><td>nRF52</td><td>Uncomment line in <code>board.h</code>:
@code
// The board supports DCDC
#define BOARD_SUPPORT_DCDC
@endcode
</td><td>Comment line in <code>board.h</code>:
@code
// The board supports DCDC
//#define BOARD_SUPPORT_DCDC
@endcode
</td></tr>

</table>

@section bootloader_api Bootloader configuration

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
</table>

@section mcu_api Low level hardware services (HAL)

This folder contain hardware-specific services.

This page contains following sections:
- @subpage mcu_common
- @subpage mcu_hal_api
- @subpage mcu_specific_files
- @subpage linker_file

@subsection mcu_common mcu/common Common MCU files

This folder contains common files for all hardware platforms.

This page contains following sections:
- @subpage mcu_common_start_c

@subsection mcu_common_start_c start.c

@ref start.c "This file" is present in all applications. It positions the application
entry point at the correct place in memory and do basic initialization:
it loads the initialized data from flash to RAM, sets the bss area to 0
in RAM, and calls the application initialization function defined in
<code>app.c</code>.

It also manages compatibility with the stack to avoid issues when
running an application built for an old stack to a newer stack. Running
an application built with a SDK newer than the stack version is not
allowed.

@subsection mcu_hal_api mcu/hal_api Low level hardware API

Second group of services is low level (HAL) hardware services. They contain the
implementations of various hardware peripherals for various hardware platforms
and boards.

Relevant services are located in @ref mcu/hal_api "mcu/hal_api" folder. Services
are following:

@note Not all of the services are available in every platform!

<table>
<tr><th>Name (related to mcu/hal_api folder)</th><th>Description</th></tr>
<tr><td>@ref ds.h "ds.h"</td><td>Deep sleep control module</td></tr>
<tr><td>@ref radio.h "radio.h"</td><td>Radio FEM (front-end module)
</td></tr>
<tr><td>@ref hal_api.h "hal_api.h"</td><td>Initialization of HAL services</td>
</tr>
<tr><td>@ref hw_delay.h "hw_delay.h"</td><td>Hardware delay module</td></tr>
<tr><td>@ref i2c.h "i2c.h"</td><td>Simple minimal I2C master driver</td></tr>
<tr><td>@ref gpio.h "gpio.h"</td><td>GPIO functions</td></tr>
<tr><td>@ref button.h "button.h"</td><td>Button functions</td></tr>
<tr><td>@ref led.h "led.h"</td><td>LED functions</td></tr>
<tr><td>@ref power.h "power.h"</td><td>Enabling of DCDC converter</td></tr>
<tr><td>@ref spi.h "spi.h"</td><td>Simple minimal SPI master driver</td></tr>
<tr><td>@ref usart.h "usart.h"</td><td>USART block handling</td>
</tr>
</table>

@subsection mcu_specific_files mcu/<hardware>

@ref mcu "These folders" contain mcu specific files to ease and factorize
application development. Header files (<code>.h</code>) from this folder can be
included in applications directly.

@subsection linker_file Linker file

Linker files are located in <code>mcu/\<processor\>/linker</code> folder. Linker
file is linker script. It ensures that the application is loadable in its
dedicated area in Flash. Particularly, it sets the application entry point at
the beginning of the area.

@note Linker files named <code>gcc_bl_*.ld</code> are for bootloader and they
should never be modified!

This page contains following sections:
- @subpage flash_memory
- @subpage ram_memory

@subsection flash_memory Flash Memory

As described with more details in OTAP documentation, the
application shares by default its flash memory area with the scratchpad area.
There is no strict limit for the size of the application but if the
application is too big, it will prevent the OTAP to store its scratchpad
in the remaining free space.

The default maximum size for an application is 40kB to ensure the above statement
but can be extended from linker script:

Example:
@code
MEMORY
{
FLASH (rx)      : ORIGIN = 0x00040000, LENGTH = 40K
...
}
@endcode

@subsection ram_memory RAM memory

RAM memory is configured in linker file. Value is fixed and should not be
modified.

@section util_api Utility and helper services

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

#endif /* API_DOC_APPLICATION_API_H_ */
