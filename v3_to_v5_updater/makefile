include makefile

.DEFAULT_GOAL := updater

HEX_UPDATE   := python v3_to_v5_updater/hexupdater.py

BUILDPREFIX_UPDATER := $(BUILDPREFIX)updater/
BOOTLOADER_UPDATER_NAME := $(MCU)$(MCU_SUB)_updater
BOOTLOADER_UPDATER_HEX := $(BUILDPREFIX_UPDATER)$(BOOTLOADER_UPDATER_NAME).hex
BOOTLOADER_UPDATER_OTAP := $(BUILDPREFIX_UPDATER)final_$(APP_NAME)_updater.otap

ifeq ($(MCU)$(MCU_SUB)$(MCU_MEM_VAR),efr32xg12pxxxf1024)
	FW_AREA_ID = 0x00000005
else ifeq ($(MCU)$(MCU_SUB)$(MCU_MEM_VAR),nrf52832)
	FW_AREA_ID = 0x00000003
else
	$(error Specified target board $(target_board) ($(MCU)$(MCU_SUB)$(MCU_MEM_VAR)) is not suported by the updater tool)
endif

.PHONY: updater
updater: $(BOOTLOADER_HEX) $(STACK_HEX) $(APP_HEX) $(BOOTLOADER_CONFIG_INI)
	+$(MAKE)  -f makefile_bootloader.mk
	+$(MAKE)  -f makefile_stack.mk
	+$(MAKE)  -f makefile_app.mk
	+$(MAKE)  -f v3_to_v5_updater/makefile_updater.mk BUILDPREFIX_UPDATER=$(BUILDPREFIX_UPDATER) BOOTLOADER_UPDATER_NAME=$(BOOTLOADER_UPDATER_NAME)
ifeq ($(MCU),efr32)
		$(HEX_UPDATE) $(STACK_HEX) -o $(BUILDPREFIX_UPDATER)wsn_stack.hex
else
		$(CP) $(STACK_HEX) $(BUILDPREFIX_UPDATER)wsn_stack.hex
endif
	$(HEXTOOL) $(BOOTLOADER_HEX) --delete 0xfe04000-0xfe04200  -o $(BUILDPREFIX_UPDATER)bootloader.hex
	$(eval output_file:=$(BUILDPREFIX_UPDATER)combined.hex)
	$(HEXTOOL) --output=hex:$(output_file) hex:$(BUILDPREFIX_UPDATER)wsn_stack.hex hex:$(BOOTLOADER_UPDATER_HEX) hex@0x3B000:$(BUILDPREFIX_UPDATER)bootloader.hex
	$(SCRAT_GEN)    --configfile=$(BOOTLOADER_CONFIG_INI) \
	                --otapseq=$(otap_seq_number) \
	                --set=APP_AREA_ID=$(app_area_id) \
	                $(BOOTLOADER_UPDATER_OTAP) \
	                $(patsubst %.hex,%.conf,$(STACK_HEX)):$(FW_AREA_ID):$(output_file) \
	                $(app_major).$(app_minor).$(app_maintenance):$(app_area_id):$(APP_HEX)

.PHONY: clean_updater
clean_updater:
	+$(MAKE)  -f makefile_bootloader.mk clean
	+$(MAKE)  -f makefile_app.mk clean
	+$(MAKE)  -f v3_to_v5_updater/makefile_updater.mk clean
	$(RM) -rf $(BUILDPREFIX_UPDATER)$(APP_NAME)_updater.otap
