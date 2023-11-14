# Build generic utility functions
SRCS += $(UTIL_PATH)crc.c     \
        $(UTIL_PATH)sl_list.c \
        $(UTIL_PATH)util.c    \
        $(UTIL_PATH)api.c     \
        $(UTIL_PATH)pack.c \
        $(UTIL_PATH)random.c \
        $(UTIL_PATH)tlv.c

ifeq ($(APP_PRINTING), yes)
CFLAGS += -DAPP_PRINTING
HAL_UART=yes
SRCS += $(UTIL_PATH)syscalls.c
SRCS += $(UTIL_PATH)uart_print.c
endif


ifeq ($(APP_PRINTING_VIA_RTT), yes)
CFLAGS += -DAPP_PRINTING_VIA_RTT
RTT_PATH = $(UTIL_PATH)RTT/
SRCS += $(RTT_PATH)SEGGER_RTT.c \
        $(RTT_PATH)SEGGER_RTT_printf.c \
        $(RTT_PATH)SEGGER_RTT_Syscalls_GCC.c \
        $(UTIL_PATH)syscalls.c
# ASM_SRCS += $(RTT_PATH)SEGGER_RTT_ASM_ARMv7M.s
INCLUDES += -I$(RTT_PATH)
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