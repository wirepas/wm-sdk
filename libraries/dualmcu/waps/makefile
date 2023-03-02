WAPS_PREFIX := $(WP_LIB_PATH)dualmcu/waps/

# Define WAPS version here
# 12 -> 13 (OTAP_Remote calls not suported anymore)
# 13 -> 14 (Change type and composition of RSSI,TX,RX power in getNeighbors())
# 14 -> 15 (- add Read/Write Target OTAP
#           - remove deprecated otap services (remote)
#           - add quality indicator)
# 15 -> 16 (- use new single mcu api to set app config diag interval
#             so sequence become obsolete)
# 16 -> 17 (- Add support for fragmented packet (TX and RX))
# 17 -> 18 (- add scratchpad read primitive
#           - add read-only MSAP attribute 14 for stored scratchpad size)

CFLAGS += -DWAPS_VERSION=18

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