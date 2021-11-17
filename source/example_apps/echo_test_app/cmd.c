#include <string.h>
#include <ctype.h>

#include "api.h"
#include "node_configuration.h"
#include "hal_api.h"

#include "cmd.h"
#include "uart_controller.h"
#include "usart.h"

#define UART_EXECUTION_TIME_US 1000
#define RECEIVE_BUF_SIZE 64
static char global_receive_buf[ RECEIVE_BUF_SIZE ];

typedef int (*cmd_cb_t)(const char *);

void uppercase ( char *sPtr );
int cmd_check(const char *str, const char *tmpl, cmd_cb_t cb);
uint32_t setSysPeriodicCb(void);

static int wrp_print_msg_st = 0;

/*
 * @brief Return the value of auto start flag from the persistent memory
 */
uint8_t get_auto_start(void)
{
    uint8_t auto_start;
    lib_storage->readPersistent(&auto_start, sizeof(auto_start));
    return auto_start;
}

/*
 * @brief Save the value of auto start flag within the persistent memory,
 * if set it to 0, the application will not launch the wirepas stack
 * at startup and it will launch the wirepas stack otherwise
 * 
 * @param auto_start Value of auto start flag
 */
void set_auto_start(uint8_t auto_start)
{
    lib_storage->writePersistent(&auto_start, sizeof(auto_start));
}


int get_wrp_print_msg_st(void)
{
    return wrp_print_msg_st;
}

void set_wrp_print_msg_st(int st)
{
    wrp_print_msg_st = st;
}

int cmd_NODE(const char *str)
{
    app_addr_t node_addr;
    app_res_e err_code;

    if(str[0] == '\r'){
        uart_fprintf("\r\nOK\r\n");
        return 0;
    }
    if(str[0] == '?'){
        err_code = lib_settings->getNodeAddress(&node_addr);
        if (err_code != APP_RES_OK){
            uart_fprintf("\r\nERROR: getNodeAddress error code %02X\r\n", err_code);
            return -1;
        }
        uart_fprintf("\r\n+NODE=%X\r\n", node_addr);
        return 0;
    }
    if(*str == '='){
        str++;
        if(isdigit((uint8_t)*str) == 0){
            uart_fprintf("\r\nERROR: Node Address shoul be a number\r\n");
            return -1;
        }
        
        // Parse string to the node address
        node_addr = strtoul(str, NULL, 0);
        
        err_code = lib_settings->setNodeAddress(node_addr);
        if (err_code != APP_RES_OK){
            uart_fprintf("\r\nERROR: setNodeAddress error code %02X\r\n", err_code);
            return -1;
        }
        uart_fprintf("\r\nOK\r\n");
        return 0;
    }
    uart_fprintf("\r\nERROR: Bad command format\r\n");
    return -1;
}

int cmd_NETWORK(const char *str)
{
    app_lib_settings_net_addr_t network_addr;
    app_res_e err_code;

    if(str[0] == '\r'){
        uart_fprintf("\r\nOK\r\n");
        return 0;
    }
    if(str[0] == '?'){
        err_code = lib_settings->getNetworkAddress(&network_addr);
        if (err_code != APP_RES_OK){
            uart_fprintf("\r\nERROR: getNetworkAddress error code %02X\r\n", err_code);
            return -1;
        }
        uart_fprintf("\r\n+NETWORK=%X\r\n", network_addr);
        return 0;
    }
    if(*str == '='){
        str++;
        if(isdigit((uint8_t)*str) == 0){
            uart_fprintf("\r\nERROR: Network Address shoul be a number\r\n");
            return -1;
        }
        
        // Parse string to the node address
        network_addr = strtoul(str, NULL, 0);
        
        err_code = lib_settings->setNetworkAddress(network_addr);
        if (err_code != APP_RES_OK){
            uart_fprintf("\r\nERROR: setNetworkAddress error code %02X\r\n", err_code);
            return -1;
        }
        uart_fprintf("\r\nOK\r\n");
        return 0;
    }
    uart_fprintf("\r\nERROR: Bad command format\r\n");
    return -1;
}

int cmd_CHNL(const char *str)
{
    app_lib_settings_net_channel_t network_ch;
    app_res_e err_code;

    if(str[0] == '\r'){
        uart_fprintf("\r\nOK\r\n");
        return 0;
    }
    if(str[0] == '?'){
        err_code = lib_settings->getNetworkChannel(&network_ch);
        if (err_code != APP_RES_OK){
            uart_fprintf("\r\nERROR: getNetworkChannel error code %02X\r\n", err_code);
            return -1;
        }
        uart_fprintf("\r\n+CHNL=%X\r\n", network_ch);
        return 0;
    }
    if(*str == '='){
        str++;
        if(isdigit((uint8_t)*str) == 0){
            uart_fprintf("\r\nERROR: Channel shoul be a number\r\n");
            return -1;
        }
        
        // Parse string to the node address
        network_ch = strtoul(str, NULL, 0);
        
        err_code = lib_settings->setNetworkChannel(network_ch);
        if (err_code != APP_RES_OK){
            uart_fprintf("\r\nERROR: setNetworkChannel error code %02X\r\n", err_code);
            return -1;
        }
        uart_fprintf("\r\nOK\r\n");
        return 0;
    }
    uart_fprintf("\r\nERROR: Bad command format\r\n");
    return -1;
}

