/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file debug_log.h
 *
 * Helper file to print debug logs.
 */

#include <stdint.h>
#include <stdarg.h>
#include "api.h"
#include "uart_print.h"

/**
 * Simple library to print only relevant log messages.
 *
 * APP_PRINTING=yes must be defined in the makefile of the application.
 *
 * Add the following lines in each file using debug logs :
 * @code
 *    #define DEBUG_LOG_MODULE_NAME "module_name"
 *    #define DEBUG_LOG_MAX_LEVEL LVL_INFO
 *    #include "debug_log.h"
 * @endcode
 *
 * By default Module name; TimeStamp (coarse) and Debug level are printed.
 *
 * By defining DEBUG_LOG_CUSTOM in the app makefile it is possible to select
 * which debug field is printed.
 *
 * Define one of the folling in the general makefile or before including
 * "debug_log.h" to enable the corresponding field in the log message :
 *  - DEBUG_LOG_PRINT_MODULE_NAME
 *  - DEBUG_LOG_PRINT_TIME
 *  - DEBUG_LOG_PRINT_TIME_HP
 * (DEBUG_LOG_PRINT_TIME and DEBUG_LOG_PRINT_TIME_HP are mutually exclusive).
 *  - DEBUG_LOG_PRINT_LEVEL
 *  - DEBUG_LOG_PRINT_FUNCTION
 *  - DEBUG_LOG_PRINT_LINE
 *
 * Usage :
 *
 * @code
 *    LOG_INIT();
 *
 *    LOG(LVL_DEBUG, "This is a debug message not printed"\
 *                   "with current DEBUG_LOG_MAX_LEVEL = LVL_INFO);
 *
 *    LOG(LVL_INFO, "This is an info message printed"\
 *                  "with current DEBUG_LOG_MAX_LEVEL = LVL_INFO);
 * @endcode
 */

#ifdef APP_PRINTING
#define Print_Log(fmt, ...) UartPrint_printf(fmt, ##__VA_ARGS__)
#ifndef DEBUG_LOG_UART_BAUDRATE
#define DEBUG_LOG_UART_BAUDRATE 115200
#endif
#define LOG_INIT() UartPrint_init(DEBUG_LOG_UART_BAUDRATE)
#else
#define Print_Log(fmt, ...)
#define LOG_INIT()
#endif

/**
 * Only logs with a level lower or equal to DEBUG_LOG_MAX_LEVEL will be printed
 *
 * \note this constant can be defined in each file including this file
 */
#ifndef DEBUG_LOG_MAX_LEVEL
/* By default only errors are displayed */
#define DEBUG_LOG_MAX_LEVEL LVL_ERROR
#endif

/**
 * \brief Macros to define several log levels: Debug, Info, Warning, Error.
 */
#define LVL_DEBUG 4
#define LVL_INFO 3
#define LVL_WARNING 2
#define LVL_ERROR 1
#define LVL_NOLOG 0

/* Do not modify; number in macro name must match the defined log levels
 * above.
 */
#define LVL_STRING_4 "D"
#define LVL_STRING_3 "I"
#define LVL_STRING_2 "W"
#define LVL_STRING_1 "E"
#define LVL_STRING_0 ""

/**
 * \brief Macro to retrieve a string from the log level.
 */
#define DEBUG_LVL_TO_STRING(level) LVL_STRING_##level

/** Time to flush Usart buffer in ms (buffer is 512 bytes,
 *  so ~45ms to flush full buffer).
 */
#define FLUSH_DELAY_MS 45


#ifndef DEBUG_LOG_CUSTOM
#    ifndef DEBUG_LOG_MODULE_NAME
         /* Name of the module must be defined. */
#        error "No module name set for logger"
#endif
     /* Use "[Module name][Time] Log level: " log prefix by default. */
#    define DEBUG_LOG_PRINT_MODULE_NAME
#    define DEBUG_LOG_PRINT_TIME
#    define DEBUG_LOG_PRINT_LEVEL
#    undef  DEBUG_LOG_PRINT_FUNCTION
#    undef  DEBUG_LOG_PRINT_LINE
#endif

