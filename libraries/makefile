ifeq ($(SHARED_LIBDATA), yes)
$(error Shared_LibData library as been renamed, please see libraries/shared_libdata.h for an explanation.)
endif

ifeq ($(SHARED_DATA), yes)
SRCS += $(WP_LIB_PATH)shared_data/shared_data.c
INCLUDES += -I$(WP_LIB_PATH)shared_data
endif

ifeq ($(SHARED_APP_CONFIG), yes)
SRCS += $(WP_LIB_PATH)shared_appconfig/shared_appconfig.c
INCLUDES += -I$(WP_LIB_PATH)shared_appconfig
endif

provisioning_tasks=0
ifeq ($(PROVISIONING), yes)
APP_SCHEDULER=yes
provisioning_tasks=2
SRCS += $(WP_LIB_PATH)provisioning/data.c
SRCS += $(WP_LIB_PATH)provisioning/joining.c
SRCS += $(WP_LIB_PATH)provisioning/provisioning.c
INCLUDES += -I$(WP_LIB_PATH)provisioning
endif

ifeq ($(PROVISIONING_PROXY), yes)
SRCS += $(WP_LIB_PATH)provisioning/proxy.c
INCLUDES += -I$(WP_LIB_PATH)provisioning
endif

control_node_tasks=0
ifeq ($(CONTROL_NODE), yes)
APP_SCHEDULER=yes
control_node_tasks=3
SRCS += $(WP_LIB_PATH)control_node/control_node.c
INCLUDES += -I$(WP_LIB_PATH)control_node
endif

ifeq ($(CONTROL_ROUTER), yes)
SRCS += $(WP_LIB_PATH)control_node/control_router.c
INCLUDES += -I$(WP_LIB_PATH)control_node
endif

ifeq ($(SHARED_NEIGHBORS), yes)
SRCS += $(WP_LIB_PATH)shared_neighbors/shared_neighbors.c
INCLUDES += -I$(WP_LIB_PATH)shared_neighbors
endif

ifeq ($(SHARED_SHUTDOWN), yes)
SRCS += $(WP_LIB_PATH)shared_shutdown/shared_shutdown.c
INCLUDES += -I$(WP_LIB_PATH)shared_shutdown
endif

ifeq ($(SHARED_BEACON), yes)
SRCS += $(WP_LIB_PATH)shared_beacon/shared_beacon.c
INCLUDES += -I$(WP_LIB_PATH)shared_beacon
endif

ifeq ($(APP_PERSISTENT), yes)
SRCS += $(WP_LIB_PATH)app_persistent/app_persistent.c
INCLUDES += -I$(WP_LIB_PATH)app_persistent
endif

positioning_tasks=0
ifeq ($(POSITIONING), yes)
APP_SCHEDULER=yes
positioning_tasks=6
SRCS += $(WP_LIB_PATH)positioning/poslib/poslib_control.c
SRCS += $(WP_LIB_PATH)positioning/poslib/poslib_measurement.c
SRCS += $(WP_LIB_PATH)positioning/poslib/poslib_event.c
SRCS += $(WP_LIB_PATH)positioning/poslib/poslib_decode.c
SRCS += $(WP_LIB_PATH)positioning/poslib/poslib_tlv.c
SRCS += $(WP_LIB_PATH)positioning/poslib.c
SRCS += $(WP_LIB_PATH)positioning/poslib/poslib_ble_beacon.c
SRCS += $(WP_LIB_PATH)positioning/poslib/poslib_mbcn.c
SRCS += $(WP_LIB_PATH)positioning/poslib/poslib_da.c
INCLUDES += -I$(WP_LIB_PATH)positioning/poslib
INCLUDES += -I$(WP_LIB_PATH)positioning
endif

offline_tasks=0
ifeq ($(SHARED_OFFLINE), yes)
APP_SCHEDULER=yes
offline_tasks=2
SRCS += $(WP_LIB_PATH)shared_offline/shared_offline.c
INCLUDES += -I$(WP_LIB_PATH)shared_offline
endif

ifeq ($(APP_SCHEDULER), yes)
library_tasks=$(shell expr $(provisioning_tasks) + $(positioning_tasks) + \
				$(control_node_tasks) + $(offline_tasks) )
SRCS += $(WP_LIB_PATH)scheduler/app_scheduler.c
INCLUDES += -I$(WP_LIB_PATH)scheduler -DAPP_SCHEDULER_LIBRARY_TASKS=$(library_tasks)
endif
