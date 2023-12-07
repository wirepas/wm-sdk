/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file wms_system.h
 *
 * The System library collects together low-level functions such as interrupt
 * handling, critical sections and power management.
 *
 * The System library also provides a simple periodic callback facility that
 * applications can use in place of a typical main loop, to perform tasks
 * alongside the protocol stack. The periodic callback facility also doubles as
 * a "bottom-half" callback, also known as a slow/soft interrupt handler or
 * deferred procedure call, which can be triggered from interrupts. This allows
 * writing interrupt routines that react to external events quickly and then do
 * the rest of the processing in an application callback whenever it is
 * convenient to do so.
 *
 * Library services are accessed via @ref app_lib_system_t "lib_system"
 * handle.
 */
#ifndef APP_LIB_SYSTEM_H_
#define APP_LIB_SYSTEM_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "wms_app.h"

/** @brief Library symbolic name */
#define APP_LIB_SYSTEM_NAME             0x77db1bd5 //!< "SYSTEM"

/** @brief Maximum supported library version */
#define APP_LIB_SYSTEM_VERSION          0x201

/**
 * @brief Constant to stop periodic callback
 *
 * When this constant is returned from a periodic callback (@ref
 * app_lib_system_periodic_cb_f) , the stack will not call the callback again.
 */
#define APP_LIB_SYSTEM_STOP_PERIODIC    UINT32_MAX

/**
 * @brief   Interrupt priority levels for application
 * @note    These do not correspond to NVIC priority levels, however, a lower
 *          number (higher priority) is guaranteed to have a higher NVIC
 *          priority as well
 */
#define APP_LIB_SYSTEM_IRQ_PRIO_HI      0
#define APP_LIB_SYSTEM_IRQ_PRIO_LO      1

/**
 * @brief Radio hardware magic number
 *
 * This number is returned by @ref app_lib_system_get_radio_info_f
 * "lib_system->getRadioInfo()" function and can be used by the application
 * to detect which radio platform it is running on.
 */
typedef enum
{
    /** Unknown radio (should never happen) */
    APP_LIB_SYSTEM_HARDWARE_MAGIC_UNKNOWN       = 0,
    /** Obsolete reserved value */
    APP_LIB_SYSTEM_HARDWARE_RESERVED_1          = 1,
    /** Obsolete reserved value */
    APP_LIB_SYSTEM_HARDWARE_RESERVED_2          = 2,
    /** Nordic Semiconductor nRF52832 */
    APP_LIB_SYSTEM_HARDWARE_MAGIC_NRF52832      = 3,
    /** Obsolete reserved value */
    APP_LIB_SYSTEM_HARDWARE_RESERVED_4          = 4,
    /** Silicon Labs EFR32XG12 1024 kB Flash, 128 kB RAM */
    APP_LIB_SYSTEM_HARDWARE_MAGIC_EFR32XG12     = 5,
    /** Nordic Semiconductor nRF52840 */
    APP_LIB_SYSTEM_HARDWARE_MAGIC_NRF52840      = 6,
    /** Silicon Labs EFR32XG12 512 kB Flash, 64 kB RAM */
    APP_LIB_SYSTEM_HARDWARE_MAGIC_EFR32XG12_512 = 7,
    /** Silicon Labs EFR32xG13 512 kB Flash, 64 kB RAM */
    APP_LIB_SYSTEM_HARDWARE_MAGIC_EFR32XG13     = 8,
    /** Nordic Semiconductor nRF52833 */
    APP_LIB_SYSTEM_HARDWARE_MAGIC_NRF52833      = 9,
    /** Silicon Labs EFR32xG21 */
    APP_LIB_SYSTEM_HARDWARE_MAGIC_EFR32XG21     = 10,
    /** Silicon Labs EFR32xG22, 512 kB Flash, 32 kB RAM */
    APP_LIB_SYSTEM_HARDWARE_MAGIC_EFR32XG22     = 11,
    /** Reserved */
    APP_LIB_SYSTEM_HARDWARE_RESERVED_5          = 12,
    /** Silicon Labs BGM220PC22HNA */
    APP_LIB_SYSTEM_HARDWARE_MAGIC_BGM220PC22HNA = 13,
    /** Silicon Labs BGM220SC22HNA */
    APP_LIB_SYSTEM_HARDWARE_MAGIC_BGM220SC22HNA = 14,
    /** Nordic Semiconductor nRF9160 (legacy) */
    APP_LIB_SYSTEM_HARDWARE_MAGIC_NRF9160       = 15,
    /** Silicon Labs EFR32xG23 */
    APP_LIB_SYSTEM_HARDWARE_MAGIC_EFR32XG23     = 16,
    /** Silicon Labs EFR32xG24 */
    APP_LIB_SYSTEM_HARDWARE_MAGIC_EFR32XG24     = 17,
    /** Nordic Semiconductor nRF9161 or nRF9131, a.k.a. nRF9120 */
    APP_LIB_SYSTEM_HARDWARE_MAGIC_NRF9120       = 18,
} app_lib_system_hardware_magic_e;

