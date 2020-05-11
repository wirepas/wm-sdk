include makefile

.DEFAULT_GOAL := updater

HEX_UPDATE   := python v5_0_updater/hexupdater.py

BUILDPREFIX_UPDATER := $(BUILDPREFIX)updater/
BOOTLOADER_UPDATER_NAME := $(MCU)$(MCU_SUB)_updater
BOOTLOADER_UPDATER_HEX := $(BUILDPREFIX_UPDATER)$(BOOTLOADER_UPDATER_NAME).hex
BOOTLOADER_UPDATER_OTAP := $(BUILDPREFIX_UPDATER)final_$(BOOTLOADER_UPDATER_NAME).otap

.PHONY: updater
updater: $(BOOTLOADER_HEX) $(STACK_HEX) $(BOOTLOADER_CONFIG_INI)
	+$(MAKE) -f makefile_bootloader.mk
	+$(MAKE) -f makefile_stack.mk
	+$(MAKE) -f v5_0_updater/makefile_updater.mk BUILDPREFIX_UPDATER=$(BUILDPREFIX_UPDATER) BOOTLOADER_UPDATER_NAME=$(BOOTLOADER_UPDATER_NAME)
	$(HEX_UPDATE) $(STACK_HEX) -o $(BUILDPREFIX_UPDATER)wsn_stack.hex
	$(HEXTOOL) $(BOOTLOADER_HEX) --delete 0xfe04000-0xfe04200  -o $(BUILDPREFIX_UPDATER)bootloader.hex
	$(eval output_file:=$(BUILDPREFIX_UPDATER)combined.hex)
	$(HEXTOOL) --output=hex:$(output_file) hex:$(BUILDPREFIX_UPDATER)wsn_stack.hex hex:$(BOOTLOADER_UPDATER_HEX) hex@0x3B000:$(BUILDPREFIX_UPDATER)bootloader.hex
	$(SCRAT_GEN)    --configfile=$(BOOTLOADER_CONFIG_INI) \
	                --otapseq=$(otap_seq_number) \
	                --set=APP_AREA_ID=$(app_area_id) \
	                $(BOOTLOADER_UPDATER_OTAP) \
	                $(patsubst %.hex,%.conf,$(STACK_HEX)):$(firmware_area_id):$(output_file)

.PHONY: clean_updater
clean_updater:
	+$(MAKE) -f makefile_bootloader.mk clean
	+$(MAKE) -f v5_0_updater/makefile_updater.mk clean
	$(RM) -rf $(BOOTLOADER_UPDATER_OTAP)
