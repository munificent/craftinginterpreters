import operator
import functools
from typing import List

from pylox import lox
from pylox.expr import *
from pylox.tokens import TokenType, Token


class RuntimeException(Exception):
    def __init__(self, token: Token, message: str) -> None:
        self.message = message
        self.token = token


def lox_type(val: object) -> str:
    if isinstance(val, float):
        return 'number'
    elif isinstance(val, str):
        return 'string'
    elif isinstance(val, bool):
        return 'bool'
    elif val is None:
        return 'nil'
    else:
        raise ValueError("Unexpected type")


def check_operand(operator: Token, operand: object, type=float):
    if isinstance(operand, type):
        return
    raise RuntimeException(operator, f"Operand must be a {lox_type(type)}.")


def check_operands(operator: Token, left: object, right: object, type=float):
    if isinstance(left, type) and isinstance(right, type):
        return
    raise RuntimeException(operator, f"Operands must be {lox_type(type)}s.")


def check_operands_then_execute(func):
    functools.wraps(func)
    def wrap(operator: Token, left: object, right: object):
        check_operands(operator, left, right)
        return func(left, right)
    return wrap


def plus(operator: Token, left: object, right: object):
    if isinstance(left, float) and isinstance(right, float):
        return left + right
    elif isinstance(left, str) and isinstance(right, str):
        return left + right
    else:
        raise RuntimeException(operator, "Operands must be two numbers or two strings.")


BINARY_OPS = {
    TokenType.MINUS: check_operands_then_execute(operator.sub),
    TokenType.SLASH: check_operands_then_execute(operator.truediv),
    TokenType.STAR: check_operands_then_execute(operator.mul),
    TokenType.PLUS: plus,

    TokenType.GREATER: check_operands_then_execute(operator.gt),
    TokenType.GREATER_EQUAL: check_operands_then_execute(operator.ge),
    TokenType.LESS: check_operands_then_execute(operator.lt),
    TokenType.LESS_EQUAL: check_operands_then_execute(operator.le),

    TokenType.BANG_EQUAL: check_operands_then_execute(operator.ne),
    TokenType.EQUAL_EQUAL: check_operands_then_execute(operator.eq),
}


class Interpreter(Visitor):

    def interpret(self, expr: Expr) -> None:
        try:
            value = self.evaluate(expr)
            print(self.stringify(value))
        except RuntimeException as e:
            lox.runtime_error(e)

    def evaluate(self, expr: Expr) -> object:
        return expr.accept(self)

    def visit_literal(self, expr: Literal) -> object:
        return expr.value

    def visit_grouping(self, expr: Grouping) -> object:
        return self.evaluate(expr.expression)

    def visit_unary(self, expr: Unary) -> object:
        right = self.evaluate(expr.right)
        if expr.operator.type == TokenType.MINUS:
            check_operand(expr.operator, expr.right)
            return -right
        elif expr.operator.type == TokenType.BANG:
            return not self.is_truthy(right)
        # unreachable
        return None

    def visit_binary(self, expr: Binary) -> object:
        left = self.evaluate(expr.left)
        right = self.evaluate(expr.right)
        func = BINARY_OPS[expr.operator.type]
        return func(expr.operator, left, right)

    def is_truthy(self, value: object) -> bool:
        if value is None:
            return False
        if isinstance(value, bool):
            return value
        return True

    def stringify(self, value: object) -> str:
        if value is None:
            return "nil"
        elif isinstance(value, float):
            text = str(value)
            if text.endswith(".0"):
                return text[:-2]
            return text
        else:
            return str(value)
