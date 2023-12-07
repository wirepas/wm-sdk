/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file wms_app.h
 *
 * The global macros, types and functions available to applications can be found
 * in the wms_app.h header.
 */
#ifndef APP_H_
#define APP_H_

#include <stdlib.h>
#include <stdint.h>

/* TODO: Move to a separate header */
#define __STATIC_INLINE static inline

/**
 * Global application API version. This is incremented if the global function
 * table format changes in an incompatible way. The value of this macro is
 * placed in the application header, so that the stack can detect and handle the
 * situation.
 */
#define APP_API_VERSION             0x200

/** Magic 16-byte string for locating a v2 application in Flash */
#define APP_V2_TAG                  ("APP2\171\306\073\165" \
                                     "\263\303\334\322\035\266\006\115")

/** Length of \ref APP_V2_TAG, in bytes */
#define APP_V2_TAG_LENGTH           16

/** Byte offset of \ref APP_V2_TAG from the start of application memory area */
#define APP_V2_TAG_OFFSET           48


/** Minimum supported application API version, when a tag is present: v2 */
#define APP_V2_TAG_MIN_API_VERSION  0x200

/** Application API version when no tag present: v1 */
#define APP_API_V1_VERSION          0x100

/** Container for \ref APP_V2_TAG, for pointer arithmetic */
typedef union
{
    uint8_t bytes[APP_V2_TAG_LENGTH];
    uint32_t array[APP_V2_TAG_LENGTH / 4];
} app_v2_tag_t;

/**
 * \brief   Application information header
 *
 * If an application is compiled to support application API v2, this header
 * can found in the beginning of the application memory area. It is placed
 * right after the \ref APP_V2_TAG, which is at \ref APP_V2_TAG_OFFSET.
 */
typedef struct
{
    /** Expected API version of application */
    uint32_t api_version;
    /** Expected start address of application memory area, for sanity checks */
    uint32_t start_address;
    /** Total number of bytes used in the application memory area */
    uint32_t length;
    /** First address used in RAM area (was forced to be 0 before) */
    uint32_t start_ram_address;
} app_information_header_t;

/**
 * Firmware version type, returned from \ref
 * app_global_functions_t.getStackFirmwareVersion(). The individual sub-fields
 * can be accessed, or the whole 32-bit value can be read all at once, using
 * the version field.
 */
typedef union
{
    struct
    {
        /** Firmware development version number component */
        uint8_t devel;
        /** Firmware maintenance version number component */
        uint8_t maint;
        /** Firmware minor version number component */
        uint8_t minor;
        /** Firmware major version number component */
        uint8_t major;
    };
    uint32_t    version;
} app_firmware_version_t;

/**
 * Get the global API version, which may be greater than the \ref
 * APP_API_VERSION macro if the stack firmware is newer than the SDK used to
 * compile the application. It is up to the stack firmware to be backward
 * compatible with any global API changes, or not run the application at all.
 * \return  Supported application API version, \ref APP_API_VERSION
 */
typedef uint32_t (*app_get_api_version_f)(void);

/**
 * \brief   Get stack firmware version
 * \return  Stack firmware version
 */
typedef app_firmware_version_t (*app_get_stack_firmware_version_f)(void);

/**
 * \brief   Open a library
 *
 * This is the most important function of the Single-MCU API. All other
 * functions are in libraries, opened using the this function. Parameters are
 * the name and version of the library. The name is actually a 32-bit value,
 * which is defined in each library header as a macro: 
 * \c APP_LIB_LIBRARY_NAME. The library version is also a macro defined in the same
 * library header: \c APP_LIB_LIBRARY_VERSION.
 *
 * Example of opening a library:
 *
 * @code
 *
 *      // The System library
 *      static const app_lib_system_t * lib_system = NULL;
 *
 *      ...
 *
 *      void App_init(const app_global_functions_t * functions)
 *      {
 *          lib_system = functions->openLibrary(APP_LIB_SYSTEM_NAME,
 *                                              APP_LIB_SYSTEM_VERSION);
 *          if (lib_system == NULL)
 *          {
 *              // Could not open the System library
 *              return;
 *          }
 *          ...
 *      }
 * @endcode
 *
 * \param   name
 *          Symbolic name of library, a macro in library header file
 * \param   version
 *          Requested library version, a macro in library header file
 * \return  Pointer to the function table of the library. Each library has its
 *          own function table. If a NULL is returned, the library could not be
 *          opened. Either it didn't exist, or the version is too old or new.
 * \note    There is no corresponding closeLibrary() call. Applications never
 *          close libraries they open.
 */
