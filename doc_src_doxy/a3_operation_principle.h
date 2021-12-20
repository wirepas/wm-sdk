
#ifndef _OPERATION_PRINCIPLE_H_
#define _OPERATION_PRINCIPLE_H_

/**
@page application_operation Single-MCU API Operation Principle

The Wirepas Mesh stack and the application can communicate via a
Single-MCU API. The application is designed to have a @ref app_init
"single-entry point". This entry point is called by the Wirepas Mesh stack at
boot. The application must do its initialization during the application entry
point call.

Also, during the application @ref app_init "entry point call", the Wirepas Mesh
stack provides a list of services to the application. These services are
implemented as a list of C callback functions. Most importantly, these services
includes opening libraries (used by \ref API_Open "API_Open()" function).

This page contains following sections:
- @subpage build
- @subpage memory_partitioning
- @subpage application_detection
- @subpage cooperative_mcu_access

@section build Build process

The build can be divided in three steps:

1. Build the application and generate a binary and an OTAP image for the
   application

2. The binary is combined with the Wirepas Mesh stack binary provided by
   Wirepas to generate an OTAP image is generated to update both stack
   and app at the same time.

3. The OTAP image is combined with the Wirepas Mesh bootloader binary
   provided by Wirepas to generate an image that can be flashed directly
   to a blank device.

@image html image8.png

@section memory_partitioning Memory Partitioning

The application entry point and different callback functions are
visualized in following picture:

@image html image3.png

The flash memory partitioning includes
the following regions:

- **Bootloader**: Used by Wirepas Mesh OTAP functionality to take new firmware
(application or/and Wirepas Mesh stack) into use.

- **Wirepas Mesh stack**: Wirepas Mesh stack firmware.

- **Customer** **application**: Customer application firmware.

- **Scratchpad**: Used by the Wirepas Mesh OTAP functionality to store new
firmware (application or/and Wirepas Mesh stack) before taking it into use.

- **Reserved area**: Reserved for future use

- **Persistent area**: Used for storing persistent variables of the Wirepas Mesh
stack.

Dedicated areas in flash and in RAM are reserved for the application. Size for
these areas are platform dependent and described in @ref sdk_environment "SDK
Environment". The C stack is shared between the application and the Wirepas Mesh
stack. The application entry point must be allocated at the start of the
application dedicated flash area. This is mandatory for the Wirepas Mesh stack
to be able to know the application entry point location at run time.

@image html image4.png

@section application_detection Application Detection

Device requires application to be working properly. Without application, the
stack itself is not started and radio communication is thus disabled. Wirepas
Mesh stack checks the presence of the application at run time. The detection
depends of the value of the first two bytes of the application dedicated flash
area. If it is equal to <code>0xFFFF</code> (default value), the Wirepas Mesh
stack detects that there is no application and execution ends there. Otherwise,
Wirepas Mesh stack calls the application @ref app_init "entry point" at this
address. It is up to the customer to have positioned the function entry point
here with the correct prototype. All the code needed to correctly position the
right function at the right position is provided in the SDK.

@section cooperative_mcu_access Cooperative MCU Access

Wirepas Mesh stack is a real-time system and is based on a cooperative
scheduler. Tasks are scheduled based on their priorities and their
execution times. To operate correctly, all the deadlines must be
respected and each task must complete within the duration reported to
the scheduler.

If a task asks the scheduler to execute for a given period, it must
finish its work in the corresponding allocated window. Being late can
<I>affect the whole system performance and may result in incorrect operation
</I>. The scheduler cannot pre-empt a task. Thus, special care must be taken
to guarantee that a task finishes on time.

Moreover as it is a cooperative scheduler, application must not ask to
be scheduled all the time. It would prevent Wirepas tasks with a lower
priority from getting access to the MCU. It may result with stack not being
able to generate diagnostics or process OTAP commands. Even if their is no
exact figure, Application must ensure a fair access to the MCU ressource.

An application task can get run-time in basically three different ways:

-# via \ref periodic_application "periodic application callback function"
-# via \ref asynchronous_application_callback_functions "asynchronous
   application callback functions"
-# via interrupt. There are two types
   of interrupts: @ref fast_interrupt "fast" and @ref deferred_interrupt
   "deferred interrupts".

This page contains following sections:
- @subpage periodic_application
- @subpage asynchronous_application_callback_functions
- @subpage application_interrupt
- @subpage which_kind_of_interrupt_for_which_purpose
- @subpage execution_time_limits

@subsection periodic_application Periodic Application Callback Function

The application can register one of its callback functions to be called
at a given time and for a given period. At the end of its execution, the
callback function returns the delay to be scheduled again. This mode
allows the application to do periodical jobs, such as reading and
sending a sensor value.

It must be noted, that the periodic application callback function is not
necessarily called exactly at the requested time. The application task
has lower priority than the Wirepas Mesh stack tasks. Thus, the
application execution can be delayed to execute Wirepas Mesh stack
tasks.

The application latency (delay between the requested start time and the
real start time) depends on the execution duration specified during the
callback function registration. The shorter the duration, the easier it
is for the scheduler to find a time slot for the task. It is important
to correctly size this time. It must be big enough to ensure that the
callback function meets its deadline but small enough to reduce the
latency.

Figure below illustrates an example of scheduling the application task.
Application registers its periodic task (callback) at time t<SUB>0</SUB>.
Time t<SUB>1</SUB> is the requested execution time of the application
task. At time t<SUB>2</SUB> there is a first free timeslot to run the
task, but it cannot be run because the requested duration (T<SUB>req</SUB>)
is longer than the free time (T<SUB>free</SUB> = t<SUB>3</SUB>-t<SUB>2</SUB>).
At time t<SUB>4</SUB> there is enough time to run the application and the
task is scheduled. The requested time includes a lot of margin
(T<SUB>exec</SUB> < T<SUB>req</SUB>) and the application task finishes before
its deadline. With a more accurate sizing (T<SUB>exec</SUB> vs.
T<SUB>req</SUB>) the application task could have also been scheduled
earlier at time t<SUB>2</SUB>.

Check out service for @ref app_lib_system_set_periodic_cb_f
"lib_system->setPeriodicCb()" and @ref app_scheduler.h "app scheduler library"
 or more information on how this is done with API services.

\image html image5.png

@subsection asynchronous_application_callback_functions Asynchronous Application Callback Functions

The application can also be executed asynchronously if the Wirepas Mesh
stack has something to communicate to it, e.g. a message received from
the network. This is done via callback functions the application
provides to Wirepas Mesh stack at initialization.

These callback functions are called during a Wirepas Mesh stack task
execution. Thus, the execution delay for these callback functions must
be kept as short as possible. The application must avoid doing long
processing or long operation (accessing a device for example). The
recommended implementation is to save a state to be processed later. A
good practice is to handle the data processing in the next periodically
scheduled work (periodic application callback function). The schedule of
the periodic work can also be updated to an earlier time if needed, but
only one periodic work can be registered at a time. If no work is
scheduled yet, it can be registered during the callback function. For
recommended maximum periods, check @ref execution_time_limits "guidance on
maximum execution time limits".

An example of processing an asynchronous callback function is presented
in Figure below. At t<SUB>1</SUB>, the Wirepas Mesh stack calls one of the
asynchronous application callback functions. The application handles it
in a very short period and schedules a work to do the processing at
t<SUB>3</SUB>.

There are plenty of various
   asynchronous callbacks served by the system. Depending on the callback, there
   may be tighter time limits, especially if the callback is called from the IRQ
   context. Most common callback used and example of such callback is @ref
   app_lib_data_t.setDataReceivedCb "unicast data reception callback".

\image html image6.png

@subsection application_interrupt Application Interrupt

The application can register to hardware interrupts. It must provide its
own interrupt handler table (same format as the platform one) to Wirepas
Mesh stack.

Application interrupts can then be enabled/disabled with a specific API services
<code>@ref app_lib_system_enable_app_irq_with_handler_f
"lib_system->enableAppIrq"</code> service and <code>@ref
app_lib_system_disable_app_irq_f "lib_system->disableAppIrq" </code> service.

There are two kinds of interrupts. Deferred interrupt and fast
interrupt.

This page contains following sections:
- @subpage deferred_interrupt
- @subpage fast_interrupt

@subsection deferred_interrupt Deferred interrupt

The interrupts are handled in two levels: Wirepas Mesh stack
implements the first-level interrupt handler which handles minimal
needed operations (fast) in the interrupt context, and the
application implements the second-level interrupt handler which is
scheduled when there is enough free time.

Like in the periodic work, the application has a maximum duration
time for its interrupt handler. As the latency to handle the
interrupt at application-level is directly linked to the execution
time of the application interrupt handler, the maximum execution
time is set to 100us, see also @ref execution_time_limits "guidance on maximum
execution time limits".

Below, there are 4 different scenarios for interrupts described:

* In scenario 1, an interrupt fires whilst the Wirepas Mesh stack
  executes one of its tasks. The first Wirepas Mesh stack first level
  interrupt handler is call immediately and the interrupt line is
  disabled. The app second-level interrupt handler is scheduled to be
  executed as soon as possible. In this scenario, the Wirepas Mesh
  stack has enough free time before its next task, it can schedule the
  application interrupt handler. The interrupt line is enabled again.

* In scenario 2, the application interrupt handler cannot be scheduled
  between the two Wirepas Mesh stack task because there is not enough
  time.

* In scenario 3, the interrupt fires during an idle state. The
  interrupt wakes up the platform and is handled by the Wirepas Mesh
  stack and the application without any additional scheduling latency.

* In scenario 4, the interrupt fires during the execution of an
  application periodic work. The first level interrupt handling
  happens during the application periodic task but the application
  handler is only executed at the end of the task. The application
  cannot be pre-empted by its own application interrupt handlers. This
  simplifies the design of the application regarding data protection.

@image html image7.png

@subsection fast_interrupt Fast interrupt

The interrupts are handled directly by the application. So it can
preempt the stack execution. For this reason, the handling must be
short (few micro seconds) to avoid breaking the stack internal
scheduling. See also @ref execution_time_limits
"guidance on maximum execution time limits".

Fast interrupts can also preempt application task, so concurrent
data access can happen and protection mechanism must be implemented.
Services in the API are available for that purpose:
<code>@ref app_lib_system_enter_critical_section_f
"lib_system->enterCriticalSection()"</code> and <code> @ref
app_lib_system_exit_critical_section_f
"lib_system->exitCriticalSection()"</code>.

@note Due to concurrency issues and execution time limits, most services should
@b not be used from fast interrupt execution context. Recommendation is to call
@ref periodic_application periodic application callback (using @ref
app_lib_system_set_periodic_cb_f "lib_system->setPeriodicCb()" service or @ref
app_scheduler.h "app scheduler" library) and call services from there. Services
that are known safe to be used from fast interrupt context are explicitly
described in service documentation.

@subsection which_kind_of_interrupt_for_which_purpose Which kind of interrupt for which purpose

Fast interrupt must be used only when latency to serve the interrupt
is crucial. For example, in case of a UART driver with a small FIFO
hardware depth, fast interrupt must be used to avoid losing bytes.
In this situation, the UART IRQ handler will just flush the byte
from the hardware FIFO and store it in a software one to keep the
handler execution short. No data processing must be done in the
handler. Handling must be deferred to a periodic work.

But in many other situations, deferred interrupts can be used
instead as it is easier to implement. In fact, as the handler is
executed in a deferred task, execution time is less critical (it is
currently limited to 100 micro seconds). As handler is executed as a
task, it cannot preempt the app periodic task so no data protection
mechanism is needed.

@subsection execution_time_limits Execution time limits

In the following table, the various types of methods on how application requests
run-time and their maximum execution time are summarized:

<table>
<tr><th>Execution type</th><th>Maximum execution time</th></tr>
<tr><td>Periodic application callback function</td>
    <td>Application may request execution time and is safe to spend execution up
    to that time (max time is set to 100 ms)</td></tr>
<tr><td>Asynchronous callback functions</td>
    <td>Generally 1 ms, (callback from thread context). Some callbacks
    (mentioned explicitly) are called from IRQ context and have maximum
    execution time of 50 us.</td></tr>
<tr><td>Deferred interrupt</td><td>1 ms</td></tr>
<tr><td>Fast interrupt</td><td>50 us</td></tr>
</table>


 */



#endif /* API_DOC_OPERATION_PRINCIPLE_H_ */
