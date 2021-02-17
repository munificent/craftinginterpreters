
import abc
from dataclasses import dataclass


from pylox.tokens import Token


class Expr:
    def accept(self, visitor: 'Visitor'):
        raise NotImplementedError


@dataclass
class Assign(Expr):
    name: Token
    value: Expr

    def accept(self, visitor: 'Visitor'):
        return visitor.visit_assign(self)


@dataclass
class Binary(Expr):
    left: Expr
    operator: Token
    right: Expr

    def accept(self, visitor: 'Visitor'):
        return visitor.visit_binary(self)


@dataclass
class Grouping(Expr):
    expression: Expr

    def accept(self, visitor: 'Visitor'):
        return visitor.visit_grouping(self)


@dataclass
class Literal(Expr):
    value: object

    def accept(self, visitor: 'Visitor'):
        return visitor.visit_literal(self)


@dataclass
class Unary(Expr):
    operator: Token
    right: Expr

    def accept(self, visitor: 'Visitor'):
        return visitor.visit_unary(self)


@dataclass
class Variable(Expr):
    name: Token

    def accept(self, visitor: 'Visitor'):
        return visitor.visit_variable(self)


class Visitor(metaclass=abc.ABCMeta):

    @abc.abstractmethod
    def visit_assign(self, expr: Assign):
        raise NotImplementedError

    @abc.abstractmethod
    def visit_binary(self, expr: Binary):
        raise NotImplementedError

    @abc.abstractmethod
    def visit_grouping(self, expr: Grouping):
        raise NotImplementedError

    @abc.abstractmethod
    def visit_literal(self, expr: Literal):
        raise NotImplementedError

    @abc.abstractmethod
    def visit_unary(self, expr: Unary):
        raise NotImplementedError

    @abc.abstractmethod
    def visit_variable(self, expr: Variable):
        raise NotImplementedError
