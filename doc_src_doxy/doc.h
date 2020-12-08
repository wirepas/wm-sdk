
#ifndef SOURCE_APP_DOC_H_
#define SOURCE_APP_DOC_H_

/**
@mainpage Single-MCU Operation Overview

The Single-MCU operation allows an application to run on the same
chip with Wirepas Mesh stack.

@image html main_components.png

Main components are following:
- @subpage application
- @subpage sdk_hal
- @subpage single_mcu_api
- @subpage sdk_libraries
- @subpage wirepas_firmware
- @subpage bootloader
- @subpage bootloader_extension
- @subpage hardware

@page application Application

Application firmware includes the application logic. There can be multiple
applications (i.e. different <I>kind of devices</I>) in the same network. For
example lighting network may contain lighting control switches and LED drivers.

With the provided SDK, a customer can write its own application, build it,
and update a Wirepas Mesh network.

@ref application_operation "Single-MCU API Operation Principle" describes
  the operation principle of the interface between the application and
  Wirepas Mesh stack. Memory partitioning and different regions are
  explained. The different ways the application is scheduled is also described.

@ref sdk_environment "SDK Environment" describes the SDK package contents
  and available free processor resources for application.

@ref how_to_develop "How to develop applications with Single-MCU SDK"
  describes guidance to write first application and various tips and
  recommendations for application development.


@page sdk_hal Application-specific Hardware Abstraction Layer (HAL)

This is commonly various software components for peripheral usage, such as
sensors/actuators and communication interfaces. Some of those are delivered as
part of the SDK. This also contains drivers made by user of the SDKs, commonly
shared between different _applications.

For HAL services offered by the SDK, see @ref api_folder "API services by SDK".


@page single_mcu_api Wirepas Mesh Single MCU API

Wirepas Mesh stack provides Wirepas Mesh Single-MCU API for application to
use <I>stack services</I> and run tasks on the MCU.

@ref programming_interface "Application API" describes the programming
  interface.

@page sdk_libraries Wirepas Mesh Single MCU libraries wrapper

These libraries are wrappers on top of the @ref single_mcu_api "single MCU api".
Some services offered by the stack are quite low level and these wrapper
libraries offer a higher abstraction level.
Main example is the app_scheduler abstracting the single periodic work offered
by stack.


@page wirepas_firmware Wirepas Mesh Stack
Stack includes the Wirepas Mesh communication
stack and Wirepas scheduler for enabling the application operation in the same
MCU. Wirepas Mesh HAL includes all hardware abstractions and drivers <I>needed by
the stack</I>, such as a radio driver. Note that drivers for peripherals that
are not needed by the stack itself, are not implemented in stack.

Wirepas Mesh scheduler provides priority-based cooperative scheduling,
i.e. all the tasks are run to completion. The tasks are scheduled based
on their priorities and their execution times. The Wirepas Mesh stack
has strict real-time requirements (accurate synchronization of
messaging) and has the highest priority. Thus, it is not recommended to
do processing intensive (time consuming) tasks on the application side.
Real-time guarantees are not provided to the application.

@page bootloader Wirepas Mesh Bootloader

Wirepas provides a bootloader binary. Its main purpose is to do the basic
basic initialization of the hardware but also handle the processing of stored
scratchpad in the flash received by the stack during an otap.
Bootloader is in charge of the flash management and contains the flash
partitioning that is configured through an @ref config_mk_ini_file ".ini file".

@page bootloader_extension Custom bootloader extension

Bootloader can be extended from the SDK. Main usage is to support an external
flash and advertise harware capabilities of the board.
How to use it is described in a separated application note that will be
available soon from here.

@page hardware The physical Hardware

This includes all the hardware of the device including the processor core,
radio for wireless communication and application-specific peripherals.

Peripherals can be grouped into three categories:
-# Peripherals used solely by the Wirepas Mesh Stack
-# Peripherals used solely by the application and
-# Peripherals shared between Wirepas Mesh Stack and application.

For details on this, check @ref efr32xg12 "EFR32 resources" or @ref nordic_nrf52832_nrf52840
"Nordic nRF52XXX resources" according to your architecture.

**/

#endif /* SOURCE_APP_DOC_H_ */
