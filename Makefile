CC = cc
CFLAGS = -Wall -Wextra
INCLUDES = -I./include
PACK_CMD = -DFOCL_REGISTER_UTILS -DFOCL_REGISTER_MATH -DFOCL_REGISTER_SYS
TARGET = focl

BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj

SRC = src/focl.c \
      src/focl_main.c \
      src/focl_register.c \
      src/focl_repl.c \
      src/system/sys_lean.c \
      src/utils/focl_math.c \
      src/utils/focl_sys.c \
      src/utils/focl_utils.c

ifneq ($(OS),Windows_NT)
SRC += src/linenoise/linenoise.c
endif

OBJ = $(addprefix $(OBJ_DIR)/, $(notdir $(SRC:.c=.o)))

.DEFAULT_GOAL := release

$(BUILD_DIR) $(OBJ_DIR):
	mkdir -p $@

vpath %.c src src/system src/utils src/linenoise

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -MMD -MP $(INCLUDES) $(PACK_CMD) -c $< -o $@

$(BUILD_DIR)/$(TARGET): $(OBJ) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(OBJ) -lm -o $@

release: CFLAGS += -O2 -flto
release: $(BUILD_DIR)/$(TARGET)
	@cp $< $(TARGET)

debug: CFLAGS += -g
debug: $(BUILD_DIR)/$(TARGET)
	@cp $< $(TARGET)

install: release
	@if [ "$(OS)" = "Windows_NT" ]; then \
		echo "Error: install is not supported on Windows"; \
		exit 1; \
	fi
	@echo "Installing $(TARGET) to /usr/local/bin"
	@sudo cp $(TARGET) /usr/local/bin/
	@echo "Installation complete"

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

-include $(OBJ_DIR)/*.d

.PHONY: release debug clean install