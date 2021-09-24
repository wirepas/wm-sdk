MCU_PREFIX := $(MCU_PATH)$(MCU_FAMILY)/$(MCU)/
MCU_CONFIG := $(MCU_PREFIX)/config.mk
MCU_COMMON := $(MCU_PATH)$(MCU_FAMILY)/common

# Add include path for nrfx vendor files
INCLUDES += -I$(MCU_COMMON)/vendor/

# Add include path for efr common header files
INCLUDES += -I$(MCU_COMMON)/

# Include mcu specific configuration
-include $(MCU_CONFIG)


# Include MCU specific HAL drivers
-include $(MCU_PREFIX)/hal/makefile

