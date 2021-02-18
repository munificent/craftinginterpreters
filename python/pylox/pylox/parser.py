from typing import List

from pylox.tokens import TokenType, Token
from pylox.expr import *
from pylox.stmt import *
from pylox import lox

"""
Grammar:

program        → declaration* EOF ;
declaration    → varDecl
               | statement
varDecl        → "var" IDENTIFIER ( "=" expression)? ";"
statement      → exprStmt
               | printStmt
               | block
block          → "{" declaration* "}" ;
exprStmt       → expression ";"
printStmt      → "print" expression ";"
expression     → assignment ;
assignment     → IDENTIFIER "=" assignment
               | equality ;
equality       → comparison ( ( "!=" | "==" ) comparison )* ;
comparison     → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
term           → factor ( ( "-" | "+" ) factor )* ;
factor         → unary ( ( "/" | "*" ) unary )* ;
unary          → ( "!" | "-" ) unary
               | primary ;
primary        → NUMBER | STRING | "true" | "false" | "nil"
               | "(" expression ")" ;
               | IDENTIFIER ;
"""

class ParseError(Exception):
    pass


class Parser:
    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.current = 0

        self.allow_expression = False
        self.found_expression = False

    def parse(self) -> List[Stmt]:
        statements = []
        while not self.is_at_end():
            statements.append(self.declaration())
        return statements

    def parse_repl(self):
        self.allow_expression = True
        statements = []
        while not self.is_at_end():
            statements.append(self.declaration())

            if self.found_expression:
                last = statements[-1]
                return last.expression
            self.allow_expression = False
        return statements

    def declaration(self) -> Stmt:
        try:
            if self.match(TokenType.VAR):
                return self.var_declaration()
            return self.statement()
        except ParseError:
            self.synchronize()

    def statement(self) -> Stmt:
        if self.match(TokenType.PRINT):
            return self.print_statement()
        elif self.match(TokenType.LEFT_BRACE):
            return Block(self.block())
        return self.expression_statement()

    def print_statement(self) -> Print:
        value = self.expression()
        self.consume(TokenType.SEMICOLON, "Expect ';' after value.")
        return Print(value)

    def var_declaration(self) -> Stmt:
        name = self.consume(TokenType.IDENTIFIER, "Expect variable name.")
        initializer = self.expression() if self.match(TokenType.EQUAL) else None
        self.consume(TokenType.SEMICOLON, "Expect ';' after variable declaration.")
        return Var(name, initializer)

    def expression_statement(self) -> Expression:
        expr = self.expression()

        if self.allow_expression and self.is_at_end():
            self.found_expression = True
        else:
            self.consume(TokenType.SEMICOLON, "Expect ';' after expression.")
        return Expression(expr)

    def block(self) -> List[Stmt]:
        statements = []
        while not self.is_at_end() and not self.check(TokenType.RIGHT_BRACE):
            statements.append(self.declaration())
        self.consume(TokenType.RIGHT_BRACE, "Expect '}' after block.")
        return statements

    def expression(self) -> Expr:
        return self.assignment()

    def assignment(self) -> Expr:
        expr = self.equality()
        if self.match(TokenType.EQUAL):
            equals = self.previous()
            value = self.assignment()
            if isinstance(expr, Variable):
                name = expr.name
                return Assign(name, value)
            self.error(equals, "Invalid assignment target.")
        return expr

    def equality(self) -> Expr:
        expr = self.comparison()
        while self.match(TokenType.BANG_EQUAL, TokenType.EQUAL_EQUAL):
            operator = self.previous()
            right = self.comparison()
            expr = Binary(expr, operator, right)
        return expr

    def comparison(self) -> Expr:
        expr = self.term()
        op_types = [TokenType.GREATER, TokenType.LESS, TokenType.GREATER_EQUAL, TokenType.LESS_EQUAL]
        while self.match(*op_types):
            operator = self.previous()
            right = self.term()
            expr = Binary(expr, operator, right)
        return expr

    def term(self) -> Expr:
        expr = self.factor()
        while self.match(TokenType.MINUS, TokenType.PLUS):
            operator = self.previous()
            right = self.factor()
            expr = Binary(expr, operator, right)
        return expr

    def factor(self) -> Expr:
        expr = self.unary()
        while self.match(TokenType.SLASH, TokenType.STAR):
            operator = self.previous()
            right = self.unary()
            expr = Binary(expr, operator, right)
        return expr

    def unary(self) -> Expr:
        if self.match(TokenType.BANG, TokenType.MINUS):
            operator = self.previous()
            right = self.unary()
            return Unary(operator, right)
        return self.primary()

    def primary(self) -> Expr:
        if self.match(TokenType.FALSE):
            return Literal(False)
        elif self.match(TokenType.TRUE):
            return Literal(True)
        elif self.match(TokenType.NIL):
            return Literal(None)
        elif self.match(TokenType.NUMBER, TokenType.STRING):
            return Literal(self.previous().literal)
        elif self.match(TokenType.IDENTIFIER):
            return Variable(self.previous())
        elif self.match(TokenType.LEFT_PAREN):
            expr = self.expression()
            self.consume(TokenType.RIGHT_PAREN, "Expect ')' after expression")
            return Grouping(expr)
        else:
            raise ParseError(self.peek(), "Expect expression.")

    def match(self, *types: List[TokenType]) -> bool:
        for type in types:
            if self.check(type):
                self.advance()
                return True
        return False

    def consume(self, type: TokenType, message: str) -> Token:
        if self.check(type):
            return self.advance()
        raise self.error(self.peek(), message)

    def check(self, type: TokenType) -> bool:
        if self.is_at_end():
            return False
        return self.tokens[self.current].type == type

    def advance(self) -> Token:
        if not self.is_at_end():
            self.current += 1
        return self.tokens[self.current - 1]

    def is_at_end(self) -> bool:
        return self.peek().type == TokenType.EOF

    def peek(self) -> Token:
        return self.tokens[self.current]

    def previous(self) -> Token:
        return self.tokens[self.current - 1]

    def error(self, token: Token, message: str):
        lox.token_error(token, message)
        raise ParseError()

    def synchronize(self):
        self.advance()
        while not self.is_at_end():
            if self.previous().type == TokenType.SEMICOLON:
                return

            possible_starts = [
                TokenType.CLASS,
                TokenType.FUN,
                TokenType.VAR,
                TokenType.FOR,
                TokenType.IF,
                TokenType.WHILE,
                TokenType.PRINT,
                TokenType.RETURN
            ]

            if self.peek().type in possible_starts:
                return

            self.advance()
