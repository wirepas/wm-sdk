# Boards compatible with this app 
TARGET_BOARDS := pca10040 pca10056 pca10059 pca10100 pca10112 promistel_rpi_hat ruuvitag silabs_brd4254a tbsense2 ublox_b204 wirepas_brd4254a nrf52832_mdk_v2 mdbt50q_rx 
# Do not modify this file.
# Bootloader is intended to be used only on already deployed network. Hence
# node configuration is not needed and already done.
# Please specify in the make command line for which application running on the
# node this updater is intended by using app_specific_area_id=0x??????.

# App version
app_major=1
app_minor=0
app_maintenance=0
app_development=0

# Default app_specific_area_id
# It is only used for automatic build in CI to check that app is building
# But in real situation, this value must be update to reflect your setup
app_specific_area_id=0xFFFFFF
