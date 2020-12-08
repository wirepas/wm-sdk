# You can add more sources here and libraries if needed
SRCS += $(SRCS_PATH)drivers/$(MCU)/flash.c
INCLUDES += -I$(SRCS_PATH)drivers/ -I$(BUILDPREFIX_APP)
LIBS +=
LDFLAGS += -Wl,--section-start=.bl_hex=0x41000

# Use LED1 (if present on the board) to display bootloader update status
HAL_LED=yes
