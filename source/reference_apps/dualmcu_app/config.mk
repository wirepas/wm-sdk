# Boards compatible with this app 
TARGET_BOARDS := mdbt50q_rx ublox_b204 pca10040 pca10100 pca10056 pca10059 nrf52832_mdk_v2 promistel_rpi_hat wuerth_261101102 tbsense2 silabs_brd4254a silabs_brd4253a silabs_brd4180b silabs_brd4181b silabs_brd4184a 

# Define a specific application area_id
app_specific_area_id=0x846B74

# App version
app_major=$(sdk_major)
app_minor=$(sdk_minor)
app_maintenance=$(sdk_maintenance)
app_development=$(sdk_development)
