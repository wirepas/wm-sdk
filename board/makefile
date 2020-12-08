INCLUDES += -Iboard/

SRCS += board/board_init.c

# Include custom board makefile, where board specific source files can be added
# Does nothing, if file does not exist
-include board/$(target_board)/makefile_board.mk

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
