#ifndef UART_CONTROLLER_H
#define UART_CONTROLLER_H

typedef void (*uart_rx_cb_t)(char *);
typedef enum {
    UART_ECHO_OFF = 0,
    UART_ECHO_ON
}uart_echo_mode_t;

void setup_uart(uart_rx_cb_t cb);
void uart_set_echo(uart_echo_mode_t echo);
uart_echo_mode_t uart_get_echo(void);
uint32_t send_dummy_arguments(void);
void uart_send(uint8_t* buf, uint8_t length);
void uart_puts(const char *);
void uart_fprintf(char const *p_fmt, ...);

#endif