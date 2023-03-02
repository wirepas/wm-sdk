# This mcu has a bootloader (enough memory)
HAS_BOOTLOADER=yes

CFLAGS += -DEFR32_PLATFORM

ifeq ($(MCU)$(MCU_SUB)$(MCU_MEM_VAR),efr32xg12pxxxf1024)
    # Hardware magic used for this architecture with 1024kB Flash, 128kB RAM
    HW_MAGIC=05
    HW_VARIANT_ID=05
    CFLAGS += -DEFR32FG12 -DEFR32FG12P232F1024GL125
    # Mcu instruction set
    ARCH=armv7e-m
    HAL_SYSTEM_C := efr32fg12/system_efr32fg12p.c
else ifeq ($(MCU)$(MCU_SUB)$(MCU_MEM_VAR),efr32xg12pxxxf512)
    # Hardware magic used for this architecture with 512kB Flash, 64kB RAM
    HW_MAGIC=07
    HW_VARIANT_ID=07
    CFLAGS += -DEFR32FG12 -DEFR32FG12P232F1024GL125
    # Mcu instruction set
    ARCH=armv7e-m
    HAL_SYSTEM_C := efr32fg12/system_efr32fg12p.c
else ifeq ($(MCU)$(MCU_SUB),efr32xg21)
    ifeq ($(radio),bgm210pa22jia)
        HW_MAGIC=0C
        HW_VARIANT_ID=0E
    else
        HW_MAGIC=0A
        ifeq ($(MCU_MEM_VAR), xxxxf1024)
            HW_VARIANT_ID=0A
            CFLAGS += -DEFR32MG21 -DEFR32MG21A010F1024IM32
        else ifeq ($(MCU_MEM_VAR), xxxxf768)
            HW_VARIANT_ID=0D
            CFLAGS += -DEFR32MG21 -DEFR32MG21A010F768IM32
        else ifeq ($(MCU_MEM_VAR), xxxxf512)
            HW_VARIANT_ID=0C
            CFLAGS += -DEFR32MG21 -DEFR32MG21A010F512IM32
        else
             $(error "Invalid xg21 memory variant $(MCU)$(MCU_SUB)$(MCU_MEM_VAR)!")
        endif
    endif
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
    ifeq ($(radio),bgm220pc22hna)
        HW_MAGIC=0D
        HW_VARIANT_ID=0F
    else ifeq ($(radio),bgm220sc22hna)
        HW_MAGIC=0E
        HW_VARIANT_ID=10
    else
        HW_MAGIC=0B
        HW_VARIANT_ID=0B
    endif
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
else ifeq ($(MCU)$(MCU_SUB)$(MCU_MEM_VAR),efr32xg23xxxxf512)
    HW_MAGIC=10
    HW_VARIANT_ID=13
    CFLAGS += -DEFR32FG23 -DEFR32FG23B020F512IM48
    CFLAGS += -DARM_MATH_ARMV8MML
    CFLAGS += -mfloat-abi=hard -mfpu=fpv5-sp-d16
    # Mcu instruction set
    ARCH=armv8-m.main
    # Libraries to be build for Cortex-M33
    CM33 := yes
    HAL_SYSTEM_C := efr32fg23/Source/system_efr32fg23.c
    # Bootloader sanity check
    ifneq ($(board_hw_crystal_32k),no)
        $(error "32kHz crystal not supported on efr32xg23 board")
    endif
else
    $(error "Invalid MCU configuration $(MCU)$(MCU_SUB)$(MCU_MEM_VAR)!")
endif

CM33 ?= no

# Add custom flags
# Remove the -Wunused-parameter flag added by -Wextra as some cortex M4 header do not respect it
CFLAGS += -Wno-unused-parameter

# This mcu uses the version 3 of the bootloader (with external flash support)
BOOTLOADER_VERSION=v3
