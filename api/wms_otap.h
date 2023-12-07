/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file wms_otap.h
 *
 * The Over-The-Air-Programming (OTAP) library allows reading and writing the
 * OTAP scratchpad memory area of a Wirepas Mesh node. Using the OTAP library,
 * it is possible to wirelessly upgrade the stack firmware and applications on a
 * running network.
 *
 * To gain better understanding of the OTAP library, getting familiar with
 * concepts outlined in WP-RM-108 - OTAP Reference Manual is recommended.
 *
 * Library services are accesses via @ref app_lib_otap_t "lib_otap" handle
 */
#ifndef APP_LIB_OTAP_H_
#define APP_LIB_OTAP_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "wms_app.h"

/** @brief Library symbolic name  */
#define APP_LIB_OTAP_NAME 0x000f2338 //!< "OTAP"

/** @brief Maximum supported library version */
#define APP_LIB_OTAP_VERSION 0x202

/**
 * @brief Different scratchpad type
 *
 * Used with the
 * @ref app_lib_otap_get_type_f "lib_otap->getType()" function
 */
typedef enum
{
    /** No valid scratchpad stored */
    APP_LIB_OTAP_TYPE_BLANK = 0,
    /** The is a valid scratchpad, but it has not yet been marked for
     * processing */
    APP_LIB_OTAP_TYPE_PRESENT = 1,
    /** There is a valid scratchpad and it has been marked for processing */
    APP_LIB_OTAP_TYPE_PROCESS = 2,
} app_lib_otap_type_e;

/**
 * @brief Status code from the bootloader
 *
 * Used with the @ref app_lib_otap_get_status_f
 * "lib_otap->getStatus()" function
 *
 * @note Other small positive values may be returned as error codes
 * from the bootloader.
 */
typedef enum
{
    /** The bootloader processed the scratchpad and everything went well */
    APP_LIB_OTAP_STATUS_OK = 0x00000000,
    /** The bootloader has not yet processed the scratchpad */
    APP_LIB_OTAP_STATUS_NEW = UINT32_MAX,
} app_lib_otap_status_e;

/**
 * @brief Write function return code
 *
 * Used with the @ref app_lib_otap_write_f "lib_otap->write()" function
 */
typedef enum
{
    /** Write succeeded */
    APP_LIB_OTAP_WRITE_RES_OK = 0,
    /** Write succeeded and was the last one */
    APP_LIB_OTAP_WRITE_RES_COMPLETED_OK = 1,
    /** Error: write was the last one but the CRC check failed */
    APP_LIB_OTAP_WRITE_RES_COMPLETED_ERROR = 2,
    /** Error: there is no write ongoing */
    APP_LIB_OTAP_WRITE_RES_NOT_ONGOING = 3,
    /** Error: start is invalid */
    APP_LIB_OTAP_WRITE_RES_INVALID_START = 4,
    /** Error: num_bytes is invalid */
    APP_LIB_OTAP_WRITE_RES_INVALID_NUM_BYTES = 5,
    /** Error: header is invalid */
    APP_LIB_OTAP_WRITE_RES_INVALID_HEADER = 6,
    /** Error: bytes is NULL */
    APP_LIB_OTAP_WRITE_RES_INVALID_NULL_BYTES = 7,
} app_lib_otap_write_res_e;

/**
 * @brief Scratchpad possible actions
 *
 * Used with the @ref app_lib_otap_set_target_scratchpad_and_action_f
 * "lib_otap->setTargetScratchpadAndAction()" function
 */
typedef enum
{
    /** No otap in the sink tree. */
    APP_LIB_OTAP_ACTION_NO_OTAP = 0,
    /** Only propagate the target scratchpad but do not process it. */
    APP_LIB_OTAP_ACTION_PROPAGATE_ONLY = 1,
    /** Propagate the target scratchpad and process it immediately. */
    APP_LIB_OTAP_ACTION_PROPAGATE_AND_PROCESS = 2,
    /** Same as previous except that the processing should happen after
     *  the given delay. Delay starts when node receive the information and the
     *  scratchpad is valid. */
    APP_LIB_OTAP_ACTION_PROPAGATE_AND_PROCESS_WITH_DELAY = 3,
} app_lib_otap_action_e;

