# You can add more sources here if needed
include $(SRCS_PATH)waps/makefile
include $(SRCS_PATH)drivers/makefile

# dma uart is used by default (if it exists on platform)
uart_use_dma ?= yes

# HAL API is needed by WAPS for UART
HAL_UART=yes
UART_USE_DMA=$(uart_use_dma)


INCLUDES += -I$(SRCS_PATH)

# Define default baudrate and hardware flow control settings
uart_br ?= 125000
uart_fc ?= false

CFLAGS += -DUART_BAUDRATE=$(uart_br)
CFLAGS += -DUART_FLOWCONTROL=$(uart_fc)
