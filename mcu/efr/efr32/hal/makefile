EFR32_HAL_PREFIX := $(MCU_PREFIX)hal/
EFR32_VENDOR_PREFIX :=$(MCU_COMMON)/vendor/

# Add include path for efr32 specific HAL header files
INCLUDES += -I$(EFR32_HAL_PREFIX)

ifeq ($(HAL_GPIO), yes)
SRCS += $(EFR32_HAL_PREFIX)gpio.c
endif

ifeq ($(HAL_UART), yes)
INCLUDES += -I$(EFR32_HAL_PREFIX)usart/series$(EFR32_SERIES)
ifeq ($(UART_USE_DMA), yes)
SRCS += $(EFR32_HAL_PREFIX)usart/usart_dma.c \
        $(EFR32_VENDOR_PREFIX)em_ldma.c \
        $(EFR32_VENDOR_PREFIX)em_usart.c
else # DMA
ifeq ($(MCU_SUB), xg23)
SRCS += $(EFR32_HAL_PREFIX)usart/eusart.c
SRCS += $(EFR32_VENDOR_PREFIX)em_eusart.c
else # xG23
SRCS += $(EFR32_HAL_PREFIX)usart/usart.c
SRCS += $(EFR32_VENDOR_PREFIX)em_usart.c
endif # xG23
endif # DMA
SRCS += $(EFR32_VENDOR_PREFIX)$(HAL_SYSTEM_C)
endif # UART



ifeq ($(HAL_I2C), yes)
SRCS += $(EFR32_HAL_PREFIX)i2c/i2c_series$(EFR32_SERIES).c 
SRCS += $(EFR32_VENDOR_PREFIX)$(HAL_SYSTEM_C)
endif

ifneq (,$(filter $(MCU_SUB), xg21 xg23 xg24 ))
INCLUDES += -I$(EFR32_VENDOR_PREFIX)rail_lib/chip/efr32/efr32xg2x
INCLUDES += -I$(EFR32_VENDOR_PREFIX)rail_lib/common
endif


ifeq ($(HAL_VOLTAGE), yes) 
ifeq ($(EFR32_SERIES), 2)
SRCS += $(EFR32_VENDOR_PREFIX)em_iadc.c \
        $(EFR32_VENDOR_PREFIX)$(HAL_SYSTEM_C)
endif
SRCS += $(EFR32_HAL_PREFIX)voltage_series_$(EFR32_SERIES).c
endif

ifeq ($(HAL_PRS), yes)
SRCS += $(EFR32_VENDOR_PREFIX)em_prs.c
endif

ifeq ($(HAL_SPI), yes) 
ifneq (,$(filter $(MCU_SUB), xg22 xg23 xg24 ))
SRCS += $(EFR32_HAL_PREFIX)spi.c
ifeq ($(MCU_SUB), xg23)
SRCS += $(EFR32_VENDOR_PREFIX)em_usart.c \
        $(EFR32_VENDOR_PREFIX)$(HAL_SYSTEM_C)
else
ifneq ($(HAL_UART), yes)
SRCS += $(EFR32_VENDOR_PREFIX)em_usart.c \
        $(EFR32_VENDOR_PREFIX)$(HAL_SYSTEM_C)
endif
endif
else
$(error Board $(target_board) doesn't support SPI (xg22, xg23, xg24 only))
endif
endif

SRCS += $(EFR32_HAL_PREFIX)power.c \
        $(EFR32_HAL_PREFIX)hal.c   \
        $(EFR32_VENDOR_PREFIX)em_emu.c\
        $(EFR32_VENDOR_PREFIX)em_cmu.c \
        $(EFR32_VENDOR_PREFIX)em_core.c \
        $(EFR32_VENDOR_PREFIX)em_gpio.c
