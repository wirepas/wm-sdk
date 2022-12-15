EFR32_HAL_PREFIX := $(MCU_PREFIX)hal/
EFR32_VENDOR_PREFIX :=$(MCU_COMMON)/vendor/

# Add include path for nrf52 specific HAL header files
INCLUDES += -I$(EFR32_HAL_PREFIX)

ifeq ($(HAL_GPIO), yes)
SRCS += $(EFR32_HAL_PREFIX)gpio.c
SRCS += $(EFR32_VENDOR_PREFIX)em_core.c
endif

ifeq ($(HAL_UART), yes)
ifeq ($(MCU_SUB), xg23)
SRCS += $(EFR32_HAL_PREFIX)eusart.c
SRCS += $(EFR32_VENDOR_PREFIX)em_eusart.c
else
ifeq ($(UART_USE_DMA), yes)
SRCS += $(EFR32_HAL_PREFIX)usart_dma.c \
        $(EFR32_VENDOR_PREFIX)em_ldma.c \
        $(EFR32_VENDOR_PREFIX)em_core.c
else
SRCS += $(EFR32_HAL_PREFIX)usart.c
endif
SRCS += $(EFR32_VENDOR_PREFIX)em_usart.c
endif
SRCS += $(EFR32_VENDOR_PREFIX)$(HAL_SYSTEM_C)
endif

ifeq ($(HAL_I2C), yes)
SRCS += $(EFR32_HAL_PREFIX)i2c.c \
        $(EFR32_VENDOR_PREFIX)$(HAL_SYSTEM_C)
endif

ifeq ($(MCU_SUB), xg21)
INCLUDES += -I$(EFR32_VENDOR_PREFIX)rail_lib/chip/efr32/efr32xg2x
INCLUDES += -I$(EFR32_VENDOR_PREFIX)rail_lib/common
endif

ifeq ($(MCU_SUB), xg23)
INCLUDES += -I$(EFR32_VENDOR_PREFIX)rail_lib/chip/efr32/efr32xg2x
INCLUDES += -I$(EFR32_VENDOR_PREFIX)rail_lib/common
endif

ifeq ($(HAL_VOLTAGE), yes)
ifneq (,$(filter $(MCU_SUB), xg21 xg22))
SRCS += $(EFR32_VENDOR_PREFIX)em_iadc.c \
        $(EFR32_VENDOR_PREFIX)$(HAL_SYSTEM_C)
endif
ifeq ($(MCU_SUB), xg22)
SRCS += $(EFR32_VENDOR_PREFIX)em_core.c
endif
SRCS += $(EFR32_HAL_PREFIX)voltage.c
endif

ifeq ($(HAL_PRS), yes)
SRCS += $(EFR32_VENDOR_PREFIX)em_prs.c
endif

ifeq ($(HAL_SPI), yes)
ifneq (,$(filter $(MCU_SUB), xg22 xg23))
SRCS += $(EFR32_HAL_PREFIX)spi.c
ifneq ($(HAL_UART), yes)
SRCS += $(EFR32_VENDOR_PREFIX)em_usart.c \
        $(EFR32_VENDOR_PREFIX)$(HAL_SYSTEM_C)
endif
else
$(error Board $(target_board) doesn't support SPI (xg22 or xg23 only))
endif
endif

SRCS += $(EFR32_HAL_PREFIX)ds.c    \
        $(EFR32_HAL_PREFIX)power.c \
        $(EFR32_HAL_PREFIX)hal.c   \
        $(EFR32_VENDOR_PREFIX)em_emu.c\
        $(EFR32_VENDOR_PREFIX)em_cmu.c \
        $(EFR32_VENDOR_PREFIX)em_gpio.c
