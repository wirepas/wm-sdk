# Version of GCC used for Wirepas testing
GCC_TESTED_VERSIONS := 4.8.4, 7.2.1

# General SDK folder structure
MCU_COMMON_SRCS_PATH := mcu/common/
API_PATH := api/
UTIL_PATH := util/
HAL_API_PATH := mcu/hal_api/
WP_LIB_PATH := libraries/
GLOBAL_BUILD := build/
BOARDS_PATH := board/

# General compiler flags (Define it before specific makefile in order to allow app to overwrite it)
CFLAGS  := -Wall -Werror -Wextra
CFLAGS  += -std=gnu99 -mthumb -nostartfiles -lgcc -lnosys -ggdb --specs=nano.specs
CFLAGS  += -Os -ffunction-sections -fdata-sections

# Flags for linker
LDFLAGS := -Wl,--gc-sections

# include global config file
-include config.mk

#
# Tools
#
# Prefix for Arm tools
PREFIX := $(arm_toolchain)arm-none-eabi-

# Toolchain programs
CC        := $(PREFIX)gcc
AR        := $(PREFIX)ar
OBJCOPY   := $(PREFIX)objcopy
RM        := rm
MV        := mv
CP        := cp
MKDIR     := mkdir -p
SCRAT_GEN := python tools/genscratchpad.py
HEXTOOL   := python tools/hextool.py
FMW_SEL   := python tools/firmware_selector.py
BOOT_CONF := python tools/bootloader_config.py
WIZARD    := python tools/sdk_wizard.py
MAKE      := make

# Check the toolchain version with GCC
GCC_VERSION := $(shell $(CC) -dumpversion)
ifneq ($(GCC_VERSION), $(findstring $(GCC_VERSION), $(GCC_TESTED_VERSIONS)))
$(warning ***********************************************************************)
$(warning "GCC version used is not one of the versions recommended and tested by Wirepas )
$(warning "Recommended versions are: $(GCC_TESTED_VERSIONS))
$(warning ***********************************************************************)
endif

# Optional suffix for application folder
# It can be used to build several versions of same application
# with different parameters: app_config0/ and app_config1/
APP_BUILD_SUFFIX := $(app_build_suffix)

# Name of app
APP_NAME := $(app_name)$(APP_BUILD_SUFFIX)

# List of available boards found under board/
AVAILABLE_BOARDS := $(patsubst $(BOARDS_PATH)%/,%,$(sort $(dir $(wildcard $(BOARDS_PATH)*/.))))

# Generic name of stack
FIRMWARE_NAME := wpc_stack


# Include board specific config
-include board/$(target_board)/config.mk

# Include mcu specific config
-include mcu/$(MCU)/config.mk

# Folder for Wirepas stack binary image
IMAGE_PATH := image/

# Add new flags as board and mcu are known
CFLAGS += -DTARGET_BOARD=$(target_board)
CFLAGS += -DMCU=$(MCU)
CFLAGS += -DMCU_SUB=$(MCU_SUB)

MCU_UPPER=$(shell echo $(MCU) | tr a-z A-Z)
CFLAGS += -D$(MCU_UPPER)

CFLAGS += -march=$(ARCH)

INCLUDES += -Imcu/$(MCU) -Imcu/$(MCU)/hal -Imcu/$(MCU)/vendor -Imcu/$(MCU)/cmsis -Iboard/$(target_board)

# Folder where the application sources are located (and config file)
APP_SRCS_PATH := source/$(app_name)/
ifeq (,$(wildcard $(APP_SRCS_PATH)))
$(error App $(app_name) doesn't exist)
endif

# Include app specific config
-include $(APP_SRCS_PATH)config.mk

# Build prefixes
BUILDPREFIX := $(GLOBAL_BUILD)$(target_board)/
BUILDPREFIX_APP := $(BUILDPREFIX)$(APP_NAME)/
# Stack is under a app specific folder as config may depend on app
BUILDPREFIX_STACK := $(BUILDPREFIX_APP)stack/
# Bootloader is under a app specific folder as config may depend on app (unlocked/locked)
BUILDPREFIX_BOOTLOADER := $(BUILDPREFIX_APP)bootloader/
BUILDPREFIX_TEST_BOOTLOADER := $(BUILDPREFIX_APP)bootloader_test/

BOOTLOADER_HEX := $(BUILDPREFIX_BOOTLOADER)bootloader.hex
BOOTLOADER_TEST_HEX := $(BUILDPREFIX_TEST_BOOTLOADER)bootloader_test.hex

STACK_HEX := $(BUILDPREFIX_STACK)$(FIRMWARE_NAME).hex

APP_HEX := $(BUILDPREFIX_APP)$(APP_NAME).hex
