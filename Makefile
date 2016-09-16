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

# TODO: Unify with make chapters, and make more demand-driven.
c_chapters:
	@ python util/split_chapters.py
	@ $(MAKE) -f util/c.make NAME=chap14_chunks MODE=release SOURCE_DIR=gen/chap14_chunks
	@ $(MAKE) -f util/c.make NAME=chap15_virtual MODE=release SOURCE_DIR=gen/chap15_virtual
	@ $(MAKE) -f util/c.make NAME=chap16_scanning MODE=release SOURCE_DIR=gen/chap16_scanning
	@ $(MAKE) -f util/c.make NAME=chap17_compiling MODE=release SOURCE_DIR=gen/chap17_compiling
	@ $(MAKE) -f util/c.make NAME=chap18_types MODE=release SOURCE_DIR=gen/chap18_types
	@ $(MAKE) -f util/c.make NAME=chap19_strings MODE=release SOURCE_DIR=gen/chap19_strings
	@ $(MAKE) -f util/c.make NAME=chap20_hash MODE=release SOURCE_DIR=gen/chap20_hash
	@ $(MAKE) -f util/c.make NAME=chap21_global MODE=release SOURCE_DIR=gen/chap21_global
	@ $(MAKE) -f util/c.make NAME=chap22_local MODE=release SOURCE_DIR=gen/chap22_local
	@ $(MAKE) -f util/c.make NAME=chap23_jumping MODE=release SOURCE_DIR=gen/chap23_jumping
	@ $(MAKE) -f util/c.make NAME=chap24_function MODE=release SOURCE_DIR=gen/chap24_function
	@ $(MAKE) -f util/c.make NAME=chap25_user MODE=release SOURCE_DIR=gen/chap25_user
	@ $(MAKE) -f util/c.make NAME=chap26_closures MODE=release SOURCE_DIR=gen/chap26_closures
	@ $(MAKE) -f util/c.make NAME=chap27_garbage MODE=release SOURCE_DIR=gen/chap27_garbage
	@ $(MAKE) -f util/c.make NAME=chap28_classes MODE=release SOURCE_DIR=gen/chap28_classes
	@ $(MAKE) -f util/c.make NAME=chap29_methods MODE=release SOURCE_DIR=gen/chap29_methods
	@ $(MAKE) -f util/c.make NAME=chap30_superclasses MODE=release SOURCE_DIR=gen/chap30_superclasses
	@ $(MAKE) -f util/c.make NAME=chap31_optimization MODE=release SOURCE_DIR=gen/chap31_optimization

diffs:
	@ mkdir -p build/diffs
	@ -diff --recursive --new-file nonexistent/ gen/chap03_pancake/com/craftinginterpreters/ > build/diffs/chap03_pancake.diff
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

	@ -diff --new-file nonexistent/ gen/chap14_chunks/ > build/diffs/chap14_chunks.diff
	@ -diff --new-file gen/chap14_chunks/ gen/chap15_virtual/ > build/diffs/chap15_virtual.diff
	@ -diff --new-file gen/chap15_virtual/ gen/chap16_scanning/ > build/diffs/chap16_scanning.diff
	@ -diff --new-file gen/chap16_scanning/ gen/chap17_compiling/ > build/diffs/chap17_compiling.diff
	@ -diff --new-file gen/chap17_compiling/ gen/chap18_types/ > build/diffs/chap18_types.diff
	@ -diff --new-file gen/chap18_types/ gen/chap19_strings/ > build/diffs/chap19_strings.diff
	@ -diff --new-file gen/chap19_strings/ gen/chap20_hash/ > build/diffs/chap20_hash.diff
	@ -diff --new-file gen/chap20_hash/ gen/chap21_global/ > build/diffs/chap21_global.diff
	@ -diff --new-file gen/chap21_global/ gen/chap22_local/ > build/diffs/chap22_local.diff
	@ -diff --new-file gen/chap22_local/ gen/chap23_jumping/ > build/diffs/chap23_jumping.diff
	@ -diff --new-file gen/chap23_jumping/ gen/chap24_function/ > build/diffs/chap24_function.diff
	@ -diff --new-file gen/chap24_function/ gen/chap25_user/ > build/diffs/chap25_user.diff
	@ -diff --new-file gen/chap25_user/ gen/chap26_closures/ > build/diffs/chap26_closures.diff
	@ -diff --new-file gen/chap26_closures/ gen/chap27_garbage/ > build/diffs/chap27_garbage.diff
	@ -diff --new-file gen/chap27_garbage/ gen/chap28_classes/ > build/diffs/chap28_classes.diff
	@ -diff --new-file gen/chap28_classes/ gen/chap29_methods/ > build/diffs/chap29_methods.diff
	@ -diff --new-file gen/chap29_methods/ gen/chap30_superclasses/ > build/diffs/chap30_superclasses.diff
	@ -diff --new-file gen/chap30_superclasses/ gen/chap31_optimization/ > build/diffs/chap31_optimization.diff

.PHONY: clean clox debug default diffs jlox test test_c test_java watch
