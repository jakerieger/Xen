BUILD_TYPE ?= debug 
TARGET_PLATFORM ?= native

HOST_UNAME := $(shell uname -s)

ifeq ($(TARGET_PLATFORM),native)
    ifeq ($(HOST_UNAME),Linux)
        TARGET_PLATFORM = linux
    endif
    ifeq ($(HOST_UNAME),Darwin)
        TARGET_PLATFORM = macos
    endif
endif

ifeq ($(TARGET_PLATFORM),linux)
    export CC = gcc
    export CFLAGS = -w -std=c11 -D_DEFAULT_SOURCE
    export LDFLAGS = -lpthread -lm -ldl
    export AR = ar
    export EXT =
    export PLATFORM = linux
endif

ifeq ($(TARGET_PLATFORM),macos)
    export CC = x86_64-apple-darwin25.2-clang
    export CFLAGS = -w -std=c11 -D_DEFAULT_SOURCE
    export LDFLAGS = -framework CoreFoundation
    export AR = x86_64-apple-darwin25.2-ar
    export EXT =
    export PLATFORM = macos
endif

ifeq ($(TARGET_PLATFORM),macos-arm)
    export CC = arm64-apple-darwin25.2-clang
    export CFLAGS = -w -std=c11 -D_DEFAULT_SOURCE
    export LDFLAGS = -framework CoreFoundation
    export AR = arm64-apple-darwin25.2-ar
    export EXT =
    export PLATFORM = macos-arm
endif

ifeq ($(TARGET_PLATFORM),windows)
    export CC = x86_64-w64-mingw32-gcc
    export CFLAGS = -w -std=c11 -D_DEFAULT_SOURCE
    export LDFLAGS = -lws2_32 -static
    export AR = x86_64-w64-mingw32-ar
    export EXT = .exe
    export PLATFORM = windows
endif

ifeq ($(BUILD_TYPE),debug)
    export CFLAGS += -g -O0 -DDEBUG
    export CONFIG = debug
else
    export CFLAGS += -O2 -DNDEBUG
    export CONFIG = release
endif

export BUILD_DIR = build/$(PLATFORM)-$(CONFIG)
export OBJ_DIR = $(BUILD_DIR)/obj
export BIN_DIR = $(BUILD_DIR)/bin
export LIB_DIR = $(BUILD_DIR)/lib
export SRC_DIR = src

# Reset targets list
ALL_TARGETS :=

# Include module makefiles
include src/xen/xen.mk
include src/xenc/xenc.mk

.PHONY: all clean dirs info all-platforms

all: dirs $(ALL_TARGETS)

dirs:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR)

clean:
	rm -rf build/$(PLATFORM)-$(CONFIG)

clean-all:
	rm -rf build

all-platforms:
	$(MAKE) TARGET_PLATFORM=linux BUILD_TYPE=release
	$(MAKE) TARGET_PLATFORM=linux BUILD_TYPE=debug
	$(MAKE) TARGET_PLATFORM=windows BUILD_TYPE=release
	$(MAKE) TARGET_PLATFORM=windows BUILD_TYPE=debug
	$(MAKE) TARGET_PLATFORM=macos BUILD_TYPE=release
	$(MAKE) TARGET_PLATFORM=macos BUILD_TYPE=debug
	$(MAKE) TARGET_PLATFORM=macos-arm BUILD_TYPE=release
	$(MAKE) TARGET_PLATFORM=macos-arm BUILD_TYPE=debug

info:
	@echo "Target Platform: $(TARGET_PLATFORM)"
	@echo "Host Platform: $(HOST_UNAME)"
	@echo "Build Type: $(BUILD_TYPE)"
	@echo "Compiler: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "Build Dir: $(BUILD_DIR)"
	@echo "Targets: $(ALL_TARGETS)"
