/**
 * Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include <stdint.h>
#include "hal_api.h"
#include "em_cmu.h"

/**
 * \brief   ADC peripheral control function.
 * \param   state
 *          true = turn peripheral clocks on, false = clocks off
 */
static void adcControl(bool state)
{
    if(state)
    {
        CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_ADC0;
    }
    else
    {
        CMU->HFPERCLKEN0 &= ~(CMU_HFPERCLKEN0_ADC0);
    }
}


/**
 * \brief   ADC max value + 1 = 12bit ADC conversion, 0xFFF + 1 = 4096u
 */
#define V_MAX_ADC           4096u


/**
 * \brief   VBGR reference voltage
 */
#define VBG_MILLIVOLTS      5000u


/**
 * \brief   Initialize voltage measurement
 */
void Mcu_voltageInit(void)
{
    /* Do nothing */
}


/**
 * \brief   Get current MCU voltage (platform voltage).
 *
 * \return  Voltage scaled by a factor of x1000
 */
uint16_t Mcu_voltageGet(void)
{
    uint32_t vbat;
    /* Enable ADC clock. */
    adcControl(true);

    /*  Make sure all ADC interrupt sources are disabled and reset */
    NVIC_DisableIRQ(ADC0_IRQn);
    NVIC_ClearPendingIRQ(ADC0_IRQn);
    ADC0->IEN = _ADC_IEN_RESETVALUE;
    ADC0->IFC = _ADC_IFC_MASK;


    /* Make sure conversion is not in progress */
    ADC0->CMD = ADC_CMD_SINGLESTOP | ADC_CMD_SCANSTOP;
    /*  Clear the FIFO */
    ADC0->SINGLEFIFOCLEAR |= ADC_SINGLEFIFOCLEAR_SINGLEFIFOCLEAR;

    ADC0->BIASPROG = ADC_BIASPROG_GPBIASACC_HIGHACC;

    /* The maximum clock frequency for adc_clk_sar is 16 MHz
     * Assume HFPERCLK 38,4MHz (or 39,0MHz with EFR32xG23)
     * ADC_CTRL_PRESC is Clock prescale factor.
     *      ADC_CLK is divided by (PRESC+1) to produce adc_clk_sar
     * ADC_CTRL_TIMEBASE field set equal to produce timing of 1us or greater
     * TIMEBASE calulated like this:
        * hfperFreq = 38400000 (or 39000000)
        * hfperFreq += 999999;
        * hfperFreq /= 1000000;
        * timebase = hfperFreq -1;
        * timebase = 38 = 0x26
    */

    /* We use adc clock 12,8MHz, 5 us warmup, 2x oversampling, 32 cycle acq time. */
    ADC0->CTRL = (0x26 << _ADC_CTRL_TIMEBASE_SHIFT) /* timebase for warmup */
               | (2 << _ADC_CTRL_PRESC_SHIFT)       /* f = 38,4/3 = 12,8 MHz  */
               | ADC_CTRL_OVSRSEL_X2                /* oversample conf 2x */
              ;

    /* Setup single measurement: res = 12bit, 32 acq cycles */
    ADC0->SINGLECTRL = ADC_SINGLECTRL_RES_12BIT        |
                       ADC_SINGLECTRL_AT_32CYCLES      |
                       ADC_SINGLECTRL_REF_5V           |
                       ADC_SINGLECTRL_POSSEL_AVDD;


    /* Perform ADC conversion */
    ADC0->CMD = ADC_CMD_SINGLESTART;

#if defined(_SILICON_LABS_32B_PLATFORM_2_GEN_1)
    /* Delay errata : ADC_E218 - SINGLEACT and SCANACT Status Flags Delayed */
     __NOP();
     __NOP();
     __NOP();
     __NOP();
     __NOP();
     __NOP();
#endif

    while (ADC0->STATUS & ADC_STATUS_SINGLEACT)
    {
        /* Wait for conversion to finish */
    }

    vbat = ADC0->SINGLEDATA; /* read result */

    /* Disable ADC0 clock. */
    adcControl(false);

    /* Scale to millivolts * 1000: adc * presc^-1 * vbg = adc * 5000 */
    vbat = (vbat * VBG_MILLIVOLTS) / V_MAX_ADC;
    return vbat;
}
