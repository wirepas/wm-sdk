
RADIO_PATH := $(MCU_COMMON_SRCS_PATH)radio/

# include power tables if defined
ifneq ($(RADIO_CUSTOM_POWER_TABLE), )
SRCS += $(RADIO_PATH)radio.c
CFLAGS += -DRADIO_CUSTOM_POWER_TABLE_H=\"$(RADIO_CUSTOM_POWER_TABLE)\"
endif