/**
 * @brief Type for OTAP sequence number
 *
 * Two numbers are special:
 * <table>
 * <tr><th>Value<th>Description
 * <tr><td>0<td>Node does not participate in OTAP
 * <tr><td>255<td>Node accepts any scratchpad from the network as newer
 * </table>
 */
typedef uint8_t app_lib_otap_seq_t;

/**
 * @brief Get maximum scratchpad size
 *
 * @return Return the maximum possible scratchpad size in bytes.
 */
typedef size_t
    (*app_lib_otap_get_max_num_bytes_f)(void);

/**
 * @brief Get stored scratchpad size
 *
 * @return Number of bytes in scratchpad, or zero if scratchpad is not valid
 */
typedef size_t
    (*app_lib_otap_get_num_bytes_f)(void);

/**
 * @brief Get maximum block size in bytes
 *
 * It is the maximum block size, in bytes, that can be fed to the @ref
 * app_lib_otap_write_f "lib_otap->write()" function
 *
 * @return Maximum number of bytes for read() or write()
 */
typedef size_t
    (*app_lib_otap_get_max_block_num_bytes_f)(void);

/**
 * @brief Get the OTAP sequence number
 *
 * Get the OTAP sequence number @ref app_lib_otap_seq_t of the stored
 * scratchpad, or 0 if there is no stored scratchpad
 *
 * @return Sequence number, or zero if scratchpad is not valid
 */
typedef app_lib_otap_seq_t
    (*app_lib_otap_get_seq_f)(void);

/**
 * @brief Get the 16-bit CRC of the stored scratchpad
 *
 * @return CRC of data in scratchpad, or zero if scratchpad is not valid
 *
 * @note Calling this function may take several tens of milliseconds
 * @note Calling this function for the first time after a scratchpad has changed may
 * take several tens of milliseconds.
 */
typedef uint16_t
    (*app_lib_otap_get_crc_f)(void);

/**
 * @brief Get the type of the stored scratchpad.
 *
 * @return Type of stored scratchpad, @ref app_lib_otap_type_e
 */
typedef app_lib_otap_type_e
    (*app_lib_otap_get_type_f)(void);

/**
 * @brief Get the bootloader status of the stored scratchpad.
 *
 * @return Status, or @ref APP_LIB_OTAP_STATUS_OK
 *         if scratchpad is not valid
 */
typedef app_lib_otap_status_e
    (*app_lib_otap_get_status_f)(void);

/**
 * @brief Get the size of scratchpad, in bytes, that produced the running stack
 * firmware.
 *
 * The bootloader stores this information in the stack firmware area,
 * when it writes the firmware.
 *
 * @return  Number of bytes in processed scratchpad, or zero if not known
 */
typedef size_t
    (*app_lib_otap_get_processed_num_bytes_f)(void);

/**
 * @brief Get the OTAP sequence number of the scratchpad that produced the running
 * stack firmware.
 *
 * The bootloader stores this information in the stack firmware area,
 * when it writes the firmware.
 *
 * @return  Sequence number of processed scratchpad, or zero if not known
 */
typedef app_lib_otap_seq_t
    (*app_lib_otap_get_processed_seq_f)(void);

/**
 * @brief Get the 16-bit CRC of the scratchpad that produced the running stack
 * firmware.
 *
 * The bootloader stores this information in the stack firmware area,
 * when it writes the firmware.
 *
 * @return  CRC of processed scratchpad, or zero if not known
 */
typedef uint16_t
    (*app_lib_otap_get_processed_crc_f)(void);

/**
 * @brief Get the area ID of the file in the scratchpad that produced the running
 * stack firmware.
 *
 * The bootloader stores this information in the stack firmware area,
 * when it writes the firmware.
 *
 * @return  Area ID of running stack firmware, or zero if not known
 */
