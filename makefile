# Global makefile to build images to be flashed
# or updated through OTAP

include makefile_common.mk

#
# Targets
#

# Scratchpads for OTAP
FULL_SCRATCHPAD_NAME := $(APP_NAME)_$(FIRMWARE_NAME)
FULL_SCRATCHPAD_BIN := $(BUILDPREFIX_APP)$(FULL_SCRATCHPAD_NAME).otap
APP_SCRATCHPAD_NAME := $(APP_NAME)
APP_SCRATCHPAD_BIN := $(BUILDPREFIX_APP)$(APP_SCRATCHPAD_NAME).otap
STACK_SCRATCHPAD_NAME := $(FIRMWARE_NAME)
STACK_SCRATCHPAD_BIN := $(BUILDPREFIX_APP)$(STACK_SCRATCHPAD_NAME).otap

BOOTLOADER_CONFIG_INI := $(BUILDPREFIX_APP)bootloader_full_config.ini
PLATFORM_CONFIG_INI := $(MCU_PATH)$(MCU_FAMILY)/$(MCU)/ini_files/$(MCU)$(MCU_SUB)$(MCU_MEM_VAR)_platform.ini

CLEAN += $(FULL_SCRATCHPAD_BIN) $(APP_SCRATCHPAD_BIN) $(STACK_SCRATCHPAD_BIN) $(BOOTLOADER_CONFIG_INI)

# Final image for programming
FINAL_IMAGE_NAME := final_image_$(APP_NAME)
FINAL_IMAGE_HEX := $(BUILDPREFIX_APP)$(FINAL_IMAGE_NAME).hex

# Hidden file to know if the License was at least displayed one time
# and accepted
LICENSE_ACCEPTED := .license_accepted

CLEAN += $(FINAL_IMAGE_HEX)

# Add targets
TARGETS += $(FINAL_IMAGE_HEX) otap

#
# Manage area id and ini files
#

# Define the app_area as a combination of app_area and HW_VARIANT_ID
ifeq ($(app_specific_area_id),)
$(error You must define a specific area id in your application config.mk file)
endif
app_area_id=$(app_specific_area_id)$(HW_VARIANT_ID)

# Defines the default firmware_area_id
firmware_area_id=0x000001$(HW_MAGIC)

# Set Default scratchpad ini file if not overriden by app makefile
ifneq ($(INI_FILE),)
$(error You are overriding the default flash partitioning with INI_FILE variable. \
	You must now override INI_FILE_APP variable with a modified version of \
	$(MCU_PATH)$(MCU_FAMILY)/$(MCU)/ini_files/$(MCU)$(MCU_SUB)$(MCU_MEM_VAR)_app.ini)
endif

INI_FILE_WP ?= $(MCU_PATH)$(MCU_FAMILY)/$(MCU)/ini_files/$(MCU)$(MCU_SUB)$(MCU_MEM_VAR)_wp.ini
INI_FILE_APP ?=$(MCU_PATH)$(MCU_FAMILY)/$(MCU)/ini_files/$(MCU)$(MCU_SUB)$(MCU_MEM_VAR)_app.ini

# By default the key file is positionned at root of SDK and used for all apps/boards
# But it can be overwritten by any app (it will be generated at first execution if needed)
KEY_FILE ?= ./custom_bootloader_keys.ini

# Default values used by bootloader updater app. They can overwritten through command line.
UPDATER_INI_FILE_WP ?= $(INI_FILE_WP)
UPDATER_INI_FILE_APP ?= $(INI_FILE_APP)
UPDATER_KEY_FILE ?= $(KEY_FILE)
updater_app_specific_area_id?=$(app_specific_area_id)
updater_app_area_id=$(updater_app_specific_area_id)$(HW_VARIANT_ID)

#
# Functions
define BUILD_FULL_SCRATCHPAD
	@echo "  Creating Full Scratchpad: $(2) + $(3) -> $(1)"
	$(SCRAT_GEN)    --configfile=$(BOOTLOADER_CONFIG_INI) \
	                $(1) \
	                $(patsubst %.hex,%.conf,$(2)):$(firmware_area_id):$(2) \
	                $(app_major).$(app_minor).$(app_maintenance).$(app_development):$(app_area_id):$(3)
endef

define BUILD_HEX
	@echo "  Creating Flashable Hex: $(2) + $(3) + $(4) + $(5) -> $(1)"
	$(HEX_GEN)      --configfile=$(BOOTLOADER_CONFIG_INI) \
					--bootloader=$(2) \
	                $(1) \
	                $(patsubst %.hex,%.conf,$(3)):$(firmware_area_id):$(3) \
	                $(app_major).$(app_minor).$(app_maintenance).$(app_development):$(app_area_id):$(4) \
	                $(5)
