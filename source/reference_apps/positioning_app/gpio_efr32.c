/* Copyright 2021 Wirepas Ltd. All Rights Reserved.
 *
 * See file LICENSE.txt for full license details.
 *
 */

/*
 * \file    gpio_efr32.c
 * \brief   This file implements the GPIO interface to EFR32 chips
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
#include "board.h"
#include "gpio.h"

#define MAX_EFR32_PIN_NUMBER 16

typedef struct
{
    uint8_t ext_int;
    uint8_t port;
    uint8_t pin;
} int_gpio_t;

static int_gpio_t m_pin_map[] = BOARD_INT_PIN_LIST;

/** \brief  Compute number of external int GPIO on the board */
#define BOARD_INT_GPIO_NUMBER   (sizeof(m_pin_map) / sizeof(m_pin_map[0]))

typedef struct
{
    uint8_t pin;            //header pin number
    gpio_pull_e pull;       //pin pull configuration
    gpio_event_e event;     //pin event type
    uint8_t debounce_ms;
    uint32_t last_event_hp;
    on_gpio_event_cb cb;    // event callback
} gpio_conf_t;

/** \brief  Table to manage gpio */
static gpio_conf_t m_gpio_conf[BOARD_INT_GPIO_NUMBER];

bool m_gpio_init = false;

static void gpio_interrupt_handler(void)
{
    // Check all possible sources
    for (uint8_t i = 0; i < BOARD_INT_GPIO_NUMBER; i++)
    {
        gpio_conf_t * gpio = &m_gpio_conf[i];

        if (gpio->pin >= MAX_EFR32_PIN_NUMBER)
        {
            continue;
        }

        // Read external interrupt flag
        if ((GPIO->IF & (((uint32_t)1) << m_pin_map[i].ext_int)) != 0)
        {
            gpio_event_e event = hal_gpio_get(m_pin_map[i].port, m_pin_map[i].pin) ? GPIO_EVENT_LH : GPIO_EVENT_HL;
            
            // Call the pin callback
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
                    if (delta_ms > gpio->debounce_ms || gpio->last_event_hp == 0)
                        {
                                gpio->last_event_hp = now_hp;
                                gpio->cb(gpio->pin, event);
                        }
                }
            }

            // Clear external interrupt flag
#if defined(_SILICON_LABS_32B_SERIES_1)
            GPIO->IFC = (uint32_t) 1 << m_pin_map[i].ext_int;
#else
            GPIO->IF_CLR = (uint32_t) 1 << m_pin_map[i].ext_int;
#endif
        }
    }
}

static bool gpio_enable_interrupt(gpio_conf_t *gpio_conf, uint8_t int_idx)
{
    int_gpio_t int_pin = m_pin_map[int_idx];
    
    uint8_t ext_int = int_pin.ext_int;
    uint8_t port = int_pin.port;
    uint8_t pin = int_pin.pin;

    hal_gpio_set_mode(port,
                      pin,
                      GPIO_MODE_IN_OD_NOPULL);
    hal_gpio_set(port, pin);

    bool enable = (gpio_conf->cb != NULL);

    bool high_reg = ext_int >= 8;
    uint8_t shift = (ext_int & 7) * 4;
    uint32_t mask = ~((uint32_t)0xf << shift);
    uint32_t set = 0;

    // Select port
    set = enable ? (uint32_t)port << shift : 0;
    if (high_reg)
    {
#if !defined(_SILICON_LABS_32B_SERIES_2_CONFIG_1)
        GPIO->EXTIPSELH = (GPIO->EXTIPSELH & mask) | set;
#endif
    }
    else
    {
        GPIO->EXTIPSELL = (GPIO->EXTIPSELL & mask) | set;
    }

    // Select pin
    set = enable ? (uint32_t)(pin & 3) << shift : 0;
    if (high_reg)
    {
#if !defined(_SILICON_LABS_32B_SERIES_2_CONFIG_1)
        GPIO->EXTIPINSELH = (GPIO->EXTIPINSELH & mask) | set;
#endif
    }
    else
    {
        GPIO->EXTIPINSELL = (GPIO->EXTIPINSELL & mask) | set;
    }

    // Set rising and falling edge sensitivity
    shift = ext_int;
    mask = ~((uint32_t)1 << shift);
    set = enable ? (uint32_t)1 << shift : 0;
    GPIO->EXTIRISE = (GPIO->EXTIRISE & mask) | set;
    GPIO->EXTIFALL = (GPIO->EXTIFALL & mask) | set;

    //Clear external interrupt flag
#if defined(_SILICON_LABS_32B_SERIES_1)
    GPIO->IFC = (uint32_t)1 << shift;
#else
    GPIO->IF_CLR = (uint32_t)1 << shift;
#endif

    // Enable or disable external interrupt
    GPIO->IEN = (GPIO->IEN & mask) | set;

    return true;
}

