/* Copyright 2019 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/**
 * \file    nrfx.h
 * \brief   This file is a hook to avoid including too much Nordic headers files
 *          It contains some definitions that prevent the buid if not defined
 */

// Needed form nrf_gpio.h
#define NRFX_ASSERT(x) (void)(x)

extern __IO uint32_t EVENT_READBACK;
