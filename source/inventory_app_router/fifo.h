/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */


#include <stdlib.h>
#include <stdint.h>


/* Utility functions for bulding a generic FIFO*/

typedef struct{
    uint8_t * buf; // pointer to FIFO buffer
    uint16_t head; // head index
    uint16_t tail; // tail index
    uint16_t size; // size of one storage element
    uint16_t max; // maximum number of elements
    bool full; // indicates if FIFO is full
} fifo_t;

/**
 *  @brief Initialize FIFO structure
 *  @param[in] fifo
 *              pointer to FIFO structure
 *  @param[in] buf
 *              pointer to storage buffer
 *  @param[in] size
 *             size of one storage element
*  @param[in] max
 *              maximum number of elements allowed in the buffer
 *  @param[out]  bool
 *              true/flase -> success/failure
 *
**/
bool fifo_init(fifo_t * fifo, void * buf, uint16_t size, uint16_t max);

/**
 *  @brief Push one data element into FIFO
 *  @param[in] fifo
 *              pointer to FIFO structure
 *  @param[in] data
 *              pointer to one storage element
 *  @param[out]  bool  true/flase -> success/failure
 *
**/
uint16_t fifo_push(fifo_t * f, void * data);

/**
 *  @brief Pop one data element into FIFO
 *  @param[in] fifo
 *              pointer to FIFO structure
 *  @param[in] data
 *              pointer to one storage element
 *  @param[out] uint16
 *              number of bytes written
**/
uint16_t fifo_pop(fifo_t * f, void * data);

/**
 *  @brief Gets the number of data elements stored info FIFO
 *  @param[in] fifo
 *              pointer to FIFO structure
 *  @param[out] uint16
 *              number of data elements
**/
uint16_t fifo_size(fifo_t * f);

/**
 *  @brief Provides the pointer to FIFO tail;  the data element is not removed *        from FIFO
 *  @param[in] fifo
 *              pointer to FIFO structure
 *  @param[out] void *
 *              pointer for FIFO tail
**/
void * fifo_get_tail(fifo_t * f);
