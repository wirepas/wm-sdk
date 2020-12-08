# This mcu has a bootloader (enough memory)
HAS_BOOTLOADER=yes

ifeq ($(MCU)$(MCU_SUB)$(MCU_MEM_VAR),efr32xg12pxxxf1024)
    # Hardware magic used for this architecture with 1024kB Flash, 128kB RAM
    HW_MAGIC=05
    CFLAGS += -DEFR32FG12 -DEFR32FG12P232F1024GL125
    # Mcu instruction set
    ARCH=armv7e-m
    HAL_SYSTEM_C := efr32fg12/system_efr32fg12p.c
else ifeq ($(MCU)$(MCU_SUB)$(MCU_MEM_VAR),efr32xg12pxxxf512)
    # Hardware magic used for this architecture with 512kB Flash, 64kB RAM
    HW_MAGIC=07
    CFLAGS += -DEFR32FG12 -DEFR32FG12P232F1024GL125
    # Mcu instruction set
    ARCH=armv7e-m
    #HAL_SYSTEM_C := efr32fg12/system_efr32fg12p.c
else ifeq ($(MCU)$(MCU_SUB)$(MCU_MEM_VAR),efr32xg13pxxxf512)
    HW_MAGIC=08
    CFLAGS += -DEFR32FG13 -DEFR32FG13P233F512GM48
    # Mcu instruction set
    ARCH=armv7e-m
    HAL_SYSTEM_C := efr32fg13/system_efr32fg13p.c
else ifeq ($(MCU)$(MCU_SUB)$(MCU_MEM_VAR),efr32xg21xxxxf1024)
    HW_MAGIC=0A
    CFLAGS += -DEFR32MG21 -DEFR32MG21A010F1024IM32
    CFLAGS += -DARM_MATH_ARMV8MML
    CFLAGS += -mfloat-abi=hard -mfpu=fpv5-sp-d16
    # Mcu instruction set
    ARCH=armv8-m.main
    # Libraries to be build for Cortex-M33
    CM33 := yes
    HAL_SYSTEM_C := efr32mg21/Source/system_efr32mg21.c
    # Bootloader sanity check
    ifneq ($(board_hw_dcdc),no)
        $(error "DCDC is not supported by efr32xg21")
    endif
else ifeq ($(MCU)$(MCU_SUB)$(MCU_MEM_VAR),efr32xg22xxxxf512)
    HW_MAGIC=0B
    CFLAGS += -DEFR32MG22 -DEFR32MG22C224F512IM32
    CFLAGS += -DARM_MATH_ARMV8MML
    CFLAGS += -mfloat-abi=hard -mfpu=fpv5-sp-d16
    # Mcu instruction set
    ARCH=armv8-m.main
    # Libraries to be build for Cortex-M33
    CM33 := yes
    HAL_SYSTEM_C := efr32mg22/Source/system_efr32mg22.c
    # Bootloader sanity check
    ifneq ($(board_hw_dcdc),yes)
        $(error "DCDC must be enabled on efr32xg22")
    endif
    ifneq ($(board_hw_crystal_32k),yes)
        $(error "32kHz crystal must be installed on efr32xg22 board")
    endif
else
    $(error "Invalid MCU configuration $(MCU)$(MCU_SUB)$(MCU_MEM_VAR)!")
endif

CM33 ?= no

# Add custom flags
# Remove the -Wunused-parameter flag added by -Wextra as some cortex M4 header do not respect it
CFLAGS += -Wno-unused-parameter

INCLUDES += -Imcu/$(MCU)/cmsis

# This mcu uses the version 3 of the bootloader (with external flash support)
BOOTLOADER_VERSION=v3
