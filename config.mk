# Name of your app. It is used to name the produced files
# It has to be the same as the app directory name under source
# It can be overwritten on command line "make app_name=<...>"
app_name=custom_app

# Name of the board to build your application
# If build is always with same board, it can be set here
# It can be overwritten on command line "make target_board=<...>"
target_board=

# Default version numbers for your app if not defined in app config.mk.
app_major=0
app_minor=0
app_maintenance=0
app_development=0

# Specify the arm toolchain to use (leave it blank if already set in your PATH)
arm_toolchain=

# OTAP sequence number, valid range 1 .. 254
# Sequence 0 = OTAP off (device will not download/upload scratchpad)
# Sequence 0xFF = Will accept any valid OTAP sequence
otap_seq_number=1

