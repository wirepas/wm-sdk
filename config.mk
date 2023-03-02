# Name of your app. It is used to name the produced files
# It has to be the same as the app directory name under source
# It can be overwritten on command line "make app_name=<...>"
app_name=dualmcu_app

# Name of the board to build your application
# If build is always with same board, it can be set here
# It can be overwritten on command line "make target_board=<...>"
target_board=nrf52840_bmd345

# Version of the SDK
sdk_major=1
sdk_minor=4
sdk_maintenance=0
sdk_development=0


# Specify the arm toolchain to use (leave it blank if already set in your PATH)
arm_toolchain=

# Only set this value if python interpreter is not found. Python3 is preferred.
# Python2 is end of life and support will be removed in the future.
# Leaving value empty uses the shebang value of the script, which is #!/usr/bin/env python3
# and fallback to "python" cmd
python_interpreter=

