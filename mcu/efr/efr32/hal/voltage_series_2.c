/**
 * Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */
#include <stdint.h>
#include "hal_api.h"
#include "em_cmu.h"
#include "em_iadc.h"


/**
 * \brief   ADC max value + 1 = 12bit ADC conversion, 0xFFF + 1 = 4096u
 */
#define V_MAX_ADC           4096u


/**
 * \brief   VBGR reference voltage, 4*1,2V
 */
#define VBG_MILLIVOLTS      4800u


/* CMU clock is divided by 4 (HSCLKRATE is set to 3). */
#define IADC_CMU_CLOCK_DIV 4UL

/* The HFXO (EM01GRPACLK) is running at 38.4 MHz or 39 MHz.
   The HSCLKRATE is set to 3 (DIV4).
   IADC_CLK_SRC_ADC is 9.6 MHz or 9.75 MHz */
#define IADC_CLK_SRC_ADC (__SYSTEM_CLOCK / IADC_CMU_CLOCK_DIV)


/**
 * \brief   ADC peripheral control function.
 * \param   state
 *          true = turn peripheral clocks on, false = clocks off
 */
static void adcControl(bool state)
{
#if (_SILICON_LABS_32B_SERIES_2_CONFIG == 1)
    (void)state;
#else
    CMU_ClockEnable(cmuClock_IADC0, state);
#endif
}


void Mcu_voltageInit(void)
{
    /* Do nothing */
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
    init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, IADC_CLK_SRC_ADC, 0);

    IADC_AllConfigs_t initAllConfigs = IADC_ALLCONFIGS_DEFAULT;
    // Use internal 1.2V Band Gap Reference (buffered).
    initAllConfigs.configs[0].reference = iadcCfgReferenceInt1V2;
    // Divides CLK_SRC_ADC by 24 ==> 400 kHz or 406 kHz.
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
