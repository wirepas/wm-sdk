# Boards compatible with this app 
TARGET_BOARDS := silabs_brd4254a pca10040 pca10100 promistel_rpi_hat ublox_b204 tbsense2 pca10056 
include $(APP_SRCS_PATH)/config.mk

# Target board must be redefined even if same as included base config

# In this alternate config, include dualmcu interface
dualmcu_interface=yes