typedef uint32_t
    (*app_lib_otap_get_processed_area_id_f)(void);

/**
 * @brief Get the scratchpad validity status
 *
 * The same information is available via the more generic function @ref
 * app_lib_otap_get_type_f "lib_otap->getType()".
 *
 * @return  True, if data in scratchpad is valid
 *
 * @note    Valid data isn't necessarily a firmware image
 */
typedef bool
    (*app_lib_otap_is_valid_f)(void);

/**
 * @brief Check if the stored scratchpad has been processed by the bootloader
 *
 * The same information is available via the more generic
 * functions @ref app_lib_otap_get_type_f "lib_otap->getType()" and
 * @ref app_lib_otap_get_status_f "lib_otap->getStatus()".
 *
 * @return True, if the scratchpad has been processed by the bootloader
 */
typedef bool
    (*app_lib_otap_is_processed_f)(void);

/**
 * @brief   Check if scratchpad is set to be processed
 *
 * @return  True, if scratchpad data is valid and marked to be processed
 */
typedef bool
    (*app_lib_otap_is_set_to_be_processed_f)(void);

/**
 * @brief Read a block of scratchpad.
 *
 * Reading can be done in a random-access fashion and stack can remain running.
 *
 * \p start is the byte offset within the scratchpad. \p num_bytes is the number
 * of bytes to read. bytes must point to a buffer of suitable size. Maximum
 * number of bytes to read at a time can be queried with the @ref
 * app_lib_otap_get_max_block_num_bytes_f "lib_otap->getMaxBlockNumBytes()" function.
 *
 * Simultaneous reading and writing of scratchpad is not supported. A @ref
 * app_lib_otap_begin_f "lib_otap->begin()" / @ref app_lib_otap_write_f "lib_otap->write()" cycle must not be
 * ongoing when calling this function.
 *
 * @param   start
 *          Byte offset from the beginning of scratchpad memory
 * @param   num_bytes
 *          Number of bytes to read
 * @param   bytes
 *          Pointer to buffer for reading bytes
 * @return  Result code, @ref APP_RES_OK if successful,
 *          @ref APP_RES_INVALID_VALUE if start or num_bytes is invalid,
 *          @ref APP_RES_INVALID_NULL_POINTER if bytes is NULL,
 *          @ref APP_RES_RESOURCE_UNAVAILABLE if scratchpad is not valid
 */
typedef app_res_e
    (*app_lib_otap_read_f)(uint32_t start,
                           size_t num_bytes,
                           void * bytes);

/**
 * @brief Erase a stored scratchpad from memory
 *
 * Erasing the scratchpad is only possible when the stack is stopped.
 *
 * When the stack is started, unless feature lock bits prevent it, a scratchpad
 * may be received again depending on otap target in network
 *
 * @return  Result code, @ref APP_RES_OK if successful,
 *          @ref APP_RES_INVALID_STACK_STATE if stack is running
 *
 * @note On some platforms, this function may take several seconds to complete
 */
typedef app_res_e
    (*app_lib_otap_clear_f)(void);

/**
 * @brief Start writing a new scratchpad.
 *
 * Any old scratchpad is implicitly erased, which may take several seconds on some platforms.
 *
 * Scratchpad can only be written when the stack is stopped. \p num_bytes must
 * be divisible by four and a minimum of 96 bytes, which is the minimum valid
 * scratchpad size. Maximum scratchpad size can be queried with the @ref
 * app_lib_otap_get_max_num_bytes_f "lib_otap->getMaxNumBytes()" function.
 *
 * When writing the scratchpad to a node, with the @ref app_lib_otap_write_f "lib_otap->write()"
 * function described below, the bytes written contain a scratchpad header, with
 * an OTAP sequence number field. The OTAP sequence number in that header is
 * replaced with the seq parameter specified here.
 *
 * Returns @ref APP_RES_INVALID_STACK_STATE if stack is running. Returns @ref
 * APP_RES_INVALID_VALUE if num_bytes is invalid. Otherwise, returns @ref
 * APP_RES_OK.
 *
 * @param   num_bytes
 *          Total number of bytes to write
 * @param   seq
 *          Scratchpad sequence number
 * @return  Result code, @ref APP_RES_OK if successful,
 *          @ref APP_RES_INVALID_VALUE if num_bytes or seq is invalid,
 *          @ref APP_RES_INVALID_STACK_STATE if stack is running
 * @note    Scratchpad memory is implicitly cleared if this call succeeds
 */
