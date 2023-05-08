ifeq ($(SHARED_LIBDATA), yes)
$(error Shared_LibData library as been renamed, please see libraries/shared_libdata.h for an explanation.)
endif

ifeq ($(SHARED_SHUTDOWN), yes)
$(error Shared_Shutdown library as been renamed, please use libraries/stack_state.h instead \
        and enable its usage with STACK_STATE_LIB)
endif

# Any library needing app_scheduler must increment this variable
# this way:
# scheduler_tasks+= + n
scheduler_tasks=0

# Any library needing stack state callbacks must increment this variable
# this way:
# stack_state_cbs+= + n
stack_state_cbs=0

# Any library needing app_config filter must increment this variable
# this way:
# library_app_config_filters+= + n
app_config_filters=0

# Any library needing shared offline modules must increment this variable
# this way:
# shared_offline_modules+= + n
shared_offline_modules=0

# Any library needing shared neighbors callbacks must increment this variable
# this way:
# shared_neighbors_cbs+= + n
shared_neighbors_cbs=0

ifeq ($(LOCAL_PROVISIONING), yes)
$(info LOCAL_PROVISIONING automatically enable PROVISIONING and PROVISIONING_PROXY)
PROVISIONING=yes
PROVISIONING_PROXY=yes
scheduler_tasks+= + 1
app_config_filters+= + 1
stack_state_cbs+= + 1
endif

ifeq ($(PROVISIONING), yes)
scheduler_tasks+= + 3
stack_state_cbs+= + 1
SHARED_DATA=yes
TINY_CBOR=yes
SW_AES=yes
endif

ifeq ($(PROVISIONING_PROXY), yes)
SHARED_DATA=yes
TINY_CBOR=yes
SW_AES=yes
endif

ifeq ($(CONTROL_NODE), yes)
scheduler_tasks+= + 3
SHARED_DATA=yes
endif

ifeq ($(POSITIONING), yes)
scheduler_tasks+= + 7
SHARED_DATA=yes
app_config_filters+= + 2
shared_neighbors_cbs+= + 2
# 2 state event cb
# - one in measurement for end of scan
# - one if route cb is implemented in poslib_contol
stack_state_cbs+= + 2
SHARED_BEACON=yes
shared_offline_modules+= + 2
endif

ifeq ($(SHARED_OFFLINE), yes)
scheduler_tasks+= + 2
endif

ifeq ($(DUALMCU_LIB), yes)
HAL_GPIO=yes
scheduler_tasks+= + 4
app_config_filters+= + 1
SHARED_DATA=yes
stack_state_cbs+= + 1
endif

# RTC library
ifeq ($(RTC), yes)
scheduler_tasks+= + 1
SHARED_DATA=yes
APP_SCHEDULER=yes
endif

#########
# Enabling libraries needed by other libs and check app input
#########

# Shared app_config
ifeq ($(SHARED_APP_CONFIG), yes)
# Enabled by app
ifndef SHARED_APP_CONFIG_FILTERS
$(error "Please define SHARED_APP_CONFIG_FILTERS from your application makefile. Previously\
         directly added as a CFLAG: -DSHARED_APP_CONFIG_MAX_FILTER=n")
endif
# Add to app libs own filters
app_config_filters+= + $(SHARED_APP_CONFIG_FILTERS)
else
ifneq ($(app_config_filters), 0)
$(info Enabling SHARED_APP_CONFIG as libraries need it)
SHARED_APP_CONFIG=yes
endif
endif

# Shared neighbors
ifeq ($(SHARED_NEIGHBORS), yes)
ifndef SHARED_NEIGHBORS_CBS
$(error "Please define SHARED_NEIGHBORS_CBS from your application makefile. Previously\
         directly added as a CFLAG: -DSHARED_NEIGHBORS_MAX_CB=n")
endif
shared_neighbors_cbs+= + $(SHARED_NEIGHBORS_CBS)
else
ifneq ($(shared_neighbors_cbs), 0)
$(info Enabling SHARED_NEIGHBORS as libraries need it)
SHARED_NEIGHBORS=yes
endif
endif

# Stack state
ifeq ($(STACK_STATE_LIB), yes)
ifdef STACK_STATE_CBS
stack_state_cbs+= + $(STACK_STATE_CBS)
endif
# It is not an issue if STACK_STATE_CBS is not defined
# as lib can be used without callbacks
else
ifneq ($(stack_state_cbs), 0)
$(info Enabling STACK_STATE_LIB as libraries need it)
STACK_STATE_LIB=yes
endif
endif

# Shared offline library
ifeq ($(SHARED_OFFLINE), yes)
ifndef SHARED_OFFLINE_MODULES
$(error "Please define SHARED_OFFLINE_MODULES from your application makefile. Previously\
         directly added as a CFLAG: -DSHARED_OFFLINE_MAX_MODULES=n")
endif
shared_offline_modules+= + $(SHARED_OFFLINE_MODULES)
else
ifneq ($(shared_offline_modules), 0)
$(info Enabling SHARED_OFFLINE as libraries need it)
SHARED_OFFLINE=yes
endif
endif

# App scheduler
ifeq ($(APP_SCHEDULER), yes)
ifndef APP_SCHEDULER_TASKS
$(error "Please define APP_SCHEDULER_TASKS from your application makefile. Previously\
         directly added as a CFLAG: -DAPP_SCHEDULER_MAX_TASKS=n")
endif
scheduler_tasks+= + $(APP_SCHEDULER_TASKS)
else
ifneq ($(scheduler_tasks), 0)
$(info Enabling APP_SCHEDULER as libraries need it but app not)
APP_SCHEDULER=yes
endif
endif
