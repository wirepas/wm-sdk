CFLAGS += -DSTORAGE_TYPE=$(storage)

# Enable Provisioning library
PROVISIONING=yes

# HAL button functions
HAL_BUTTON=yes

# Enable the debug logs (has no effect on board without serial port)
APP_PRINTING=yes

ifeq ($(storage),chipid)
	SRCS += $(SRCS_PATH)storage_chipid.c
else ifeq ($(storage),memarea)
	SRCS += $(SRCS_PATH)storage_memarea.c
else
    $(error Select one storage method.)
endif
