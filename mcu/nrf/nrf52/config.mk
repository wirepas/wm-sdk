# Mcu instruction set
ARCH=armv7e-m

# This mcu has a bootloader (enough memory)
HAS_BOOTLOADER=yes

CFLAGS += -DNRF52_PLATFORM

ifeq ($(MCU_SUB),832)
	# Hardware magic used for this architecture
	HW_MAGIC=03
	HW_VARIANT_ID=03
else ifeq ($(MCU_SUB),840)
	# Hardware magic used for this architecture
	HW_MAGIC=06
	HW_VARIANT_ID=06
else ifeq ($(MCU_SUB),833)
	# Hardware magic used for this architecture
	HW_MAGIC=09
	HW_VARIANT_ID=09
else
	$(error "Invalid MCU_SUB for nrf52! $(MCU_SUB) only 832, 833 and 840 supported")
endif

# Add custom flags
# Remove the -Wunused-parameter flag added by -Wextra as some cortex M4 header do not respect it
CFLAGS += -Wno-unused-parameter

# This mcu uses the version 3 of the bootloader (with external flash support)
BOOTLOADER_VERSION=v3
