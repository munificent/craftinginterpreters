
function Token(type, text, value) {
  this.type = type;
  this.text = text;
  this.value = value;
}

// Token types.
Token.LEFT_PAREN = "LEFT_PAREN";
Token.RIGHT_PAREN = "RIGHT_PAREN";
Token.LEFT_BRACKET = "LEFT_BRACKET";
Token.RIGHT_BRACKET = "RIGHT_BRACKET";
Token.LEFT_BRACE = "LEFT_BRACE";
Token.RIGHT_BRACE = "RIGHT_BRACE";
Token.SEMICOLON = "SEMICOLON";
Token.DOT = "DOT";
Token.PLUS = "PLUS";
Token.MINUS = "MINUS";
Token.STAR = "STAR";
Token.SLASH = "SLASH";
Token.EQUALS = "EQUALS";
Token.EQUALS_EQUALS = "EQUALS_EQUALS";
Token.LESS = "LESS";
Token.GREATER = "GREATER";
Token.LESS_EQUALS = "LESS_EQUALS";
Token.GREATER_EQUALS = "GREATER_EQUALS";
Token.IDENTIFIER = "IDENTIFIER";
Token.NUMBER = "NUMBER";
Token.END = "END";
Token.ERROR = "ERROR";