/**
 * @brief Protocol profiles
 *
 * Protocol profile, which is a value that depends on the radio band, regulatory
 * domain and platform capabilities such as available modulation types, channel
 * spacing, output power levels, etc. This number is returned by
 * @ref app_lib_system_get_radio_info_f "lib_system->getRadioInfo()".
 */
typedef enum
{
    /** Unknown protocol profile (should never happen) */
    APP_LIB_SYSTEM_PROTOCOL_PROFILE_UNKNOWN             = 0,
    /** 2.4 GHz with BLE Phy */
    APP_LIB_SYSTEM_PROTOCOL_PROFILE_ISM_24GHZ           = 1,
    /** Obsolete reserved value */
    APP_LIB_SYSTEM_PROTOCOL_PROFILE_RESERVED_2          = 2,
    /** Obsolete reserved value */
    APP_LIB_SYSTEM_PROTOCOL_PROFILE_RESERVED_3          = 3,
    /** Obsolete reserved value */
    APP_LIB_SYSTEM_PROTOCOL_PROFILE_RESERVED_4          = 4,
    /** Obsolete reserved value */
    APP_LIB_SYSTEM_PROTOCOL_PROFILE_RESERVED_5          = 5,
    /** Obsolete reserved value */
    APP_LIB_SYSTEM_PROTOCOL_PROFILE_RESERVED_6          = 6,
    /** Obsolete reserved value */
    APP_LIB_SYSTEM_PROTOCOL_PROFILE_RESERVED_7          = 7,
    /** India 865 MHz with SubG Phy */
    APP_LIB_SYSTEM_PROTOCOL_PROFILE_SUB_INDIA865        = 8,
    /** Obsolete reserved value */
    APP_LIB_SYSTEM_PROTOCOL_PROFILE_RESERVED_9          = 9,
    /** Obsolete reserved value  */
    APP_LIB_SYSTEM_PROTOCOL_PROFILE_RESERVED_10         = 10,
    /** Aus 915 MHz with SubG Phy */
    APP_LIB_SYSTEM_PROTOCOL_PROFILE_SUB_AUS915          = 11,
    /** Smart metering  */
    APP_LIB_SYSTEM_PROTOCOL_PROFILE_DECT_TS_103_874_2   = 12,
    /** Obsolete reserved value */
    APP_LIB_SYSTEM_PROTOCOL_PROFILE_RESERVED_13         = 13,
    /** Obsolete reserved value */
    APP_LIB_SYSTEM_PROTOCOL_PROFILE_RESERVED_14         = 14,
    /** Obsolete reserved value */
    APP_LIB_SYSTEM_PROTOCOL_PROFILE_RESERVED_15         = 15,
} app_lib_system_protocol_profile_e;

/**
 * @brief   Radio information structure
 *
 * Information returned by @ref app_lib_system_get_radio_info_f
 * "lib_system->getRadioInfo()"
 */
