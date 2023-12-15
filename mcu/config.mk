# Button lib requires the GPIO lib
ifeq ($(HAL_BUTTON), yes)
HAL_GPIO=yes
endif

# LED lib requires the GPIO lib
ifeq ($(HAL_LED), yes)
HAL_GPIO=yes
endif

# UART lib requires the GPIO lib
ifeq ($(HAL_UART), yes)
HAL_GPIO=yes
endif