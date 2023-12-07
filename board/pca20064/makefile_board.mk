SRCS += board/$(target_board)/board_custom_init.c

SRCS += board/$(target_board)/npm1300_init.c
SRCS += mcu/nrf/nrf91/hal/i2c.c
INCLUDES += -Imcu/hal_api
INCLUDES += -Ibootloader