typedef struct
{
    /** Radio hardware magic number, @ref app_lib_system_hardware_magic_e */
    uint32_t hardware_magic;
    /** Protocol profile, @ref app_lib_system_protocol_profile_e */
    uint32_t protocol_profile;
} app_lib_system_radio_info_t;

/**
 * @brief Startup callback
 *
 * This callback is called after every interrupt, until
 * the stack is started by calling @ref app_lib_state_start_stack_f
 * "lib_state->startStack()" function in the State library ( @ref wms_state.h). If
 * the stack is running initially, the startup callback is not called at all.
 * Any code, including interrupts can use @ref app_lib_system_set_startup_cb_f
 * "lib_system->setStartupCb()" function to set or change the startup callback.
 *
 * There is no return value from this function. Returning from this callback
 * will cause the node to enter a low power sleep, until another interrupt will
 * cause it to wake up again.
 */
typedef void (*app_lib_system_startup_cb_f)(void);

/**
 * @brief Shutdown callback
 *
 * This callback is called when the @ref
 * app_lib_state_stop_stack_f "lib_state->stopStack"() function in the State
 * library ( @ref wms_state.h) is called, or some other reason (e.g. OTAP, Remote
 * API) is causing the stack to stop. The @ref app_lib_system_set_shutdown_cb_f
 * "lib_system->setShutdownCb"() function can be used to set or change the
 * shutdown callback.
 *
 * There is no return value from this function. Returning from the callback
 * reboots the node.
 */
typedef void (*app_lib_system_shutdown_cb_f)(void);

/**
 * @brief   Periodic callback
 *
 * This function is called after a set delay. If the set
 * delay is zero, this function is called as soon as possible.
 *
 * Any code, including interrupts can use @ref app_lib_system_set_periodic_cb_f
 * "lib_system->setPeriodicCb"() function to set or change the periodic
 * callback. This can be used the defer processing to a "bottom half", also
 * known as a slow/soft interrupt handler, or deferred procedure call.
 *
 * The return value is used as the next delay value, in microseconds. If @ref
 * APP_LIB_SYSTEM_STOP_PERIODIC is returned, the callback is not called again.
 *
 * Because of limited range of return value (32 bits) and the internal
 * comparison made by the scheduler, the maximum delay before being scheduled
 * cannot be higher than the value returned by the @ref
 * app_lib_time_get_max_delay_hp_us_f "lib_time->getMaxHpDelay" service (This
 * value is around 30 minutes).
 *
 * @return   Delay in us, for this function to be called again
 *           or @ref APP_LIB_SYSTEM_STOP_PERIODIC to stop
 */
typedef uint32_t (*app_lib_system_periodic_cb_f)(void);

/**
 * @brief   Interrupt handler callback
 *
 * Interrupt handlers registered with @ref
 * app_lib_system_enable_app_irq_with_handler_f "lib_system->enableAppIrq"().
 *
 * Interrupt handlers have no parameters or return value.
 */
typedef void (*app_lib_system_irq_handler_f)(void);

/**
 * @brief   Set a callback to be called when the system starts up
 * @param   startup_cb
 *          The function to be executed, or NULL to unset
 * @return  Result code, always @ref APP_RES_OK
 * @note    The callback will be called once after returning from
 *          @c app_init "App_init()" and then each time after an enabled
 *          interrupt handler is run, until the stack is started in the callback
 */
typedef app_res_e
    (*app_lib_system_set_startup_cb_f)(app_lib_system_startup_cb_f startup_cb);

/**
 * @brief   Set a callback to be called just before the system shuts down
 * @param   shutdown_cb
 *          The function to be executed, or NULL to unset
 * @return  Result code, always @ref APP_RES_OK
 */
typedef app_res_e
    (*app_lib_system_set_shutdown_cb_f)
    (app_lib_system_shutdown_cb_f shutdown_cb);