/* Module name string. */
#ifdef DEBUG_LOG_PRINT_MODULE_NAME
#    define S_MOD_NAME_PREFIX "["DEBUG_LOG_MODULE_NAME"]"
#else
#    define S_MOD_NAME_PREFIX
#endif
/* Timestamp string. */
#ifdef DEBUG_LOG_PRINT_TIME_HP
#    define S_TIME_PREFIX "[%09u]"
#    define S_TIME_SUFFIX , lib_time->getTimestampHp()
#elif defined(DEBUG_LOG_PRINT_TIME)
#    define S_TIME_PREFIX "[%09u]"
#    define S_TIME_SUFFIX , lib_time->getTimestampCoarse()
#else
#    define S_TIME_PREFIX
#    define S_TIME_SUFFIX
#endif
/* Log level string. */
#ifdef DEBUG_LOG_PRINT_LEVEL
#    define S_LEVEL_PREFIX(level) " "DEBUG_LVL_TO_STRING(level)": "
#else
#    define S_LEVEL_PREFIX(level)
#endif
/* Function string. */
#ifdef DEBUG_LOG_PRINT_FUNCTION
#    define S_FUNCTION_PREFIX "func:%s, "
#    define S_FUNCTION_SUFFIX , __FUNCTION__
#else
#    define S_FUNCTION_PREFIX
#    define S_FUNCTION_SUFFIX
#endif
/* Line string. */
#ifdef DEBUG_LOG_PRINT_LINE
#    define S_LINE_PREFIX "line: %03d, "
#    define S_LINE_SUFFIX , __LINE__
#else
#    define S_LINE_PREFIX
#    define S_LINE_SUFFIX
#endif


/**
 * \brief   Print a log message if its severity is lower or
 *          equal to DEBUG_LOG_MAX_LEVEL.
 * \param   level
 *          Severity of the log.
 * \param   fmt
 *          Format of the log
 * \param   ...
 *          The list of parameters
 */
#define LOG(level, fmt, ...) \
{ \
    ((uint8_t) level) <= ((uint8_t) DEBUG_LOG_MAX_LEVEL) ? \
    Print_Log( \
        S_MOD_NAME_PREFIX S_TIME_PREFIX S_LEVEL_PREFIX(level) \
        S_FUNCTION_PREFIX S_LINE_PREFIX \
        fmt"\n" S_TIME_SUFFIX S_FUNCTION_SUFFIX S_LINE_SUFFIX \
        , ##__VA_ARGS__) : \
    (void)NULL; \
}

/**
 * \brief   Print a buffer if its severity is lower or
 *          equal to DEBUG_LOG_MAX_LEVEL.
 * \param   level
 *          Severity of the log.
 * \param   buffer
 *          Pointer to the buffer to print.
 * \param   size
 *          Size in bytes of the buffer.
 */
#define LOG_BUFFER(level, buffer, size) \
{ \
    if(((uint8_t) level) <= ((uint8_t) DEBUG_LOG_MAX_LEVEL)) { \
        for (uint8_t i = 0; i < size; i++) \
        { \
            Print_Log("%02X ", buffer[i]); \
            if ((i & 0xF) == 0xF && i != (uint8_t)(size-1)) \
            { \
                Print_Log("\n"); \
            } \
        } \
    Print_Log("\n"); \
    } \
}

/**
 * \brief   Actively wait that Uart buffer as been sent.
 * \param   level
 *          Only wait if level is lower or equal to DEBUG_LOG_MAX_LEVEL.
 */
#define LOG_FLUSH(level) \
{ \
    if(((uint8_t) level) <= ((uint8_t) DEBUG_LOG_MAX_LEVEL)) { \
        app_lib_time_timestamp_hp_t end; \
        end = lib_time->addUsToHpTimestamp(lib_time->getTimestampHp(), \
                                           FLUSH_DELAY_MS * 1000); \
        while (lib_time->isHpTimestampBefore(lib_time->getTimestampHp(),end)); \
    } \
}
