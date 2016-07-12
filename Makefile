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
	@ python script/test.py c

test_java: jvox
	@ python script/test.py java

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

	@ $(MAKE) -f util/java.make DIR=gen/chap01_framework PACKAGE=vox

	@ $(MAKE) -f util/java.make DIR=gen/chap02_scanning PACKAGE=vox

	@ $(MAKE) -f util/java.make DIR=gen/chap03_representing PACKAGE=tool
	$(call run_generate_ast,chap03_representing)
	@ $(MAKE) -f util/java.make DIR=gen/chap03_representing PACKAGE=vox

	@ $(MAKE) -f util/java.make DIR=gen/chap04_parsing PACKAGE=tool
	$(call run_generate_ast,chap04_parsing)
	@ $(MAKE) -f util/java.make DIR=gen/chap04_parsing PACKAGE=vox

	@ $(MAKE) -f util/java.make DIR=gen/chap05_evaluating PACKAGE=tool
	$(call run_generate_ast,chap05_evaluating)
	@ $(MAKE) -f util/java.make DIR=gen/chap05_evaluating PACKAGE=vox

	@ $(MAKE) -f util/java.make DIR=gen/chap06_statements PACKAGE=tool
	$(call run_generate_ast,chap06_statements)
	@ $(MAKE) -f util/java.make DIR=gen/chap06_statements PACKAGE=vox

	@ $(MAKE) -f util/java.make DIR=gen/chap07_control PACKAGE=tool
	$(call run_generate_ast,chap07_control)
	@ $(MAKE) -f util/java.make DIR=gen/chap07_control PACKAGE=vox

	@ $(MAKE) -f util/java.make DIR=gen/chap08_functions PACKAGE=tool
	$(call run_generate_ast,chap08_functions)
	@ $(MAKE) -f util/java.make DIR=gen/chap08_functions PACKAGE=vox

	@ $(MAKE) -f util/java.make DIR=gen/chap09_blocks PACKAGE=tool
	$(call run_generate_ast,chap09_blocks)
	@ $(MAKE) -f util/java.make DIR=gen/chap09_blocks PACKAGE=vox

.PHONY: clean cvox debug default jvox test test_c test_java watch
