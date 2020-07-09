ifeq ($(APP_SCHEDULER), yes)
SRCS += $(WP_LIB_PATH)app_scheduler.c
endif

ifeq ($(APP_PRINTING), yes)
CFLAGS += -DAPP_PRINTING
HAL_UART=yes
SRCS += $(WP_LIB_PATH)syscalls.c
SRCS += $(WP_LIB_PATH)uart_print.c
endif

ifeq ($(SHARED_LIBDATA), yes)
$(error Shared_LibData library as been renamed, please see libraries/shared_libdata.h for an explanation.)
endif

ifeq ($(SHARED_DATA), yes)
SRCS += $(WP_LIB_PATH)shared_data.c
endif

ifeq ($(PROVISIONING), yes)
SRCS += $(WP_LIB_PATH)provisioning/data.c
SRCS += $(WP_LIB_PATH)provisioning/joining.c
SRCS += $(WP_LIB_PATH)provisioning/provisioning.c
INCLUDES += -I$(WP_LIB_PATH)provisioning
endif

ifeq ($(PROVISIONING_PROXY), yes)
SRCS += $(WP_LIB_PATH)provisioning/proxy.c
INCLUDES += -I$(WP_LIB_PATH)provisioning
endif