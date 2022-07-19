NRF52_HAL_PREFIX := $(MCU_PREFIX)hal/

# Add include path for nrf52 specific HAL header files
INCLUDES += -I$(NRF52_HAL_PREFIX)

# Build nrf52 specific HAL drivers
ifeq ($(HAL_UART), yes)
# Check version
ifeq ($(UART_USE_DMA), yes)
SRCS += $(NRF52_HAL_PREFIX)usart_dma.c
else ifeq ($(UART_USE_USB), yes)
ifeq ($(filter 833 840,$(MCU_SUB)),)
$(error Board $(target_board) doesn't support USB (only nrf52833 or nrf52840))
else
$(info Uart used with USB)
endif
include $(NRF52_HAL_PREFIX)usb_uart/makefile
SRCS += $(NRF52_HAL_PREFIX)usart_usb.c
else
SRCS += $(NRF52_HAL_PREFIX)usart.c
endif
endif

ifeq ($(HAL_I2C), yes)
SRCS += $(NRF52_HAL_PREFIX)i2c.c
endif

ifeq ($(HAL_VOLTAGE), yes)
# Different subprocessors have different
ifeq ($(MCU_SUB), 832)
SRCS += $(NRF52_HAL_PREFIX)voltage_52832.c
else
SRCS += $(NRF52_HAL_PREFIX)voltage_52833_52840.c
endif
endif

SRCS += $(NRF52_HAL_PREFIX)power.c