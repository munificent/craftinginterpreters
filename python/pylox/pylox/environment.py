import pylox
from typing import Dict

from pylox import interpreter
from pylox.tokens import Token


class Environment:
    def __init__(self) -> None:
        self.values: Dict[str, object] = {}

    def define(self, name: str, value: object) -> None:
        self.values[name] = value

    def get(self, name: Token) -> object:
        if name.lexeme in self.values:
            return self.values[name.lexeme]
        raise interpreter.RuntimeException(name, f"Undefined variable '{name.lexeme}'.")

    def assign(self, name: Token, value: object) -> None:
        if name.lexeme in self.values:
            self.values[name.lexeme] = value
        else:
            raise interpreter.RuntimeException(name, f"Undefined variable '{name.lexeme}'.")
