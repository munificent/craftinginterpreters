from pylox.expr import Expr
import sys
import readline
from typing import List
from pylox import interpreter

from pylox.scanner import Scanner
from pylox.tokens import TokenType, Token
from pylox.parser import Parser
from pylox.ast_printer import AstPrinter
from pylox.interpreter import RuntimeException, Interpreter

## static states

gHadError = False
gHadRuntimeError = False
gInterpreter = Interpreter()

## run utils

def run_file(path: str) -> None:
    with open(path, encoding='utf-8') as f:
        run(f.read())
    if gHadError:
        sys.exit(65)
    if gHadRuntimeError:
        sys.exit(70)


def run_prompt(prompt: str="lox> ") -> None:
    while True:
        global gHadError
        gHadError = False

        try:
            line = input(prompt)
        except EOFError:
            break

        scanner = Scanner(line)
        tokens = scanner.scan_tokens()
        parser = Parser(tokens)
        syntax = parser.parse_repl()

        # Ignore it if there was a syntax error.
        if gHadError:
            continue

        if isinstance(syntax, list):
            gInterpreter.interpret(syntax)
        elif isinstance(syntax, Expr):
            result = gInterpreter.interpret_repl(syntax)
            if result is not None:
                print(f"= {result}")

def run(source: str) -> None:
    scanner = Scanner(source)
    tokens = scanner.scan_tokens()

    parser = Parser(tokens)
    statements = parser.parse()

    if gHadError:
        return

    gInterpreter.interpret(statements)

## error handling

def token_error(token: Token, message: str) -> None:
    if token.type == TokenType.EOF:
        report(token.line, " at end", message)
    else:
        report(token.line, f" at '{token.lexeme}'", message)


def error(line: int, message: str) -> None:
    report(line, "", message)


def report(line: int, where: str, message: str) -> None:
    print(f"[line {line}] Error {where}: {message}", file=sys.stderr)
    global gHadError
    gHadError = True


def runtime_error(error: RuntimeException) -> None:
    print(f"{error.message} \n[line {error.token.line}]")
    global gHadRuntimeError
    gHadRuntimeError = True

## main

def main() -> None:
    args = sys.argv[1:]
    if len(args) > 1:
        print("Usage: pylox [script]")
        sys.exit(64)
    elif len(args) == 1:
        run_file(args[0])
    else:
        run_prompt()


if __name__ == "__main__":
    main()
