BUILD_DIR := build
JVOX_DIR := com/craftinginterpreters/vox

JAVA_OPTIONS := -Werror

JAVA_SOURCES := $(wildcard java/$(JVOX_DIR)/*.java)
JAVA_CLASSES := $(addprefix $(BUILD_DIR)/, $(JAVA_SOURCES:.java=.class))

default: cvox jvox

debug:
	@ $(MAKE) -f c/cvox.make MODE=debug

# TODO: Get this working even if the first returns non-zero.
test: test_java test_c
	@ python script/test.py

test_c: debug
	@ python script/test.py --c

test_java: jvox
	@ python script/test.py --java

# Remove all build outputs and intermediate files.
clean:
	@ rm -rf $(BUILD_DIR)

# Compile the C interpreter.
cvox:
	@ $(MAKE) -f c/cvox.make MODE=release
	@ cp build/cvox cvox # For convenience, copy the interpreter to the top level.

# Compile the Java interpreter .java files to .class files.
jvox: $(JAVA_CLASSES)

# Compile a single .java file to .class.
$(BUILD_DIR)/java/$(JVOX_DIR)/%.class: java/$(JVOX_DIR)/%.java
	@ mkdir -p $(BUILD_DIR)/java
	@ javac -cp java -d $(BUILD_DIR)/java $(JAVA_OPTIONS) -implicit:none $<
	@ printf "%10s %-60s %s\n" javac $< "$(JAVA_OPTIONS)"

.PHONY: clean cvox debug default jvox test test_c test_java