typedef app_res_e
    (*app_lib_otap_begin_f)(size_t num_bytes,
                            app_lib_otap_seq_t seq);

/**
 * @brief Write a block of scratchpad
 *
 * Write must have been started with the @ref app_lib_otap_begin_f
 * "lib_otap->begin()" function.
 *
 * \p start must follow the previous block exactly and \p num_bytes must be
 * divisible by four. The first block must start from zero. Maximum number of
 * bytes to write at a time can be queried with the @ref
 * app_lib_otap_get_max_block_num_bytes_f "lib_otap->getMaxBlockNumBytes"() function.
 *
 * @param   start
 *          Byte offset from the beginning of scratchpad memory
 * @param   num_bytes
 *          Number of bytes to write
 * @param   bytes
 *          Pointer to bytes to write
 * @return  A return code: @ref app_lib_otap_write_res_e
 * @note    \p num_bytes needs to be divisible by four
 * @note    \p start byte offset needs to be right after
 *          previous write, with no gaps or overlap
 */
typedef app_lib_otap_write_res_e
    (*app_lib_otap_write_f)(uint32_t start,
                            size_t num_bytes,
                            const void * bytes);

/**
 * @brief Mark the stored scratchpad to be eligible for processing by the bootloader
 *
 * Whenever the node will be rebooted, the bootloader will process the
 * scratchpad. If the scratchpad was already marked for processing or was
 * already processed, this function does nothing.
 *
 * @return  Result code, @ref APP_RES_OK if successful,
 *          @ref APP_RES_RESOURCE_UNAVAILABLE if scratchpad data is invalid
 *
 * @note    Does nothing if scratchpad was already set to be bootable
 */
typedef app_res_e
    (*app_lib_otap_set_to_be_processed_f)(void);

/**
 * @brief   This service allows to set the information for scratchpad
 *          in the sink tree. What is the target scratchpad and what is the
 *          action to do with it
 * @param   target_sequence
 *          The scratchpad target sequence for this Sink tree. Only needed if
 *          action is @ref APP_LIB_OTAP_ACTION_PROPAGATE_ONLY,
 *          @ref APP_LIB_OTAP_ACTION_PROPAGATE_AND_PROCESS
 *          or @ref APP_LIB_OTAP_ACTION_PROPAGATE_AND_PROCESS_WITH_DELAY.
 *          Valid values are 1..255.
 * @param   target_crc
 *          The scratchpad target crc for this Sink tree. Only needed if
 *          action is @ref APP_LIB_OTAP_ACTION_PROPAGATE_ONLY,
 *          @ref APP_LIB_OTAP_ACTION_PROPAGATE_AND_PROCESS
 *          or @ref APP_LIB_OTAP_ACTION_PROPAGATE_AND_PROCESS_WITH_DELAY.
 * @param   action
 *          The action to do with the target scratchpad
 * @param   delay
 *          The delay to process the scratchpad. Only needed if action is
 *          @ref APP_LIB_OTAP_ACTION_PROPAGATE_AND_PROCESS_WITH_DELAY. This
 *          delay can't be 0 minute/hour/day.
 * @return  Result code, @ref APP_RES_OK if successful.
 * @note    Returns @ref APP_RES_INVALID_VALUE if delay is the same than the
 *          actual one when action is
 *          @ref APP_LIB_OTAP_ACTION_PROPAGATE_AND_PROCESS_WITH_DELAY. This use
 *          case is invalid because setting the delay two consecutive times is
 *          ambiguous. In fact, as a node can switch from a sink to the other
 *          with the same delay, it must not reset its timer in this condition.
 *          So to be coherent, setting the same delay through the api is
 *          forbidden.
 *
 * Example of use to load and set a scratcphad of N bytes with seq S to be
 * processed in Sink tree
 * @code
 *     // Write a scratchpad to the sink
 *     lib_otap->begin(N, S);
 *     while (res == APP_LIB_OTAP_WRITE_RES_COMPLETED_OK)
 *     {
 *        res = lib_otap->write(x,x,x)
 *     }
 *
 *     // Check that scratchpad is correctly written
 *     ...
 *
 *     lib_data->setTargetScratchpadAndAction(lib_otap->getSeq(),
 *                                            lib_otap->getCrc(),
 *                                            APP_LIB_OTAP_ACTION_PROPAGATE_AND_PROCESS,
 *                                            0 // Not used for this action);
 * }
 * @endcode
 *
 * The delay parameter is encoded as follows :
 * @code
 * struct
 * {
 *     //Bits 0.. 5 - The time to delay.
 *     uint8_t delay :6;
 *     //Bits 6  .7 - Time unit used (1:Minutes, 2:Hours, 3:Days).
 *     uint8_t unit  :2;
 * };
 * @endcode
 */
