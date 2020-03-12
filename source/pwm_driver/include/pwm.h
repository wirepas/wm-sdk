#ifndef _PWM_H__
#define _PWM_H__

#include <stdint.h>
#include <stdbool.h>

#include "board.h"

/**
 * \brief   Initialize Pwm module
 */
void Pwm_init();

/**
 * \brief   Set the duty cycle of the pwm
 * \param   pwm_value
 *          New pwm value in %
 * \return  True if value changed
 */
bool Pwm_set_value(uint32_t pwm_value);

#endif
