# You can add more sources here and libraries if needed
SRCS +=
INCLUDES +=

# Define default network settings
CFLAGS += -DNETWORK_ADDRESS=$(default_network_address)
CFLAGS += -DNETWORK_CHANNEL=$(default_network_channel)

# Generic util functions are needed by joining test app
include $(UTIL_PATH)makefile

# Enable shared libdata library (needed by provisioning library)
SHARED_LIBDATA=yes

# Enable CBOR library (needed by provisioning library)
TINY_CBOR=yes

# Enable software AES (needed by provisioning library)
SW_AES=yes

# Enable Provisioning library
PROVISIONING=yes

# HAL button functions
HAL_BUTTON=yes

# Uncomment to enable debug prints
# APP_PRINTING=yes
