# Boards compatible with this app 
TARGET_BOARDS := pca10040 ublox_b204 pca10056 pca10100 promistel_rpi_hat tbsense2 silabs_brd4254a 
include $(APP_SRCS_PATH)/config.mk

# Target board must be redefined even if same as included base config

# In this alternate config, include dualmcu interface
dualmcu_interface=yes
