CC = gcc
WIN_CC = x86_64-w64-mingw32-gcc
CFLAGS = -w -std=c11 -D_DEFAULT_SOURCE
LDFLAGS = -lm 

SRC_DIR = src
OBJ_DIR = obj
WIN_OBJ_DIR = obj_win
BIN_DIR = bin
WIN_BIN_DIR = bin_win
EXE_NAME = xen
WIN_EXE_NAME = xen.exe

TARGET = $(BIN_DIR)/$(EXE_NAME)
WIN_TARGET = $(WIN_BIN_DIR)/$(WIN_EXE_NAME)

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)
WIN_OBJS = $(SRCS:%.c=$(WIN_OBJ_DIR)/%.o)

all: debug 

debug: CFLAGS += -O0 -g -DDEBUG
debug: clean_build $(TARGET)

release: CFLAGS += -O2 -DNDEBUG
release: clean_build $(TARGET)

windows-debug: CFLAGS += -O0 -g -DDEBUG
windows-debug: clean_build_win $(WIN_TARGET)

windows-release: CFLAGS += -O2 -DNDEBUG
windows-release: clean_build_win $(WIN_TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(WIN_TARGET): $(WIN_OBJS) | $(WIN_BIN_DIR)
	$(WIN_CC) $(WIN_OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(WIN_OBJ_DIR)/%.o: %.c | $(WIN_OBJ_DIR)
	@mkdir -p $(dir $@)
	$(WIN_CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR) $(WIN_BIN_DIR) $(WIN_OBJ_DIR):
	mkdir -p $@

clean_build:
	@rm -rf $(OBJ_DIR)

clean_build_win:
	@rm -rf $(WIN_OBJ_DIR)

run: $(TARGET)
	./$(TARGET)

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

install: $(TARGET)
	install -D -m 0755 $(TARGET) $(DESTDIR)$(BINDIR)/xen

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(WIN_OBJ_DIR) $(WIN_BIN_DIR)

.PHONY: all clean debug release windows-debug windows-release run clean_build clean_build_win
