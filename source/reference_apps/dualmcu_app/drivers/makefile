
DRIVERS_PREFIX := $(SRCS_PATH)drivers/

INCLUDES += -I$(DRIVERS_PREFIX)

ifneq (,$(filter $(MCU), nrf52 nrf91))
    include $(DRIVERS_PREFIX)nrf/makefile
else ifeq ($(MCU),efr32)
    include $(DRIVERS_PREFIX)efr32/makefile
else
    $(error Cannot determine MCU for drivers)
endif