typedef const void * (*app_open_library_f)(uint32_t name, uint32_t version);

/**
 * \brief  List of global functions, passed to \ref App_entrypoint()
 */
typedef struct
{
    app_get_api_version_f               getApiVersion;
    app_get_stack_firmware_version_f    getStackFirmwareVersion;
    app_open_library_f                  openLibrary;
} app_global_functions_t;

/**
 * \brief   Application initial entrypoint
 * \param   functions
 *          Pointer to a global function table, \ref app_global_functions_t
 * \param   reserved1
 *          Reserved for future use
 * \param   reserved2
 *          Reserved for future use
 * \param   ram_top
 *          Pointer to a pointer of first free RAM address. The application may
 *          reduce this value, if it does not need all the RAM provided to it
 */
intptr_t App_entrypoint(const void * functions,
                        size_t reserved1,
                        const void ** reserved2,
                        void ** ram_top);

/** A function to safely call either getApiVersion() or getCurrentRole() */
typedef uint32_t (*get_api_version_compatible_f)(void);

__STATIC_INLINE uint32_t App_getApiVersion(const void * const global_cb)
{
    uint32_t version =
        ((const get_api_version_compatible_f)*(const void **)global_cb)();
    if (version < APP_API_V1_VERSION)
    {
        // Called function was actually the getCurrentRole() function,
        // so the firmware only supports the v1 application API
        return APP_API_V1_VERSION;
    }
    return version;
}

/**
 * This is the most common return type from library functions. Functions use
 * these return values, unless more specific return values are required.
 */
typedef enum
{
    /** Everything is OK */
    APP_RES_OK = 0,
    /** Error: Other or internal error */
    APP_RES_UNSPECIFIED_ERROR = 1,
    /** Error: Feature is not implemented */
    APP_RES_NOT_IMPLEMENTED = 2,
    /** Error: One or more parameter value is invalid */
    APP_RES_INVALID_VALUE = 3,
    /** Error: One or more required pointer parameter is NULL */
    APP_RES_INVALID_NULL_POINTER = 4,
    /** Error: Current configuration does not support the requested operation */
    APP_RES_INVALID_CONFIGURATION = 5,
    /** Error: Requested resource is not available */
    APP_RES_RESOURCE_UNAVAILABLE = 6,
    /** Error: Stack is in invalid state for the requested operation */
    APP_RES_INVALID_STACK_STATE = 7,
    /** Error: Feature lock bits forbid the requested operation */
    APP_RES_ACCESS_DENIED = 8,
} app_res_e;

/**
 * Node address. Each node must have a unique address in the network. There are
 * special addresses that can be used as destination addresses, see \ref
 * app_special_addr_e. For various addressing modes, see @c addressing.
 */
typedef uint32_t app_addr_t;

/**
 * \brief   Special destination addresses for sending packets
 */
typedef enum
{
    /** Send packet to the best available sink */
    APP_ADDR_ANYSINK     = 0xfffffffeu,
    /**
     * @brief Send packet as broadcast to all nodes
     *
     * @note When transmitting from a sink, note that a broadcast will only be
     *       transmitted to the nodes directly under the sink's routing tree.
     *       To reach all nodes on the network, it is necessary to send the
     *       broadcast from all sinks.
     */
    APP_ADDR_BROADCAST   = 0xffffffffu,
    /**
     * This is a bitmask that should be ORed with group address to send data
     * packet to the multicast group. For example: 0x80000002 address sends
     * packet to the multicast group #2.
     */
    APP_ADDR_MULTICAST   = 0x80000000u,
    /** This is last valid multicast address. Addresses larger than this (until
     * @ref APP_ADDR_ANYSINK) are unicast addresses. */
    APP_ADDR_MULTICAST_LAST = 0x80ffffffu,
} app_special_addr_e;

#endif  /* APP_H_ */
