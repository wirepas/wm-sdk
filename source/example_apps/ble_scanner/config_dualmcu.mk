# Boards compatible with this app 
TARGET_BOARDS := silabs_brd4254a promistel_rpi_hat pca10040 pca10056 pca10100 tbsense2 ublox_b204 
include $(APP_SRCS_PATH)/config.mk

# Target board must be redefined even if same as included base config

# In this alternate config, include dualmcu interface
dualmcu_interface=yes
