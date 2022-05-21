BUILD_DIR := build
TOOL_SOURCES := tool/pubspec.lock $(shell find tool -name '*.dart')
BUILD_SNAPSHOT := $(BUILD_DIR)/build.dart.snapshot
TEST_SNAPSHOT := $(BUILD_DIR)/test.dart.snapshot

default: book jlox

# Run dart pub get on tool directory.
get:
	@ cd ./tool; dart pub get

# Remove all build outputs and intermediate files.
clean:
	@ rm -rf $(BUILD_DIR)
	@ rm -rf gen

# Build the site.
book: $(BUILD_SNAPSHOT)
	@ dart $(BUILD_SNAPSHOT)

# Run a local development server for the site that rebuilds automatically.
serve: $(BUILD_SNAPSHOT)
	@ dart $(BUILD_SNAPSHOT) --serve

$(BUILD_SNAPSHOT): $(TOOL_SOURCES)
	@ mkdir -p build
	@ echo "Compiling Dart snapshot..."
	@ dart --snapshot=$@ --snapshot-kind=app-jit tool/bin/build.dart >/dev/null

# Run the tests for the final versions of jlox.
test: jlox $(TEST_SNAPSHOT)
	@ dart $(TEST_SNAPSHOT) jlox

# Run the tests for the final version of jlox.
test_jlox: jlox $(TEST_SNAPSHOT)
	@ dart $(TEST_SNAPSHOT) jlox

# Run the tests for every chapter's version of jlox.
test_java: jlox java_chapters $(TEST_SNAPSHOT)
	@ dart $(TEST_SNAPSHOT) java

# Run the tests for every chapter's version of and jlox.
test_all: jlox java_chapters compile_snippets $(TEST_SNAPSHOT)
	@ dart $(TEST_SNAPSHOT) all

$(TEST_SNAPSHOT): $(TOOL_SOURCES)
	@ mkdir -p build
	@ echo "Compiling Dart snapshot..."
	@ dart --snapshot=$@ --snapshot-kind=app-jit tool/bin/test.dart jlox # >/dev/null

# Compile and run the AST generator.
generate_ast:
	@ $(MAKE) -f util/java.make DIR=java PACKAGE=tool
	@ java -cp build/java com.craftinginterpreters.tool.GenerateAst \
			java/com/craftinginterpreters/lox

# Compile the Java interpreter .java files to .class files.
jlox: generate_ast
	@ $(MAKE) -f util/java.make DIR=java PACKAGE=lox

run_generate_ast = @ java -cp build/gen/$(1) \
			com.craftinginterpreters.tool.GenerateAst \
			gen/$(1)/com/craftinginterpreters/lox

java_chapters: split_chapters
	@ $(MAKE) -f util/java.make DIR=gen/chap04_scanning PACKAGE=lox

	@ $(MAKE) -f util/java.make DIR=gen/chap05_representing PACKAGE=tool
	$(call run_generate_ast,chap05_representing)
	@ $(MAKE) -f util/java.make DIR=gen/chap05_representing PACKAGE=lox

	@ $(MAKE) -f util/java.make DIR=gen/chap06_parsing PACKAGE=tool
	$(call run_generate_ast,chap06_parsing)
	@ $(MAKE) -f util/java.make DIR=gen/chap06_parsing PACKAGE=lox

	@ $(MAKE) -f util/java.make DIR=gen/chap07_evaluating PACKAGE=tool
	$(call run_generate_ast,chap07_evaluating)
	@ $(MAKE) -f util/java.make DIR=gen/chap07_evaluating PACKAGE=lox

	@ $(MAKE) -f util/java.make DIR=gen/chap08_statements PACKAGE=tool
	$(call run_generate_ast,chap08_statements)
	@ $(MAKE) -f util/java.make DIR=gen/chap08_statements PACKAGE=lox

	@ $(MAKE) -f util/java.make DIR=gen/chap09_control PACKAGE=tool
	$(call run_generate_ast,chap09_control)
	@ $(MAKE) -f util/java.make DIR=gen/chap09_control PACKAGE=lox

	@ $(MAKE) -f util/java.make DIR=gen/chap10_functions PACKAGE=tool
	$(call run_generate_ast,chap10_functions)
	@ $(MAKE) -f util/java.make DIR=gen/chap10_functions PACKAGE=lox

	@ $(MAKE) -f util/java.make DIR=gen/chap11_resolving PACKAGE=tool
	$(call run_generate_ast,chap11_resolving)
	@ $(MAKE) -f util/java.make DIR=gen/chap11_resolving PACKAGE=lox

	@ $(MAKE) -f util/java.make DIR=gen/chap12_classes PACKAGE=tool
	$(call run_generate_ast,chap12_classes)
	@ $(MAKE) -f util/java.make DIR=gen/chap12_classes PACKAGE=lox

	@ $(MAKE) -f util/java.make DIR=gen/chap13_inheritance PACKAGE=tool
	$(call run_generate_ast,chap13_inheritance)
	@ $(MAKE) -f util/java.make DIR=gen/chap13_inheritance PACKAGE=lox

diffs: split_chapters java_chapters
	@ mkdir -p build/diffs
	@ -diff --recursive --new-file nonexistent/ gen/chap04_scanning/com/craftinginterpreters/ > build/diffs/chap04_scanning.diff
	@ -diff --recursive --new-file gen/chap04_scanning/com/craftinginterpreters/ gen/chap05_representing/com/craftinginterpreters/ > build/diffs/chap05_representing.diff
	@ -diff --recursive --new-file gen/chap05_representing/com/craftinginterpreters/ gen/chap06_parsing/com/craftinginterpreters/ > build/diffs/chap06_parsing.diff
	@ -diff --recursive --new-file gen/chap06_parsing/com/craftinginterpreters/ gen/chap07_evaluating/com/craftinginterpreters/ > build/diffs/chap07_evaluating.diff
	@ -diff --recursive --new-file gen/chap07_evaluating/com/craftinginterpreters/ gen/chap08_statements/com/craftinginterpreters/ > build/diffs/chap08_statements.diff
	@ -diff --recursive --new-file gen/chap08_statements/com/craftinginterpreters/ gen/chap09_control/com/craftinginterpreters/ > build/diffs/chap09_control.diff
	@ -diff --recursive --new-file gen/chap09_control/com/craftinginterpreters/ gen/chap10_functions/com/craftinginterpreters/ > build/diffs/chap10_functions.diff
	@ -diff --recursive --new-file gen/chap10_functions/com/craftinginterpreters/ gen/chap11_resolving/com/craftinginterpreters/ > build/diffs/chap11_resolving.diff
	@ -diff --recursive --new-file gen/chap11_resolving/com/craftinginterpreters/ gen/chap12_classes/com/craftinginterpreters/ > build/diffs/chap12_classes.diff
	@ -diff --recursive --new-file gen/chap12_classes/com/craftinginterpreters/ gen/chap13_inheritance/com/craftinginterpreters/ > build/diffs/chap13_inheritance.diff

split_chapters:
	@ dart tool/bin/split_chapters.dart

compile_snippets:
	@ dart tool/bin/compile_snippets.dart

# Generate the XML for importing into InDesign.
xml: $(TOOL_SOURCES)
	@ dart --enable-asserts tool/bin/build_xml.dart

.PHONY: book clean compile_snippets default diffs \
	get java_chapters jlox serve split_chapters test test_all test_java
