#ifndef CMD_H
#define CMD_H

#ifdef OTAP_VERSION_ALT
#define APPLICATION_VERSION "alt-1.0.0"
#else
#define APPLICATION_VERSION "main-1.0.0"
#endif // OTAP_VERSION_MAIN

void uart_rx_cb(char *str);
int get_wrp_print_msg_st(void);
void set_wrp_print_msg_st(int st);
uint8_t get_auto_start(void);

#endif // CMD_H