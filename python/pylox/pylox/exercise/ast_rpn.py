from typing import List

from pylox.expr import *
from pylox.tokens import TokenType


class AstPrinter(Visitor):

    def print(self, expr: Expr) -> str:
        return expr.accept(self)

    def visit_binary(self, expr: Binary) -> str:
        return f"{expr.left.accept(self)} {expr.right.accept(self)} {expr.operator.lexeme}"

    def visit_grouping(self, expr: Grouping) -> str:
        return f"{expr.expression.accept(self)} group"

    def visit_literal(self, expr: Literal) -> str:
        if expr.value is None:
            return "nil"
        return str(expr.value)

    def visit_unary(self, expr: Unary) -> str:
        return f"{expr.right.accept(self)} {expr.operator.lexeme}"


def main():
    expression = Binary(
        Binary(Literal(1), Token(TokenType.PLUS, "+", None, 1), Literal(2)),
        Token(TokenType.STAR, "*", None, 1),
        Binary(Literal(4), Token(TokenType.MINUS, "-", None, 1), Literal(3)))

    ast_printer = AstPrinter()
    print(ast_printer.print(expression))


if __name__ == "__main__":
    main()
