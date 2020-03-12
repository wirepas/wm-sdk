include makefile_common.mk

.DEFAULT_GOAL := all


$(STACK_HEX): FORCE
	# Get the right stack from the image folder
	$(MKDIR) $(@D)
	@$(FMW_SEL)	--firmware_path=$(IMAGE_PATH)\
				--firmware_type="wp_stack"\
				--output_path=$(@D)\
				--output_name="wpc_stack"\
				--mcu=$(MCU)\
				--mcu_sub=$(MCU_SUB)$(MCU_MEM_VAR)\
				--mac_profile=$(mac_profile)\
				--mac_profileid=$(mac_profileid)\
				--radio=$(radio)\
				--support_test=$(test_library)\
				--version=$(stack_version)

.PHONY: all
all: $(STACK_HEX)

clean:
	$(RM) -rf $(STACK_HEX)

# Special ruel to force other rule to run every time
FORCE:
