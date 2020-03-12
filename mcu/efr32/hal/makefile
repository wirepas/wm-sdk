EFR32_HAL_PREFIX := $(MCU_PREFIX)hal/

ifeq ($(HAL_UART), yes)
SRCS += $(EFR32_HAL_PREFIX)hfperclk.c
ifeq ($(UART_USE_DMA), yes)
SRCS += $(EFR32_HAL_PREFIX)usart_dma.c
else
SRCS += $(EFR32_HAL_PREFIX)usart.c
endif
endif

ifeq ($(HAL_BUTTON), yes)
SRCS += $(EFR32_HAL_PREFIX)button.c
endif

ifeq ($(HAL_LED), yes)
SRCS += $(EFR32_HAL_PREFIX)led.c
endif

SRCS += $(EFR32_HAL_PREFIX)ds.c    \
        $(EFR32_HAL_PREFIX)power.c \
        $(EFR32_HAL_PREFIX)hal.c
