include makefile_common.mk

.DEFAULT_GOAL := all

# Linker for the updater
LDSCRIPT := v3_to_v5_updater/linker/$(MCU)$(MCU_SUB)/gcc_bl_updater_$(MCU)$(MCU_SUB)$(MCU_MEM_VAR).ld

BUILDPREFIX_UPDATER := $(BUILDPREFIX)updater/
BOOTLOADER_UPDATER_NAME := $(MCU)$(MCU_SUB)_updater
BOOTLOADER_UPDATER_ELF := $(BUILDPREFIX_UPDATER)$(BOOTLOADER_UPDATER_NAME).elf
BOOTLOADER_UPDATER_HEX := $(BUILDPREFIX_UPDATER)$(BOOTLOADER_UPDATER_NAME).hex

#
# Sources & includes paths
#
SRCS += v3_to_v5_updater/drivers/$(MCU)$(MCU_SUB)/flash.c
SRCS += v3_to_v5_updater/main.c

CFLAGS += -Iv3_to_v5_updater/drivers/
CFLAGS += -Imcu/$(MCU)/

# Objects list
OBJS_ = $(SRCS:.c=.o)
OBJS = $(addprefix $(BUILDPREFIX_UPDATER), $(OBJS_))

# Files to be cleaned
CLEAN := $(OBJS) $(BOOTLOADER_UPDATER_ELF) $(BOOTLOADER_UPDATER_HEX)

$(BUILDPREFIX_UPDATER)%.o : %.c
	$(MKDIR) $(@D)
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

$(BOOTLOADER_UPDATER_ELF): $(OBJS)
	$(MKDIR) $(@D)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ \
	      -Wl,-Map=$(BUILDPREFIX_UPDATER)$(BOOTLOADER_UPDATER_NAME).map \
	      -Wl,-T,$(LDSCRIPT) $(LIBS)

$(BOOTLOADER_UPDATER_HEX): $(BOOTLOADER_UPDATER_ELF)
	@echo "Generating $(BOOTLOADER_UPDATER_HEX)"
	$(OBJCOPY) $< -O ihex $@

.PHONY: all
all: $(BOOTLOADER_UPDATER_HEX)

clean:
	$(RM) -rf $(CLEAN)
