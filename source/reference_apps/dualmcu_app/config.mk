# Boards compatible with this app 
TARGET_BOARDS := pca20064 pca20049 pca10153 pca10040 ublox_b204 wuerth_261101102 pca10056 pca10100 promistel_rpi_hat mdbt50q_rx nrf52832_mdk_v2 pca10059 tbsense2 silabs_brd4254a silabs_brd4253a silabs_brd4181b bgm220-ek4314a silabs_brd4184a silabs_brd4312a silabs_brd4180b silabs_brd4210a silabs_brd2703a silabs_brd2601b silabs_brd4187c pan1780 silabs_brd4158a 

# Define a specific application area_id
app_specific_area_id=0x846B74

# App version
app_major=$(sdk_major)
app_minor=$(sdk_minor)
app_maintenance=$(sdk_maintenance)
app_development=$(sdk_development)

# Uncomment to allow reading scratchpad via dual-MCU API
#allow_scratchpad_read=yes
