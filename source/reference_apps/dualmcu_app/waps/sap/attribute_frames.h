/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef ATTRIBUTE_FRAMES_H_
#define ATTRIBUTE_FRAMES_H_

/**
 * \file    attribute_frames.h
 *          Both CSAP and MSAP contain attributes for reading/writing.
 *          This header provides common frames for this purpose.
 */

/* How much space is reserved for attributes in attribute related primitive */
#define WAPS_MAX_ATTR_LEN   16

/** Read attribute request */
typedef struct __attribute__ ((__packed__))
{
    attr_t      attr_id;
} read_req_t;

/** Result of writing/reading a parameter */
typedef enum
{
    ATTR_SUCCESS = 0,
    ATTR_UNSUPPORTED_ATTRIBUTE = 1,
    ATTR_INVALID_STACK_STATE = 2,
    ATTR_INV_LENGTH = 3,
    ATTR_INV_VALUE = 4,
    ATTR_WRITE_ONLY = 5,
    ATTR_ACCESS_DENIED = 6,
} attribute_result_e;

/** Read attribute confirmation */
typedef struct __attribute__ ((__packed__))
{
    uint8_t result;
    attr_t  attr_id;
    uint8_t attr_len;
    uint8_t attr[WAPS_MAX_ATTR_LEN];
} read_cnf_t;

#define FRAME_READ_CNF_HEADER_SIZE      \
    (sizeof(read_cnf_t)-WAPS_MAX_ATTR_LEN)

/** Write attribute request */
typedef struct __attribute__ ((__packed__))
{
    attr_t  attr_id;
    uint8_t attr_len;
    uint8_t attr[WAPS_MAX_ATTR_LEN];
} write_req_t;

#define FRAME_WRITE_REQ_HEADER_SIZE     \
    (sizeof(write_req_t)-WAPS_MAX_ATTR_LEN)

/** Write attribute confirmation */
typedef struct __attribute__ ((__packed__))
{
    uint8_t result;
} write_cnf_t;

//assert_static(sizeof(read_cnf_t)  == 20);
//assert_static(sizeof(read_req_t)  == 2);
//assert_static(sizeof(write_req_t) == 19);
//assert_static(sizeof(write_cnf_t) == 1);

typedef union
{
    read_cnf_t  read_cnf;
    read_req_t  read_req;
    write_req_t write_req;
    write_cnf_t write_cnf;
} frame_attr;

#endif /* ATTRIBUTE_FRAMES_H_ */
