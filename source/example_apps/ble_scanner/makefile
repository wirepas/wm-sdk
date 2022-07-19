# Adding ble scanner module
SRCS += $(SRCS_PATH)ble_scanner.c
INCLUDES += -I$(SRCS_PATH)

# Two tasks are needed for the scanner part
APP_SCHEDULER=yes
APP_SCHEDULER_TASKS=2

SHARED_DATA=yes
STACK_STATE_LIB=yes

HAL_LED=yes

ifeq ($(dualmcu_interface), yes)
# Enable dualmcu with UART and
# and dma to support high baudrate
HAL_UART=yes
UART_USE_DMA=yes
DUALMCU_LIB=yes
CFLAGS += -DDUALMCU_INTERFACE
else
APP_PRINTING=yes
endif
