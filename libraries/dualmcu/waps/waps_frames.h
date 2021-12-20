/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef WAPS_FRAMES_H__
#define WAPS_FRAMES_H__

#include <stdint.h>
#include <string.h>

/** Precision of "end point" fields */
typedef uint8_t ep_t;

/** Precision of "node id" fields */
typedef uint32_t w_addr_t;

/** Precision of "attribute id" fields */
typedef uint16_t attr_t;

/** Precision of "PDU id" fields */
typedef uint16_t pduid_t;

#include "sap/attribute_frames.h"
#include "sap/dsap_frames.h"
#include "sap/msap_frames.h"
#include "sap/csap_frames.h"

/** Length of frame header */
#define WAPS_MIN_FRAME_LENGTH   (uint16_t)(sizeof(frame_header_t))

/** Length of frame payload */
#define WAPS_MAX_FRAME_PAYLOAD  (uint16_t)(sizeof(waps_frame_payload_t))

/** Length of frame headers and payload */
#define WAPS_MAX_FRAME_LENGTH   (uint16_t)(sizeof(waps_frame_t))

/** Many methods have a simple one byte confirmation */
typedef struct __attribute__ ((__packed__))
{
    uint8_t     result;
} simple_cnf_t;

typedef struct __attribute__ ((__packed__))
{
    // Service/function code,
    uint8_t sfunc;
    // Frame identifier
    uint8_t sfid;
    // Payload length in bytes
    uint8_t splen;
} frame_header_t;

typedef union
{
    frame_dsap      dsap;
    frame_msap      msap;
    frame_csap      csap;
    frame_attr      attr;
    simple_cnf_t    simple_cnf;
} waps_frame_payload_t;

typedef struct __attribute__ ((__packed__))
{
    // Service/function code,
    uint8_t sfunc;
    // Frame identifier
    uint8_t sfid;
    // Payload length in bytes
    uint8_t splen;
    // Upper level payload, length range is 0-36
    union
    {
        frame_dsap      dsap;
        frame_msap      msap;
        frame_csap      csap;
        frame_attr      attr;
        simple_cnf_t    simple_cnf;
        uint8_t         spld[WAPS_MAX_FRAME_PAYLOAD];
    };
} waps_frame_t;

#endif
