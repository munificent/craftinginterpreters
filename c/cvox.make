# Makefile for building a single configuration of the C interpreter. It expects
# a MODE variable to be passed in, which can be either "debug" or "release".

BUILD_DIR := build

CFLAGS := -std=c99 -Wall -Wextra -Werror -Wno-unused-parameter

# Mode configuration.
ifeq ($(MODE),debug)
	CVOX := cvoxd
	CFLAGS += -O0 -DDEBUG -g
	BUILD_DIR := $(BUILD_DIR)/debug
else
	CVOX += cvox
	CFLAGS += -O3
	BUILD_DIR := $(BUILD_DIR)/release
endif

# Files.
HEADERS := $(wildcard c/*.h)
SOURCES := $(wildcard c/*.c)
OBJECTS := $(addprefix $(BUILD_DIR)/c/, $(notdir $(SOURCES:.c=.o)))

# Targets ---------------------------------------------------------------------

cvox: build/$(CVOX)

# Link the interpreter.
build/$(CVOX): $(OBJECTS)
	@ printf "%10s %-30s %s\n" $(CC) $@ "$(CFLAGS)"
	@ mkdir -p bin
	@ $(CC) $(CFLAGS) $^ -o $@

# Compile object files.
$(BUILD_DIR)/c/%.o: c/%.c $(HEADERS)
	@ printf "%10s %-30s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)/c
	@ $(CC) -c $(CFLAGS) -o $@ $<

.PHONY: cvox
