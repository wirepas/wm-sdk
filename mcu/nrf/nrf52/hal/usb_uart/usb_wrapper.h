/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    usb_wrapper.h
 * \brief   This file is the Wirepas wrapper for tinyusb stack
 */

#include <stdlib.h>

/**
 * \brief   Callback to be called when bytes are received
 */
typedef void (*Usb_wrapper_rx_callback_f)(uint8_t * ch, size_t n);

/**
 * \brief   Send a buffer
 * \param   buffer
 *          Buffer to send
 * \param   length
 *          Size of the buffer
 */
uint32_t Usb_wrapper_sendBuffer(const void * buffer, uint32_t length);

/**
 * \brief   Initialize the tiny usb wrapper
 * \param   cb
 *          Callback to be called when bytes are received
 * \return  True if successful, false otherwise
 */
bool Usb_wrapper_init(Usb_wrapper_rx_callback_f cb);

