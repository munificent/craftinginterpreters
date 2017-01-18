# Compile the Java file to a class file.
Hello.class: Hello.java
	@ javac Hello.java

# Convenience target to build and run it.
run: Hello.class
	@ java Hello

# Tell make that "run" is not the name of a file.
.PHONY: run
