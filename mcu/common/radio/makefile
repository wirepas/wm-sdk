
RADIO_PATH := $(MCU_COMMON_SRCS_PATH)radio/
VENDOR_PREFIX := $(MCU_PREFIX)vendor/rail_lib
INCLUDES += -I$(VENDOR_PREFIX)/chip/efr32/efr32xg2x
INCLUDES += -I$(VENDOR_PREFIX)/common

# include power tables if defined
ifneq ($(RADIO_CUSTOM_POWER_TABLE), )
SRCS += $(RADIO_PATH)radio.c
CFLAGS += -DRADIO_CUSTOM_POWER_TABLE_H=\"$(RADIO_CUSTOM_POWER_TABLE)\"
endif