int cmd_FW(const char *str)
{
    app_firmware_version_t fw;

    if(str[0] == '\r'){
        uart_fprintf("\r\nOK\r\n");
        return 0;
    }
    if(str[0] == '?'){
        fw = global_func->getStackFirmwareVersion();
        uart_fprintf("\r\n+FW=%02X.%02X.%02X.%02X\r\n",   fw.major,
                                                    fw.minor,
                                                    fw.maint,
                                                    fw.devel);
        return 0;
    }
    uart_fprintf("\r\nERROR: Bad command format\r\n");
    return -1;
}

int cmd_BTL(const char *str)
{
    if(str[0] == '\r'){
        uart_fprintf("\r\nOK\r\n");
        return 0;
    }
    if(str[0] == '?'){
        uart_fprintf("\r\n+BTL=%X\r\n", lib_system->getBootloaderVersion());
        return 0;
    }
    uart_fprintf("\r\nERROR: Bad command format\r\n");
    return -1;
}

int cmd_VER(const char *str)
{
    if(str[0] == '\r'){
        uart_fprintf("\r\nOK\r\n");
        return 0;
    }
    if(str[0] == '?'){
        uart_fprintf("\r\n+VER=%s\r\n", APPLICATION_VERSION);
        return 0;
    }
    uart_fprintf("\r\nERROR: Bad command format\r\n");
    return -1;
}

/*
 * @brief Convert string to 16 byte array of the cipher key
 * 
 * @param str String represantation of the cipher key,
 *            the string should contain 16 hex octets
 * @param key Pointer to 16 byte array to store the cipher key
 * 
 * @return 0 if success, negative number otherwise
 */
int parseKey(const char *str, uint8_t *key)
{
    int i;
    char *next_ptr;
    char octet[3] = {0};

    for(i = 0; i < 16; i++){
        memcpy(octet, str, 2); 
        key[i] = strtoul(octet, &next_ptr, 16);
        if(next_ptr == octet){
            return -1;
        }
        str = str + 2;
    }
    return 0;
}

int cmd_EKEY(const char *str)
{
    uint8_t key[16];
    
    if(str[0] == '\r'){
        uart_fprintf("\r\nOK\r\n");
        return 0;
    }
    if(str[0] == '?'){
        if (lib_settings->getEncryptionKey(NULL) == APP_RES_OK)
            uart_fprintf("\r\n+EKEY=SET\r\n");
        else
            uart_fprintf("+EKEY=NOTSET\r\n");
        return 0;
    }
    if(str[0] == '='){
        str++;
        // Parse string to key
         if(parseKey(str, key)){
            uart_fprintf("\r\nERROR: Bad key format\r\n");
            return -1;
        }
        // Set new address
        if (lib_settings->setEncryptionKey(key) != APP_RES_OK){
            uart_fprintf("\r\nERROR: Bad key\r\n");
            return -1;
        }
        
        uart_fprintf("\r\nOK\r\n");
        return 0;
    }
    uart_fprintf("\r\nERROR: Bad command format\r\n");
    return -1;
}

int cmd_AKEY(const char *str)
{
    uint8_t key[16];

    if(str[0] == '\r'){
        uart_fprintf("\r\nOK\r\n");
        return 0;
    }
    if(str[0] == '?'){
        if (lib_settings->getAuthenticationKey(NULL) == APP_RES_OK)
            uart_fprintf("\r\n+AKEY=SET\r\n");
        else
            uart_fprintf("+AKEY=NOTSET\r\n");
        return 0;
    }
    if(str[0] == '='){
        str++;
        // Parse string to key
         if(parseKey(str, key)){
            uart_fprintf("\r\nERROR: Bad key format\r\n");
            return -1;
        }
        // Set new address
        if (lib_settings->setAuthenticationKey(key) != APP_RES_OK){
            uart_fprintf("\r\nERROR: Bad key\r\n");
            return -1;
        }
        
        uart_fprintf("\r\nOK\r\n");
        return 0;
    }
    uart_fprintf("\r\nERROR: Bad command format\r\n");
    return -1;
}

int cmd_STACK(const char *str)
{
    app_res_e err_code = APP_RES_UNSPECIFIED_ERROR;
    static char start[] = "START";
    static char stop[] = "STOP";

    if(str[0] == '\r'){
        uart_fprintf("\r\nOK\r\n");
        return 0;
    }
    if(str[0] == '?'){
        uart_fprintf("\r\n+STACK=%X\r\n", lib_state->getStackState());
        return 0;
    }
    if(*str == '='){
        str++;
        if(strncmp(str, start, strlen(start)) == 0){
            set_auto_start(0xFF);
            err_code = lib_state->startStack();
        }else if(strncmp(str, stop, strlen(stop)) == 0){
            set_auto_start(0x00);
            uart_fprintf("\r\nOK\r\n");
            err_code = lib_state->stopStack();
        }
        
        if (err_code != APP_RES_OK){
            uart_fprintf("\r\nERROR: set stack state error code %02X\r\n", err_code);
            return -1;
        }
        uart_fprintf("\r\nOK\r\n");
        return 0;
    }
    uart_fprintf("\r\nERROR: Bad command format\r\n");
    return -1;
}

