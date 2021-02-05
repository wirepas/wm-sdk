# Dual MCU app needs UART
HAL_UART=yes
ifeq ($(uart_use_usb),yes)
UART_USE_DMA=no
UART_USE_USB=yes
CFLAGS += -DUART_USE_USB
else ifeq ($(uart_use_dma),no)
UART_USE_DMA=no
UART_USE_USB=no
else
# dma uart is used by default if not explicitly set to no
UART_USE_DMA=yes
UART_USE_USB=no
endif

INCLUDES += -I$(SRCS_PATH)

# Define default baudrate and hardware flow control settings
uart_br ?= 125000
uart_fc ?= false

CFLAGS += -DUART_BAUDRATE=$(uart_br)
CFLAGS += -DUART_FLOWCONTROL=$(uart_fc)

# Use App Scheduler and configure it
APP_SCHEDULER=yes
CFLAGS += -DAPP_SCHEDULER_MAX_TASKS=3

include $(SRCS_PATH)waps/makefile
include $(SRCS_PATH)drivers/makefile
