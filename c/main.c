//>= Chunks of Bytecode
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
/*>= Chunks of Bytecode <= A Virtual Machine
#include "chunk.h"
*/
/*>= Chunks of Bytecode <= A Virtual Machine
#include "debug.h"
*/
//>= A Virtual Machine
#include "vm.h"
//>= Scanning on Demand

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
  InterpretResult result = interpret(source);
  free(source);

  if (result == INTERPRET_COMPILE_ERROR) exit(65);
  if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}
//>= Chunks of Bytecode

int main(int argc, const char* argv[]) {
//>= A Virtual Machine
  initVM();

/*>= Chunks of Bytecode <= A Virtual Machine
  Chunk chunk;
  initChunk(&chunk);

  addConstant(&chunk, 1.2);
  writeChunk(&chunk, OP_CONSTANT, 100);
  writeChunk(&chunk, 0, 100);

  addConstant(&chunk, 3.4);
  writeChunk(&chunk, OP_CONSTANT, 101);
  writeChunk(&chunk, 1, 101);

*/
/*== A Virtual Machine
  writeChunk(&chunk, OP_ADD, 101);

  addConstant(&chunk, 5.6);
  writeChunk(&chunk, OP_CONSTANT, 102);
  writeChunk(&chunk, 2, 100);

  writeChunk(&chunk, OP_MULTIPLY, 102);
  writeChunk(&chunk, OP_NEGATE, 102);

  writeChunk(&chunk, OP_RETURN, 102);

*/
/*>= Chunks of Bytecode <= A Virtual Machine
  disassembleChunk(&chunk, "test chunk");
*/
/*== A Virtual Machine

  interpret(&chunk);
*/
//>= Scanning on Demand
  if (argc == 1) {
    repl();
  } else if (argc == 2) {
    runFile(argv[1]);
  } else {
    fprintf(stderr, "Usage: clox [path]\n");
    exit(64);
  }
//>= A Virtual Machine

  endVM();
//>= Chunks of Bytecode
  return 0;
}
