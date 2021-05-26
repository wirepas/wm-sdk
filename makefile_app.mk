include makefile_common.mk

.DEFAULT_GOAL := all

# Linker script
LDSCRIPT = mcu/$(MCU)/linker/gcc_app_$(MCU)$(MCU_SUB)$(MCU_MEM_VAR).ld

LIBS :=

ifeq ($(filter $(TARGET_BOARDS),$(target_board)),)
 $(error Board $(target_board) is not supported board list: ($(TARGET_BOARDS)))
else
 $(info Building app for $(target_board))
endif

# App different formats
APP_ELF := $(BUILDPREFIX_APP)$(APP_NAME).elf

# For backward compatibility as app makefile except SRCS_PATH variable
SRCS_PATH := $(APP_SRCS_PATH)

# Convert default network settings to CFLAGS to be used in code
ifneq ($(default_network_address),)
CFLAGS += -DNETWORK_ADDRESS=$(default_network_address)
endif
ifneq ($(default_network_channel),)
CFLAGS += -DNETWORK_CHANNEL=$(default_network_channel)
endif
ifneq ($(default_network_cipher_key),)
CFLAGS += -DNET_CIPHER_KEY=$(default_network_cipher_key)
endif
ifneq ($(default_network_authen_key),)
CFLAGS += -DNET_AUTHEN_KEY=$(default_network_authen_key)
endif

# Include board init part
-include board/makefile

# Include app specific makefile
-include $(APP_SRCS_PATH)makefile


# Generic util functions are needed for all apps (api.c)
-include $(UTIL_PATH)makefile

# Include libraries code
-include $(WP_LIB_PATH)makefile
INCLUDES += -I$(WP_LIB_PATH)

# Include HAL drivers code
-include $(HAL_API_PATH)makefile

# Include common MCU sources
include $(MCU_PATH)common/makefile

#
# Sources & includes paths
#
SRCS += $(APP_SRCS_PATH)app.c
INCLUDES += -iquote$(API_PATH) -I$(APP_SRCS_PATH)include -I$(UTIL_PATH)

# Objects list
OBJS_ = $(SRCS:.c=.o) $(ASM_SRCS:.s=.o)
OBJS = $(addprefix $(BUILDPREFIX_APP), $(OBJS_))

# Dependent list
DEPS_ = $(SRCS:.c=.d)
DEPS = $(addprefix $(BUILDPREFIX_APP), $(DEPS_))


# Files to be cleaned
CLEAN := $(OBJS) $(APP_ELF) $(APP_HEX) $(DEPS)

$(BUILDPREFIX_APP)%.o : %.c $(APP_SRCS_PATH)makefile $(APP_CONFIG) $(BOARD_CONFIG) $(MCU_CONFIG)
	$(MKDIR) $(@D)
	$(CC) $(INCLUDES) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILDPREFIX_APP)%.o : %.s
	$(MKDIR) $(@D)
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@


$(APP_ELF): $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) -o $@ $^ \
	      -Wl,-Map=$(BUILDPREFIX_APP)$(APP_NAME).map \
	      -Wl,-T,$(LDSCRIPT) $(LIBS) $(LDFLAGS)

$(APP_HEX): $(APP_ELF)
	@echo "Generating $(APP_HEX)"
	$(OBJCOPY) $< -O ihex $@

.PHONY: all
all: $(APP_HEX)

clean:
	$(RM) -rf $(CLEAN)

-include $(DEPS)

