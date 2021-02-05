NRF52_HAL_PREFIX := $(MCU_PREFIX)hal/

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

ifeq ($(HAL_SPI), yes)
SRCS += $(NRF52_HAL_PREFIX)spi.c
endif

ifeq ($(HAL_I2C), yes)
SRCS += $(NRF52_HAL_PREFIX)i2c.c
endif

ifeq ($(HAL_BUTTON), yes)
SRCS += $(NRF52_HAL_PREFIX)button.c
endif

ifeq ($(HAL_LED), yes)
SRCS += $(NRF52_HAL_PREFIX)led.c
endif

ifeq ($(HAL_PERSISTENT_MEMORY), yes)
SRCS += $(NRF52_HAL_PREFIX)persistent.c
endif

ifeq ($(HAL_HW_DELAY), yes)
SRCS += $(NRF52_HAL_PREFIX)hw_delay.c
endif

SRCS += $(NRF52_HAL_PREFIX)ds.c             \
        $(NRF52_HAL_PREFIX)power.c          \
        $(NRF52_HAL_PREFIX)hal.c
