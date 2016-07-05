BUILD_DIR := build

default: cvox jvox

debug:
	@ $(MAKE) -f c/cvox.make MODE=debug

watch:
	@ python script/build.py --watch

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
	@ $(MAKE) -f util/java.make DIR=gen/chap03_syntax PACKAGE=tool
	$(call run_generate_ast,chap03_syntax)
	@ $(MAKE) -f util/java.make DIR=gen/chap04_parsing PACKAGE=tool
	$(call run_generate_ast,chap04_parsing)
	@ $(MAKE) -f util/java.make DIR=gen/chap05_interpreting PACKAGE=tool
	$(call run_generate_ast,chap05_interpreting)
	@ $(MAKE) -f util/java.make DIR=gen/chap06_variables PACKAGE=tool
	$(call run_generate_ast,chap06_variables)

.PHONY: clean cvox debug default jvox test test_c test_java watch
