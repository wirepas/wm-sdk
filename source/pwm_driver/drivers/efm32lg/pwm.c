#include "pwm.h"

#include "mcu.h"
#include "vendor/em_gpio.h"
#include "api.h"

#include <stdlib.h>

/** Is PWM Initialized */
static bool m_initialized = false;

void Pwm_init()
{
    uint8_t shift_output;

    // Enable TIMER2
    CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_TIMER2;

#if (BOARD_PWM_OUTPUT_PIN > 7)
    shift_output = (BOARD_PWM_OUTPUT_PIN - 8) * 4;
    GPIO->P[BOARD_PWM_OUTPUT_PORT].MODEH &= ~(0xf << shift_output);
    GPIO->P[BOARD_PWM_OUTPUT_PORT].MODEH |= (0x4 << shift_output);
#else
    shift_output = (BOARD_PWM_OUTPUT_PIN) * 4;
    GPIO->P[BOARD_PWM_OUTPUT_PORT].MODEL &= ~(0xf << shift_output);
    GPIO->P[BOARD_PWM_OUTPUT_PORT].MODEL |= (0x4 << shift_output);
#endif
    GPIO->P[BOARD_PWM_OUTPUT_PORT].DOUTCLR = 1 << 14;  // On by default

    // Add prescaler to have a 250 KHz frequency
    TIMER2->CTRL |= (TIMER_CTRL_PRESC_DIV64 | TIMER_CTRL_DEBUGRUN);

    // Configure TOP value to configure global period
    // With 100, it gives a GP of 2,5KHz and an easy configuration
    TIMER2->TOP = 100;

    // Configure timer CNT
    TIMER2->CNT = 0;

    // Set timer1 channel 2 in PWM mode
    TIMER2->CC[2].CTRL = TIMER_CC_CTRL_MODE_PWM;

    m_initialized = true;
}

bool Pwm_set_value(uint32_t pwm_value)
{
    if (pwm_value > 100 || !m_initialized)
        return false;

    if (pwm_value == 100)
    {
        // Disable output
        TIMER2->ROUTE &= ~(TIMER_ROUTE_LOCATION_LOC1 | TIMER_ROUTE_CC2PEN);

        // Stop timer
        TIMER2->CMD = TIMER_CMD_STOP;

        // Disable timer 2
        CMU->HFPERCLKEN0 &= ~(CMU_HFPERCLKEN0_TIMER2);
        lib_system->disableDeepSleep(false);

        // Set output to 1
        GPIO->P[BOARD_PWM_OUTPUT_PORT].DOUTSET = 1 << 14;
    }
    else if (pwm_value == 0)
    {
        // Disable output
        TIMER2->ROUTE &= ~(TIMER_ROUTE_LOCATION_LOC1 | TIMER_ROUTE_CC2PEN);
        // Stop timer
        TIMER2->CMD = TIMER_CMD_STOP;

        // Disable timer 2
        CMU->HFPERCLKEN0 &= ~(CMU_HFPERCLKEN0_TIMER2);
        lib_system->disableDeepSleep(false);

        // Set output to 0
        GPIO->P[BOARD_PWM_OUTPUT_PORT].DOUTCLR = 1 << 14;
    }
    else
    {
        lib_system->disableDeepSleep(true);
        // Enable TIMER2
        CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_TIMER2;

        // Configure pwm timer
        TIMER2->CC[2].CCV = pwm_value;
        TIMER2->CMD = TIMER_CMD_START;

        // Set output route
        TIMER2->ROUTE |= (TIMER_ROUTE_LOCATION_LOC1 | TIMER_ROUTE_CC2PEN);
    }

    return true;
}
