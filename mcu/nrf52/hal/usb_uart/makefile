FOLDER_PREFIX := $(MCU_PREFIX)hal/usb_uart/

TINY_USB_FOLDER=$(FOLDER_PREFIX)tinyusb
VENDOR=nordic
CHIP_FAMILY=nrf5x

# Add tinyUSB sources (same as in examples/rules.mk)
SRCS_TINY_USB +=	\
	$(TINY_USB_FOLDER)/src/tusb.c \
	$(TINY_USB_FOLDER)/src/common/tusb_fifo.c \
	$(TINY_USB_FOLDER)/src/device/usbd.c \
	$(TINY_USB_FOLDER)/src/device/usbd_control.c \
	$(TINY_USB_FOLDER)/src/class/cdc/cdc_device.c

# Instead of tinyusb port, use our own versionned locally
SRCS_TINY_USB += $(FOLDER_PREFIX)dcd_nrf5x.c

SRCS_TINY_USB += $(FOLDER_PREFIX)usb_descriptors.c
SRCS_TINY_USB += $(FOLDER_PREFIX)usb_wrapper.c

SRCS += $(SRCS_TINY_USB)

CFLAGS += -I$(TINY_USB_FOLDER)/src/
CFLAGS += -I$(TINY_USB_FOLDER)/hw/mcu/nordic/nrfx/drivers/src
CFLAGS += -I$(TINY_USB_FOLDER)/hw/mcu/nordic/nrfx/mdk

# For tusb_config.h
CFLAGS += -I$(FOLDER_PREFIX)

# Define USB constant (can be override in config.mk)
usb_vid ?=  0xCafe
usb_manufacturer_str ?= "Wirepas"
usb_product_str ?= "Wirepas Example"

CFLAGS  += -DUSB_VID=$(usb_vid)
CFLAGS  += -DUSB_MANUFACTURER_STRING='$(usb_manufacturer_str)'
CFLAGS  += -DUSB_PRODUCT_STRING='$(usb_product_str)'

# Define a specific target to clone the tiny usb stack
# DEFAULT_GOAL is define as this target will be declared as the first one in global makefile
# and would become the default target without explicitely defining it
.DEFAULT_GOAL := all

# When trying to access one of the tinyusb file the first time
# (or one needing the headers), do the clone
$(SRCS_TINY_USB): $(FOLDER_PREFIX).repo_cloned

# Checkout tinyusb source
# And also the submodule for nordic only
$(FOLDER_PREFIX).repo_cloned:
	git clone https://github.com/hathach/tinyusb.git $(TINY_USB_FOLDER) \
		&& cd $(TINY_USB_FOLDER) && git checkout dbced9911d67fb2063b6eff9e51d463b9062687a
	cd $(TINY_USB_FOLDER) && git submodule update --init hw/mcu/nordic/nrfx
	touch $(FOLDER_PREFIX).repo_cloned
