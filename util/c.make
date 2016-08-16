# Makefile for building a single configuration of the C interpreter. It expects
# variables to be passed in for:
#
# MODE         "debug" or "release".
# NAME         Name of the output executable (and object file directory).
# SOURCE_DIR   Directory where source files and headers are found.

CFLAGS := -std=c99 -Wall -Wextra -Werror -Wno-unused-parameter

# Mode configuration.
ifeq ($(MODE),debug)
	CFLAGS += -O0 -DDEBUG -g
	BUILD_DIR := build/debug
else
	CFLAGS += -O3
	BUILD_DIR := build/release
endif

# Files.
HEADERS := $(wildcard $(SOURCE_DIR)/*.h)
SOURCES := $(wildcard $(SOURCE_DIR)/*.c)
OBJECTS := $(addprefix $(BUILD_DIR)/$(NAME)/, $(notdir $(SOURCES:.c=.o)))

# Targets ---------------------------------------------------------------------

# Link the interpreter.
build/$(NAME): $(OBJECTS)
	@ printf "%10s %-32s %s\n" $(CC) $@ "$(CFLAGS)"
	@ mkdir -p build
	@ $(CC) $(CFLAGS) $^ -o $@

# Compile object files.
$(BUILD_DIR)/$(NAME)/%.o: $(SOURCE_DIR)/%.c $(HEADERS)
	@ printf "%10s %-32s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)/$(NAME)
	@ $(CC) -c $(CFLAGS) -o $@ $<

.PHONY: default
