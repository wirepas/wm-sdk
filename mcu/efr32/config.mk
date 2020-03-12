# Mcu instruction set
ARCH=armv7e-m

# This mcu has a bootloader (enough memory)
HAS_BOOTLOADER=yes

ifeq ($(MCU)$(MCU_SUB)$(MCU_MEM_VAR),efr32xg12pxxxf1024)
	# Hardware magic used for this architecture with 1024kB Flash, 128kB RAM
  	HW_MAGIC=05
    CFLAGS += -DEFR32FG12 -DEFR32FG12P232F1024GL125
else ifeq ($(MCU)$(MCU_SUB)$(MCU_MEM_VAR),efr32xg12pxxxf512)
	# Hardware magic used for this architecture with 512kB Flash, 64kB RAM
  	HW_MAGIC=07
    CFLAGS += -DEFR32FG12 -DEFR32FG12P232F1024GL125
else ifeq ($(MCU)$(MCU_SUB)$(MCU_MEM_VAR),efr32xg13pxxxf512)
    HW_MAGIC=08
    CFLAGS += -DEFR32FG13 -DEFR32FG13P233F512GM48
else
	$(error "Invalid MCU configuration $(MCU)$(MCU_SUB)$(MCU_MEM_VAR)!")
endif

# Add custom flags
# Remove the -Wunused-parameter flag added by -Wextra as some cortex M4 header do not respect it
CFLAGS += -Wno-unused-parameter

INCLUDES += -Imcu/$(MCU)/cmsis

# This mcu uses the version 3 of the bootloader (with external flash support)
BOOTLOADER_VERSION=v3