/**
 * @brief   Set the periodic callback
 *
 * Set the periodic callback to the given function. If NULL is passed, the
 * periodic callback is disabled. The periodic callback is also disabled if the
 * callback function returns @ref APP_LIB_SYSTEM_STOP_PERIODIC.
 *
 * Parameter @p initial_delay_us is the amount of time to wait, in microseconds,
 * before calling the callback. If set to zero, the callback is called
 * immediately whenever there is enough time. Parameter @p execution_time_us
 * tells the stack what is the longest amount of time the function call is
 * expected to take, in microseconds. The stack will then schedule the callback
 * to the next available slot that has enough time. If the callback overruns
 * its allocated time slot and returns late, the protocol timing is compromised.
 *
 * Returns @ref APP_RES_INVALID_VALUE if execution_time_us is greater than
 * 100000 (100 ms). Otherwise, returns @ref APP_RES_OK.
 *
 * Because of limited range of return value (32 bits) and the internal
 * comparison made by the scheduler, the maximum delay before being scheduled
 * cannot be higher than the value returned by the @ref
 * app_lib_time_get_max_delay_hp_us_f "lib_time->getMaxHpDelay()" service (This
 * value is around 30 minutes).
 *
 * For more information on scheduling, see @c cooperative_mcu_access.
 *
 * @param   work_cb
 *          The function to be executed, or NULL to unset
 * @param   initial_delay_us
 *          Delay from now in us to call the work function
 * @param   execution_time_us
 *          Maximum time for the work function to execute in us
 * @return  Result code, @ref APP_RES_OK if successful, @ref
 *          APP_RES_INVALID_VALUE if @p execution_time_us is too high
 * @note    When the callback returns @ref APP_LIB_SYSTEM_STOP_PERIODIC,
 *          it will not be called again
 * @note    Unlike most services, this service is safe to be used from
 *          @c fast_interrupt "fast interrupt execution context"
 */
typedef app_res_e
    (*app_lib_system_set_periodic_cb_f)(app_lib_system_periodic_cb_f work_cb,
                                        uint32_t initial_delay_us,
                                        uint32_t execution_time_us);

/**
 * @brief Enter a critical section
 *
 * Interrupts are disabled for the duration of the
 * critical section. This function and @ref
 * app_lib_system_exit_critical_section_f "lib_system->exitCriticalSection"()
 * can be nested.
 *
 * Code in critical sections must be kept extremely short and fast. Otherwise
 * communication degradation or even total failure can result. The types of
 * protocol timing bugs that arise from keeping interrupts disabled for too long
 * are very difficult to diagnose.
 */
typedef void (*app_lib_system_enter_critical_section_f)(void);

/**
 * @brief Exit a critical section
 *
 * Exit a critical section which was entered using the @ref
 * app_lib_system_enter_critical_section_f "lib_system->enterCriticalSection()"
 * function. This function and @ref app_lib_system_enter_critical_section_f
 * "lib_system->enterCriticalSection()" can be nested. Interrupts are enabled
 * again, if this was the outermost nesting level.
 */
typedef void (*app_lib_system_exit_critical_section_f)(void);

/**
 * @brief Disable/Enable deep sleep on stack
 *
 * Tell the stack if it is permissible to enter deep sleep or not. This can be
 * used if the application is using a peripheral that cannot operate correctly
 * if the stack turns off power to certain features of the hardware when
 * sleeping.
 *
 * A @p disable parameter value true disables deep sleep, at the expense of
 * greatly increased power consumption. @p disable parameter value false, the
 * default, will restore back normal operation. This function always returns
 * @ref APP_RES_OK.
 *
 * @param   disable
 *          If True, prevent the device from going to deep sleep.
 *          If False, enable the device to go to deep sleep state.
 * @return  Result code, Always @ref APP_RES_OK
 * @note    Disabling the deep sleep mode will increase the power consumption
 *          of the device. It must only be disable if needed (for example to
 *          keep using a peripheral not available in deep sleep mode: timer,
 *          ...)
 */
typedef app_res_e (*app_lib_system_disable_deep_sleep)(bool disable);

/**
 * @brief   Register an interrupt handler table for the app
 * @param   table_p
 *          Pointer to the app interrupt handler table
 * @return  Result code, Always @ref APP_RES_OK
 */
