/* Copyright 2020 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    gpio.c
 * \brief   This file implements the GPIO interface to nRF52 chips
 *         
 */

#define DEBUG_LOG_MODULE_NAME "GPIO"
#ifdef DEBUG_POSLIB_LOG_MAX_LEVEL
#define DEBUG_LOG_MAX_LEVEL DEBUG_POSLIB_LOG_MAX_LEVEL
#else
#define DEBUG_LOG_MAX_LEVEL LVL_NOLOG
#endif

#include "api.h"
#include "mcu.h"
#include "debug_log.h"
#include "gpio.h"

#define MAX_NRF52_PINS 30 //FixMe: use the nrf52.h define if exists

typedef struct
{
    uint8_t pin;            //pin number
    gpio_pull_e pull;       //pin pull configuration
    gpio_event_e event;     //pin event type
    uint8_t debounce_ms;
    uint32_t last_event_hp;
    on_gpio_event_cb cb;    // event callback
} gpio_conf_t;

/** \brief  Table to manage gpio */
static gpio_conf_t m_gpio_conf[GPIO_MAX];
bool m_gpio_init = false;


static void gpiote_interrupt_handler(void)
{
    if (NRF_GPIOTE->EVENTS_PORT == 0)
    {
        return;
    }

    NRF_GPIOTE->EVENTS_PORT = 0;
    // read any event from peripheral to flush the write buffer:
    EVENT_READBACK = NRF_GPIOTE->EVENTS_PORT;

    // Detect the pin generating the irq
    for (uint8_t i = 0; i < GPIO_MAX; i++)
    {
        
        gpio_conf_t * gpio = &m_gpio_conf[i];
    
        if (gpio->pin >= MAX_NRF52_PINS)
        {
            continue;
        }

        if (nrf_gpio_pin_latch_get(gpio->pin))
        {
            gpio_event_e event = nrf_gpio_pin_read(gpio->pin) ? GPIO_EVENT_LH : GPIO_EVENT_HL;
           
            //call the pin callback
            if (gpio->cb != NULL && 
                (gpio->event == event ||
                 gpio->event == GPIO_EVENT_ALL)) 
            {  
               // call imediatelly if debounce not requested
               if (gpio->debounce_ms == 0)
               {
                   gpio->cb(gpio->pin, event);
               }
               else
               {
                   uint32_t now_hp = lib_time->getTimestampHp();
                   uint32_t delta_ms = lib_time->getTimeDiffUs(now_hp, gpio->last_event_hp) * 1000;

                   // call calback only if debounce is satisfied
                   if ( delta_ms > gpio->debounce_ms || gpio->last_event_hp == 0)
                       {
                            gpio->last_event_hp = now_hp;
                            gpio->cb(gpio->pin, event);
                       }
                }
            }   

            //detect the gpio pin state and set sense to oposite level
            if ( gpio->event == GPIO_EVENT_ALL )  
            {
                nrf_gpio_pin_sense_t sense;

                sense = (nrf_gpio_pin_read(gpio->pin) == 0) ?
                    NRF_GPIO_PIN_SENSE_HIGH : NRF_GPIO_PIN_SENSE_LOW;
                nrf_gpio_cfg_sense_set(gpio->pin, sense);
            }
        }  
        // Clear the line
         nrf_gpio_pin_latch_clear(gpio->pin);
    }    
}

static bool gpio_enable_interrupt(gpio_conf_t *gpio_conf)
{
    nrf_gpio_pin_pull_t  pull;
    nrf_gpio_pin_sense_t sense;
    bool sense_all = false;

    switch (gpio_conf->pull)
     {
         case GPIO_PULLDOWN:
            pull = NRF_GPIO_PIN_PULLDOWN;
            break;
        case GPIO_PULLUP:
             pull = NRF_GPIO_PIN_PULLUP;
             break;
        case GPIO_NOPULL:
            pull = NRF_GPIO_PIN_NOPULL;
            break;
        default:
            return false;
     }

    switch (gpio_conf->event)
    {
        case GPIO_EVENT_LH:
            sense = NRF_GPIO_PIN_SENSE_HIGH;
            break;
        case GPIO_EVENT_HL:
             sense = NRF_GPIO_PIN_SENSE_LOW;
             break;
        case GPIO_EVENT_ALL:
            sense = NRF_GPIO_PIN_SENSE_HIGH; //initial state guess
            sense_all = true;
            break;
        default:
            return false;
    }

    NRF_GPIOTE->INTENCLR = GPIOTE_INTENSET_PORT_Msk;
    nrf_gpio_cfg_sense_input(gpio_conf->pin, pull, sense);

    if (sense_all)  //detect the gpio pin state and set sense to oposite level
    {
        sense = (nrf_gpio_pin_read(gpio_conf->pin) == 0) ?
                    NRF_GPIO_PIN_SENSE_HIGH : NRF_GPIO_PIN_SENSE_LOW;
        nrf_gpio_cfg_sense_set(gpio_conf->pin, sense);
    }

    // Clear the line before enabling IRQ
    nrf_gpio_pin_latch_clear(gpio_conf->pin);
    NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_PORT_Msk;
    return true;
}

