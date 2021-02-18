
import abc
from dataclasses import dataclass
from typing import List
from pylox.expr import Expr
from pylox.tokens import Token


class Stmt:
    def accept(self, visitor: 'Visitor'):
        raise NotImplementedError


@dataclass
class Block(Stmt):
    statements: List[Stmt]

    def accept(self, visitor: 'Visitor'):
        return visitor.visit_block(self)


@dataclass
class If(Stmt):
    condition: Expr
    then_branch: Stmt
    else_branch: Stmt

    def accept(self, visitor: 'Visitor'):
        return visitor.visit_if(self)


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


@dataclass
class Var(Stmt):
    name: Token
    initializer: Expr

    def accept(self, visitor: 'Visitor'):
        return visitor.visit_var(self)


class Visitor(metaclass=abc.ABCMeta):

    @abc.abstractmethod
    def visit_block(self, stmt: Block):
        raise NotImplementedError

    @abc.abstractmethod
    def visit_if(self, stmt: If):
        raise NotImplementedError

    @abc.abstractmethod
    def visit_expression(self, stmt: Expression):
        raise NotImplementedError

    @abc.abstractmethod
    def visit_print(self, stmt: Print):
        raise NotImplementedError

    @abc.abstractmethod
    def visit_var(self, stmt: Var):
        raise NotImplementedError
