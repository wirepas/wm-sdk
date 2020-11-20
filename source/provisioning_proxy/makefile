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

# Generic util functions are needed by joining test app
include $(UTIL_PATH)makefile

# Enable shared data library (needed by provisioning proxy library)
SHARED_DATA=yes

# Enable CBOR library (needed by provisioning proxy library)
TINY_CBOR=yes

# Enable software AES (needed by provisioning proxy library)
SW_AES=yes

# Enable Provisioning Proxy library
PROVISIONING_PROXY=yes

# HAL button functions
HAL_BUTTON=yes

# Uncomment to enable debug prints
# APP_PRINTING=yes
