CC = cc
CFLAGS = -std=c99 -Wall -Wextra
SRC = focl_main.c focl_utils.c sys_lean.c focl.c
TARGET = focl

release:
	$(CC) $(CFLAGS) -O2 -flto $(SRC) -o $(TARGET)

debug:
	$(CC) $(CFLAGS) -g $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: release debug clean