# Build generic utility functions
SRCS += $(UTIL_PATH)crc.c     \
        $(UTIL_PATH)sl_list.c \
        $(UTIL_PATH)util.c    \
        $(UTIL_PATH)api.c     \
        $(UTIL_PATH)pack.c \
        $(UTIL_PATH)random.c \
        $(UTIL_PATH)tlv.c    \
        $(UTIL_PATH)syscalls.c

ifeq ($(APP_PRINTING), yes)
CFLAGS += -DAPP_PRINTING
HAL_UART=yes
SRCS += $(UTIL_PATH)uart_print.c
endif
ifeq ($(APP_PRINTING), custom)
CFLAGS += -DAPP_PRINTING
endif

ifeq ($(WIRESHARK), yes)
ifeq ($(APP_PRINTING), yes)
# Both are using same uart so cannot be used together
$(error APP_PRINTING and WIRESHARK option are mutually exclusive and cannot both be set to yes)
endif
ifneq ($(SHARED_DATA), yes)
$(error WIRESHARK util lib can only be used if SHARED_DATA is also used in your application)
endif
WIRESHARK_PATH = $(UTIL_PATH)wireshark/
CFLAGS += -DWIRESHARK
INCLUDES += -I$(WIRESHARK_PATH)
HAL_UART=yes
# HAL_UART_BUFFER_SIZE=2048 // Not supported yet
SRCS += $(WIRESHARK_PATH)wireshark.c
endif

ifeq ($(TINY_CBOR), yes)
CBOR_PATH = $(UTIL_PATH)tinycbor/src/
SRCS += $(CBOR_PATH)cborencoder.c \
        $(CBOR_PATH)cborencoder_close_container_checked.c \
        $(CBOR_PATH)cborerrorstrings.c \
        $(CBOR_PATH)cborparser.c \
        $(CBOR_PATH)cborparser_dup_string.c \
        $(CBOR_PATH)cborpretty.c \
        $(CBOR_PATH)cborpretty_stdio.c \
        $(CBOR_PATH)cbortojson.c \
        $(CBOR_PATH)cborvalidation.c
INCLUDES += -I$(CBOR_PATH)
CFLAGS += -DNDEBUG
endif

ifeq ($(SW_AES), yes)
AES_PATH = $(UTIL_PATH)tinyaes/
SRCS += $(AES_PATH)aes.c \
        $(UTIL_PATH)aessw.c
INCLUDES += -I$(AES_PATH)
endif