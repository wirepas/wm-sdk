/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#include <string.h> // For memcpy(), memcmp()

#include "aessw.h"
#include "aes.h"

#if !defined( min )
#define min(a, b)               ((a) < (b) ? (a) : (b))
#endif

/** OMAC1 constant for subkey derivation, see the OMAC1 / CMAC specification */
#define AES_OMAC1_CONSTANT 0x87 // Binary polynomial 10000111

/**
 * \brief Pure null plaintext
 * Used for OMAC1 subkey generation
 */
const uint8_t aes_plaintext_null[AES_128_KEY_BLOCK_SIZE] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void aes_setupStream(aes_data_stream_t * stream_ptr,
                     const uint8_t * key128_ptr,
                     const uint8_t * iv_ctr_ptr)
{
    // store key and iv_ctr to 32-bit aligned words
    memcpy(&stream_ptr->key[0], key128_ptr, AES_128_KEY_BLOCK_SIZE);
    memcpy(&stream_ptr->iv_ctr[0], iv_ctr_ptr, AES_128_KEY_BLOCK_SIZE);
}

/**
 * \brief   Generate OMAC1 internal subkeys
 *          Converts L to HL1, HL1 to HL2
 */
static void aes_omac1GenerateSubkey(uint8_t * tgt_ptr, const uint8_t * src_ptr)
{
    // Copy and left shift all 128 bits
    // Done backwards just like in OMAC1 / CMAC specification
    uint_fast8_t carry = 0x00u;
    for (int_fast8_t i = 15 ; i >= 0 ; i--)
    {
        tgt_ptr[i] = (src_ptr[i] << 1) | carry;
        if (src_ptr[i] & 0x80u)
        {
            carry = 0x01u;
        }
        else
        {
            carry = 0x00u;
        }
    }
    // Handle final carry
    if (carry)
    {
        tgt_ptr[15] ^= AES_OMAC1_CONSTANT; // See the OMAC1 / CMAC specification
    }
}

void aes_initOmac1(aes_omac1_state_t * state_ptr, const uint8_t * mic_key_ptr)
{
    uint8_t dummy_out[AES_128_KEY_BLOCK_SIZE]; // This data is discarded

    aes_setupStream(&state_ptr->data,
                    mic_key_ptr,
                    aes_plaintext_null);
    aes_crypto128Ctr(&state_ptr->data,
                     aes_plaintext_null,
                     dummy_out,
                     AES_128_KEY_BLOCK_SIZE);
    // state_ptr->data.aes_out is now the OMAC1 "L" constant
    // From "L", generate required OMAC1 subkeys HL1 and HL2
    aes_omac1GenerateSubkey(state_ptr->hl1.bytes,
                            (uint8_t *)state_ptr->data.aes_out); // "L"
    aes_omac1GenerateSubkey(state_ptr->hl2.bytes, state_ptr->hl1.bytes);
}

void aes_omac1(aes_omac1_state_t * state,
               uint8_t * mic_out_ptr,
               uint_fast8_t mic_out_bytes,
               const uint8_t * intext_ptr,
               size_t intext_bytes)
{
    // For details, see "NIST Special Publication 800-38B" or "RFC 4493"
    uint32_t * final_xor_mask;
    uint32_t i;
    uint8_t * cbc_stream;
    struct AES_ctx ctx;

    AES_init_ctx(&ctx, (const uint8_t * )state->data.key);

    state->data.aes_out[0] = 0;
    state->data.aes_out[1] = 0;
    state->data.aes_out[2] = 0;
    state->data.aes_out[3] = 0;

    // Repeat AES crunching in CBC mode for 16 byte data blocks.
    // Final block requires padding and subkey xoring.
    while (intext_bytes)
    {
        uint_fast8_t bytes = min(16, intext_bytes);
        intext_bytes -= bytes;
        cbc_stream = (uint8_t *)&state->data.aes_in[0];
        for (i = 0 ; i < bytes ; i++)
        {
            *cbc_stream++ = *intext_ptr++;
        }

        // final block?
        if (intext_bytes == 0)
        {
            // This is final block.
            uint_fast8_t padding_bytes = 16 - bytes;
            if (padding_bytes)
            {
                // Final block and we need padding for CBC:
                // First padding byte is always 0x80, rest are 0x00:
                *cbc_stream++ = 0x80;
                while (--padding_bytes)
                {
                    *cbc_stream++ = 0x00;
                }
                final_xor_mask = &state->hl2.words[0];
            }
            else
            {
                // final block but no padding needed:
                final_xor_mask = &state->hl1.words[0];
            }
            // Execute final block xor:
            for (i = 0 ; i <= 3 ; i++)
            {
                state->data.aes_in[i] ^= final_xor_mask[i];
            }
        } // end final block

        // CBC feedforward xor and AES input data loading:
        for (i = 0 ; i <= 3 ; i++)
        {
            state->data.aes_in[i] ^= state->data.aes_out[i];
        }

        memcpy(state->data.aes_out, state->data.aes_in, AES_BLOCKLEN);
        AES_ECB_encrypt(&ctx, (uint8_t *)state->data.aes_out);
    } // while (bytecount)

    // write out OMAC1 MAC:
    memcpy(mic_out_ptr, state->data.aes_out, mic_out_bytes);
}

void aes_crypto128Ctr(aes_data_stream_t * stream_ptr,
                      const uint8_t * intext_ptr,
                      uint8_t * outtext_ptr,
                      size_t bytecount)
{
    struct AES_ctx ctx;

    AES_init_ctx(&ctx, (const uint8_t * )stream_ptr->key);

    // Repeat ECB crunching for 16 byte data blocks.
    // No padding required even if the final block is not full.
    while (bytecount)
    {
        memcpy(&stream_ptr->aes_out[0], stream_ptr->iv_ctr, AES_BLOCKLEN);
        AES_ECB_encrypt(&ctx, (uint8_t *)&stream_ptr->aes_out[0]);

        // Update 128-bit iv_ctr by basic increment:
        if (++(stream_ptr->iv_ctr[0]) == 0)
        {
            if (++(stream_ptr->iv_ctr[1]) == 0)
            {
                if (++(stream_ptr->iv_ctr[2]) == 0)
                {
                    ++(stream_ptr->iv_ctr[3]);
                }
            }
        }

        // Implement final XORing for CTR mode.
        // One AES run can handle 1 to 16 bytes.
        uint8_t * ecb_stream_ptr = (uint8_t *)&stream_ptr->aes_out[0];
        uint_fast8_t bytes = min(bytecount, 16);
        bytecount -= bytes;
        while (bytes--)
        {
            *outtext_ptr++ = *intext_ptr++ ^ *ecb_stream_ptr++;
        }
        // Repeat AES if unhandled bytes
    } // while (bytecount)
}