endef

define BUILD_APP_SCRATCHPAD
	@echo "  Creating App Scratchpad: $(2) -> $(1)"
	$(SCRAT_GEN)    --configfile=$(BOOTLOADER_CONFIG_INI) \
	                $(1) \
	                $(app_major).$(app_minor).$(app_maintenance).$(app_development):$(app_area_id):$(2)
endef

define BUILD_STACK_SCRATCHPAD
	@echo "  Creating Stack Scratchpad: $(2) -> $(1)"
	$(SCRAT_GEN)    --configfile=$(BOOTLOADER_CONFIG_INI) \
	                $(1) \
	                $(patsubst %.hex,%.conf,$(2)):$(firmware_area_id):$(2)
endef

# Params: (1) Final image (2) bootloader (3) Ini file (4) Test app
define BUILD_BOOTLOADER_TEST_APP
	@echo "  Creating test application for bootloader: $(2) + $(3) + $(4) -> $(1)"
	$(eval output_file:=$(BUILDPREFIX_TEST_BOOTLOADER)temp_file.hex)
	$(HEX_GEN)      --configfile=$(3) \
					--bootloader=$(2) \
	                $(1) \
	                1.0.0.0:$(firmware_area_id):$(4)
endef

.PHONY: all app_only otap
all: $(TARGETS)

app_only: $(APP_HEX) $(APP_SCRATCHPAD_BIN)

otap: $(FULL_SCRATCHPAD_BIN) $(APP_SCRATCHPAD_BIN) $(STACK_SCRATCHPAD_BIN)

bootloader: $(BOOTLOADER_HEX)

need_board:
	# Check if target board is defined
	$(if $(target_board),,$(error No target_board defined.\
	        Please specify one with target_board=<..> argument from command line.\
	        Available boards are: $(AVAILABLE_BOARDS)\
	        A default value can be set in main config.mk file))

	# Check if board really exist
	@test -s $(BOARD_FOLDER)/config.mk || \
		   { echo "Specified target board $(target_board) doesn't exist. Available boards are: $(AVAILABLE_BOARDS)"; exit 1; }

initial_setup: $(LICENSE_ACCEPTED) $(KEY_FILE)
	@ # Rule to ensure initial setup is done

$(KEY_FILE): | $(LICENSE_ACCEPTED)
	@	# Run the wizard to create key file
	@	# It depends on LICENSE to avoid error when building with -j option
	@	# | (pipe) is intentional to avoid regenerating key file if license is newer
	$(WIZARD) --gen_keys -o $@

$(LICENSE_ACCEPTED):
	@cat LICENSE.txt
	@echo -e "\n\nWirepas SDK is covered by the above License that must be read and accepted.\n\
	For additionnal questions or clarifications, please contact sales@wirepas.com.\n"

	@echo -n -e "\nDo you accept the License Terms? [y/N] " && read ans && [ $${ans:-N} = y ]
	@touch $@

# Add $(STACK_HEX) to PHONY to always call firmware makefile
.PHONY: $(STACK_HEX)
$(STACK_HEX): initial_setup need_board
	@	# Call app makefile to get the hex file of stack
	+$(MAKE) -f makefile_stack.mk

# This rule override APP_HEX rule when building the bootloader updater
$(BOOTLOADER_UPDATER_HEX):: $(BOOTLOADER_UPDATER_DATA_BIN)

# Add $(APP_HEX) to PHONY to always call app makefile
.PHONY: $(APP_HEX)
$(APP_HEX):: initial_setup $(BUILDPREFIX_APP) need_board
	@	# Call app makefile to get the hex file of app
	+$(MAKE) -f makefile_app.mk

# Add $(BOOTLOADER_HEX) to PHONY to always call bootloader makefile
.PHONY: $(BOOTLOADER_HEX)
$(BOOTLOADER_HEX): initial_setup need_board
	@	# Call bootloader makefile to get the hex file of bootloader
	+$(MAKE) -f makefile_bootloader.mk

.PHONY: $(BOOTLOADER_TEST_HEX)
$(BOOTLOADER_TEST_HEX): initial_setup need_board $(BOOTLOADER_CONFIG_INI)
	@	# Call bootloader test makefile to get the test application hex file
	+$(MAKE) -f makefile_bootloader_test.mk

