/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef DOUBLEBUFFER_H_
#define DOUBLEBUFFER_H_

/*
 * Simple helper to manage a double buffering
 */
#if !defined BUFFER_SIZE
#error Please define BUFFER_SIZE before including doublebuffer.h!
#endif

/* Generic structure to manage double buffering with a current active one */
typedef struct
{
    uint8_t * active_buffer_p;       //< Address of active buffer
    uint8_t current_writing_index;   //< Current writing index in active buffer
    uint8_t buffer_1[BUFFER_SIZE];   //< First buffer
    uint8_t buffer_2[BUFFER_SIZE];   //< Second buffer
} double_buffer_t;

/** Initialize double buffer struct */
#define DoubleBuffer_init(buffers) do {                 \
        (buffers).active_buffer_p = (buffers).buffer_1; \
        (buffers).current_writing_index = 0;           \
    } while(0)

/** Get active buffer */
#define DoubleBuffer_getActive(buffers) ((buffers).active_buffer_p)

/** Swipe buffers and reset writing index */
#define DoubleBuffer_swipe(buffers)  do {                    \
        if ((buffers).active_buffer_p == (buffers).buffer_1) \
        {                                                    \
            (buffers).active_buffer_p = (buffers).buffer_2;  \
        }                                                    \
        else                                                 \
        {                                                    \
            (buffers).active_buffer_p = (buffers).buffer_1;  \
        }                                                    \
        (buffers).current_writing_index = 0;                \
    } while(0)

/** Get current writing index */
#define DoubleBuffer_getIndex(buffers)  ((buffers).current_writing_index)

/** Increment writing index */
#define DoubleBuffer_incrIndex(buffers, inc) ((buffers).current_writing_index += inc)


#endif /* DOUBLEBUFFER_H_ */
