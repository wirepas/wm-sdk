# You can add more sources here if needed
SRCS += bootloader/early_init.c
SRCS += bootloader/external_flash.c
SRCS += bootloader/bl_hardware.c

INCLUDES += -Ibootloader/

ifneq ("$(wildcard $(BOARD_FOLDER)/bootloader/custom_early_init.c)","")
	SRCS += $(BOARD_FOLDER)/bootloader/custom_early_init.c
endif

ifneq ("$(wildcard $(BOARD_FOLDER)/bootloader/makefile)","")
    include $(BOARD_FOLDER)/bootloader/makefile
endif

ifneq ("$(wildcard $(APP_SRCS_PATH)/bootloader/app_early_init.c)","")
	SRCS += $(APP_SRCS_PATH)/bootloader/app_early_init.c
endif

ifneq ("$(wildcard $(APP_SRCS_PATH)/bootloader/makefile)","")
    include $(APP_SRCS_PATH)/bootloader/makefile
endif
