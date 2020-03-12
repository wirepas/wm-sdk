INCLUDES += -Iboard/

SRCS += board/board_init.c

ifneq ("$(wildcard board/$(target_board)/board_custom_init.c)","")
	SRCS += board/$(target_board)/board_custom_init.c
endif
