CC = cc
CFLAGS = -Wall -Wextra
INCLUDES = -I./include
PACK_CMD = -DFOCL_REGISTER_UTILS -DFOCL_REGISTER_MATH -DFOCL_REGISTER_SYS
TARGET = focl

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

release:
	$(CC) $(CFLAGS) -O2 -flto $(INCLUDES) $(PACK_CMD) $(SRC) -lm -o $(TARGET)

debug:
	$(CC) $(CFLAGS) -g $(INCLUDES) $(PACK_CMD) $(SRC) -lm -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: release debug clean