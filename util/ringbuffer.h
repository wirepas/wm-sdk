/* Copyright 2017 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

/*----------------------------------------------------------------------------
 Notes:
 The length of the receive and transmit buffers must be a power of 2.
 Each buffer has a next_in and a next_out index.
 If next_in = next_out, the buffer is empty.
 (next_in - next_out) % buffer_size = the number of characters in the buffer.
 *----------------------------------------------------------------------------*/

#if !defined BUFFER_SIZE
#error Please define BUFFER_SIZE before including this header!
#endif

#if BUFFER_SIZE < 2
#error BUFFER_SIZE is too small.  It must be larger than 1.
#elif ((BUFFER_SIZE & (BUFFER_SIZE-1)) != 0)
#error BUFFER_SIZE must be a power of 2.
#endif

#define MASK    (BUFFER_SIZE-1)

typedef struct
{
    uint16_t    in;                     // Next In Index
    uint16_t    out;                    // Next Out Index
    uint8_t     buf[BUFFER_SIZE];       // Buffer
} ringbuffer_t;

/** Total capacity of the buffer in bytes */
#define Ringbuffer_size(p)          (BUFFER_SIZE)
/** Number of used bytes */
#define Ringbuffer_usage(p)         ((uint16_t)((p).in - (p).out))
/** Number of free bytes */
#define Ringbuffer_free(p)          (Ringbuffer_size(p)-Ringbuffer_usage(p))

/** Access the contents of the buffer */
#define Ringbuffer_contents(p)      ((p).buf)

/** Reset the buffer */
#define Ringbuffer_reset(p)         do{ (p).out = 0; (p).in = 0; }while(false)
/** Check if buffer is reset */
#define Ringbuffer_isReset(p)       ((p).out == 0 && (p).in == 0)

/** Array index of the head */
#define Ringbuffer_getHeadIdx(p)    ((p).in & MASK)
/** Byte at the current head position */
#define Ringbuffer_getHeadByte(p)   ((p).buf[Ringbuffer_getHeadIdx(p)])
/** Pointer to the current head position */
#define Ringbuffer_getHeadPtr(p)    (uint8_t *)(&Ringbuffer_getHeadByte(p))
/** Move head forward by 'a' bytes */
#define Ringbuffer_incrHead(p, a)   ((p).in += (a))

/** Array index of the tail */
#define Ringbuffer_getTailIdx(p)    ((p).out & MASK)
/** Byte at the current tail position */
#define Ringbuffer_getTailByte(p)   ((p).buf[Ringbuffer_getTailIdx(p)])
/** Pointer to the current tail position */
#define Ringbuffer_getTailPtr(p)    (uint8_t *)(&Ringbuffer_getTailByte(p))
/** Move tail forward by 'a' bytes */
#define Ringbuffer_incrTail(p, a)   ((p).out += (a))

/**
 * \file ringbuffer.h
 * Example:
 \code
    #define BUFFER_SIZE 128
    #include "ringbuffer.h"

    static ringbuffer_t RxBuffer = { 0, 0, };  //tbuf.in = 0 tbuf.out = 0

    static void StdioRxHandler(uint8_t ch);

    int16_t StdioReceive(void)
    {
        int16_t ret;
        EnterCriticalSection();
        if (Ringbuffer_elements(RxBuffer) == 0)
        {
            ret = (-1);
            // Buffer empty -> get out
        }
        else
        {
            ret = Ringbuffer_getTailByte(RxBuffer);
            Ringbuffer_incrementTail(RxBuffer, 1);
        }
        ExitCriticalSection();
        return ret;
    }

    static void StdioRxHandler(uint8_t ch)
    {
        if (Ringbuffer_elements(RxBuffer) == 0)
        {
            Ringbuffer_getHeadByte(RxBuffer) = (uint8_t) (ch & 0xFF);
            Ringbuffer_incrementHead(RxBuffer,1);
        }
    }
 \endcode
*/
#endif /* RINGBUFFER_H_ */
