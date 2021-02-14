from typing import List

from pylox.expr import *
from pylox.tokens import TokenType


class AstPrinter(Visitor):

    def print(self, expr: Expr) -> str:
        return expr.accept(self)

    def visit_binary(self, expr: Binary) -> str:
        return self.parenthesize(expr.operator.lexeme, expr.left, expr.right)

    def visit_grouping(self, expr: Grouping) -> str:
        return self.parenthesize("group", expr.expression)

    def visit_literal(self, expr: Literal) -> str:
        if expr.value is None:
            return "nil"
        return str(expr.value)

    def visit_unary(self, expr: Unary) -> str:
        return self.parenthesize(expr.operator.lexeme, expr.right)

    def parenthesize(self, name: str, *exprs: List[Expr]) -> str:
        items = ["(", name]
        for expr in exprs:
            items += (" ", expr.accept(self))
        items.append(")")
        return "".join(items)


def main():
    expression = Binary(
        Unary(
            Token(TokenType.MINUS, "-", None, 1),
            Literal(123)),
        Token(TokenType.STAR, "*", None, 1),
        Grouping(Literal(45.67)))

    ast_printer = AstPrinter()
    print(ast_printer.print(expression))


if __name__ == "__main__":
    main()
