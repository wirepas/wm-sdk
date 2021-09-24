/* Copyright 2018 Wirepas Ltd. All Rights Reserved.
*
* See file LICENSE.txt for full license details.
*
*/

/*
* \file    hw_delay.c
* \brief   hardware delay module for nrf52
*/

#include "mcu.h"
#include "api.h"
#include "hw_delay.h"

#define MINIMUM_TICKS   3
#define _1US_          (1)
#define _1MS_          (1000)
#define _1S_           (1000*1000)

/* RTC Clocked bt LFCLK at 32kHz */
#define LFCLK_FREQ      32768UL

/* RTC increment at 32kHz */
#define RTC_PRESCALER   0UL

/* Maximum RTC Value in Tick */
#define MAX_RTC0_TICK   0x00FFFFFF

/* Maximum RTC Value in seconds */
#define MAX_RTC0_SEC    (MAX_RTC0_TICK/(LFCLK_FREQ*(RTC_PRESCALER+1UL)))

/* Maximum RTC Value in usec */
#define MAX_RTC0_USEC   (_1S_*MAX_RTC0_SEC)

/* How many RTC tick in 1 us */
#define TICK_IN_US      ((_1S_ * (1+RTC_PRESCALER)) / (LFCLK_FREQ))

/* Cb called on compare event */
static volatile hw_delay_callback_f m_hw_delay_callback;

/* Set once module initialized */
static bool m_initialized;

/* set if module is running */
static bool m_started;

/** \brief  Disable COMPARE0 event and interrupt */
static void timer_event_disable(void)
{
    NRF_RTC0->EVTENCLR = RTC_EVTENSET_COMPARE0_Msk;
    NRF_RTC0->INTENCLR = RTC_INTENSET_COMPARE0_Msk;
}

/** \brief  Enable COMPARE0 event and interrupt */
static void timer_event_enable(void)
{
    NRF_RTC0->EVTENSET = RTC_EVTENSET_COMPARE0_Msk;
    NRF_RTC0->INTENSET = RTC_INTENSET_COMPARE0_Msk;
}

/** \brief Perform read to ensure clearing is effective */
static void event_clear(volatile uint32_t * p_event)
{
    *p_event = 0;

    volatile uint32_t dummy = *p_event;
    (void)dummy;
}

/**
* \brief   Start Hardware Delay Module
* \return  HW_DELAY_OK if started
*          HW_DELAY_ERR if already started or not initialized
*/
static hw_delay_res_e rtc_start(void)
{
    if ((m_started) || (!m_initialized))
    {
        return HW_DELAY_ERR;
    }
    else
    {
        NRF_RTC0->TASKS_START = 1;
        m_started=true;
        return HW_DELAY_OK;
    }
}

/**
* \brief   Convert delay from us to tick
* \param   time_us
*          time in us
* \return  time in tick
*/
static uint32_t convert_us_to_tick(uint32_t time_us)
{
    uint32_t time_tick;

    time_tick  = MAX_RTC0_TICK & ( time_us / TICK_IN_US );

    return time_tick;
}

/**
* \brief   Trigger delay in clock tick
* \param   time_tick
*          delay to wait in clock tick
*/
static void setup_trigger_in_tick(uint32_t time_tick)
{
    uint32_t event;

    time_tick = time_tick > MINIMUM_TICKS ? time_tick : MINIMUM_TICKS;

    event = NRF_RTC0->COUNTER + time_tick;

    rtc_start();
    timer_event_enable();

    NRF_RTC0->CC[0] = event;
}

/**
* \brief   Trigger delay in us
* \param   time_us
*          delay to wait in us
*/
static void setup_trigger_in_us(uint32_t time_us)
{
    /* Convert us to tick */
    uint32_t time_tick = convert_us_to_tick(time_us);

    /* Trigger RTC with a time in tick */
    setup_trigger_in_tick(time_tick);
}

/**
* \brief   IRQ Handler
*/
static void IRQHandler(void)
{
    uint32_t time_us;
    hw_delay_callback_f callback = m_hw_delay_callback;

    if ((NRF_RTC0->EVENTS_COMPARE[0] != 0) &&
        ((NRF_RTC0->INTENSET & RTC_INTENSET_COMPARE0_Msk) != 0))
    {
        event_clear(&NRF_RTC0->EVENTS_COMPARE[0]);

        if (callback)
        {
            time_us =  callback();
            if ((time_us > 0) && (time_us < MAX_RTC0_USEC))
            {
                /* Auto rearm */
                setup_trigger_in_us(time_us);
                return;
            }
        }

        /* Do Event-Disable / RTC-Stop only if we don't auto-rearm */
        timer_event_disable();
        hw_delay_cancel();
    }
}

/**
* \brief   Initialize Hardware Delay Module
*/
hw_delay_res_e hw_delay_init(void)
{
    if (m_initialized)
    {
        return HW_DELAY_ERR;
    }

    m_hw_delay_callback = (hw_delay_callback_f)NULL;

    NRF_RTC0->TASKS_STOP = 1;
    NRF_RTC0->TASKS_CLEAR = 1;
    NRF_RTC0->CC[0] = 0;
    event_clear(&NRF_RTC0->EVENTS_COMPARE[0]);

    NRF_RTC0->PRESCALER = RTC_PRESCALER;
    NRF_RTC0->INTENCLR  = 0xFFFFFFFF; /* Disable all interrupt */

    lib_system->clearPendingFastAppIrq(RTC0_IRQn);
    lib_system->enableAppIrq(true, RTC0_IRQn, APP_LIB_SYSTEM_IRQ_PRIO_LO, IRQHandler);

    m_initialized = true;
    m_started = false;

    return HW_DELAY_OK;
}

/**
* \brief   Setup timer trigger
*/
hw_delay_res_e hw_delay_trigger_us(hw_delay_callback_f callback, uint32_t time_us)
{
    if (!m_initialized)
    {
        return HW_DELAY_ERR;
    }

    /* check time value. Could not be more than ~511s */
    if ((time_us > MAX_RTC0_USEC) || (callback == NULL))
    {
        return HW_DELAY_PARAM_ERR;
    }

    m_hw_delay_callback = callback;

    setup_trigger_in_us(time_us);

    return HW_DELAY_OK;
}

/**
* \brief   Cancel Hardware delay
*/
hw_delay_res_e hw_delay_cancel(void)
{
    if (m_hw_delay_callback == NULL)
    {
        return HW_DELAY_NOT_TRIGGERED;
    }

    if (m_initialized)
    {
        NRF_RTC0->TASKS_STOP = 1;
        NRF_RTC0->EVENTS_COMPARE[0] = 0;
        m_started = false;
        m_hw_delay_callback = NULL;
        return HW_DELAY_OK;
    }
    else
    {
        return HW_DELAY_ERR;
    }
}
