# Mcu instruction set
ARCH=armv8-m.main
CFLAGS += -mfloat-abi=hard -mfpu=fpv5-sp-d16
# Libraries to be build for Cortex-M33
CM33 := yes
CFLAGS += -DARM_MATH_ARMV8MML
# This mcu has a bootloader (enough memory)
HAS_BOOTLOADER=yes

CFLAGS += -DNRF91_PLATFORM
mac_profile?=dect_nr_19_ghz
radio?=none
ifeq ($(MCU_SUB),60)
	# Hardware magic used for this architecture
	HW_MAGIC=12
	HW_VARIANT_ID=12
else
	$(error "Invalid MCU_SUB for nrf91! $(MCU_SUB) only 60  supported")
endif

# Add custom flags
# Remove the -Wunused-parameter flag added by -Wextra as some cortex M4 header do not respect it
CFLAGS += -Wno-unused-parameter

# This mcu uses the version 3 of the bootloader (with external flash support)
BOOTLOADER_VERSION=v3
