SRCS += $(SRCS_PATH)BME280_driver/bme280.c
SRCS += $(SRCS_PATH)bme280_wrapper.c
SRCS += $(SRCS_PATH)LIS2DH12_driver/lis2dh12_STdC/driver/lis2dh12_reg.c
SRCS += $(SRCS_PATH)lis2dh12_wrapper.c
SRCS += $(SRCS_PATH)app_config.c
SRCS += $(SRCS_PATH)format_data.c

INCLUDES += -I$(SRCS_PATH)BME280_driver/
INCLUDES += -I$(SRCS_PATH)LIS2DH12_driver/lis2dh12_STdC/driver/

# Define default network settings
CFLAGS += -DNETWORK_ADDRESS=$(default_network_address)
CFLAGS += -DNETWORK_CHANNEL=$(default_network_channel)

HAL_SPI=yes

# Use App Scheduler and configure it
APP_SCHEDULER=yes
CFLAGS += -DAPP_SCHEDULER_MAX_TASKS=4
CFLAGS += -DAPP_SCHEDULER_MAX_EXEC_TIME_US=100

# Define a specific target to clone the Bosh Library
# tested on revision 958cef65daa46123bdc3a8da9fb2a1b0abffc653
# DEFAULT_GOAL is define as this target will be declared as the first one in global makefile
# and would become the default target without explicitely defining it
.DEFAULT_GOAL := all
$(SRCS_PATH)BME280_driver/bme280.c:
	$(shell git clone https://github.com/BoschSensortec/BME280_driver.git $(SRCS_PATH)BME280_driver)
$(SRCS_PATH)LIS2DH12_driver/lis2dh12_STdC/driver/lis2dh12_reg.c:
	$(shell git clone https://github.com/STMicroelectronics/STMems_Standard_C_drivers.git $(SRCS_PATH)LIS2DH12_driver \
		&& cd $(SRCS_PATH)LIS2DH12_driver && git checkout v1.01 -b v1.01)
