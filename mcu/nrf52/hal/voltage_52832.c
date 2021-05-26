/** Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 *  See file LICENSE.txt for full license details.
 *
 * @note SAADC one-shot mode requires that only one channel is configured!
 *       This is simple as long as we only need battery voltage measurement.
 *       Extending the A/D system requires centralized driver, because
 *       DMA writes the results of the all configured channels.
 */

#include <stdint.h>
#include "hal_api.h"

#define V_V_CH              0      /* SAADC channel used */

// Energy scaling:
#define V_MIN_MILLIVOLTS    2200u
#define V_MAX_MILLIVOLTS    2800u

// Voltage scaling:
#define V_MAX_ADC           1023u  /* 10 bit a/d */
#define V_VREF_MV           600    /* 600mV reference */
#define V_INV_GAIN          6      /* a/d internal gain is 1/6 */

void Mcu_voltageInit(void)
{
}

/**
 * Voltage measurement, NRF52 implementation (SAADC totally redesigned).
 * We use SAADC channel SAADC_V_CH for VBAT measurement:
 *    SE mode, internal 0.6V ref, no oversampling, 10 bit
 * RESULT = [V(P) – V(N) ] * GAIN / REFERENCE[V] * (2**(RESOLUTION - mode))
 *     mode=0 for SingleEnded, mode=1 for Differential
 * \return millivolts
 */
uint16_t Mcu_voltageGet(void)
{
    volatile uint32_t adc_result = 0;    // written by DMA

    // According to nRF52832 Product Specification v1.4 :
    // 37.5.1 : "One-shot operation is configured by enabling only one of
    //           the available channels.."
    // Clear All CH[n].PSELP registers
    // Will force a clean One-Shot operation mode
    for (uint8_t i = 0; i<8; i++)
    {
        NRF_SAADC->CH[i].PSELP =
             (SAADC_CH_PSELP_PSELP_NC << SAADC_CH_PSELP_PSELP_Pos);
    }

    // SAADC configure:
    NRF_SAADC->RESULT.PTR = (uint32_t)&adc_result; // set DMA target
    NRF_SAADC->RESULT.MAXCNT = 1;        // set DMA count = one result (16-bit)
    NRF_SAADC->RESOLUTION =
        (SAADC_RESOLUTION_VAL_10bit << SAADC_RESOLUTION_VAL_Pos);
    NRF_SAADC->OVERSAMPLE =
        (SAADC_OVERSAMPLE_OVERSAMPLE_Bypass << SAADC_OVERSAMPLE_OVERSAMPLE_Pos);
    NRF_SAADC->CH[V_V_CH].CONFIG =
        (SAADC_CH_CONFIG_MODE_SE << SAADC_CH_CONFIG_MODE_Pos) |
        (SAADC_CH_CONFIG_TACQ_10us << SAADC_CH_CONFIG_TACQ_Pos) |
        (SAADC_CH_CONFIG_REFSEL_Internal << SAADC_CH_CONFIG_REFSEL_Pos) |
        (SAADC_CH_CONFIG_GAIN_Gain1_6 << SAADC_CH_CONFIG_GAIN_Pos) |
        (SAADC_CH_CONFIG_RESN_Bypass << SAADC_CH_CONFIG_RESN_Pos) |
        (SAADC_CH_CONFIG_RESP_Bypass << SAADC_CH_CONFIG_RESP_Pos);
    NRF_SAADC->CH[V_V_CH].PSELN =
        (SAADC_CH_PSELN_PSELN_NC << SAADC_CH_PSELN_PSELN_Pos);
    NRF_SAADC->CH[V_V_CH].PSELP =
        (SAADC_CH_PSELP_PSELP_VDD << SAADC_CH_PSELP_PSELP_Pos); // (VDD==VBAT)
    // SAADC enable (activates input access):
    NRF_SAADC->ENABLE =
        (SAADC_ENABLE_ENABLE_Enabled << SAADC_ENABLE_ENABLE_Pos);

    // SAADC execute:
    NRF_SAADC->EVENTS_STARTED = 0;
    NRF_SAADC->TASKS_START = 1;
    while (!NRF_SAADC->EVENTS_STARTED)
    {
        // Wait for SAADC start
    }
    NRF_SAADC->EVENTS_END = 0;
    NRF_SAADC->TASKS_SAMPLE = 1;
    while (!NRF_SAADC->EVENTS_END)
    {
        // Wait for conversion to be done
    }
    // SAADC disable:
    NRF_SAADC->ENABLE =
        (SAADC_ENABLE_ENABLE_Disabled << SAADC_ENABLE_ENABLE_Pos);
    NRF_SAADC->TASKS_STOP = 1;
    // Scaling the result:
    adc_result *= (V_VREF_MV * V_INV_GAIN);
    adc_result /= V_MAX_ADC;
    return (uint16_t)adc_result;
}

