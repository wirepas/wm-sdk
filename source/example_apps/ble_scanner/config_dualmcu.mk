# Boards compatible with this app 
TARGET_BOARDS := tbsense2 ublox_b204 pca10100 pca10056 promistel_rpi_hat silabs_brd4254a pca10040 
include $(APP_SRCS_PATH)/config.mk

# Target board must be redefined even if same as included base config

# In this alternate config, include dualmcu interface
dualmcu_interface=yes
