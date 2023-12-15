INCLUDES += -Iboard/

SRCS += board/board_init.c

# Include custom board makefile, where board specific source files can be added
# Does nothing, if file does not exist
-include $(BOARD_FOLDER)/makefile_board.mk

# HARDWARE CAPABILITIES #
## Is 32kHz crystal mounted on the board? default:yes, possible values:yes, no
ifeq ("$(board_hw_crystal_32k)","yes")
    CFLAGS += -DBOARD_HW_CRYSTAL_32K=1
else ifeq ("$(board_hw_crystal_32k)","no")
    CFLAGS += -DBOARD_HW_CRYSTAL_32K=0
else
    $(error board_hw_crystal_32k must be defined to yes or no)
endif

## Is DCDC used on this board? default:yes, possible values:yes, no
ifeq ("$(board_hw_dcdc)","yes")
    CFLAGS += -DBOARD_HW_DCDC=1
else ifeq ("$(board_hw_dcdc)","no")
    CFLAGS += -DBOARD_HW_DCDC=0
else
    $(error board_hw_dcdc must be defined to yes or no)
endif

## HFXO crystal characteristics on this board
ifdef board_hw_hfxo_ctune
    CFLAGS += -DBOARD_HW_HFXO_CTUNE=$(board_hw_hfxo_ctune)
endif

## LFXO crystal characteristics on this board
ifdef board_hw_lfxo_ctune
    CFLAGS += -DBOARD_HW_LFXO_CTUNE=$(board_hw_lfxo_ctune)
endif
ifdef board_hw_lfxo_gain
    CFLAGS += -DBOARD_HW_LFXO_GAIN=$(board_hw_lfxo_gain)
endif

# FTDI enabled for serial communication
ifeq ("$(board_use_ftdi)","yes")
    CFLAGS += -DUSE_FTDI
endif
