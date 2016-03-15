#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "vm.h"

#define MAX_LINE_LENGTH 1024

static void repl() {
  char line[MAX_LINE_LENGTH];
  for (;;) {
    printf("> ");
    
    if (!fgets(line, MAX_LINE_LENGTH, stdin)) {
      printf("\n");
      break;
    }
    
    interpret(line);
  }
}

// Reads the contents of the file at [path] and returns it as a heap allocated
// string.
//
// Exits if it was found but could not be found or read.
static char* readFile(const char* path) {
  FILE* file = fopen(path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Could not find file \"%s\".\n", path);
    exit(74);
  }
  
  // Find out how big the file is.
  fseek(file, 0L, SEEK_END);
  size_t fileSize = ftell(file);
  rewind(file);
  
  // Allocate a buffer for it.
  char* buffer = (char*)malloc(fileSize + 1);
  if (buffer == NULL) {
    fprintf(stderr, "Could not read file \"%s\".\n", path);
    exit(74);
  }
  
  // Read the entire file.
  size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
  if (bytesRead < fileSize) {
    fprintf(stderr, "Could not read file \"%s\".\n", path);
    exit(74);
  }
  
  // Terminate the string.
  buffer[bytesRead] = '\0';
  
  fclose(file);
  return buffer;
}

static void runFile(const char* path) {
  char* source = readFile(path);
  bool compiled = interpret(source);
  free(source);
  
  if (!compiled) exit(65);
}

int main(int argc, const char* argv[]) {
  initVM();
  
  if (argc == 1) {
    repl();
  } else if (argc == 2) {
    runFile(argv[1]);
  } else {
    fprintf(stderr, "Usage: cvox [path]\n");
    exit(64);
  }
  
  endVM();
  
  return 0;
}