typedef app_res_e (*app_lib_otap_set_target_scratchpad_and_action_f)(
                                            app_lib_otap_seq_t target_sequence,
                                            uint16_t target_crc,
                                            app_lib_otap_action_e action,
                                            uint8_t delay);

/**
 * @brief   This service allows to get the information for scratchpad
 *          in the sink tree. What is the target scratchpad and what is the
 *          action to do with it
 * @param   target_sequence
 *          The scratchpad target sequence for this sink tree.
 * @param   target_crc
 *          The scratchpad target crc for this sink tree.
 * @param   action
 *          The action to do with the target scratchpad
 * @param   delay
 *          The delay to process the scratchpad. Only needed if action is
 *          @c APP_LIB_OTAP_ACTION_PROPAGATE_AND_PROCESS_WITH_DELAY.
 *
 * @return  Result code, @ref APP_RES_OK if successful
 */
typedef app_res_e
    (*app_lib_otap_get_target_scratchpad_and_action_f)(
                                            app_lib_otap_seq_t * target_sequence,
                                            uint16_t * target_crc,
                                            app_lib_otap_action_e * action,
                                            uint8_t * delay);

/**
 * The function table returned from @ref app_open_library_f
 */
typedef struct
{
    app_lib_otap_get_max_num_bytes_f                getMaxNumBytes;
    app_lib_otap_get_num_bytes_f                    getNumBytes;
    app_lib_otap_get_max_block_num_bytes_f          getMaxBlockNumBytes;
    app_lib_otap_get_seq_f                          getSeq;
    app_lib_otap_get_crc_f                          getCrc;
    app_lib_otap_get_type_f                         getType;
    app_lib_otap_get_status_f                       getStatus;
    app_lib_otap_get_processed_num_bytes_f          getProcessedNumBytes;
    app_lib_otap_get_processed_seq_f                getProcessedSeq;
    app_lib_otap_get_processed_crc_f                getProcessedCrc;
    app_lib_otap_get_processed_area_id_f            getProcessedAreaId;
    app_lib_otap_is_valid_f                         isValid;
    app_lib_otap_is_processed_f                     isProcessed;
    app_lib_otap_is_set_to_be_processed_f           isSetToBeProcessed;
    app_lib_otap_read_f                             read;
    app_lib_otap_clear_f                            clear;
    app_lib_otap_begin_f                            begin;
    app_lib_otap_write_f                            write;
    app_lib_otap_set_to_be_processed_f              setToBeProcessed;
    app_lib_otap_set_target_scratchpad_and_action_f setTargetScratchpadAndAction;
    app_lib_otap_get_target_scratchpad_and_action_f getTargetScratchpadAndAction;
} app_lib_otap_t;

#endif /* APP_LIB_OTAP_H_ */
