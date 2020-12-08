
DRIVERS_PREFIX := $(SRCS_PATH)drivers/

INCLUDES += -I$(DRIVERS_PREFIX)

ifeq ($(MCU),nrf52)
    include $(DRIVERS_PREFIX)nrf52/makefile
else ifeq ($(MCU),efr32)
    include $(DRIVERS_PREFIX)efr32/makefile
else
    $(error Cannot determine MCU for drivers)
endif
