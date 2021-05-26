/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file hardware.h
 *
 * The hardware library allows telling the stack that a certain hardware
 * peripheral is in use by the application, so that the stack does not use the
 * peripheral at the same time. Also vice versa, hardware library tells
 * application if it is trying to use hardware peripheral used by the stack.
 * The stack needs to know chip temperature to do periodic RF calibrations on
 * some platforms, and that may require the stack to access hardware peripherals
 * that the application also uses.
 *
 * The application may use a peripheral either intermittently or permanently.
 *
 * This library also allows the activation of a hardware peripheral. In fact,
 * some peripheral may be needed by the application or the stack with a
 * concurrent access as the activation of an external crystal for more
 * precision.
 *
 * Library services are accessed  via @ref app_lib_hardware_t "lib_hw"
 * handle.
 */
#ifndef APP_LIB_HARDWARE_H_
#define APP_LIB_HARDWARE_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "app/app.h"

/** \brief Library symbolic name  */
#define APP_LIB_HARDWARE_NAME 0x014eff15 //!< "HWARE"

/** \brief Maximum supported library version */
#define APP_LIB_HARDWARE_VERSION 0x202

/**
 * @brief Hardware peripherals to reserve.
 *
 * To be used with services @ref app_lib_hardware_reserve_peripheral_f
 * "lib_hw->reservePeripheral()" and @ref app_lib_hardware_release_peripheral_f
 * "lib_hw->releasePeripheral()".
 *
 * @note Not all platforms have all peripherals available.
 */
typedef enum
{
    /** Reserved for future use */
    APP_LIB_HARDWARE_PERIPHERAL_RESERVED = 3,
} app_lib_hardware_peripheral_e;

/**
 * @brief Hardware peripherals to activate.
 *
 * To be used with services @ref app_lib_hardware_activate_peripheral_f
 * "lib_hw->activatePeripheral()" and @ref
 * app_lib_hardware_deactivate_peripheral_f "lib_hw->deactivatePeripheral()".
 *
 * @note Not all platforms have all peripherals available.
 */
typedef enum
{
    /** General high frequency external crystal */
    APP_LIB_HARDWARE_PERIPHERAL_HFXO = 0,
} app_lib_hardware_activable_peripheral_e;

/**
 * @brief   Reserve a hardware peripheral
 *
 * @note This is for stack versions <5.1.0. For 5.1.0 devices, stack does not
 * reserve any peripherals, therefore @ref APP_RES_OK is returned.
 *
 * @param   peripheral
 *          Hardware peripheral to reserve
 * @return  Returns @ref APP_RES_OK
 */
typedef app_res_e
    (*app_lib_hardware_reserve_peripheral_f)
    (app_lib_hardware_peripheral_e peripheral);

/**
 * @brief  Release a reserved hardware peripheral back for stack use.
 *
 * @note This is for stack versions <5.1.0. For 5.1.0 devices, stack does not
 * reserve any peripherals, therefore @ref APP_RES_OK is returned.
 *
 * @param   peripheral
 *          Hardware peripheral to reserve
 * @return  Returns @ref APP_RES_RESOURCE_OK
 */
typedef app_res_e
    (*app_lib_hardware_release_peripheral_f)
    (app_lib_hardware_peripheral_e peripheral);

/**
 * @brief Query voltage value
 *
 * @note This is for stack versions <5.1.0.
 *
 * \return  Return always 0
 *
 * Example: See @ref inventory_app_tag/app.c "inventory_app_tag" example.
 */
typedef uint16_t
    (*app_lib_hardware_read_supply_voltage_f)(void);

/**
 * @brief   Ask the last read chip temperature from the stack.
 *
 * @note This is for stack versions <5.1.0.
 *
 * @return  Return always 0
 */
typedef int32_t
    (*app_lib_hardware_read_temperature_f)(void);

/**
 * \brief   Request for peripheral activation.
 *
 * \param   peripheral
 *          Hardware peripheral to activate
 * \return  If peripheral is not recognized or not reservable, \ref
 *          APP_RES_RESOURCE_UNAVAILABLE is returned. Otherwise, \ref APP_RES_OK
 *          is returned.
 */
typedef app_res_e
    (*app_lib_hardware_activate_peripheral_f)(app_lib_hardware_activable_peripheral_e peripheral);

/**
 * \brief   Request for peripheral deactivation.
 *
 * \param   peripheral
 *          Hardware peripheral to deactivate
 * \return  If peripheral is not recognized or not reservable, \ref
 *          APP_RES_RESOURCE_UNAVAILABLE is returned. If peripheral was not
 *          previously activated, \ref APP_RES_INVALID_CONFIGURATION. Otherwise,
 *          \ref APP_RES_OK is returned.
 */
typedef app_res_e
    (*app_lib_hardware_deactivate_peripheral_f)(app_lib_hardware_activable_peripheral_e peripheral);

/**
 * \brief   Request for peripheral status.
 *
 * The given \p timeout_us is the maximum time to wait for peripheral
 * activation. Each peripheral has a maximum timeout dependent on the platform.
 *
 * \param   peripheral
 *          Hardware peripheral to check status
 * \param   activated_p
 *          Pointer to store the status: true if activated, false otherwise
 * \param   timeout_us
 *          Maximum time to wait for activation
 * \return  If peripheral is not recognized or not reservable, \ref
 *          APP_RES_RESOURCE_UNAVAILABLE is returned. If peripheral was not
 *          previously activated, \ref APP_RES_INVALID_CONFIGURATION. If
 *          \p timeout_us is too long \ref APP_RES_INVALID_VALUE is returned.
 *          Otherwise, \ref APP_RES_OK is returned and \p activated_p is
 *          updated.
 * \note    Each peripheral has a maximum timeout dependent on the platform
 */
typedef app_res_e
    (*app_lib_hardware_is_peripheral_activated_f)(app_lib_hardware_activable_peripheral_e peripheral,
                                                  bool * activated_p,
                                                  uint32_t timeout_us);


/**
 * \brief       List of library functions
 */
typedef struct
{
    app_lib_hardware_reserve_peripheral_f      reservePeripheral;
    app_lib_hardware_release_peripheral_f      releasePeripheral;
    app_lib_hardware_read_supply_voltage_f     readSupplyVoltage;
    app_lib_hardware_read_temperature_f        readTemperature;
    app_lib_hardware_activate_peripheral_f     activatePeripheral;
    app_lib_hardware_deactivate_peripheral_f   deactivatePeripheral;
    app_lib_hardware_is_peripheral_activated_f isPeripheralActivated;
} app_lib_hardware_t;

#endif /* APP_LIB_HARDWARE_H_ */