int cmd_ECHO(const char *str)
{
    static char on[] = "ON";
    static char off[] = "OFF";
    int rez = -1;

    if(str[0] == '\r'){
        uart_fprintf("\r\nOK\r\n");
        return 0;
    }
    if(str[0] == '?'){
        if(uart_get_echo() == UART_ECHO_ON)
            uart_fprintf("\r\n+ECHO=ON\r\n");
        else
            uart_fprintf("\r\n+ECHO=OFF\r\n");
        return 0;
    }
    if(*str == '='){
        str++;
        if(strncmp(str, on, strlen(on)) == 0){
            uart_set_echo(UART_ECHO_ON);
            rez = 0;
        }else if(strncmp(str, off, strlen(off)) == 0){
            uart_set_echo(UART_ECHO_OFF);
            rez = 0;
        }
        if(rez == 0){
            uart_fprintf("\r\nOK\r\n");
            return 0;
        }
    }
    uart_fprintf("\r\nERROR: Bad command format\r\n");
    return rez;
}

int cmd_WRPPR(const char *str)
{
    static char on[] = "ON";
    static char off[] = "OFF";
    int rez = -1;

    if(str[0] == '\r'){
        uart_fprintf("\r\nOK\r\n");
        return 0;
    }
    if(str[0] == '?'){
        if(get_wrp_print_msg_st())
            uart_fprintf("\r\n+WRPPR=ON\r\n");
        else
            uart_fprintf("\r\n+WRPPR=OFF\r\n");
        return 0;
    }
    if(*str == '='){
        str++;
        if(strncmp(str, on, strlen(on)) == 0){
            set_wrp_print_msg_st(1);
            rez = 0;
        }else if(strncmp(str, off, strlen(off)) == 0){
            rez = 0;
            set_wrp_print_msg_st(0);
        }
        if(rez == 0){
            uart_fprintf("\r\nOK\r\n");
            return 0;
        }
    }
    uart_fprintf("\r\nERROR: Bad command format\r\n");
    return rez;
}

/*
 * @brief Handle the received by UART line
 */
uint32_t uart_rx_periodic_cb(void) {

    char *str = global_receive_buf;

    // Move the received line to uppercase
    uppercase(str);
    static char prefix[] = "WRP+";

    // Check the line has the right prfix
    if(strncmp(str, prefix, sizeof(prefix) - 1) == 0){
        str = str + sizeof(prefix) - 1; // skip the prefix "WRP+"

        // Get/Set Node Address
        cmd_check(str, "NODE", cmd_NODE);
        // Get/Set Network Address
        cmd_check(str, "NETWORK", cmd_NETWORK);
        // Get/Set Network Channel
        cmd_check(str, "CHNL", cmd_CHNL);
        // Get Wirepas Firmware Version
        cmd_check(str, "FW", cmd_FW);
        // Get Bootloader Version
        cmd_check(str, "BTL", cmd_BTL);
        // Get Application version
        cmd_check(str, "VER", cmd_VER);
        // Get/Set Authentication Key
        cmd_check(str, "AKEY", cmd_AKEY);
        // Get/Set Encryption Key
        cmd_check(str, "EKEY", cmd_EKEY);
        // Get/Set Stack State
        cmd_check(str, "STACK", cmd_STACK);
        // Get/Set UART echo mode
        cmd_check(str, "ECHO", cmd_ECHO);
        // Get/Set Wirepas print mode
        cmd_check(str, "WRPPR", cmd_WRPPR);
    }
    // After the request is handled, reset the system periodic call back
    return setSysPeriodicCb();
}

/*
 * @brief This function is the call back function called from UART rx handler
 * 
 * @param str Received line
 */
void uart_rx_cb(char *str)
{
    strncpy(global_receive_buf, str, sizeof(global_receive_buf));
    // Use periodic call back to handle the interrupt ASAP
    lib_system->setPeriodicCb(uart_rx_periodic_cb,
                              0,
                              UART_EXECUTION_TIME_US);
}

/*
 * @brief Compare the beginning of string with the command template and
 * if they matched then calls the command handler
 * 
 * @param str Comparing string
 * @param tmpl Command template
 * @param cb Pointer to the command handler
 * 
 * @return 0 if success, negative number otherwise
 */
int cmd_check(const char *str, const char *tmpl, cmd_cb_t cb)
{
    int len = strlen(tmpl);
    
    if(strncmp(str, tmpl, len) == 0)
        return cb(str + len);
    return -1;
}

/*
 * @brief Convert string to uppercase
 * 
 * @param str String to be converted
 */
void uppercase ( char *sPtr )
{
    while ( *sPtr != '\0' ) {
        *sPtr = toupper ( ( unsigned char ) *sPtr );
        sPtr++;
    }
}