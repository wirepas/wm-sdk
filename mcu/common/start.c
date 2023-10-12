/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file start.c
 *
 * Application tag, information header and entrypoint
 */

#include "api.h"
#include "board_init.h"
#include "libraries_init.h"
#include "hal_init.h"

/** Addresses determined by the linker */
extern unsigned int __text_start__;
extern unsigned int __data_src_start__;
extern unsigned int __data_start__;
extern unsigned int __data_end__;
extern unsigned int __bss_start__;
extern unsigned int __bss_end__;
extern unsigned int __ram_start__;
extern unsigned int __total_size_bytes__;

// The real used app ram end pointer.
// NOTE! The value is NOT sometimes equal to &__bss_end__
// e.g. when dynamic waps items are reserved in dualmcu_app.
uint32_t * m_used_app_ram_end = (uint32_t *)&__bss_end__;

/** Application initialization function */
void App_init(const app_global_functions_t * functions);

/** Application header with tag and information */
typedef struct
{
    uint8_t tag[APP_V2_TAG_LENGTH];
    app_information_header_t info;
} app_header_t;

/**
 * \brief   Application header
 */
const app_header_t app_header __attribute__ ((section (".app_header"))) =
{
    .tag = APP_V2_TAG,
    .info = {
            .api_version = APP_API_VERSION,
            .start_address = (uint32_t)&__text_start__,
            .length = (uint32_t)(&__total_size_bytes__),
            .start_ram_address = (uint32_t)&__ram_start__
    }
};

/** Keys declared from config.mk files must be stored in flash somewhere
 * to be used by the application */
#ifdef NET_CIPHER_KEY
static const uint8_t cipher_key[] = {NET_CIPHER_KEY};
_Static_assert((sizeof(cipher_key) == 16), "Cipher key must be 16 bytes");
const uint8_t * cipher_key_p = cipher_key;
#else
const uint8_t * cipher_key_p = NULL;
#endif
#ifdef NET_AUTHEN_KEY
static const uint8_t authen_key[] = {NET_AUTHEN_KEY};
_Static_assert((sizeof(authen_key) == 16), "Authentication key must be 16 bytes");
const uint8_t * authen_key_p = authen_key;
#else
const uint8_t * authen_key_p = NULL;
#endif


/**
 * \brief   Application entrypoint, called from entrypoint.s
 */
intptr_t _start(const app_global_functions_t * functions,
                size_t reserved1,
                const void ** reserved2,
                void ** ram_top)
{
    /* Unused parameters */
    (void)reserved1;
    (void)reserved2;

    if (App_getApiVersion(functions) < APP_V2_TAG_MIN_API_VERSION)
    {
        // Stack firmware only supports v1 application API
        return 0;
    }

    unsigned int * src, * dst;

    /* Copy data from flash to RAM */
    for(src = &__data_src_start__,
        dst = &__data_start__;
        dst != &__data_end__;)
    {
        *dst++ = *src++;
    }

    /* Initialize the .bss section */
    for(dst = &__bss_start__; dst != &__bss_end__;)
    {
        *dst++ = 0;
    }

    /* Open Wirepas public API (it loads all libs pointer to global variables) */
    API_Open(functions);


    /* Initialize HAL drivers */
    Hal_init();

    /* Call any board specific initialization */
    Board_init();

    /* Initialize libraries in use */
    Libraries_init();

    /* Call application initialization function */
    App_init(functions);

    /*
     * Set the last RAM address used by the application, in order to give
     * back any unused RAM to the stack firmware. If heap is implemented
     * in the SDK, this needs to be updated.
     */
    *ram_top = (void *) m_used_app_ram_end;

    /* Nothing to return */
    return 0;
}
