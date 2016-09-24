# Makefile for building a single directory of Java source files. It requires
# a DIR variable to be set.

BUILD_DIR := build

SOURCES := $(wildcard $(DIR)/com/craftinginterpreters/$(PACKAGE)/*.java)
CLASSES := $(addprefix $(BUILD_DIR)/, $(SOURCES:.java=.class))

JAVA_OPTIONS := -Werror

default: $(CLASSES)

# Compile a single .java file to .class.
$(BUILD_DIR)/$(DIR)/%.class: $(DIR)/%.java
	@ mkdir -p $(BUILD_DIR)/$(DIR)
	@ javac -cp $(DIR) -d $(BUILD_DIR)/$(DIR) $(JAVA_OPTIONS) -implicit:none $<
	@ printf "%10s %-60s %s\n" javac $< "$(JAVA_OPTIONS)"

.PHONY: default
