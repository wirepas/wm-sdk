MCU_PREFIX := $(MCU_PATH)$(MCU_FAMILY)/$(MCU)/
MCU_CONFIG := $(MCU_PREFIX)config.mk
VENDOR_PREFIX := $(MCU_PATH)$(MCU_FAMILY)/common/vendor/
NRF_FAMILY_HAL_PREFIX  := $(MCU_PATH)$(MCU_FAMILY)/common/hal/

# Add include path for nrfx vendor files
INCLUDES += -I$(VENDOR_PREFIX)
INCLUDES += -I$(VENDOR_PREFIX)mdk/
INCLUDES += -I$(VENDOR_PREFIX)templates/
INCLUDES += -I$(VENDOR_PREFIX)drivers/

# Add include path for nrfx common header files
INCLUDES += -I$(MCU_PATH)$(MCU_FAMILY)/common/

# Include mcu specific configuration
-include $(MCU_CONFIG)

# Include MCU specific HAL drivers
-include $(MCU_PREFIX)hal/makefile

# Build common HAL drivers for nrf-family
ifeq ($(HAL_GPIO), yes)
SRCS += $(NRF_FAMILY_HAL_PREFIX)gpio.c
endif

ifeq ($(HAL_SPI), yes)
SRCS += $(NRF_FAMILY_HAL_PREFIX)spi.c
endif

ifeq ($(HAL_PERSISTENT_MEMORY), yes)
SRCS += $(NRF_FAMILY_HAL_PREFIX)persistent.c
endif

ifeq ($(HAL_HW_DELAY), yes)
SRCS += $(NRF_FAMILY_HAL_PREFIX)hw_delay.c
endif
SRCS += $(NRF_FAMILY_HAL_PREFIX)hal.c
