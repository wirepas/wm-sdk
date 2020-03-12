/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    app.c
 * \brief   This file is a app to test the software AES library.
 */

#include <stdlib.h>

#include "api.h"
#include "node_configuration.h"

#define DEBUG_LOG_MODULE_NAME "AES TEST"
#define DEBUG_LOG_MAX_LEVEL LVL_INFO
#include "debug_log.h"

#include "aessw.h"
#include <string.h>

// Test data length is 2 full blocks + 6 bytes = 40bytes
static const uint8_t m_test_datain[40] =
{
    0x6B, 0xC1, 0xBE, 0xE2, 0x2E, 0x40, 0x9F, 0x96,
    0xE9, 0x3D, 0x7E, 0x11, 0x73, 0x93, 0x17, 0x2A,
    0xAE, 0x2D, 0x8A, 0x57, 0x1E, 0x03, 0xAC, 0x9C,
    0x9E, 0xB7, 0x6F, 0xAC, 0x45, 0xAF, 0x8E, 0x51,
    0x30, 0xC8, 0x1C, 0x46, 0xA3, 0x5C, 0xE4, 0x11
};

// Ok result for 36 byte encryption test
static const uint8_t m_test_dataout_ok[36] =
{
    0x87, 0x4D, 0x61, 0x91, 0xB6, 0x20, 0xE3, 0x26,
    0x1B, 0xEF, 0x68, 0x64, 0x99, 0x0D, 0xB6, 0xCE,
    0x40, 0x94, 0x25, 0x91, 0xD7, 0xB4, 0x4F, 0x49,
    0xAB, 0xC1, 0x9D, 0x33, 0xA4, 0x4E, 0xF6, 0x54,
    0xCE, 0x58, 0xD2, 0xF0
};

// Key must be 16 bytes, from lsb to msb
static const uint8_t m_test_key[AES_128_KEY_BLOCK_SIZE] =
{
    0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
    0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
};

