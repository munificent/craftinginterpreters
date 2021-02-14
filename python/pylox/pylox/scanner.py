import pylox.lox as lox
from pylox.tokens import Token, TokenType


class Scanner:
    def __init__(self, source: str) -> None:
        self.source = source
        self.tokens = []

        self.start = 0
        self.current = 0
        self.line = 1

    def scan_tokens(self):
        while not self.is_at_end():
            self.start = self.current
            self.scan_token()
        self.tokens.append(Token(TokenType.EOF, "", None, self.line))
        return self.tokens

    def is_at_end(self):
        return self.current >= len(self.source)

    def scan_token(self):
        c: str = self.advance()
        add_token, match = self.add_token, self.match
        try:
            type = TokenType(c)
            add_token(type)
        except ValueError:
            if c == '!':
                add_token(TokenType.BANG_EQUAL if match('=') else TokenType.BANG)
            elif c == '=':
                add_token(TokenType.EQUAL_EQUAL if match('=') else TokenType.EQUAL)
            elif c == '<':
                add_token(TokenType.LESS_EQUAL if match('=') else TokenType.LESS)
            elif c == '>':
                add_token(TokenType.GREATER_EQUAL if match('=') else TokenType.GREATER)
            elif c == '/':
                if match('/'):
                    while self.peek() != '\n' and not self.is_at_end():
                        self.advance()
                else:
                    add_token(TokenType.SLASH)
            elif c == '\n':
                self.line += 1
            elif c in ' \r\t':
                pass
            elif c == '"':
                self.string()
            elif c.isdigit():
                self.number()
            elif self.isalpha(c):
                self.identifier()
            else:
                lox.error(self.line, "Unexpected character '{c}'.")

    def string(self):
        while self.peek() != '"' and not self.is_at_end():
            if self.peek() == '\n':
                self.line += 1
            self.advance()
        if self.is_at_end():
            lox.error(self.line, "Unterminated string.")
        self.advance()
        value = self.source[self.start + 1: self.current - 1]
        self.add_token(TokenType.STRING, value)

    def number(self):
        while self.isdigit(self.peek()):
            self.advance()

        # look for a fractional part
        if self.peek() == '.' and self.isdigit(self.peek_next()):
            self.advance()
            while self.isdigit(self.peek()):
                self.advance()

        self.add_token(TokenType.NUMBER, float(self.source[self.start: self.current]))

    def identifier(self):
        while self.isalnum(self.peek()):
            self.advance()

        text = self.source[self.start: self.current]
        type = TokenType.IDENTIFIER
        try:
            type = TokenType(text)
        except ValueError:
            pass
        self.add_token(type)

    def isdigit(self, c: str) -> bool:
        return c is not None and c.isdigit()

    def isalpha(self, c: str) -> bool:
        return c is not None and (c.isalpha() or c == '_')

    def isalnum(self, c: str) -> bool:
        return self.isalpha(c) or self.isdigit(c)

    def match(self, expected: str) -> bool:
        if self.is_at_end():
            return False
        if self.source[self.current] != expected:
            return False

        self.current += 1
        return True

    def peek(self) -> str:
        if self.is_at_end():
            return None
        return self.source[self.current]

    def peek_next(self) -> str:
        if self.current + 1 >= len(self.source):
            return None
        return self.source[self.current + 1]

    def advance(self) -> str:
        self.current += 1
        return self.source[self.current - 1]

    def add_token(self, type: TokenType, literal: object=None):
        text = self.source[self.start: self.current]
        self.tokens.append(Token(type, text, literal, self.line))
