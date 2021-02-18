import os
import sys


def main():
    args = sys.argv[1:]
    if len(args) != 1:
        print("Usage: generate_ast.py <output_dir>")
        sys.exit(64)
    output_dir = args[0]

    define_ast(output_dir, "Expr", [
        "Assign : Token name, Expr value",
        "Binary : Expr left, Token operator, Expr right",
        "Grouping : Expr expression",
        "Literal : object value",
        "Unary : Token operator, Expr right",
        "Variable : Token name",
    ])

    define_ast(output_dir, "Stmt", [
        "Block : List[Stmt] statements",
        "If : Expr condition, Stmt then_branch, Stmt else_branch",
        "Expression : Expr expression",
        "Print : Expr expression",
        "Var : Token name, Expr initializer",
    ])


def define_ast(output_dir, basename, types):
    path = os.path.join(output_dir, f"{basename.lower()}.py")
    with open(path, "w") as f:
        f.write(f"""
import abc
from dataclasses import dataclass
{"from typing import List" if basename == "Stmt" else ""}
{"from pylox.expr import Expr" if basename == "Stmt" else ""}
from pylox.tokens import Token


class {basename}:
    def accept(self, visitor: 'Visitor'):
        raise NotImplementedError
""")

        for type in types:
            classname, fields = [x.strip() for x in type.split(":")]
            define_type(f, basename, classname, fields)

        define_visitor(f, basename, types)


def define_type(f, basename, classname, fields):
    f.write(f"""

@dataclass
class {classname}({basename}):
""")

    for field in fields.split(", "):
        type, name = field.split(" ")
        f.write(f"    {name}: {type}\n")

    f.write(f"""
    def accept(self, visitor: 'Visitor'):
        return visitor.visit_{classname.lower()}(self)
""")


def define_visitor(f, basename, types):
    f.write("""

class Visitor(metaclass=abc.ABCMeta):
""")
    for type in types:
        typename = type.split(":")[0].strip()
        f.write(f"""
    @abc.abstractmethod
    def visit_{typename.lower()}(self, {basename.lower()}: {typename}):
        raise NotImplementedError
""")


if __name__ == "__main__":
    main()