// IV (nonce and counter) must be 16 bytes, from lsb to msb
static const uint8_t m_test_iv[AES_128_KEY_BLOCK_SIZE] =
{
    0xf0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
    0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

// OMAC1 tests use the same test input and key as cryptology tests.
// Correct OMAC1 HL1 subkey:
static const uint8_t m_test_omac1_HL1_ok[AES_128_KEY_BLOCK_SIZE] =
{
    0xfb, 0xee, 0xd6, 0x18, 0x35, 0x71, 0x33, 0x66,
    0x7c, 0x85, 0xe0, 0x8f, 0x72, 0x36, 0xa8, 0xde
};
// Correct OMAC1 HL2 subkey:
static const uint8_t m_test_omac1_HL2_ok[AES_128_KEY_BLOCK_SIZE] =
{
    0xf7, 0xdd, 0xac, 0x30, 0x6a, 0xe2, 0x66, 0xcc,
    0xf9, 0x0b, 0xc1, 0x1e, 0xe4, 0x6d, 0x51, 0x3b
};
// Correct OMAC1 MAC for 16 bytes:
static const uint8_t m_test_omac1_Mac16_ok[AES_128_KEY_BLOCK_SIZE] =
{
    0x07, 0x0a, 0x16, 0xb4, 0x6b, 0x4d, 0x41, 0x44,
    0xf7, 0x9b, 0xdd, 0x9d, 0xd0, 0x4a, 0x28, 0x7c
};
// Correct OMAC1 MAC for 40 bytes:
static const uint8_t m_test_omac1_Mac40_ok[AES_128_KEY_BLOCK_SIZE] =
{
    0xdf, 0xa6, 0x67, 0x47, 0xde, 0x9a, 0xe6, 0x30,
    0x30, 0xca, 0x32, 0x61, 0x14, 0x97, 0xc8, 0x27
};

// Use non-const data length, can change it with debugger...
static uint32_t m_test_datalen = 36; // for encryption test

// For real output:
static uint8_t m_test_output[64]; // support full MIC tests

void aes_ctr_test1(void)
{
    aes_data_stream_t data_stream;

    // ENCRYPT:
    LOG(LVL_INFO, "AES CTR mode - Encrypt %d bytes.", m_test_datalen);
    aes_setupStream(&data_stream,
                    &m_test_key[0],
                    &m_test_iv[0]);
    aes_crypto128Ctr(&data_stream,
                     &m_test_datain[0],
                     &m_test_output[0],
                     m_test_datalen);
    // Compare result to right result:
    if (memcmp(&m_test_output[0], &m_test_dataout_ok[0], m_test_datalen))
    {
        LOG(LVL_ERROR, "AES CTR mode - Error encrypt.");
        LOG_BUFFER(LVL_ERROR, m_test_datain, m_test_datalen);
        LOG_BUFFER(LVL_ERROR, m_test_output, m_test_datalen);
        LOG_BUFFER(LVL_ERROR, m_test_dataout_ok, m_test_datalen);
        return;
    }

    // DECRYPT:
    LOG(LVL_INFO, "AES CTR mode - Decrypt %d bytes.", m_test_datalen);
    aes_setupStream(&data_stream,
                    &m_test_key[0],
                    &m_test_iv[0]);
    aes_crypto128Ctr(&data_stream,
                     &m_test_output[0],
                     &m_test_output[0], // overwrite tested
                     m_test_datalen);
    // Compare result to original data:
    if (memcmp(&m_test_output[0], &m_test_datain[0], m_test_datalen))
    {
        LOG(LVL_ERROR, "AES CTR mode - Error decrypt.");
        return;
    }

    // ENCRYPT in 2 parts
    // Note: Must be divided at 16-byte data boundary.
    LOG(LVL_INFO, "AES CTR mode - Encrypt %d bytes (in 2 calls).",
                  m_test_datalen);
    aes_setupStream(&data_stream,
                    &m_test_key[0],
                    &m_test_iv[0]);
    aes_crypto128Ctr(&data_stream,
                     &m_test_datain[0],
                     &m_test_output[0],
                     AES_128_KEY_BLOCK_SIZE); // first 16B ...
    aes_crypto128Ctr(&data_stream,
                     &m_test_datain[AES_128_KEY_BLOCK_SIZE],
                     &m_test_output[AES_128_KEY_BLOCK_SIZE],
                     m_test_datalen - AES_128_KEY_BLOCK_SIZE); // ... and the
                                                               // rest (20B)
    // Compare result to right result:
    if (memcmp(&m_test_output[0], &m_test_dataout_ok[0], m_test_datalen))
    {
        LOG(LVL_ERROR, "AES CTR mode - Error encrypt.");
        return;
    }
    LOG(LVL_INFO, "AES CTR mode - tests passed.");
}

void aes_omac1_test1(void)
{
    aes_omac1_state_t omac1_state;
    uint_fast8_t mic_size = AES_128_KEY_BLOCK_SIZE; // Test full 16-byte MIC

    // Check OMAC1 subkey generation:
    LOG(LVL_INFO, "AES OMAC1 - check subkey generation.");
    aes_initOmac1(&omac1_state, (uint8_t *)&m_test_key[0]);
    if (memcmp(&omac1_state.hl1.bytes[0],
               &m_test_omac1_HL1_ok[0],
               AES_128_KEY_BLOCK_SIZE))
    {
        LOG(LVL_ERROR, "AES OMAC1 - error subkey HL1.");
        return;
    }
    if (memcmp(&omac1_state.hl2.bytes[0],
               &m_test_omac1_HL2_ok[0],
               AES_128_KEY_BLOCK_SIZE))
    {
        LOG(LVL_ERROR, "AES OMAC1 - error subkey HL2.");
        return;
    }

    // Check 1 full block (no internal padding):
    LOG(LVL_INFO, "AES OMAC1 - check 1 full block (no padding).");
    aes_omac1(&omac1_state,
              &m_test_output[0],
              mic_size,
              &m_test_datain[0],
              AES_128_KEY_BLOCK_SIZE);
    if (memcmp(&m_test_output[0], &m_test_omac1_Mac16_ok[0], mic_size))
    {
        LOG(LVL_ERROR, "AES OMAC1 - error check 1 full block.");
        return;
    }

    // Check 2 blocks + 8 bytes (internal padding):
    LOG(LVL_INFO, "AES OMAC1 - check 3 partial blocks (internal padding).");
    aes_omac1(&omac1_state,
              &m_test_output[0],
              mic_size,
              &m_test_datain[0],
              40);
    if (memcmp(&m_test_output[0], &m_test_omac1_Mac40_ok[0], mic_size))
    {
        LOG(LVL_ERROR, "AES OMAC1 - error check 2 blocks.");
        return;
    }

    LOG(LVL_INFO, "AES OMAC1 - tests passed.");
 }

/**
 * \brief   Initialization callback for application
 *
 * This function is called after hardware has been initialized but the
 * stack is not yet running.
 *
 */
void App_init(const app_global_functions_t * functions)
{
    LOG_INIT();
    LOG(LVL_INFO, "Starting");

    aes_ctr_test1();
    aes_omac1_test1();

    //No need to start the stack. We only test software AES.
}
