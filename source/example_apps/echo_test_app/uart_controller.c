#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "nrf_fprintf_format.h"

#include "leds.h"
#include "usart.h"
#include "uart_controller.h"

#define UART_LOCAL_RX_BUF_SIZE (64)
#define UART_LOCAL_TX_BUF_SIZE (64)

#ifndef UART_BAUDRATE
#define UART_BAUDRATE (115200)
#endif

#ifndef UART_FLOWCONTROL
#define UART_FLOWCONTROL (false)
#endif

static uart_rx_cb_t uart_rx_cb = NULL;
static int uart_echo_mode = UART_ECHO_OFF;
static void uart_rcv_cb(uint8_t *ch, size_t n);
static void uart_receive_one(uint8_t ch);

/*
 * @brief This function sets the current ECHO mode
 * 
 * @param echo - UART_ECHO_ON to switch echo mode on, UART_ECHO_OFF to switch echo mode off
 */
void uart_set_echo(uart_echo_mode_t echo)
{
    uart_echo_mode = echo;
}

/*
 * @brief This function returns the current ECHO mode
 * 
 * @return UART_ECHO_ON if echo mode on, or UART_ECHO_OFF if echo mode off
 */
uart_echo_mode_t uart_get_echo(void)
{
    return uart_echo_mode;
}

/*
 * @brief This function configures UART's parameters
 * 
 * @param cb Pointer to the call back that handles lines received by UART
 */
void setup_uart(uart_rx_cb_t cb)
{
    uint32_t baud = UART_BAUDRATE;
    bool flow_ctrl = UART_FLOWCONTROL;
    uart_flow_control_e flow;
    flow_ctrl ? (flow = UART_FLOW_CONTROL_HW) :
                (flow = UART_FLOW_CONTROL_NONE);
    uart_rx_cb = cb;
    Usart_init(baud, UART_FLOW_CONTROL_NONE);
    // Set up the custom handler for rx interrupt 
    Usart_enableReceiver((serial_rx_callback_f)uart_rcv_cb);
    Usart_setEnabled(true);
    Usart_receiverOn();
}

/*
 * @brief Call back function that is called from nrf_fprintf module
 * to send the formatted buffer to UART
 * 
 * @param p_context Pointer to the nrf_fprintf module context.
 * @param buffer the formatted buffer to send to UART
 * @param len Lenght of buffer
 */
static void serial_tx(void const * p_context, char const * buffer, size_t len)
{
    Usart_sendBuffer((const uint8_t *)buffer, len);
    Usart_flush();
}

/*
 * @brief Wrapper function over nrf_fprintf_fmt
 * 
 * @param p_fmt Pointer to the string that contains the text to be written to UART.
 * It can optionally contain embedded format tags that are replaced by the values
 * specified in subsequent additional arguments and formatted as requested. 
 */
void uart_fprintf(char const *p_fmt, ...)
{
    static char uart_tx_buf[UART_LOCAL_TX_BUF_SIZE];

    static nrf_fprintf_ctx_t fprintf_ctx = {
        .p_io_buffer = uart_tx_buf,
        .io_buffer_size = UART_LOCAL_TX_BUF_SIZE,
        .io_buffer_cnt = 0,
        .auto_flush = true,
        .p_user_ctx = NULL,
        .fwrite = serial_tx
    };

    if (p_fmt == NULL){
        return;
    }

    va_list args = {0};
    va_start(args, p_fmt);

    nrf_fprintf_fmt(&fprintf_ctx, p_fmt, &args);

    va_end(args);
}

static void uart_rcv_cb(uint8_t *ch, size_t n) 
{
    for(size_t i = 0; i < n; i++) {
        uart_receive_one(ch[i]);
    }
}
/*
 * @brief Handle the character have been received by uart interrupt handler
 *
 * @param ch Received character
 */
static void uart_receive_one(uint8_t ch)
{   
    static char uart_buf[UART_LOCAL_RX_BUF_SIZE];
    static char *p_buf = uart_buf;

    // Ignore '\n' character
    if(ch == '\n')
        return;

    *p_buf = ch;

    // If echo mode is on, retransmit received character
    if(uart_echo_mode == UART_ECHO_ON) {
        Usart_sendBuffer(p_buf, 1);
    }

    // If '\r' is received, the whole line is received
    if(*p_buf == '\r'){
        p_buf++;
        *p_buf = 0;
    }

    // If the end of the buffer is reached, finish the receiving line
    if (p_buf == (uart_buf + UART_LOCAL_RX_BUF_SIZE - 1))
        *p_buf = 0;
    if (*p_buf == 0) {
        // The whole line is received, call the call back to process it
        p_buf = uart_buf;
        if(uart_rx_cb)
            uart_rx_cb(p_buf);
        return;
    }
    p_buf++;
}
