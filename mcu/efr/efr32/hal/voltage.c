/**
 * Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include <stdint.h>
#include "hal_api.h"
#include "em_cmu.h"
/* Include only for xg21/xg22. */
#ifndef _SILICON_LABS_32B_SERIES_1
#include "em_iadc.h"
#endif

/**
 * \brief   ADC peripheral control function.
 * \param   state
 *          true = turn peripheral clocks on, false = clocks off
 */
#if defined(_SILICON_LABS_32B_SERIES_1)
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
#elif defined(_SILICON_LABS_32B_SERIES_2_CONFIG_1)
static void adcControl(bool state)
{
    (void)state;
}
#elif defined(_SILICON_LABS_32B_SERIES_2_CONFIG_2)
static void adcControl(bool state)
{
    CMU_ClockEnable(cmuClock_IADC0, state);
}
#else
#error "Unsupported EFR32 Series & Config"
#endif


/**
 * \brief   ADC max value + 1 = 12bit ADC conversion, 0xFFF + 1 = 4096u
 */
#define V_MAX_ADC           4096u

#if defined(_SILICON_LABS_32B_SERIES_1)

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

#else // defined(_SILICON_LABS_32B_SERIES_1)

/**
 * \brief   VBGR reference voltage, 4*1,2V
 */
#define VBG_MILLIVOLTS      4800u



#define ADC_PORT        gpioPortC
#define ADC_PIN         3

#define PRS_CHANNEL     2

#define clksrcadc 9600000 // The HFXO (EM01GRPACLK) is running at 38.4 MHz.
                          // The HSCLKRATE is set to 3 (DIV4).
                          // CLK_SRC_ADC is 9.6 MHz


void Mcu_voltageInit(void)
{
}

uint16_t Mcu_voltageGet(void)
{
    uint32_t vbat;
    IADC_Result_t adcResult;

    // EM01 Peripheral Group A Clock to use HFXO as source.
    CMU_ClockSelectSet(cmuClock_EM01GRPACLK, cmuSelect_HFXO);

    // Configure IADC Clock to use EM01GRPACLK as source.
    adcControl(true);
    CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_EM01GRPACLK);

    IADC_Init_t init = IADC_INIT_DEFAULT;
    init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, clksrcadc, 0);

    IADC_AllConfigs_t initAllConfigs = IADC_ALLCONFIGS_DEFAULT;
    // Use internal 1.2V Band Gap Reference (buffered).
    initAllConfigs.configs[0].reference = iadcCfgReferenceInt1V2;
    // Divides CLK_SRC_ADC by 24 ==> 400kHz.
    initAllConfigs.configs[0].adcClkPrescale = 23;

    // Initialize the IADC.
    IADC_init(IADC0, &init, &initAllConfigs);

    // Assign pin.
    IADC_SingleInput_t initSingleInput = IADC_SINGLEINPUT_DEFAULT;
    // Avdd is attenuated by a factor of 4.
    // This is compensated by setting VBGR 4 times the reference voltage.
    initSingleInput.posInput = iadcPosInputAvdd;

    // Initialize the Single conversion inputs.
    IADC_InitSingle_t initSingle = IADC_INITSINGLE_DEFAULT;
    initSingle.triggerSelect = iadcTriggerSelImmediate;
    initSingle.triggerAction = iadcTriggerActionOnce;
    initSingle.start = true;
    IADC_initSingle(IADC0, &initSingle, &initSingleInput);

    // Wait conversion to complete.
    while(!(IADC_getStatus(IADC0) & IADC_STATUS_SINGLEFIFODV));

    // Get value.
    adcResult = IADC_readSingleResult(IADC0);

    // Save power by switching off clocks from IADC0.
    adcControl(false);

    // Scale result.
    vbat = (adcResult.data * VBG_MILLIVOLTS) / V_MAX_ADC;
    return vbat;
}

#endif // defined(_SILICON_LABS_32B_SERIES_1)
