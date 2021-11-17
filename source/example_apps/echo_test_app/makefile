
# HAL API is needed by Klika_app
HAL_UART=yes
include $(HAL_API_PATH)makefile

# You can add more sources here if needed
SRCS += $(SRCS_PATH)leds.c\
		$(SRCS_PATH)uart_controller.c\
		$(SRCS_PATH)cmd.c\
		$(SRCS_PATH)fprintf/nrf_fprintf_format.c\
		$(SRCS_PATH)fprintf/nrf_fprintf.c
INCLUDES += -I$(SRCS_PATH)fprintf -Imcu/$(MCU)/vendor

uart_br ?= 115200
uart_fc ?= false

UART_USE_DMA=no

CFLAGS += -DUART_BAUDRATE=$(uart_br)
CFLAGS += -DUART_FLOWCONTROL=$(uart_fc)

# Define OTAP settings
#CFLAGS += -D$(app_otap_version)

# Define default network settings
CFLAGS += -DNETWORK_ADDRESS=$(default_network_address)
CFLAGS += -DNETWORK_CHANNEL=$(default_network_channel)

# Generic util functions are needed (api.c)
include $(UTIL_PATH)makefile
