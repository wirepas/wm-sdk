# You can add more sources here and libraries if needed
SRCS +=
INCLUDES +=
LIBS +=
LDFLAGS +=

# Example:
# SRCS += $(SRCS_PATH)my_driver.c
# LIBS += my_lib.a
# include math.h lib
# LDFLAGS += -lm

# Enable shared data library (needed by control library)
SHARED_DATA=yes

# Enable Control library (for router)
CONTROL_ROUTER=yes

# Enable shared appconfig (needed by app)
SHARED_APP_CONFIG=yes
CFLAGS += -DSHARED_APP_CONFIG_MAX_FILTER=1

# Enable the debug logs (has no effect on board without serial port)
APP_PRINTING=yes