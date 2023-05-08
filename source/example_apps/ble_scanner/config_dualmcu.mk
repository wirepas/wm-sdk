# Boards compatible with this app 
TARGET_BOARDS := ublox_b204 promistel_rpi_hat tbsense2 silabs_brd4254a pca10040 pca10056 pca10100 
include $(APP_SRCS_PATH)/config.mk

# Target board must be redefined even if same as included base config

# In this alternate config, include dualmcu interface
dualmcu_interface=yes