static void gpio_init(void)
{
    if (m_gpio_init)
    {
        return;
    }
    
    // Enable clocks
#if defined(_SILICON_LABS_32B_SERIES_1)
    CMU->HFBUSCLKEN0 |= CMU_HFBUSCLKEN0_GPIO;
#elif !defined (EFR32MG21)
    CMU->CLKEN0_SET = CMU_CLKEN0_GPIO;
#endif
    
    LOG(LVL_DEBUG, "GPIO init");

    for (uint8_t i = 0; i < BOARD_INT_GPIO_NUMBER; i++)
    {
        m_gpio_conf[i].pin = MAX_EFR32_PIN_NUMBER;
        m_gpio_conf[i].cb = NULL;
        m_gpio_conf[i].event = GPIO_EVENT_ALL;
        m_gpio_conf[i].pull = GPIO_NOPULL;

        //Input No Pull, DOUT enables filter.
        hal_gpio_set_mode(m_pin_map[i].port,
                          m_pin_map[i].pin,
                          GPIO_MODE_IN_OD_NOPULL);
        hal_gpio_set(m_pin_map[i].port, m_pin_map[i].pin);
    }

    // Enable even or odd external interrupts
    lib_system->enableAppIrq(false,
                            BOARD_LIS2_USE_EVEN_INT ? GPIO_EVEN_IRQn : GPIO_ODD_IRQn,
                            APP_LIB_SYSTEM_IRQ_PRIO_LO,
                            gpio_interrupt_handler);

    m_gpio_init = true;
}

gpio_res_e GPIO_register_for_event(uint8_t pin,
                                    gpio_pull_e pull,
                                    gpio_event_e event,
                                    uint8_t debounce_ms,
                                    on_gpio_event_cb cb)
{
    gpio_res_e ret;
    uint8_t idx = BOARD_INT_GPIO_NUMBER;
    uint8_t idx_free = BOARD_INT_GPIO_NUMBER;

    if(!m_gpio_init)
    {
        gpio_init();
    }

    if( (pin >= MAX_EFR32_PIN_NUMBER) || cb == NULL || 
        event >= GPIO_EVENT_MAX || pull >= GPIO_PULL_MAX )
    {
        return GPIO_RES_FAIL;
    }

    for (uint8_t i = 0; i < BOARD_INT_GPIO_NUMBER; i++)
    {   
        if (m_gpio_conf[i].pin == pin)
        {
            idx = i;
            break;
        } 
        else if (idx_free == BOARD_INT_GPIO_NUMBER && m_gpio_conf[i].cb == NULL)
        {
           idx_free = i; 
        }
    }
    idx = (idx == BOARD_INT_GPIO_NUMBER) ? idx_free : idx;

    if (idx < BOARD_INT_GPIO_NUMBER)
    {
        m_gpio_conf[idx].pin = pin;
        m_gpio_conf[idx].cb = cb;
        m_gpio_conf[idx].event = event;
        m_gpio_conf[idx].pull = pull;
        m_gpio_conf[idx].debounce_ms = debounce_ms;
        gpio_enable_interrupt(&m_gpio_conf[idx], idx);     
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
    if (!m_gpio_init)
    {
        gpio_init();
    }

    for (uint8_t i = 0; i < BOARD_INT_GPIO_NUMBER; i++)
    {
        if (m_gpio_conf[i].pin == pin)
        {
            int_gpio_t int_pin = m_pin_map[i];
            hal_gpio_clear(int_pin.port, int_pin.pin);

            m_gpio_conf[i].pin = MAX_EFR32_PIN_NUMBER;
            m_gpio_conf[i].cb = NULL;
        }
    }
    
    return GPIO_RES_OK;
}