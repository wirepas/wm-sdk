/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * voltage.c
 *
 *  Modified for NRF52840 optional VDDH input.
 *
 * \note SAADC one-shot mode requires that only one channel is configured!
 *       This is simple as long as we only need battery voltage measurement.
 *       Extending the A/D system requires centralized driver, because
 *       DMA writes the results of the all configured channels.
 */
#include <stdint.h>
#include "hal_api.h"

#define V_V_CH              0      /* SAADC channel used */

// Voltage scaling:
#define V_MAX_ADC           1023u  /* 10 bit a/d */
#define V_VREF_MV           600    /* 600mV reference */
#define VDD_INV_GAIN        6      /* 1/(1/6) */
#define VDDH_INV_GAIN       10     /* 1/((1/2)*(1/5)) */

// For ADC DMA target and extra room for voltage scaling.
// Single-ended one-shot ADC DMA writes uint16_t to this.
static volatile uint32_t m_adc_result;

// MCU in VDDH mode?
static bool m_voltage_vddh;

void Mcu_voltageInit(void)
{
    m_adc_result = 0;
    if ((NRF_POWER->MAINREGSTATUS & POWER_MAINREGSTATUS_MAINREGSTATUS_Msk) == \
        POWER_MAINREGSTATUS_MAINREGSTATUS_High)
    {
        // MCU is using VDDH as power input (VDD is programmable IO voltage):
        m_voltage_vddh = true;
    }
    else
    {
        // MCU is using VDD as power input (and as IO voltage):
        m_voltage_vddh = false;
    }
}

/**
 * Voltage measurement, NRF52840 implementation.
 * We measure either VDDH or VDD for VBAT (after checking which one is used).
 * When using VDDHDIV5 the acquisition time needs to be >= 10 µs. Use 15us.
 * SE mode, internal 0.6V ref, no oversampling, 10 bit
 * RESULT = [V(P) – V(N) ] * GAIN / REFERENCE[V] * (2**(RESOLUTION - mode))
 *     mode=0 for SingleEnded, mode=1 for Differential
 * \return millivolts
 */
uint16_t Mcu_voltageGet(void)
{
    // According to nRF52840 Product Specification v1.0 :
    // 6.23:
    //   An input channel is enabled and connected to an analog
    //   input pin using the registers CH[n].PSELP (n=0..7)
    // Clear All CH[n].PSELP registers
    // Will force a clean One-Shot operation mode
    for (uint8_t i = 0; i<8; i++)
    {
        NRF_SAADC->CH[i].PSELP =
             (SAADC_CH_PSELP_PSELP_NC << SAADC_CH_PSELP_PSELP_Pos);
    }

    Mcu_voltageInit();

    // SAADC configure:
    NRF_SAADC->RESULT.PTR = (uint32_t)&m_adc_result; // set DMA target
    NRF_SAADC->RESULT.MAXCNT = 1;        // set DMA count = one result (16-bit)
    NRF_SAADC->RESOLUTION =
        (SAADC_RESOLUTION_VAL_10bit << SAADC_RESOLUTION_VAL_Pos);
    NRF_SAADC->OVERSAMPLE =
        (SAADC_OVERSAMPLE_OVERSAMPLE_Bypass << SAADC_OVERSAMPLE_OVERSAMPLE_Pos);
    NRF_SAADC->CH[V_V_CH].PSELN =
        (SAADC_CH_PSELN_PSELN_NC << SAADC_CH_PSELN_PSELN_Pos);
    if (m_voltage_vddh == true)
    {   // VDDHDIV5:
        NRF_SAADC->CH[V_V_CH].CONFIG =
            (SAADC_CH_CONFIG_MODE_SE << SAADC_CH_CONFIG_MODE_Pos) |
            (SAADC_CH_CONFIG_TACQ_15us << SAADC_CH_CONFIG_TACQ_Pos) |
            (SAADC_CH_CONFIG_REFSEL_Internal << SAADC_CH_CONFIG_REFSEL_Pos) |
            (SAADC_CH_CONFIG_GAIN_Gain1_2 << SAADC_CH_CONFIG_GAIN_Pos) |
            (SAADC_CH_CONFIG_RESN_Bypass << SAADC_CH_CONFIG_RESN_Pos) |
            (SAADC_CH_CONFIG_RESP_Bypass << SAADC_CH_CONFIG_RESP_Pos);
        NRF_SAADC->CH[V_V_CH].PSELP =
            (SAADC_CH_PSELP_PSELP_VDDHDIV5 << SAADC_CH_PSELP_PSELP_Pos); // (VDDH==VBAT)
    }
    else
    {   // VDD:
        NRF_SAADC->CH[V_V_CH].CONFIG =
            (SAADC_CH_CONFIG_MODE_SE << SAADC_CH_CONFIG_MODE_Pos) |
            (SAADC_CH_CONFIG_TACQ_10us << SAADC_CH_CONFIG_TACQ_Pos) |
            (SAADC_CH_CONFIG_REFSEL_Internal << SAADC_CH_CONFIG_REFSEL_Pos) |
            (SAADC_CH_CONFIG_GAIN_Gain1_6 << SAADC_CH_CONFIG_GAIN_Pos) |
            (SAADC_CH_CONFIG_RESN_Bypass << SAADC_CH_CONFIG_RESN_Pos) |
            (SAADC_CH_CONFIG_RESP_Bypass << SAADC_CH_CONFIG_RESP_Pos);
        NRF_SAADC->CH[V_V_CH].PSELP =
            (SAADC_CH_PSELP_PSELP_VDD << SAADC_CH_PSELP_PSELP_Pos); // (VDD==VBAT)
    }
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
    if (m_voltage_vddh)
    {   // VDDHDIV5:
        m_adc_result *= (V_VREF_MV * VDDH_INV_GAIN);
    }
    else
    {   // VDD:
        m_adc_result *= (V_VREF_MV * VDD_INV_GAIN);
    }
    m_adc_result /= V_MAX_ADC;
    return (uint16_t)m_adc_result;
}

