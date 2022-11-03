#ifndef _HOW_TO_DEVELOP_H_
#define _HOW_TO_DEVELOP_H_

/**
@page how_to_develop How to Develop Application with SDK

The Single-MCU feature allows an application to be executed on the same
chip as the Wirepas Connectivity stack. Wirepas provides an SDK
containing multiple applications examples. Each application describes a
different aspect and can be used as a starting point.

This document will focus on a practical
approach of writing an application. This document will highlight some
crucial point like the minimal steps to follow in the initialization of
an application and will give some practical guidance to correctly write
an application.

This page contains following sections:
- @subpage licensing
- @subpage development_of_a_new_application
- @subpage build_application
- @subpage test_application
- @subpage flashing_device
- @subpage using_otap
- @subpage define_custom_board
- @subpage app_init
- @subpage adding_new_source_files_to_the_application
- @subpage recommendations

@section licensing Pre-requirements

In order to develop the software with SDK, you need to have a license for
Wirepas Mesh. Then, you have access to the firmware image according to your
architecture. If you don't have a license,
<a href="https://wirepas.com/contact/">contact Wirepas sales</a>.

This page contains following sections:
- @subpage debugging
- @subpage firmware_linking

@subsection debugging Debugging

In order to debug the devices with JTAG debugger, you need to use <I>
unprotected bootloader</I>. Whereas normal licensed Wirepas Mesh has JTAG
debugging capabilities prevented, this one has JTAG debugging active. It is
intended to be used during development phase to speed up application software
development. If you don't yet have an access to unlocked bootloader,
<a href="https://wirepas.com/contact/">contact Wirepas sales</a>.

@note It is forbidden to install devices with unprotected bootloader in public
areas!

@subsection firmware_linking Installing firmware image

If you have licensed Wirepas Mesh, you have access to the firmware image. In
order to link that with the SDK, you have to unzip the firmware delivery to
<code>@ref image_folder "image/"</code> folder.

@section development_of_a_new_application Create new application

In this chapter we will create a new application named <code>new_app</code>.

It describes the initial steps to start writing a new application.

This page contains following sections:
- @subpage copy_of_an_application_example
- @subpage change_default_network_address_and_channel
- @subpage change_of_app_area_id
- @subpage configuration_of_a_node

@subsection copy_of_an_application_example Copy of an application example

The provided SDK contains several @ref source_folder "example applications".
They can be used as a starting point (Here, @ref custom_app/app.c "custom_app"
is used as an reference).

To quickly start the development of a new application you can copy an
already existing application and use it as a template. Any application
from <code>source/</code> folder can be used as a starting point.

To start developing the new application, copy folder <code>custom_app</code> to
a new folder named <code>new_app</code> in <code>source/</code> directory.

@subsection change_default_network_address_and_channel Change default network
address and channel

To form a network, all nodes must share the same network address and
network channel.

These information can come from multiple sources like NFC, provisioned in
persistent memory during production,...
In order to ease the setup, build system allows to set those settings at build
time of an application from the application config.mk file.

In <code>@ref custom_app/app.c "custom_app"</code> application example, this
information can be seen in file <code>@ref source_config_mk "config.mk"
</code>
In the new application folder created, you can modified these variables
<code>default_network_address</code> and <code>default_network_channel</code> to
arbitrary values that feat your needs. For example:

@code
    # Define default network settings
    default_network_address ?= 0x67EB4A
    default_network_channel ?= 12
@endcode

These variables are then automatically assigned to constants
<code> NETWORK_ADDRESS</code> and <code>NETWORK_CHANNEL</code> and accessible
from the code.

Additionally, you can define network keys the same way.
Those keys are 16 bytes long and must be kept secret. It is very important to
set them to protect your network.

@code
default_network_cipher_key ?= 0x??,..,0x?? // Must be 16 bytes long
default_network_authen_key ?= 0x??,..,0x?? // Must be 16 bytes long
@endcode

Those settings are used with @ref configureNodeFromBuildParameters utility
function.

@subsection change_of_app_area_id Change of app_area_id

It is mandatory to have unique @ref app_specific_area_id "app_area_id" in order
to update specific application independently on other applications. Thus,
modify file <code>@ref source_config_mk "config.mk" </code> following by using
new arbitrary area id specific for this application:

@code
    # Define a specific application area_id
    app_specific_area_id=0x8054AA
@endcode

@subsection configuration_of_a_node Configuration of a node

The \ref app_init "App_Init()" function is the entry point for the application.
It is called by the stack after each boot.

The Wirepas Connectivity stack is in the stopped state during this call.
All the API calls that require the stack to be in the stopped state,
like configuring node settings, must be done in this function.

The code below shows the minimal steps for an application to configure
a node and start the stack.

@code

    void App_init(const app_global_functions_t * functions)
    {
        // Basic configuration of the node with a unique node address
        if (configureNode(getUniqueAddress(),
                          NETWORK_ADDRESS,
                          NETWORK_CHANNEL) != APP_RES_OK)
        {
            // Could not configure the node. It should not happen
            // except if one of the config value is invalid
            return;
        }

        ...

        lib_state->startStack();
    }
@endcode

A newly flashed device starts with its role set to \ref
APP_LIB_SETTINGS_ROLE_AUTOROLE_LE by default.

To be able to join a network, the application must set at least a unique
node address, a common network address and a common network channel.
Consequently, it is important to check if a node setting (role, node
address, etc.) is already set before updating it from application
initialization code. Otherwise it would break the Remote API, as the
remotely updated value would be overwritten in \ref app_init "App_Init()".

This is the role of the \ref configureNode "configureNode()" function. It sets
the node address, network address and network channel, but **ONLY** if these
settings are missing from the node. This is the case on first boot after
flashing but not after any reboots after that, unless the settings are
explicitly cleared by the application.

Note that the @ref app_lib_state_stop_stack_f "lib_state->stopStack()" function
will cause a reboot of the device and the \ref app_init "App_Init()" function
will be called again.

Once the node is correctly configured, the stack must be started.

This initialization is just an example and can be something different,
depending on the use case. For example, the application can wait for
configuration via another interface (@ref source_makefile_hal_uart "UART", @ref
source_makefile_hal_spi "SPI", NFC, ...). This is the case with the
<code>@ref dualmcu_app/app.c "dualmcu_app"</code> application, for example.

@note Application **must** call @ref app_lib_state_start_stack_f
"lib_state->startStack()". It can be at the end of @ref app_init "App_Init()"
function or in a @ref app_lib_system_set_periodic_cb_f "deferred context" but it
must be called! Without this call, the node will not be part of any network.

@note Remote API built-in feature of the stack (described in \ref relmat7 "[7]")
allows the change of a setting remotely. This change can happen any time from
the application point of view and will generate a reboot and a new call to @ref
app_init "App_Init()" after the update.

@note Some node settings must be the same across all the nodes in the network.
More general information about node configuration can be found in @ref relmat4
"[4]".

@section build_application Build application

Building is done in root of the SDK file structure. It is done by calling the
<code> @ref makefile "makefile"</code>. There can be many options for building but
necessary ones are in <code>@ref source_makefile "application makefile"</code>.
For example by using <code>pca10040</code> as a @ref board_folder "target board",
following build command can be issued:

@code
make app_name=new_app target_board=pca10040
@endcode

For detailed information on build process, see @ref build "here".

@note Many times the application must be built on custom hardware, not yet
having board definition. See documentation @ref define_custom_board "here".

@section test_application Test application

To practically test the application, a minimum of two boards is needed. One of
them must be configured as a @ref APP_LIB_SETTINGS_ROLE_SINK_LE "sink Low Energy" or
a @ref APP_LIB_SETTINGS_ROLE_SINK_LL "sink Low Latency" and the
other as a node (i.e. something else than sink). In a first step, the sink can
be connected to a PC running the Wirepas Terminal.

The application runs on the board configured as a node. Even if it is
technically possible to run the application on a sink, it implies that
the board has another network connection (WiFi, Ethernet,...) and
everything is managed by the same MCU. In this basic configuration, it
is assumed that it is not the case.

To program the sink, application <code>@ref dualmcu_app/app.c "dualmcu_app"
</code> can be flashed and then configured by using Wirepas Terminal.

@section flashing_device Flashing device

Now when application has been compiled, next step is to program that to the
device. The generated binary is located in following path (based on the <code>
@ref makefile "makefile"</code> options used):
@code
<code>build/[target_name]/[app_name]/final_image_[app_name].hex
@endcode
For example:
@code
build/pca10040/new_app/final_image_new_app.hex
@endcode

To flash the image to the device, see @ref flashing_guideline "here".

@section using_otap Using OTAP

For more information about OTAP, see \ref relmat1 "[1]".

OTAP images can be found in following paths (based on the <code>
@ref makefile "makefile"</code> options used):

To update application image only:
@code
<code>build/[target_name]/[app_name]/[app_name].otap
@endcode
To update both application and stack images:
@code
<code>build/[target_name]/[app_name]/[app_name]_wpc_stack.otap
@endcode
For example:
@code
build/pca10040/new_app/new_app.otap
build/pca10040/new_app/new_app_wpc_stack.otap
@endcode

@section define_custom_board Define custom board definition

Many times application requires custom board definition when application is
intended to be executed on board not defined in existing board definitions.
Albeit it is totally possible to discard the board definition altogether and
hard-code all board-specific definitions (such as GPIO pin numbers) directly
in application source code, it is still recommended to define board properly.
This allows many benefits, such as:
- Easily compile other applications to board
- To port the application to multiple boards, such as new variants of the
product.

To implement new board, check out the documentation of @ref board_folder
"board definitions" and modify existing board template (according to processor
architecture).

@section app_init Application startup

The Wirepas Mesh Single-MCU SDK low-level initialization code sets up the
application environment to run C code. The low-level setup is outside the scope
of this document, but once the setup is done, the application initialization
function <code>App_init()</code> will be run::

@code
void App_init(const app_global_functions_t * functions)
{
    ...
}
@endcode

@note This <I>entry point function</I> <B>must</B> be implemented in every
application!

<code>functions</code> parameter is a global list of function pointers for the
application. Normally this is not needed at all and is mainly needed for
backwards compatibility of the applications.

The stack is not yet running when <code>App_init()</code> is called.
Depending on the stored settings and stack state, the stack may start right
after returning from <code>App_init()</code>.

@section adding_new_source_files_to_the_application Adding new source files

For better code readability and organization, the application can be
split in to multiple source files. Adding a new source file is as simple
as declaring it in application specific @ref source_makefile_srcs "source files"
in <code>@ref source_makefile "makefile"</code>, for example:

@code
    # You can add more sources here if needed
    SRCS += new_source.c
    INCLUDES +=
@endcode

The file named <code>new_source.c</code> is created alongside <code>app.c</code>
in this example.

By default, the <code>\<app_folder\>/include</code> folder is added to the list of
paths to check for header files. Any additional folders can be added to
the <code>@ref source_makefile_includes "INCLUDES"</code> variable in the
application <code>@ref source_makefile "makefile"</code>.

@section recommendations Recommendations

This chapter contains various recommendations and best practices to use
with application development.

This page contains following sections:
- @subpage optimization_of_network_throughput
- @subpage free_resources
- @subpage power_consumption
- @subpage persistent_memory

@subsection optimization_of_network_throughput Optimization of throughput

The throughput of a Wirepas Connectivity network is expressed in packet
per seconds. To optimize this throughput, it is important to fill the
packet to the maximum available PDU size when possible.

It is even more important when operating in time-slotted mode. The network
will handle the same number of packets independently of its payload
size.

@subsection free_resources Free resources

All hardware resources that are not used by the Wirepas Connectivity
stack can be used freely by the application.

@note All the hardware that is **not** used by the stack is left in its initial
boot state. It must be configured by the application as needed.

For example, unused GPIOs must be properly configured by the
application, to avoid unnecessary power consumption due to pull-up or
pull-down resistors.

@subsection power_consumption Power consumption

The Wirepas Connectivity stack will try to enter the deepest possible
sleep state of the platform, to optimize power consumption.

But as the application may require staying in a higher power state (to
keep a peripheral clock enabled for example), the application can ask
the stack to prevent entering the deep sleep state.

Please see the \ref app_lib_system_disable_deep_sleep
"lib_system->disableDeepSleep()" function in the \ref system.h "System library".

@subsection persistent_memory How to store data in persistent memory

It is often necessary to store data from an application to persistent memory
in order to still have access to it across reboots of the node.
The different methods to achieve it and their pros and cons are described in this
section.

This page contains following sections:
- @subpage storage_library
- @subpage platform_specific_storage
- @subpage dedicated_area

@subsection storage_library Using storage library

Wirepas Mesh Stack uses reserved areas for its own usage in internal flash.
To avoid reserving too much flash, this area is kept as small as possible.
However, a small amount of this area is reserved for the application.

This application area can be accessed through the @ref app_lib_storage_t "storage library"
handle.

  - Pros
     -# Really easy to use as no flash driver needed
     -# Stack is using wear leveling for this area (but area shouldn't be write
     too often < 1/30 minutes)
  - Cons
     -# Very limited in size. Maximum size can be asked with @ref
     app_lib_storage_get_persistent_max_size_f "lib_stoarge->getPersistentMaxSize"()
     -# It cannot be pre-flashed before first execution
     -# Application must manage validity of the content (with magic number for example)

@subsection platform_specific_storage Using platform specific storage

Most of the platforms supported by Wirepas have their own specific persistent area to
store persistent data.
   - On Nrf52 it is called UICR area (limited to 128 bytes)
   - On EFR32 it is called User Data (limited to 2 kB)
As the Wirepas Stack doesn't use it, it can freely be used by the application to store
its own persistent data.

To use it, users must refer to platform specific reference manual.
For NRF52 users, Wirepas developed a wrapper to use it. You can find it @ref persistent.h here.

  - Pros
     -# Relatively easy to use depending on the platform
     -# Can be pre-flashed on the production line and later accessed by application
      (easy solution for initial provisioning)
  - Cons
     -# Limited in size (especially for NRF52)
     -# Application must manage validity of the content (with magic number for example).
     @note It is done with the Wirepas wrapper for NRF52.


@subsection dedicated_area Using a dedicated area in flash

The internal flash is partitioned in multiple areas. Some of these areas are used for Wirepas usage
like the bootloader or the firmware that cannot be moved.
But the remaining part of flash can be freely partitioned by the application. Each area has a dedicated
area ID and a given size.
More information about area ids and its usage can be found in @ref relmat2 "[2]"

The steps to realize are:
   -# Defining a new partitioning in the @ref config_mk_ini_file "Ini file" by adding a new area
   with flag user (i.e. flags = 0x00000014)
   -# Access it through the @ref app_lib_memory_area_t "memory area library" handle from your application.
   -# Application can also directly access the memory area without using the library, but it requires the
   application to know the absolute address of the area (not needed with the memory library)

An example can be found from the @ref provisioning_joining_node/app.c "provisioning joining node" with the code to access the memory
area in @ref storage_memarea.c "storage_memarea.c" and example of modified ini file in the scratchpad_ini folder

  - Pros
     -# Reserved persistent area for application can be quite large as long as enough room is reserved
      for receiving new scratchpads
     -# Content of persistent data is kept unchanged when doing an OTAP of the application
     -# Content of the persistent data area can be OTAP independently of the application. It is particularly
     handy if application needs a big configuration file to operate that can evolve during product life. Like
     a local schedule for a luminar to operate in an autonomous way.
     -# Can be pre-flashed on the production line and later accessed by application
      (easy solution for initial provisioning)
  - Cons
     -# Flash partitioning must be modified and must be done carefully. Adding it just after the app
     may reduce the possibility of the application to expand later. Or defining a too big area may reduce
     the maximum scratchpad size the node can receive.
     -# Application must manage validity of the content (with magic number for example).


Related Material
================

@anchor relmat1 [1] https://developer.wirepas.com/support/solutions/articles/77000496639

@anchor relmat2 [2] https://developer.wirepas.com/support/solutions/articles/77000496582

@anchor relmat4 [4] https://github.com/wirepas/wm-sdk/blob/master/source/reference_apps/dualmcu_app/api/DualMcuAPI.md
Manual

@anchor relmat7 [7] https://developer.wirepas.com/support/solutions/articles/77000407101


 */

#endif /* API_DOC_HOW_TO_DEVELOP_H_ */
