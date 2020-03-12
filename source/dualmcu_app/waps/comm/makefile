COMM_PREFIX := $(WAPS_PREFIX)comm/

SRCS += $(COMM_PREFIX)uart/waps_uart.c \
        $(COMM_PREFIX)uart/waps_uart_power.c

ifeq ($(waps_diagnostics),yes)
    $(info PROFILE: waps diagnostics)
    CFLAGS += -DWAPS_DIAGNOSTICS
endif
