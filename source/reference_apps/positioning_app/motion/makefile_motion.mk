
#General settings

CFLAGS += -DPOSAPP_MOTION_ENABLED=$(default_motion_enabled)
CFLAGS += -DPOSAPP_MOTION_THRESHOLD_MG=$(default_motion_threshold_mg)
CFLAGS += -DPOSAPP_MOTION_DURATION_MS=$(default_motion_duration_ms)
CFLAGS += -DMOTION_SUPPORTED

# Generic accelerometer interface
SRCS += $(SRCS_PATH)motion/motion.c
SRCS += $(SRCS_PATH)gpio_nrf52.c
INCLUDES += -I$(SRCS_PATH)motion/

###### LIS2DH12 support start ######
#LIS2DH12 I2C
ifeq ($(motion_sensor),lis2dh12_i2c)
HAL_I2C=yes
CFLAGS += -DLIS2DH12_I2C
lis2dh12=yes
SRCS += $(SRCS_PATH)motion/lis2/lis2_dev_i2c.c
endif

#LIS2DH12 SPI
ifeq ($(motion_sensor),lis2dh12_spi)
HAL_SPI=yes
CFLAGS += -DLIS2DH12_SPI
lis2dh12=yes
SRCS += $(SRCS_PATH)motion/lis2/lis2_dev_spi.c
endif

#LIS2DH12 common
ifeq ($(lis2dh12),yes)
INCLUDES += -I$(SRCS_PATH)motion/lis2
SRCS += $(SRCS_PATH)motion/lis2/lis2dh12_wrapper.c
SRCS += $(SRCS_PATH)motion/lis2/STMems_drivers/lis2dh12_STdC/driver/lis2dh12_reg.c
INCLUDES += -I$(SRCS_PATH)motion/lis2/STMems_drivers/lis2dh12_STdC/driver/
.DEFAULT_GOAL := all

$(SRCS_PATH)motion/lis2/STMems_drivers/lis2dh12_STdC/driver/lis2dh12_reg.c:
$(shell git clone https://github.com/STMicroelectronics/STMems_Standard_C_drivers.git $(SRCS_PATH)motion/lis2/STMems_drivers \
		&& cd $(SRCS_PATH)motion/lis2/STMems_drivers && git checkout v1.03 -b v1.03)
endif
###### LIS2DH12 support end ######

