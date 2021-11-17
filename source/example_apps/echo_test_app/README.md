# Echo test application

## Purpose

This is a simple apllication to perfom "echo" tests over the wirepas network.

Actions performed:

 - Sending a data packet every 5 seconds. The data packet is a counter, 4 bytes length. The counter is incremented by 1 after every transmitting.

 - Receiving data packets at endpoint 14 and retransmiting them to the wirepas network - ECHO TEST.

 - Transmitting info about the node configuration (network address, network channel, node address, information about presence of the authentication and encryption keys) over UART

 - Transmitting info about received and transmited packets over UART

## Requirements

nrf52832/nrf52840 DK

Communication over the virtual serial port (/dev/ttyACM*)

- UART speed - 115200
- Stop bits - 1
- Parity - none
- Length - 8
- Flow control - none

## Build

To build the main version type

```
make app_name=echo_test_app clean
make app_name=echo_test_app
```

To build the alternative version type

```
make app_name=echo_test_app clean
make app_name=echo_test_app app_otap_version=OTAP_VERSION_ALT
```

## General format of command

Write Command:
```   
WRP+<command name>[=options]<CR>
```

Read Command:
```
WRP+<command name>?<CR>
```
Response:
```    
<CR><LF><reponse><CR><LF>
```
Error Response:
```
<CR><LF>Error: <error description><CR><LF>
CR - The Carriage Return character (0x0D, \r)
LF - The Line Feed character (0x0A, \n)
```

### List of commands
- WRP+NODE:     set/get Node Address
- WRP+NETWORK:  set/get Network Address
- WRP+CHNL:     set/get Network Channel
- WRP+FW:       get Wirepas Stack Firmware Version
- WRP+BTL:      get Bootloader Version
- WRP+STACK:    set/get Stack State
- WRP+VER:      get Application version
- WRP+AKEY:     set/get Authentication Key
- WRP+EKEY:     set/get Encryption Key
- WRP+ECHO:     set/get UART echo mode
- WRP+WRPPR:    set/get Wirepas print mode

### The command description

**WRP+NODE - set/get Node Address**

|Request                    |Responce  
|---------------------------|----------
|WRP+NODE=\<address\>       |OK
|WRP+NODE?                  |+NODE=\<address\>

**WRP+NETWORK - set/get Network Address**

|Request                    |Responce  
|---------------------------|----------
|WRP+NETWORK=\<address\>    |OK
|WRP+NETWORK?               |+NETWORK=\<address\>

**WRP+CHNL: set/get Network Channel**

|Request                    |Responce 
|---------------------------|----------
|WRP+CHNL=\<channel\>       |OK
|WRP+CHNL?                  |+CHNL=\<channel\>

**WRP+FW: get Wirepas Stack Firmware Version**

|Request                    |Responce 
|---------------------------|----------
|WRP+FW=\<version\>         |OK
|WRP+FW?                    |+FW=\<version\>

**WRP+BTL: get Bootloader Firmware Version**

|Request                    |Responce 
|---------------------------|----------
|WRP+BTL=\<version\>        |OK
|WRP+BTL?                   |+BTL=\<version\>

**WRP+STACK: set/get Stack State**

The read command of this request returns the byte of the stack state
The write command is intended to start or stop of the stack

|Request                    |Responce
|---------------------------|----------
|WRP+STACK=start\|stop      |OK
|WRP+STACK?                 |+STACK=\<value\>

**WRP+VER: get Application version number**

|Request                    |Responce
|---------------------------|----------
|WRP+VER                    |OK
|WRP+VER?                   |+VER=\<application version\>

**WRP+AKEY: set/get Authentication Key**

Key format - HEX string represents 16 bytes array

```
WRP+AKEY=00112233445566778899AABBCCDDEEFF
```

|Request                    |Responce
|---------------------------|----------
|WRP+AKEY=\<key\>           |OK
|WRP+AKEY?                  |+AKEY=set\|notset

**WRP+AKEY: set/get Encryption Key**

Key format - HEX string represents 16 bytes array

```
WRP+EKEY=00112233445566778899AABBCCDDEEFF
```

|Request                    |Responce
|---------------------------|----------
|WRP+EKEY=\<key\>           |OK
|WRP+EKEY?                  |+EKEY=set\|notset

**WRP+ECHO: set/get UART echo mode**

Enable/disable echo over UART during typing the commands

|Request                    |Responce
|---------------------------|----------
|WRP+ECHO=on\|off           |OK
|WRP+ECHO?                  |+ECHO=on\|off

**WRP+WRPPR: set/get Wirepas print mode**

Enable/disable printing Wirepass traffic messages over UART

|Request                    |Responce
|---------------------------|----------
|WRP+WRPPR=on\|off          |OK
|WRP+WRPPR?                 |+WRPPR=on\|off