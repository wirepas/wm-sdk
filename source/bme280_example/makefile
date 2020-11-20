# You can add more sources here and libraries if needed
SRCS += $(SRCS_PATH)BME280_driver/bme280.c
SRCS += $(SRCS_PATH)bme280_wrapper.c
INCLUDES += -I$(SRCS_PATH)BME280_driver/
LIBS +=
LDFLAGS +=

# Example:
# SRCS += $(SRCS_PATH)my_driver.c
# LIBS += my_lib.a
# include math.h lib
# LDFLAGS += -lm

# Include app drivers (for Io)
include $(SRCS_PATH)drivers/makefile

HAL_SPI=yes

# Use the 32bits integer data format for BME280 driver.
CFLAGS += -DBME280_32BIT_ENABLE

# Define a specific target to clone the Bosh Library
# DEFAULT_GOAL is define as this target will be declared as the first one in global makefile
# and would become the default target without explicitely defining it
.DEFAULT_GOAL := all
$(SRCS_PATH)BME280_driver/bme280.c:
	$(shell git clone https://github.com/BoschSensortec/BME280_driver.git $(SRCS_PATH)BME280_driver \
		&& cd $(SRCS_PATH)BME280_driver && git checkout bme280_v3.4.3 -b bme280_v3.4.3)
