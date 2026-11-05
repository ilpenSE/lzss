# Build options
CC    ?= clang
BUILD ?= build
EXE   ?= lzss

# Options to configure output executable
r    ?= 0
asan ?= 0
msan ?= 0

CFLAGS = -Wall -Wextra
CFLAGS_UBASAN = -fsanitize=undefined,address
CFLAGS_MSAN = -fsanitize=memory -fno-omit-frame-pointer
CFLAGS_DEBUG = -O0 -ggdb
CFLAGS_RELEASE = -O3 -DNDEBUG

ifeq ($(r),1)
	CFLAGS += $(CFLAGS_RELEASE)
else
	CFLAGS += $(CFLAGS_DEBUG)
endif

ifeq ($(asan),1)
	CFLAGS += $(CFLAGS_UBASAN)
endif

ifeq ($(msan),1)
	CFLAGS += $(CFLAGS_MSAN)
endif

all: $(BUILD) $(BUILD)/$(EXE)

$(BUILD):
	mkdir -p $@

$(BUILD)/$(EXE): main.c str.h bw.h
	$(CC) $(CFLAGS) -o ./$@ $<