typedef app_res_e
    (*app_lib_system_register_app_irq_table_f)
    (app_lib_system_irq_handler_f * table_p);

/**
 * @brief   Enable an app interrupt
 * @param   irq_n
 *          The IRQ number to enable
 * @return  Result code, @ref APP_RES_OK if successful, @ref
 *          APP_RES_INVALID_VALUE if @p irq_n is invalid, @ref
 *          APP_RES_RESOURCE_UNAVAILABLE if @p irq_n is not available for the
 *          application or @ref APP_RES_INVALID_CONFIGURATION if no interrupt
 *          handler or table set
 */
typedef app_res_e
    (*app_lib_system_enable_app_irq_f)(uint8_t irq_n);

/**
 * @brief   Disable an app interrupt
 *
 * Disable a platform-specific interrupt @p irq_n. Calling this function is
 * permitted even if the interrupt was not enabled previously.
 *
 * @param   irq_n
 *          The IRQ number to disable
 * @return  Result code, @ref APP_RES_OK if successful, @ref
 *          APP_RES_INVALID_VALUE if @p irq_n is invalid, @ref
 *          APP_RES_RESOURCE_UNAVAILABLE if @p irq_n is not available for the
 *          application
 */
typedef app_res_e
    (*app_lib_system_disable_app_irq_f)(uint8_t irq_n);

/**
 * @brief   Enable a fast app interrupt
 * @param   irq_n
 *          The IRQ number to enable
 * @param   priority
 *          Interrupt priority level.
 *          Available levels depend on platform, but two levels are guaranteed:
 *          @ref APP_LIB_SYSTEM_IRQ_PRIO_HI
 *          @ref APP_LIB_SYSTEM_IRQ_PRIO_LO
 * @return  Result code, @ref APP_RES_OK if successful, @ref
 *          APP_RES_INVALID_VALUE if @p irq_n is invalid, @ref
 *          APP_RES_RESOURCE_UNAVAILABLE if @p irq_n is not available for the
 *          application or @ref APP_RES_INVALID_CONFIGURATION if no interrupt
 *          handler or table set
 * @note    Fast interrupts and normal interrupts are
 *          disabled with the same function
 */
typedef app_res_e
    (*app_lib_system_enable_fast_app_irq_f)(uint8_t irq_n, uint8_t priority);

/**
 * @brief   Enable a platform-specific interrupt with given handler and priority.
 *
 * The @p fast parameter allows to choose between the two supported types of
 * application interrupt modes.
 *
 * Fast interrupt handlers should be kept very short as they are executed in
 * interrupt context. Most of the handling should be done later, in a
 * "bottom-half" handler, also known as a slow/soft interrupt handler, or
 * deferred procedure call. To facilitate this, the interrupt handler can call
 * the @ref app_lib_system_set_periodic_cb_f "lib_system->setPeriodicCb"()
 * function with an initial delay of
 * zero. This causes the periodic callback to be called as soon as possible.
 *
 * If @p fast is false, the interrupt is considered as deferred interrupt. Then,
 * the top half of the interrupt is handled by the stack and interrupt handler
 * (bottom half)  set by the application is called as normal application task
 * context. Then, the top half does not need to be implemented in application.
 * Con is that it calling of the bottom half may be too late and deferred
 * interrupt is not necessarily feasible for all peripherals.
 *
 * Not all interrupts are available for application use. Each supported platform
 * reserves some interrupts for protocol stack use. Trying to enable a reserved
 * interrupt will result in an error. Application interrupt handlers will never
 * be called for reserved interrupts.
 *
 * @param   fast
 *          True for a fast irq
 * @param   irq_n
 *          The IRQ number to enable
 * @param   priority
 *          Interrupt priority level (only for fast interrupt)
 *          Available levels depend on platform, but two levels are guaranteed:
 *          @ref APP_LIB_SYSTEM_IRQ_PRIO_HI
 *          @ref APP_LIB_SYSTEM_IRQ_PRIO_LO
 * @param   handler
 *          Interrupt handler to be called
 * @return  Result code, @ref APP_RES_OK if successful, @ref
 *          APP_RES_INVALID_VALUE if @p irq_n is invalid, @ref
 *          APP_RES_RESOURCE_UNAVAILABLE if @p irq_n is not available for the
 *          application or @ref APP_RES_INVALID_CONFIGURATION if no interrupt
 *          handler or table set
 * @note    Fast interrupts and normal interrupts are
 *          disabled with the same function
 *
 * Example:
 * @code
 *
 * static void temp_interrupt_handler(void)
 * {
 *     NRF_TEMP->INTENCLR = 1;
 *     if (NRF_TEMP->EVENTS_DATARDY != 0)
 *     {
 *         temperature = NRF_TEMP->TEMP;
 *     }
 *     else
 *     {
 *         temperature = 0;
 *     }
 * }
 *
 * void App_init(const app_global_functions_t * functions)
 * {
 *     // Enable interrupt
 *     lib_system->enableAppIrq(false,
 *                              TEMP_IRQn,
 *                              APP_LIB_SYSTEM_IRQ_PRIO_LO,
 *                              temp_interrupt_handler);
 *
 *     ...
 * }
 * @endcode
 */
