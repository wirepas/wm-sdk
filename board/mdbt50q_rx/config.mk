# Mcu of the board
MCU=nrf52
MCU_SUB=840

uart_use_usb=yes

# To be changed
usb_vid=0xf00d
usb_manufacturer_str="Raytac Wirepas"
usb_product_str="Raytac Wirepas Dongle"

# Uncomment, and set path to custom power table here, if set, the custom power
# table will be set during application startup, and the stack will use that
# instead of the default
# For convenience, an example of a +4dBm power table is provided, uncomment the
# line below to use that instead of the default +8dBm power table
#RADIO_CUSTOM_POWER_TABLE=mcu/nrf52/hal/radio/radio_power_table_nrf52840_4dBm.h
