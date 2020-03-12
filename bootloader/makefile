# You can add more sources here if needed
SRCS += bootloader/early_init.c
SRCS += bootloader/external_flash.c

INCLUDES += -Ibootloader/

ifneq ("$(wildcard board/$(target_board)/bootloader/makefile)","")
	include board/$(target_board)/bootloader/makefile
endif
