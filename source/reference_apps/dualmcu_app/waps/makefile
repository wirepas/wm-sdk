WAPS_PREFIX := $(SRCS_PATH)waps/

# Define WAPS version here
# 12 -> 13 (OTAP_Remote calls not suported anymore)
# 13 -> 14 (Change type and composition of RSSI,TX,RX power in getNeighbors())
CFLAGS += -DWAPS_VERSION=14

INCLUDES += -I$(WAPS_PREFIX)

include $(WAPS_PREFIX)comm/makefile
include $(WAPS_PREFIX)protocol/makefile

SRCS +=  $(WAPS_PREFIX)waps.c                   \
         $(WAPS_PREFIX)waps_item.c              \
         $(WAPS_PREFIX)waddr.c                  \
         $(WAPS_PREFIX)sap/function_codes.c     \
         $(WAPS_PREFIX)sap/csap.c               \
         $(WAPS_PREFIX)sap/dsap.c               \
         $(WAPS_PREFIX)sap/msap.c               \
         $(WAPS_PREFIX)sap/lock_bits.c          \
         $(WAPS_PREFIX)sap/persistent.c         \
         $(WAPS_PREFIX)sap/multicast.c