BUILD_DIR := build

default: cvox jvox

debug:
	@ $(MAKE) -f util/c.make NAME=cvoxd MODE=debug SOURCE_DIR=c

watch:
	@ python script/build.py --watch

# TODO: Get this working even if the first returns non-zero.
test: test_java test_c
	@ python script/test.py

test_c: debug
	@ python script/test.py c

test_java: jvox
	@ python script/test.py java

# Remove all build outputs and intermediate files.
clean:
	@ rm -rf $(BUILD_DIR)

# Compile the C interpreter.
cvox:
	@ $(MAKE) -f util/c.make NAME=cvox MODE=release SOURCE_DIR=c
	@ cp build/cvox cvox # For convenience, copy the interpreter to the top level.

# Compile and run the AST generator.
generate_ast:
	@ $(MAKE) -f util/java.make DIR=java PACKAGE=tool
	@ java -cp build/java com.craftinginterpreters.tool.GenerateAst \
			java/com/craftinginterpreters/vox

# Compile the Java interpreter .java files to .class files.
jvox: generate_ast
	@ $(MAKE) -f util/java.make DIR=java PACKAGE=vox

run_generate_ast = 	@ java -cp build/gen/$(1) \
			com.craftinginterpreters.tool.GenerateAst \
			gen/$(1)/com/craftinginterpreters/vox

chapters:
	@ python script/split_chapters.py

	@ $(MAKE) -f util/java.make DIR=gen/chap04_framework PACKAGE=vox

	@ $(MAKE) -f util/java.make DIR=gen/chap05_scanning PACKAGE=vox

	@ $(MAKE) -f util/java.make DIR=gen/chap06_representing PACKAGE=tool
	$(call run_generate_ast,chap06_representing)
	@ $(MAKE) -f util/java.make DIR=gen/chap06_representing PACKAGE=vox

	@ $(MAKE) -f util/java.make DIR=gen/chap07_parsing PACKAGE=tool
	$(call run_generate_ast,chap07_parsing)
	@ $(MAKE) -f util/java.make DIR=gen/chap07_parsing PACKAGE=vox

	@ $(MAKE) -f util/java.make DIR=gen/chap08_evaluating PACKAGE=tool
	$(call run_generate_ast,chap08_evaluating)
	@ $(MAKE) -f util/java.make DIR=gen/chap08_evaluating PACKAGE=vox

	@ $(MAKE) -f util/java.make DIR=gen/chap09_statements PACKAGE=tool
	$(call run_generate_ast,chap09_statements)
	@ $(MAKE) -f util/java.make DIR=gen/chap09_statements PACKAGE=vox

	@ $(MAKE) -f util/java.make DIR=gen/chap10_control PACKAGE=tool
	$(call run_generate_ast,chap10_control)
	@ $(MAKE) -f util/java.make DIR=gen/chap10_control PACKAGE=vox

	@ $(MAKE) -f util/java.make DIR=gen/chap11_functions PACKAGE=tool
	$(call run_generate_ast,chap11_functions)
	@ $(MAKE) -f util/java.make DIR=gen/chap11_functions PACKAGE=vox

	@ $(MAKE) -f util/java.make DIR=gen/chap12_resolving PACKAGE=tool
	$(call run_generate_ast,chap12_resolving)
	@ $(MAKE) -f util/java.make DIR=gen/chap12_resolving PACKAGE=vox

	@ $(MAKE) -f util/java.make DIR=gen/chap13_classes PACKAGE=tool
	$(call run_generate_ast,chap13_classes)
	@ $(MAKE) -f util/java.make DIR=gen/chap13_classes PACKAGE=vox

	@ $(MAKE) -f util/java.make DIR=gen/chap14_inheritance PACKAGE=tool
	$(call run_generate_ast,chap14_inheritance)
	@ $(MAKE) -f util/java.make DIR=gen/chap14_inheritance PACKAGE=vox

# TODO: Unify with make chapters, and make more demand-driven.
c_chapters:
	@ python script/split_chapters.py
	@ $(MAKE) -f util/c.make NAME=chap16_chunks MODE=release SOURCE_DIR=gen/chap16_chunks
	@ $(MAKE) -f util/c.make NAME=chap17_virtual MODE=release SOURCE_DIR=gen/chap17_virtual
	@ $(MAKE) -f util/c.make NAME=chap18_scanning MODE=release SOURCE_DIR=gen/chap18_scanning
	@ $(MAKE) -f util/c.make NAME=chap19_compiling MODE=release SOURCE_DIR=gen/chap19_compiling
	@ $(MAKE) -f util/c.make NAME=chap20_types MODE=release SOURCE_DIR=gen/chap20_types
	@ $(MAKE) -f util/c.make NAME=chap21_strings MODE=release SOURCE_DIR=gen/chap21_strings
	@ $(MAKE) -f util/c.make NAME=chap22_hash MODE=release SOURCE_DIR=gen/chap22_hash

diffs:
	@ mkdir -p build/diffs
	@ -diff --recursive --new-file gen/chap04_framework/com/craftinginterpreters/ gen/chap05_scanning/com/craftinginterpreters/ > build/diffs/chap05_scanning.diff
	@ -diff --recursive --new-file gen/chap05_scanning/com/craftinginterpreters/ gen/chap06_representing/com/craftinginterpreters/ > build/diffs/chap06_representing.diff
	@ -diff --recursive --new-file gen/chap06_representing/com/craftinginterpreters/ gen/chap07_parsing/com/craftinginterpreters/ > build/diffs/chap07_parsing.diff
	@ -diff --recursive --new-file gen/chap07_parsing/com/craftinginterpreters/ gen/chap08_evaluating/com/craftinginterpreters/ > build/diffs/chap08_evaluating.diff
	@ -diff --recursive --new-file gen/chap08_evaluating/com/craftinginterpreters/ gen/chap09_statements/com/craftinginterpreters/ > build/diffs/chap09_statements.diff
	@ -diff --recursive --new-file gen/chap09_statements/com/craftinginterpreters/ gen/chap10_control/com/craftinginterpreters/ > build/diffs/chap10_control.diff
	@ -diff --recursive --new-file gen/chap10_control/com/craftinginterpreters/ gen/chap11_functions/com/craftinginterpreters/ > build/diffs/chap11_functions.diff
	@ -diff --recursive --new-file gen/chap11_functions/com/craftinginterpreters/ gen/chap12_resolving/com/craftinginterpreters/ > build/diffs/chap12_resolving.diff
	@ -diff --recursive --new-file gen/chap12_resolving/com/craftinginterpreters/ gen/chap13_classes/com/craftinginterpreters/ > build/diffs/chap13_classes.diff
	@ -diff --recursive --new-file gen/chap13_classes/com/craftinginterpreters/ gen/chap14_inheritance/com/craftinginterpreters/ > build/diffs/chap14_inheritance.diff
	@ -diff --recursive --new-file gen/chap14_inheritance/com/craftinginterpreters/ gen/chap15_reaching/com/craftinginterpreters/ > build/diffs/chap15_reaching.diff
	# TODO: C chapters.

.PHONY: clean cvox debug default diffs jvox test test_c test_java watch
