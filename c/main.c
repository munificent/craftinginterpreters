#include <stdbool.h>
#include <stdio.h>

#include "lexer.h"

int main(int argc, const char * argv[]) {
//  const char* source = "([{)]},;.!";
//  const char* source = "+-*/% = == != < > <= >=";
//  const char* source = "1 // comment\n + // comment\n2";
//  const char* source = "an and andy";
//  const char* source = "1234 0 000 -123 12.34 .45 56. 1.2.3 4..5";
  const char* source = "\"\" \"string\"";
  Lexer lexer;
  initLexer(&lexer, source);
  
  while (true) {
    Token token = nextToken(&lexer);
    printf("%d '%.*s'\n", token.type, token.length, token.start);
    if (token.type == TOKEN_EOF) break;
  }
  
  return 0;
}
