
import abc
from dataclasses import dataclass
from pylox.expr import Expr
from pylox.tokens import Token


class Stmt:
    def accept(self, visitor: 'Visitor'):
        raise NotImplementedError


@dataclass
class Expression(Stmt):
    expression: Expr

    def accept(self, visitor: 'Visitor'):
        return visitor.visit_expression(self)


@dataclass
class Print(Stmt):
    expression: Expr

    def accept(self, visitor: 'Visitor'):
        return visitor.visit_print(self)


class Visitor(metaclass=abc.ABCMeta):

    @abc.abstractmethod
    def visit_expression(self, stmt: Expression):
        raise NotImplementedError

    @abc.abstractmethod
    def visit_print(self, stmt: Print):
        raise NotImplementedError
