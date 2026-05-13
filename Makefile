CC       := gcc
CFLAGS   := -Wall -Wextra -std=c99 -pedantic -Iinclude/ -MMD -MP
CXX      := g++
CXXFLAGS := -Wall -Wextra -std=c++11 $(addprefix -I, $(LIB_DIRS))
SRCS     := $(wildcard src/*.c)
NAME     := devault

LIB_DIRS  := $(wildcard libs/*)
LIB_NAMES := $(notdir $(LIB_DIRS))

CFLAGS += $(addprefix -I, $(LIB_DIRS))

DEBUG_OBJS   := $(patsubst src/%.c, build/debug/%.o,   $(SRCS))
RELEASE_OBJS := $(patsubst src/%.c, build/release/%.o, $(SRCS))

DEBUG_LIBS   := $(addprefix build/debug/libs/lib,   $(addsuffix .a, $(LIB_NAMES)))
RELEASE_LIBS := $(addprefix build/release/libs/lib, $(addsuffix .a, $(LIB_NAMES)))

DEBUG_LDFLAGS   := -Lbuild/debug/libs   $(addprefix -l, $(LIB_NAMES))
RELEASE_LDFLAGS := -Lbuild/release/libs $(addprefix -l, $(LIB_NAMES))
DEBUG_LDFLAGS += -lcurl
RELEASE_LDFLAGS += -lcurl


.PHONY: all debug release clean help

all: debug

debug:   build/$(NAME)_debug
release: build/$(NAME)

build/debug/%.o: src/%.c | build/debug
	$(CC) $(CFLAGS) -g -O0 -c $< -o $@

build/release/%.o: src/%.c | build/release
	$(CC) $(CFLAGS) -O2 -DNDEBUG -c $< -o $@

# Pattern: libs/name/name.{c,cpp}  →  build/.../libs/libname.a
define LIB_RULES
LIB_$(1)_SRC := $(or $(wildcard libs/$(1)/$(1).cpp), libs/$(1)/$(1).c)
LIB_$(1)_CC  := $(if $(wildcard libs/$(1)/$(1).cpp), $$(CXX), $$(CC))
LIB_$(1)_FLAGS := $(if $(wildcard libs/$(1)/$(1).cpp), $$(CXXFLAGS), $$(CFLAGS))

build/debug/libs/$(1).o: $$(LIB_$(1)_SRC) | build/debug/libs
	$$(LIB_$(1)_CC) $$(LIB_$(1)_FLAGS) -g -O0 -c $$< -o $$@

build/release/libs/$(1).o: $$(LIB_$(1)_SRC) | build/release/libs
	$$(LIB_$(1)_CC) $$(LIB_$(1)_FLAGS) -O2 -DNDEBUG -c $$< -o $$@

build/debug/libs/lib$(1).a: build/debug/libs/$(1).o | build/debug/libs
	$$(AR) rcs $$@ $$^

build/release/libs/lib$(1).a: build/release/libs/$(1).o | build/release/libs
	$$(AR) rcs $$@ $$^
endef

$(foreach lib, $(LIB_NAMES), $(eval $(call LIB_RULES,$(lib))))

build/$(NAME)_debug: $(DEBUG_OBJS) $(DEBUG_LIBS) | build
	$(CXX) $(LDFLAGS) $(DEBUG_OBJS) $(DEBUG_LDFLAGS) -o $@

build/$(NAME): $(RELEASE_OBJS) $(RELEASE_LIBS) | build
	$(CXX) $(LDFLAGS) $(RELEASE_OBJS) $(RELEASE_LDFLAGS) -o $@

build build/debug build/release build/debug/libs build/release/libs:
	mkdir -p $@

help:
	@echo "Targets: all, debug, release, clean"

clean:
	rm -rf build/debug build/release

-include $(wildcard build/debug/*.d build/release/*.d \
                    build/debug/libs/*.d build/release/libs/*.d)
