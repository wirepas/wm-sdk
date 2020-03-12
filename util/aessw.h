/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef AESSW_H_
#define AESSW_H_

#include <stdint.h>

/** \brief AES 128 block size in bytes. */
#define AES_128_KEY_BLOCK_SIZE 16

/**
 * \brief For storing 128bit keys, IVs, MACs etc. Allows both byte and faster
 * 32-bit word-aligned access.
 */
typedef union
{
    uint32_t words[4];
    uint8_t bytes[16];
} aes_128_t;

/**
 * \brief AES-128 key, input data and output data
 *
 * Every independent stream cipher flow should have one of these. This
 * structure layout was created specifically for the Nordic nRF51822 AES ECB
 * peripheral. However, this software implementations also use the same
 * structure, for symmetry.
 */
typedef struct
{
    uint32_t key[4];         // Secret AES-128 key
    union
    {
        uint32_t iv_ctr[4];  // Nonce and counter combined
        uint32_t aes_in[4];  // Alias for AES input (if not iv_ctr)
    };
    uint32_t aes_out[4];     // AES-128 ECB output of last block
} aes_data_stream_t;

/**
 * \brief AES-128 OMAC1 state
 *
 * This state contains the HL1 and HL2 subkeys, so that they don't need to
 * be recalculated when switching between authentication keys.
 */
typedef struct
{
    aes_data_stream_t data; // OMAC1 AES-128 state
    aes_128_t hl1;          // Derived subkey HL1 (for non-padded last block)
    aes_128_t hl2;          // Derived subkey HL2 (for padded last block)
} aes_omac1_state_t;

/**
 * \brief   Setup generic AES128 CTR mode stream
 * \param   stream_ptr
 *          Pointer to aes_data_stream_t to be set up
 * \param   key128_ptr
 *          Pointer to key, copied to the stream struct
 * \param   iv_ctr_ptr
 *          Pointer to iv_ctr, copied to the stream struct
 */
void aes_setupStream(aes_data_stream_t * stream_ptr,
                     const uint8_t * key128_ptr,
                     const uint8_t * iv_ctr_ptr);

/**
 * \brief   Initializes an OMAC1 state
 * \param   state_ptr
 *          Pointer to aes_omac1_state_t to be set up
 * \param   mic_key_ptr
 *          Pointer to the secret 16-byte MIC key, which is no longer needed
 *          after this function returns
 */
void aes_initOmac1(aes_omac1_state_t * state_ptr, const uint8_t * mic_key_ptr);

/**
 * \brief   Calculate and write out OMAC1 (CMAC) MIC for input text
 * \note    This is a software implementation based on tiny AES.
 *          NIST recommends using at least 8 byte MICs.
 *          See "NIST Special Publication 800-38B" appendix A and B.
 * \param   state         Pointer to OMAC1 AES-128 state
 * \param   mic_out_ptr   Pointer for writing MIC
 * \param   mic_out_bytes Amount of MIC bytes wanted (1...16)
 * \param   intext_ptr    Pointer to input text
 * \param   intext_bytes  Length of input text
 */
void aes_omac1(aes_omac1_state_t * state,
               uint8_t * mic_out_ptr,
               uint_fast8_t mic_out_bytes,
               const uint8_t * intext_ptr,
               size_t intext_bytes);

/**
 * \brief   Run the AES128 cryptography in CTR mode
 * \note    This is a software implementation based on tiny AES.
 *          The same algorithm handles both encryption and decryption.
 *          The receiver must know the initial iv_ctr for decryption.
 *          The stream.iv_ctr is autoincremented after every AES block exec.
 *          Supports partial operation: data can be split to consecutive parts,
 *          provided that the splits are done at 16-byte boundaries.
 * \param   stream_ptr    Pointer to aes_data_stream_t to be used
 * \param   intext_ptr    Pointer to input text
 * \param   outtext_ptr   Pointer to output text. Supports intext overwrite
 * \param   bytecount     Number of bytes to handle
 */
void aes_crypto128Ctr(aes_data_stream_t * stream_ptr,
                      const uint8_t * intext_ptr,
                      uint8_t * outtext_ptr,
                      size_t bytecount);

#endif /* AESSW_H_ */
