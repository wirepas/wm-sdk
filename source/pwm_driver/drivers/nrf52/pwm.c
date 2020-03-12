#include "pwm.h"

#include "mcu.h"

#include "app.h"

static uint32_t pwm_max_value = 128;
static uint16_t pwm_seq[4] = {64, 0, 0, 0};

/** Is PWM Initialized */
static bool m_initialized = false;

void Pwm_init()
{
    nrf_gpio_cfg_output(BOARD_PWM_OUTPUT_GPIO);
    nrf_gpio_pin_clear(BOARD_PWM_OUTPUT_GPIO);

    // Select output pin
    NRF_PWM0->PSEL.OUT[0] = (BOARD_PWM_OUTPUT_GPIO << PWM_PSEL_OUT_PIN_Pos) |
                            (PWM_PSEL_OUT_CONNECT_Connected <<
                                                     PWM_PSEL_OUT_CONNECT_Pos);
    // Enable PWM module
    NRF_PWM0->ENABLE      = (PWM_ENABLE_ENABLE_Enabled << PWM_ENABLE_ENABLE_Pos);

    // Set Mode
    NRF_PWM0->MODE        = (PWM_MODE_UPDOWN_Up << PWM_MODE_UPDOWN_Pos);

    // Set prescaler to set PWM clock (500KHz)
    NRF_PWM0->PRESCALER   = (PWM_PRESCALER_PRESCALER_DIV_32 <<
                                                     PWM_PRESCALER_PRESCALER_Pos);
    // Set counter top to have a frequency of 3.9 KHz
    NRF_PWM0->COUNTERTOP  = (pwm_max_value << PWM_COUNTERTOP_COUNTERTOP_Pos);

    NRF_PWM0->LOOP        = (PWM_LOOP_CNT_Disabled << PWM_LOOP_CNT_Pos);

    NRF_PWM0->DECODER   = (PWM_DECODER_LOAD_Individual << PWM_DECODER_LOAD_Pos) |
                          (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);

    NRF_PWM0->SEQ[0].PTR  = ((uint32_t)(pwm_seq) << PWM_SEQ_PTR_PTR_Pos);
    NRF_PWM0->SEQ[0].CNT  = (4 << PWM_SEQ_CNT_CNT_Pos);
    NRF_PWM0->SEQ[0].REFRESH  = 0;
    NRF_PWM0->SEQ[0].ENDDELAY = 0;
    NRF_PWM0->TASKS_SEQSTART[0] = 1;

    m_initialized = true;
}

bool Pwm_set_value(uint32_t pwm_value)
{
    if (pwm_value > 100 || !m_initialized)
        return false;

   // PWM value is inverted (starts low) so 90% is 10%
   pwm_seq[0] = (100 - pwm_value) * pwm_max_value / 100;
   // Restart or start the sequence 0 to take new value into account
   NRF_PWM0->TASKS_SEQSTART[0] = 1;

   return true;
}
