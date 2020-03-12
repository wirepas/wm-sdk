/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    hw_delay.h
 * \brief   hardware delay module for nrf52
 */

#ifndef HAL_HW_DELAY_H_
#define HAL_HW_DELAY_H_

/** Return codes of hardware delay functions */
typedef enum {
    HW_DELAY_OK,
    HW_DELAY_PARAM_ERR,
    HW_DELAY_NOT_STARTED,
    HW_DELAY_NOT_TRIGGERED,
    HW_DELAY_ERR
} hw_delay_res_e;


/**
 * \brief   Callback to be registered, and called after timer expire.
 * \return  Delay before being executed again in us
 * \note    If it returns 0 or a value greater than 511s,
 *          it will not be executed again.
 *          Granularity  is ~30us, and it cannot be called again before 90us
 *          Any value less than 90us will be round-up to 90us
 */
typedef uint32_t (* hw_delay_callback_f)(void);

/**
* \brief   Initialize Hardware Delay Module
* \return  HW_DELAY_OK if successfully initialized
*          HW_DELAY_ERR if already initialized
*/
hw_delay_res_e hw_delay_init(void);


/**
* \brief   Setup timer trigger
* \param   callback
*          Callback to be called when delay will expire
* \param   time_us
*          Callback set by hw_delay_register will be called in time_us us
*          Granularity  is ~30us, and it cannot be called again before 90us
*          Any value less than 90us will be round-up to 90us
* \return  HW_DELAY_OK is timer is set
*          HW_DELAY_ERR is module is not initialized
*          HW_DELAY_PARAM_ERR is delay is more than 511s, or callback is NULL
*/
hw_delay_res_e hw_delay_trigger_us(hw_delay_callback_f callback, uint32_t time_us);

/**
* \brief   Cancel Hardware delay
* \return  HW_DELAY_OK if stopped
*          HW_DELAY_NOT_TRIGGERED if no trigger configured
*          HW_DELAY_ERR if not initialized
*/
hw_delay_res_e hw_delay_cancel(void);

#endif /* HAL_HW_DELAY_H_ */
