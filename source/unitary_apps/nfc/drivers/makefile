DRIVERS_PREFIX := $(SRCS_PATH)drivers/

ifeq ($(MCU),nrf52)
	SRCS += $(SRCS_PATH)drivers/$(MCU)/nfc_hw.c
else
    $(error Cannot determine MCU for drivers)
endif
