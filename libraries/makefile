
ifeq ($(RTC), yes)
SRCS += $(WP_LIB_PATH)rtc/rtc.c
INCLUDES += -I$(WP_LIB_PATH)rtc
CFLAGS += -DRTC_MAX_CB=$(RTC_CBS)
endif

ifeq ($(SHARED_DATA), yes)
SRCS += $(WP_LIB_PATH)shared_data/shared_data.c
INCLUDES += -I$(WP_LIB_PATH)shared_data
endif

ifeq ($(SHARED_APP_CONFIG), yes)
SRCS += $(WP_LIB_PATH)shared_appconfig/shared_appconfig.c
INCLUDES += -I$(WP_LIB_PATH)shared_appconfig
CFLAGS += -DSHARED_APP_CONFIG_MAX_FILTER=$(shell expr $(app_config_filters))
endif

ifeq ($(LOCAL_PROVISIONING), yes)
SRCS += $(WP_LIB_PATH)local_provisioning/local_provisioning.c
INCLUDES += -I$(WP_LIB_PATH)local_provisioning
endif

ifeq ($(PROVISIONING), yes)
SRCS += $(WP_LIB_PATH)provisioning/data.c
SRCS += $(WP_LIB_PATH)provisioning/joining.c
SRCS += $(WP_LIB_PATH)provisioning/provisioning.c
INCLUDES += -I$(WP_LIB_PATH)provisioning
endif

ifeq ($(PROVISIONING_PROXY), yes)
SRCS += $(WP_LIB_PATH)provisioning/proxy.c
INCLUDES += -I$(WP_LIB_PATH)provisioning
endif

ifeq ($(CONTROL_NODE), yes)
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
CFLAGS += -DSHARED_NEIGHBORS_MAX_CB=$(shell expr $(shared_neighbors_cbs))
endif

ifeq ($(SHARED_BEACON), yes)
SRCS += $(WP_LIB_PATH)shared_beacon/shared_beacon.c
INCLUDES += -I$(WP_LIB_PATH)shared_beacon
endif

ifeq ($(APP_PERSISTENT), yes)
SRCS += $(WP_LIB_PATH)app_persistent/app_persistent.c
INCLUDES += -I$(WP_LIB_PATH)app_persistent
endif

ifeq ($(POSITIONING), yes)
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

ifeq ($(SHARED_OFFLINE), yes)
SRCS += $(WP_LIB_PATH)shared_offline/shared_offline.c
INCLUDES += -I$(WP_LIB_PATH)shared_offline
CFLAGS += -DSHARED_OFFLINE_MAX_MODULES=$(shell expr $(shared_offline_modules))
endif

ifeq ($(STACK_STATE_LIB), yes)
SRCS += $(WP_LIB_PATH)stack_state/stack_state.c
INCLUDES += -I$(WP_LIB_PATH)stack_state
CFLAGS += -DSTACK_STATE_CB=$(shell expr $(stack_state_cbs))
endif

ifeq ($(APP_SCHEDULER), yes)
SRCS += $(WP_LIB_PATH)scheduler/app_scheduler.c
INCLUDES += -I$(WP_LIB_PATH)scheduler
# Set number of Library tasks
CFLAGS += -DAPP_SCHEDULER_ALL_TASKS=$(shell expr $(scheduler_tasks))
endif

ifeq ($(DUALMCU_LIB), yes)
SRCS += $(WP_LIB_PATH)dualmcu/dualmcu_lib.c
INCLUDES += -I$(WP_LIB_PATH)dualmcu
include $(WP_LIB_PATH)dualmcu/waps/makefile
include $(WP_LIB_PATH)dualmcu/drivers/makefile
endif

SRCS += $(WP_LIB_PATH)libraries_init.c
INCLUDES += -I$(WP_LIB_PATH)
