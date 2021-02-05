EFR32_HAL_PREFIX := $(MCU_PREFIX)hal/
EFR32_VENDOR_PREFIX := $(MCU_PREFIX)vendor/

ifeq ($(HAL_UART), yes)
ifeq ($(UART_USE_DMA), yes)
SRCS += $(EFR32_HAL_PREFIX)usart_dma.c \
        $(EFR32_VENDOR_PREFIX)em_ldma.c \
        $(EFR32_VENDOR_PREFIX)em_core.c
else
SRCS += $(EFR32_HAL_PREFIX)usart.c
endif
SRCS += $(EFR32_VENDOR_PREFIX)em_usart.c \
        $(EFR32_VENDOR_PREFIX)$(HAL_SYSTEM_C)
endif

ifeq ($(HAL_BUTTON), yes)
SRCS += $(EFR32_HAL_PREFIX)button.c
endif

ifeq ($(HAL_LED), yes)
SRCS += $(EFR32_HAL_PREFIX)led.c
endif

SRCS += $(EFR32_HAL_PREFIX)ds.c    \
        $(EFR32_HAL_PREFIX)power.c \
        $(EFR32_HAL_PREFIX)hal.c   \
        $(EFR32_VENDOR_PREFIX)em_emu.c\
        $(EFR32_VENDOR_PREFIX)em_cmu.c \
        $(EFR32_VENDOR_PREFIX)em_gpio.c