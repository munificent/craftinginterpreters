import operator
import functools
from typing import List

from pylox import lox, expr as Expr, stmt as Stmt
from pylox.tokens import TokenType, Token
from pylox.environment import Environment


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


class Interpreter(Expr.Visitor, Stmt.Visitor):
    def __init__(self) -> None:
        self.environment: Environment = Environment()

    def interpret(self, statements: List[Stmt.Stmt]) -> None:
        try:
            for stmt in statements:
                self.execute(stmt)
        except RuntimeException as e:
            lox.runtime_error(e)

    def interpret_repl(self, expr: Expr.Expr) -> object:
        try:
            value = self.evaluate(expr)
            return self.stringify(value)
        except RuntimeException as e:
            lox.runtime_error(e)

    def evaluate(self, expr: Expr.Expr) -> object:
        return expr.accept(self)

    def execute(self, stmt: Stmt.Stmt) -> None:
        stmt.accept(self)

    def execute_block(self, statements: List[Stmt.Stmt], env: Environment):
        previous = self.environment
        try:
            self.environment = env
            for statement in statements:
                self.execute(statement)
        finally:
            self.environment = previous

    def visit_block(self, stmt: Stmt.Block) -> None:
        self.execute_block(stmt.statements, Environment(self.environment))

    def visit_expression(self, stmt: Stmt.Expression) -> None:
        self.evaluate(stmt.expression)

    def visit_if(self, stmt: Stmt.If) -> None:
        if self.is_truthy(self.evaluate(stmt.condition)):
            self.execute(stmt.then_branch)
        elif stmt.else_branch is not None:
            self.execute(stmt.else_branch)

    def visit_print(self, stmt: Stmt.Print) -> None:
        value = self.evaluate(stmt.expression)
        print(self.stringify(value))

    def visit_var(self, stmt: Stmt.Var) -> None:
        value = stmt.initializer and self.evaluate(stmt.initializer)
        self.environment.define(stmt.name.lexeme, value)

    def visit_assign(self, expr: Expr.Assign):
        value = self.evaluate(expr.value)
        self.environment.assign(expr.name, value)
        return value

    def visit_literal(self, expr: Expr.Literal) -> object:
        return expr.value

    def visit_grouping(self, expr: Expr.Grouping) -> object:
        return self.evaluate(expr.expression)

    def visit_unary(self, expr: Expr.Unary) -> object:
        right = self.evaluate(expr.right)
        if expr.operator.type == TokenType.MINUS:
            check_operand(expr.operator, expr.right)
            return -right
        elif expr.operator.type == TokenType.BANG:
            return not self.is_truthy(right)
        # unreachable
        return None

    def visit_variable(self, expr: Expr.Variable):
        return self.environment.get(expr.name)

    def visit_binary(self, expr: Expr.Binary) -> object:
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
        elif isinstance(value, bool):
            return str(value).lower()
        else:
            return str(value)
