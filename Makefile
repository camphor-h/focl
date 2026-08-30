CC = cc
CFLAGS = -Wall -Wextra -std=gnu99
LDFLAGS = -lm
INCLUDES = -I./include
PACK_CMD = -DFOCL_REGISTER_UTILS -DFOCL_REGISTER_MATH -DFOCL_REGISTER_SYS -DFOCL_REGISTER_TERM
TARGET = focl
TARGET_C = foclc
LIBRARY = libfocl.a

BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
LIB_DIR = $(BUILD_DIR)/lib

SRC_COMMON = src/focl.c \
             src/focl_register.c \
             src/focl_repl.c \
             src/system/sys_lean.c \
             src/utils/focl_math.c \
             src/utils/focl_sys.c \
             src/utils/focl_utils.c \
			 src/utils/focl_term.c

SRC_FOCL = src/focl_main.c
SRC_FOCLC = src/foclc_main.c

ifneq ($(OS),Windows_NT)
SRC_COMMON += src/linenoise/linenoise.c
endif

OBJ_COMMON = $(addprefix $(OBJ_DIR)/, $(notdir $(SRC_COMMON:.c=.o)))
OBJ_FOCL = $(OBJ_DIR)/focl_main.o
OBJ_FOCLC = $(OBJ_DIR)/foclc_main.o

-include local.mk

.DEFAULT_GOAL := release

$(BUILD_DIR) $(OBJ_DIR) $(LIB_DIR):
	mkdir -p $@

vpath %.c src src/system src/utils src/linenoise

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -MMD -MP $(INCLUDES) $(PACK_CMD) $(CONFIG_DEF) -c $< -o $@

$(LIB_DIR)/$(LIBRARY): $(OBJ_COMMON) | $(LIB_DIR)
	ar rcs $@ $(OBJ_COMMON)

$(BUILD_DIR)/$(TARGET): $(OBJ_COMMON) $(OBJ_FOCL) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(OBJ_COMMON) $(OBJ_FOCL) $(LDFLAGS) -o $@

$(BUILD_DIR)/$(TARGET_C): $(OBJ_COMMON) $(OBJ_FOCLC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(OBJ_COMMON) $(OBJ_FOCLC) $(LDFLAGS) -o $@

release: CFLAGS += -O2 -flto -DNDEBUG
release: LDFLAGS += -s
release: $(BUILD_DIR)/$(TARGET) $(BUILD_DIR)/$(TARGET_C) $(LIB_DIR)/$(LIBRARY)
	@cp $(BUILD_DIR)/$(TARGET) .
	@cp $(BUILD_DIR)/$(TARGET_C) .
	@cp $(LIB_DIR)/$(LIBRARY) .

release-nostrip: CFLAGS += -O2 -flto -DNDEBUG -g -pg -no-pie
release-nostrip: $(BUILD_DIR)/$(TARGET) $(BUILD_DIR)/$(TARGET_C) $(LIB_DIR)/$(LIBRARY)
	@cp $(BUILD_DIR)/$(TARGET) .
	@cp $(BUILD_DIR)/$(TARGET_C) .
	@cp $(LIB_DIR)/$(LIBRARY) .

debug: CFLAGS += -g -DMEMORY_ALLOC_CHECK
debug: $(BUILD_DIR)/$(TARGET) $(BUILD_DIR)/$(TARGET_C) $(LIB_DIR)/$(LIBRARY)
	@cp $(BUILD_DIR)/$(TARGET) .
	@cp $(BUILD_DIR)/$(TARGET_C) .
	@cp $(LIB_DIR)/$(LIBRARY) .

install: release
	@if [ "$(OS)" = "Windows_NT" ]; then \
		echo "Error: install is not supported on Windows"; \
		exit 1; \
	fi
	@echo "Installing $(TARGET) and $(TARGET_C) to /usr/local/bin"
	@sudo cp $(TARGET) /usr/local/bin/
	@sudo cp $(TARGET_C) /usr/local/bin/
	@mkdir -p "/usr/local/lib/focl"
	@sudo install -m644 $(LIBRARY) "/usr/local/lib/focl/"
	@echo "Installation complete"

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TARGET_C) $(LIBRARY)

-include $(OBJ_DIR)/*.d

.PHONY: release debug clean install