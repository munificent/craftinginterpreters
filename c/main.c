//> Chunks of Bytecode main-c
//> Scanning on Demand not-yet
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//< Scanning on Demand not-yet
#include "common.h"
/* Chunks of Bytecode main-include-chunk < Scanning on Demand not-yet
#include "chunk.h"
*/
/* Chunks of Bytecode main-include-debug < Scanning on Demand not-yet
#include "debug.h"
*/
//> A Virtual Machine main-include-vm
#include "vm.h"
//< A Virtual Machine main-include-vm
//> Scanning on Demand not-yet

#define MAX_LINE_LENGTH 1024

static void repl() {
  char line[MAX_LINE_LENGTH];
  for (;;) {
    printf("> ");

    if (!fgets(line, MAX_LINE_LENGTH, stdin)) {
      printf("\n");
      break;
    }

    line[strlen(line) - 1] = '\0';
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
  InterpretResult result = interpret(source);
  free(source);

  if (result == INTERPRET_COMPILE_ERROR) exit(65);
  if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}
//< Scanning on Demand not-yet

int main(int argc, const char* argv[]) {
//> A Virtual Machine main-init-vm
  initVM();

//< A Virtual Machine main-init-vm
/* Chunks of Bytecode main-chunk < Scanning on Demand not-yet
  Chunk chunk;
  initChunk(&chunk);
*/
/* Chunks of Bytecode main-constant < Scanning on Demand not-yet
 
  int constant = addConstant(&chunk, 1.2);
*/
/* Chunks of Bytecode main-constant < Chunks of Bytecode main-chunk-line
  writeChunk(&chunk, OP_CONSTANT);
  writeChunk(&chunk, constant);

*/
/* Chunks of Bytecode main-chunk-line < Scanning on Demand not-yet
  writeChunk(&chunk, OP_CONSTANT, 123);
  writeChunk(&chunk, constant, 123);
*/
/* A Virtual Machine main-chunk < Scanning on Demand not-yet
 
  constant = addConstant(&chunk, 3.4);
  writeChunk(&chunk, OP_CONSTANT, 123);
  writeChunk(&chunk, constant, 123);
 
  writeChunk(&chunk, OP_ADD, 123);
 
  constant = addConstant(&chunk, 5.6);
  writeChunk(&chunk, OP_CONSTANT, 123);
  writeChunk(&chunk, constant, 123);
 
  writeChunk(&chunk, OP_SUBTRACT, 123);
*/
/* A Virtual Machine main-negate < Scanning on Demand not-yet
  writeChunk(&chunk, OP_NEGATE, 123);
*/
/* Chunks of Bytecode main-chunk < Chunks of Bytecode main-chunk-line
  writeChunk(&chunk, OP_RETURN);
*/
/* Chunks of Bytecode main-chunk-line < Scanning on Demand not-yet

  writeChunk(&chunk, OP_RETURN, 123);
*/
/* Chunks of Bytecode main-disassemble-chunk < Scanning on Demand not-yet
 
  disassembleChunk(&chunk, "test chunk");
*/
/* A Virtual Machine main-interpret < Scanning on Demand not-yet
  interpret(&chunk);
*/
//> Scanning on Demand not-yet
  if (argc == 1) {
    repl();
  } else if (argc == 2) {
    runFile(argv[1]);
  } else {
    fprintf(stderr, "Usage: clox [path]\n");
    exit(64);
  }
//< Scanning on Demand not-yet
//> A Virtual Machine main-free-vm
  freeVM();
//< A Virtual Machine main-free-vm
/* Chunks of Bytecode main-chunk < Scanning on Demand not-yet
  freeChunk(&chunk);
*/
  return 0;
}
