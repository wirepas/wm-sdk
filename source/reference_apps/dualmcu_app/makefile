# Dual MCU app needs UART
HAL_UART=yes
ifeq ($(uart_use_usb),yes)
UART_USE_DMA=no
UART_USE_USB=yes
CFLAGS += -DUART_USE_USB
else ifeq ($(uart_use_dma),no)
UART_USE_DMA=no
UART_USE_USB=no
else
# dma uart is used by default if not explicitly set to no
UART_USE_DMA=yes
UART_USE_USB=no
endif

INCLUDES += -I$(SRCS_PATH)

# Define default baudrate and hardware flow control settings
uart_br ?= 125000
uart_fc ?= false

CFLAGS += -DUART_BAUDRATE=$(uart_br)
CFLAGS += -DUART_FLOWCONTROL=$(uart_fc)

# Force Otap action to LEGACY at firstboot (flash or otap)
otap_force_legacy ?= no
ifeq ($(otap_force_legacy),yes)
    CFLAGS += -DOTAP_FORCE_LEGACY
endif

# Use App Scheduler and configure it
APP_SCHEDULER=yes
APP_SCHEDULER_TASKS=3

# Enable Shared app config
SHARED_APP_CONFIG=yes
SHARED_APP_CONFIG_FILTERS=1

SHARED_NEIGHBORS=yes
SHARED_NEIGHBORS_CBS=1

# Enable shared data module
SHARED_DATA=yes

# Enable stack state lib
STACK_STATE_LIB=yes
STACK_STATE_CBS=0

LOCAL_PROVISIONING=yes

include $(SRCS_PATH)waps/makefile
include $(SRCS_PATH)drivers/makefile
