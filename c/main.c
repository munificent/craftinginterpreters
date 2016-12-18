//>> Chunks of Bytecode 99
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
/*>= Chunks of Bytecode 99 < Scanning on Demand 99
#include "chunk.h"
#include "debug.h"
*/
//>> A Virtual Machine 99
#include "vm.h"
//<< A Virtual Machine 99
//>> Scanning on Demand 99

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
//<< Scanning on Demand 99

int main(int argc, const char* argv[]) {
//>> A Virtual Machine 99
  initVM();

/*>= Chunks of Bytecode 99 < Scanning on Demand 99
  Chunk chunk;
  initChunk(&chunk);

  addConstant(&chunk, 1.2);
  writeChunk(&chunk, OP_CONSTANT, 100);
  writeChunk(&chunk, 0, 100);

  addConstant(&chunk, 3.4);
  writeChunk(&chunk, OP_CONSTANT, 101);
  writeChunk(&chunk, 1, 101);

*/
/*>= A Virtual Machine 99 < Scanning on Demand 99
  writeChunk(&chunk, OP_ADD, 101);

  addConstant(&chunk, 5.6);
  writeChunk(&chunk, OP_CONSTANT, 102);
  writeChunk(&chunk, 2, 100);

  writeChunk(&chunk, OP_MULTIPLY, 102);
  writeChunk(&chunk, OP_NEGATE, 102);

  writeChunk(&chunk, OP_RETURN, 102);

*/
/*>= Chunks of Bytecode 99 < Scanning on Demand 99
  disassembleChunk(&chunk, "test chunk");
*/
/*>= A Virtual Machine 99 < Scanning on Demand 99

  interpret(&chunk);
*/
//>> Scanning on Demand 99
  if (argc == 1) {
    repl();
  } else if (argc == 2) {
    runFile(argv[1]);
  } else {
    fprintf(stderr, "Usage: clox [path]\n");
    exit(64);
  }
//<< Scanning on Demand 99

  endVM();
//<< A Virtual Machine 99
  return 0;
}
