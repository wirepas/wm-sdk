include makefile_common.mk

.DEFAULT_GOAL := all


stack_mode?=normal
mac_profile?=ism_24_ghz

$(STACK_HEX): FORCE
	# Get the right stack from the image folder
	$(MKDIR) $(@D)
	@$(FMW_SEL)	--firmware_path=$(IMAGE_PATH)\
				--firmware_type="wp_stack"\
				--output_path=$(@D)\
				--output_name="wpc_stack"\
				--mcu=$(MCU)\
				--mcu_sub=$(MCU_SUB)\
				--mcu_mem_var=$(MCU_MEM_VAR)\
				--mac_profile=$(mac_profile)\
				--mac_profileid=$(mac_profileid)\
				--mode=$(stack_mode)\
				--radio=$(radio)\
				--radio_config=$(radio_config)\
				--version=$(MIN_STACK_VERSION)

.PHONY: all
all: $(STACK_HEX)

clean:
	$(RM) -rf $(STACK_HEX)

# Special ruel to force other rule to run every time
FORCE:
