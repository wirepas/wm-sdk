# Boards compatible with this app 
TARGET_BOARDS := silabs_brd4254a pca10056 promistel_rpi_hat pca10100 ublox_b204 tbsense2 pca10040 
include $(APP_SRCS_PATH)/config.mk

# Target board must be redefined even if same as included base config

# In this alternate config, include dualmcu interface
dualmcu_interface=yes
