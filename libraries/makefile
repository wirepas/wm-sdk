ifeq ($(SHARED_LIBDATA), yes)
$(error Shared_LibData library as been renamed, please see libraries/shared_libdata.h for an explanation.)
endif

ifeq ($(SHARED_DATA), yes)
SRCS += $(WP_LIB_PATH)shared_data/shared_data.c
INCLUDES += -I$(WP_LIB_PATH)shared_data
endif

provisioning_tasks=0
ifeq ($(PROVISIONING), yes)
APP_SCHEDULER=yes
provisioning_tasks=2
SRCS += $(WP_LIB_PATH)provisioning/data.c
SRCS += $(WP_LIB_PATH)provisioning/joining.c
SRCS += $(WP_LIB_PATH)provisioning/provisioning.c
INCLUDES += -I$(WP_LIB_PATH)provisioning
endif

ifeq ($(PROVISIONING_PROXY), yes)
SRCS += $(WP_LIB_PATH)provisioning/proxy.c
INCLUDES += -I$(WP_LIB_PATH)provisioning
endif

# TODO move elsewhere util?
ifeq ($(APP_PRINTING), yes)
CFLAGS += -DAPP_PRINTING
HAL_UART=yes
SRCS += $(WP_LIB_PATH)syscalls.c
SRCS += $(WP_LIB_PATH)uart_print.c
endif

ifeq ($(APP_SCHEDULER), yes)
library_tasks=$(provisioning_tasks)
SRCS += $(WP_LIB_PATH)scheduler/app_scheduler.c
INCLUDES += -I$(WP_LIB_PATH)scheduler -DAPP_SCHEDULER_LIBRARY_TASKS=$(library_tasks)
endif