static void gpio_init()
{
    if(m_gpio_init)
    {
        return;
    }
    
    LOG(LVL_DEBUG, "GPIO init");   
    for (uint8_t i = 0; i < GPIO_MAX; i++)
    {
        m_gpio_conf[i].pin = MAX_NRF52_PINS;
        m_gpio_conf[i].cb = NULL;
        m_gpio_conf[i].event = GPIO_EVENT_ALL;
        m_gpio_conf[i].pull = GPIO_NOPULL;
    }

    NRF_GPIOTE->INTENCLR = GPIOTE_INTENSET_PORT_Msk;
    NRF_GPIOTE->EVENTS_PORT = 0;

    // Enable interrupt
    lib_system->clearPendingFastAppIrq(GPIOTE_IRQn);
    lib_system->enableAppIrq(true,
                             GPIOTE_IRQn,
                             APP_LIB_SYSTEM_IRQ_PRIO_LO,
                             gpiote_interrupt_handler);
    m_gpio_init = true;
}

gpio_res_e GPIO_register_for_event(uint8_t pin,
                                    gpio_pull_e pull,
                                    gpio_event_e event,
                                    uint8_t debounce_ms,
                                    on_gpio_event_cb cb)
{
    gpio_res_e ret;
    uint8_t idx = GPIO_MAX;
    uint8_t idx_free = GPIO_MAX;

    if(!m_gpio_init)
    {
        gpio_init();
    }

    /** FixME: check the macro for nRF52 max pin */

    if( (pin >= MAX_NRF52_PINS) || cb == NULL || 
        event >= GPIO_EVENT_MAX || pull >= GPIO_PULL_MAX )
    {
        return GPIO_RES_FAIL;
    }

    // find if pin is already registered | find a free slot
    // only one event registration per pin is allowed

    for (uint8_t i = 0; i < GPIO_MAX; i++)
    {   
        if (m_gpio_conf[i].pin == pin)
        {
            idx = i;
            break;
        } else if (idx_free == GPIO_MAX && m_gpio_conf[i].cb == NULL)
        {
           idx_free = i; 
        }
    }

    idx = (idx == GPIO_MAX) ? idx_free : idx;

   // update if a valid index found
    if (idx < GPIO_MAX)
     {
        m_gpio_conf[idx].pin = pin;
        m_gpio_conf[idx].cb = cb;
        m_gpio_conf[idx].event = event;
        m_gpio_conf[idx].pull = pull;
        m_gpio_conf[idx].debounce_ms = debounce_ms;
        gpio_enable_interrupt(&m_gpio_conf[idx]);     
        ret = GPIO_RES_OK;
    }
    else
    {
        ret = GPIO_RES_FAIL;
    }
    return ret;
}

gpio_res_e GPIO_deregister_for_event(uint8_t pin)
{
    
    bool pin_active = false;

    if(!m_gpio_init)
    {
        gpio_init();
    }

    for (uint8_t i = 0; i < GPIO_MAX; i++)
    {
        if (m_gpio_conf[i].pin == pin)
        {
            nrf_gpio_input_disconnect(pin);
            m_gpio_conf[i].pin = MAX_NRF52_PINS;
            m_gpio_conf[i].cb = NULL;
        }
        else if(m_gpio_conf[i].cb != NULL)
        {
            pin_active = true;
        }
    }

    // disable PORT interrupt if no pin is active
    if(!pin_active)
    {
        NRF_GPIOTE->INTENCLR = GPIOTE_INTENSET_PORT_Msk;
        NRF_GPIOTE->EVENTS_PORT = 0;
    }
    return GPIO_RES_OK;
}

