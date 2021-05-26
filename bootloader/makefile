# You can add more sources here if needed
SRCS += bootloader/early_init.c
SRCS += bootloader/external_flash.c
SRCS += bootloader/bl_hardware.c

INCLUDES += -Ibootloader/

ifneq ("$(wildcard board/$(target_board)/bootloader/custom_early_init.c)","")
	SRCS += board/$(target_board)/bootloader/custom_early_init.c
endif

ifneq ("$(wildcard board/$(target_board)/bootloader/makefile)","")
    include board/$(target_board)/bootloader/makefile
endif
