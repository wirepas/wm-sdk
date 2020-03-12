SRCS += $(SRCS_PATH)BME280_driver/bme280.c
SRCS += $(SRCS_PATH)bme280_wrapper.c

INCLUDES += -I$(SRCS_PATH)BME280_driver/

# Define default network settings
CFLAGS += -DNETWORK_ADDRESS=$(default_network_address)
CFLAGS += -DNETWORK_CHANNEL=$(default_network_channel)

# Include app drivers (for Io)
include $(SRCS_PATH)drivers/makefile

HAL_SPI=yes

# Define a specific target to clone the Bosh Library
# tested on revision 958cef65daa46123bdc3a8da9fb2a1b0abffc653
# DEFAULT_GOAL is define as this target will be declared as the first one in global makefile
# and would become the default target without explicitely defining it
.DEFAULT_GOAL := all
$(SRCS_PATH)BME280_driver/bme280.c:
	$(shell git clone https://github.com/BoschSensortec/BME280_driver.git $(SRCS_PATH)BME280_driver)