$(STACK_SCRATCHPAD_BIN): initial_setup $(FIMWARE_HEX) $(BOOTLOADER_HEX) $(BUILDPREFIX_APP) $(BOOTLOADER_CONFIG_INI)
	$(call BUILD_STACK_SCRATCHPAD,$(STACK_SCRATCHPAD_BIN),$(STACK_HEX))

$(APP_SCRATCHPAD_BIN): initial_setup $(APP_HEX) $(BOOTLOADER_CONFIG_INI)
	$(call BUILD_APP_SCRATCHPAD,$(APP_SCRATCHPAD_BIN),$(APP_HEX))

$(FULL_SCRATCHPAD_BIN): initial_setup $(STACK_HEX) $(APP_HEX) $(BOOTLOADER_CONFIG_INI)
	$(call BUILD_FULL_SCRATCHPAD,$(FULL_SCRATCHPAD_BIN),$(STACK_HEX),$(APP_HEX))

$(FINAL_IMAGE_HEX): initial_setup $(STACK_HEX) $(APP_HEX) $(BOOTLOADER_HEX) $(BOOTLOADER_CONFIG_INI)
	$(call BUILD_HEX,$(FINAL_IMAGE_HEX),$(BOOTLOADER_HEX),$(STACK_HEX),$(APP_HEX),$(EXTRA_HEX))

$(BOOTLOADER_CONFIG_INI): initial_setup $(INI_FILE_WP) $(INI_FILE_APP) $(BUILDPREFIX_APP)
	@	# Rule to create the full config file based on multiple ini files and store it per build folder
	$(BOOT_CONF) -i $(INI_FILE_WP) -i $(INI_FILE_APP) -i $(KEY_FILE) -i $(PLATFORM_CONFIG_INI) -o $(BOOTLOADER_CONFIG_INI) -ol APP_AREA_ID:$(app_area_id) -ol STACK_AREA_ID:$(firmware_area_id)

bootloader_test: $(BOOTLOADER_HEX) $(BOOTLOADER_TEST_HEX) $(BOOTLOADER_CONFIG_INI)
	$(call BUILD_BOOTLOADER_TEST_APP,$(BUILDPREFIX_APP)final_bootloader_test.hex,$<,$(BOOTLOADER_CONFIG_INI),$(word 2,$^))

# Generates a binary containing the bootloader for the bootloader updater app.
$(BOOTLOADER_UPDATER_DATA_BIN): initial_setup $(BUILDPREFIX_APP) need_board $(BOOTLOADER_HEX)
	$(BOOT_CONF)   -i $(UPDATER_INI_FILE_WP) -i $(UPDATER_INI_FILE_APP) -i $(UPDATER_KEY_FILE) \
				   -o $(BUILDPREFIX_APP)bootloader_updater_config.ini \
				   -ol APP_AREA_ID:$(updater_app_area_id) \
				   -ol STACK_AREA_ID:$(firmware_area_id)
	$(HEX_GEN)     --configfile=$(BUILDPREFIX_APP)bootloader_updater_config.ini \
				   --bootloader=$(BOOTLOADER_HEX) \
	               $(BUILDPREFIX_APP)bootloader_with_config.hex
	$(HEX2ARRAY32) $(BUILDPREFIX_APP)bootloader_with_config.hex $(BUILDPREFIX_APP)bootloader_updater_data.bin

$(BUILDPREFIX_APP):
	$(MKDIR) $(@D)

.PHONY: doxygen
doxygen:
	@	# If build folder does not exist, create it
	mkdir -p $(GLOBAL_BUILD)
	doxygen projects/doxygen/Doxyfile.template
	@	# Replace search engine
	cp -rf projects/doxygen/search.js $(GLOBAL_BUILD)html/search/search.js
	rm $(GLOBAL_BUILD)html/search/search*.png
	echo "<script id=\"searchdata\" type=\"text/xmldata\">" >> $(GLOBAL_BUILD)html/search.html
	cat searchdata.xml >> $(GLOBAL_BUILD)html/search.html
	echo "</script>" >> $(GLOBAL_BUILD)html/search.html
	rm searchdata.xml

# clean the specified app
.PHONY: clean
clean: need_board
	+$(MAKE) -f makefile_app.mk clean
	+$(MAKE) -f makefile_bootloader.mk clean
	rm -rf $(BUILDPREFIX_APP)

# clean all the apps
.PHONY: clean_all
clean_all:
	rm -rf $(GLOBAL_BUILD)