typedef app_res_e
    (*app_lib_system_enable_app_irq_with_handler_f)
    (bool fast,
     uint8_t irq_n,
     uint8_t priority,
     app_lib_system_irq_handler_f handler);

/**
 * @brief   Clear fast interrupt flag from NVIC
 *
 * Clear platform-specific fast application interrupt irq_n in the interrupt
 * controller. On most platforms it is also necessary to clear interrupt bits in
 * the interrupting peripherals, which this function does not do.
 *
 * @param   irq_n
 *          The pending IRQ number to clear
 * @return  Result code, @ref APP_RES_OK if successful,
 *          @ref APP_RES_INVALID_VALUE if @p irq_n is invalid,
 *          @ref APP_RES_RESOURCE_UNAVAILABLE if @p irq_n is not available for
 *          the application
 */
typedef app_res_e
    (*app_lib_system_clear_pending_fast_app_irq_f)(uint8_t irq_n);

/**
 * @brief   Get bootloader version
 *
 * Return a small positive integer that represents the version of the
 * bootloader. For platforms without bootloader, this function returns 0.
 *
 * @return  Bootloader version
 */
typedef uint32_t
    (*app_lib_system_get_bootloader_version_f)(void);

/**
 * @brief   Return radio hardware and platform information.
 * @param   info_p
 *          pointer to store system radio info result
 * @param   num_bytes
 *          the number of bytes of information to copy, at most the size of @ref
 *          app_lib_system_radio_info_t. This allows making the information
 *          struct bigger in the future.
 *
 * @return  Result code, @ref APP_RES_OK if successful,
 *          @ref APP_RES_INVALID_VALUE if @p info_num_bytes is invalid,
 *          @ref APP_RES_INVALID_NULL_POINTER if @p info_p is NULL
 */
typedef app_res_e
    (*app_lib_system_get_radio_info_f)(app_lib_system_radio_info_t * info_p,
                                       size_t info_num_bytes);

/**
 * @brief       List of library functions
 */
typedef struct
{
    app_lib_system_set_startup_cb_f setStartupCb;
    app_lib_system_set_shutdown_cb_f setShutdownCb;
    app_lib_system_set_periodic_cb_f setPeriodicCb;
    app_lib_system_enter_critical_section_f enterCriticalSection;
    app_lib_system_exit_critical_section_f exitCriticalSection;
    app_lib_system_disable_deep_sleep disableDeepSleep;
    app_lib_system_enable_app_irq_with_handler_f enableAppIrq;
    app_lib_system_disable_app_irq_f disableAppIrq;
    app_lib_system_clear_pending_fast_app_irq_f clearPendingFastAppIrq;
    app_lib_system_get_bootloader_version_f getBootloaderVersion;
    app_lib_system_get_radio_info_f getRadioInfo;
} app_lib_system_t;

#endif /* APP_LIB_SYSTEM_H_ */
