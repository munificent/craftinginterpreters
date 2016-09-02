BUILD_DIR := build

default: clox jlox

html:
	@ python util/build.py

debug:
	@ $(MAKE) -f util/c.make NAME=cloxd MODE=debug SOURCE_DIR=c

watch:
	@ python util/build.py --watch

# TODO: Get this working even if the first returns non-zero.
test: test_java test_c
	@ python util/test.py

test_c: debug
	@ python util/test.py c

test_java: jlox
	@ python util/test.py java

# Remove all build outputs and intermediate files.
clean:
	@ rm -rf $(BUILD_DIR)
	@ rm -rf gen

# Compile the C interpreter.
clox:
	@ $(MAKE) -f util/c.make NAME=clox MODE=release SOURCE_DIR=c
	@ cp build/clox clox # For convenience, copy the interpreter to the top level.

# Compile and run the AST generator.
generate_ast:
	@ $(MAKE) -f util/java.make DIR=java PACKAGE=tool
	@ java -cp build/java com.craftinginterpreters.tool.GenerateAst \
			java/com/craftinginterpreters/lox

# Compile the Java interpreter .java files to .class files.
jlox: generate_ast
	@ $(MAKE) -f util/java.make DIR=java PACKAGE=lox

run_generate_ast = 	@ java -cp build/gen/$(1) \
			com.craftinginterpreters.tool.GenerateAst \
			gen/$(1)/com/craftinginterpreters/lox

chapters:
	@ python util/split_chapters.py

	@ $(MAKE) -f util/java.make DIR=gen/chap03_pancake PACKAGE=pancake

	@ $(MAKE) -f util/java.make DIR=gen/chap04_read PACKAGE=lox

	@ $(MAKE) -f util/java.make DIR=gen/chap05_scanning PACKAGE=lox

	@ $(MAKE) -f util/java.make DIR=gen/chap06_representing PACKAGE=tool
	$(call run_generate_ast,chap06_representing)
	@ $(MAKE) -f util/java.make DIR=gen/chap06_representing PACKAGE=lox

	@ $(MAKE) -f util/java.make DIR=gen/chap07_parsing PACKAGE=tool
	$(call run_generate_ast,chap07_parsing)
	@ $(MAKE) -f util/java.make DIR=gen/chap07_parsing PACKAGE=lox

	@ $(MAKE) -f util/java.make DIR=gen/chap08_evaluating PACKAGE=tool
	$(call run_generate_ast,chap08_evaluating)
	@ $(MAKE) -f util/java.make DIR=gen/chap08_evaluating PACKAGE=lox

	@ $(MAKE) -f util/java.make DIR=gen/chap09_statements PACKAGE=tool
	$(call run_generate_ast,chap09_statements)
	@ $(MAKE) -f util/java.make DIR=gen/chap09_statements PACKAGE=lox

	@ $(MAKE) -f util/java.make DIR=gen/chap10_control PACKAGE=tool
	$(call run_generate_ast,chap10_control)
	@ $(MAKE) -f util/java.make DIR=gen/chap10_control PACKAGE=lox

	@ $(MAKE) -f util/java.make DIR=gen/chap11_functions PACKAGE=tool
	$(call run_generate_ast,chap11_functions)
	@ $(MAKE) -f util/java.make DIR=gen/chap11_functions PACKAGE=lox

	@ $(MAKE) -f util/java.make DIR=gen/chap12_resolving PACKAGE=tool
	$(call run_generate_ast,chap12_resolving)
	@ $(MAKE) -f util/java.make DIR=gen/chap12_resolving PACKAGE=lox

	@ $(MAKE) -f util/java.make DIR=gen/chap13_classes PACKAGE=tool
	$(call run_generate_ast,chap13_classes)
	@ $(MAKE) -f util/java.make DIR=gen/chap13_classes PACKAGE=lox

	@ $(MAKE) -f util/java.make DIR=gen/chap14_inheritance PACKAGE=tool
	$(call run_generate_ast,chap14_inheritance)
	@ $(MAKE) -f util/java.make DIR=gen/chap14_inheritance PACKAGE=lox

# TODO: Unify with make chapters, and make more demand-driven.
c_chapters:
	@ python util/split_chapters.py
	@ $(MAKE) -f util/c.make NAME=chap16_chunks MODE=release SOURCE_DIR=gen/chap16_chunks
	@ $(MAKE) -f util/c.make NAME=chap17_virtual MODE=release SOURCE_DIR=gen/chap17_virtual
	@ $(MAKE) -f util/c.make NAME=chap18_scanning MODE=release SOURCE_DIR=gen/chap18_scanning
	@ $(MAKE) -f util/c.make NAME=chap19_compiling MODE=release SOURCE_DIR=gen/chap19_compiling
	@ $(MAKE) -f util/c.make NAME=chap20_types MODE=release SOURCE_DIR=gen/chap20_types
	@ $(MAKE) -f util/c.make NAME=chap21_strings MODE=release SOURCE_DIR=gen/chap21_strings
	@ $(MAKE) -f util/c.make NAME=chap22_hash MODE=release SOURCE_DIR=gen/chap22_hash
	@ $(MAKE) -f util/c.make NAME=chap23_global MODE=release SOURCE_DIR=gen/chap23_global
	@ $(MAKE) -f util/c.make NAME=chap24_local MODE=release SOURCE_DIR=gen/chap24_local
	@ $(MAKE) -f util/c.make NAME=chap25_jumping MODE=release SOURCE_DIR=gen/chap25_jumping
	@ $(MAKE) -f util/c.make NAME=chap26_function MODE=release SOURCE_DIR=gen/chap26_function
	@ $(MAKE) -f util/c.make NAME=chap27_user MODE=release SOURCE_DIR=gen/chap27_user
	@ $(MAKE) -f util/c.make NAME=chap28_closures MODE=release SOURCE_DIR=gen/chap28_closures
	@ $(MAKE) -f util/c.make NAME=chap29_garbage MODE=release SOURCE_DIR=gen/chap29_garbage
	@ $(MAKE) -f util/c.make NAME=chap30_classes MODE=release SOURCE_DIR=gen/chap30_classes
	@ $(MAKE) -f util/c.make NAME=chap31_methods MODE=release SOURCE_DIR=gen/chap31_methods
	@ $(MAKE) -f util/c.make NAME=chap32_superclasses MODE=release SOURCE_DIR=gen/chap32_superclasses
	@ $(MAKE) -f util/c.make NAME=chap33_optimization MODE=release SOURCE_DIR=gen/chap33_optimization

diffs:
	@ mkdir -p build/diffs
	@ -diff --recursive --new-file nonexistent/ gen/chap03_pancake/com/craftinginterpreters/ > build/diffs/chap03_pancake.diff
	@ -diff --recursive --new-file gen/chap03_pancake/com/craftinginterpreters/ gen/chap04_read/com/craftinginterpreters/ > build/diffs/chap04_read.diff
	@ -diff --recursive --new-file gen/chap04_read/com/craftinginterpreters/ gen/chap05_scanning/com/craftinginterpreters/ > build/diffs/chap05_scanning.diff
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

	@ -diff --new-file nonexistent/ gen/chap16_chunks/ > build/diffs/chap16_chunks.diff
	@ -diff --new-file gen/chap16_chunks/ gen/chap17_virtual/ > build/diffs/chap17_virtual.diff
	@ -diff --new-file gen/chap17_virtual/ gen/chap18_scanning/ > build/diffs/chap18_scanning.diff
	@ -diff --new-file gen/chap18_scanning/ gen/chap19_compiling/ > build/diffs/chap19_compiling.diff
	@ -diff --new-file gen/chap19_compiling/ gen/chap20_types/ > build/diffs/chap20_types.diff
	@ -diff --new-file gen/chap20_types/ gen/chap21_strings/ > build/diffs/chap21_strings.diff
	@ -diff --new-file gen/chap21_strings/ gen/chap22_hash/ > build/diffs/chap22_hash.diff
	@ -diff --new-file gen/chap22_hash/ gen/chap23_global/ > build/diffs/chap23_global.diff
	@ -diff --new-file gen/chap23_global/ gen/chap24_local/ > build/diffs/chap24_local.diff
	@ -diff --new-file gen/chap24_local/ gen/chap25_jumping/ > build/diffs/chap25_jumping.diff
	@ -diff --new-file gen/chap25_jumping/ gen/chap26_function/ > build/diffs/chap26_function.diff
	@ -diff --new-file gen/chap26_function/ gen/chap27_user/ > build/diffs/chap27_user.diff
	@ -diff --new-file gen/chap27_user/ gen/chap28_closures/ > build/diffs/chap28_closures.diff
	@ -diff --new-file gen/chap28_closures/ gen/chap29_garbage/ > build/diffs/chap29_garbage.diff
	@ -diff --new-file gen/chap29_garbage/ gen/chap30_classes/ > build/diffs/chap30_classes.diff
	@ -diff --new-file gen/chap30_classes/ gen/chap31_methods/ > build/diffs/chap31_methods.diff
	@ -diff --new-file gen/chap31_methods/ gen/chap32_superclasses/ > build/diffs/chap32_superclasses.diff
	@ -diff --new-file gen/chap32_superclasses/ gen/chap33_optimization/ > build/diffs/chap33_optimization.diff

.PHONY: clean clox debug default diffs jlox test test_c test_java watch
