import sys
import readline

from pylox.scanner import Scanner
from pylox.tokens import TokenType, Token
from pylox.parser import Parser
from pylox.ast_printer import AstPrinter
from pylox.interpreter import RuntimeException, Interpreter


gHadError = False
gHadRuntimeError = False
gInterpreter = Interpreter()


def main():
    args = sys.argv[1:]
    if len(args) > 1:
        print("Usage: pylox [script]")
        sys.exit(64)
    elif len(args) == 1:
        run_file(args[1])
    else:
        run_prompt()


def run_file(path: str):
    with open(path, encoding='utf-8') as f:
        run(f.read())
    if gHadError:
        sys.exit(65)
    if gHadRuntimeError:
        sys.exit(70)


def run_prompt(prompt="lox> "):
    while True:
        try:
            line = input(prompt)
        except EOFError:
            break
        run(line)
        HadError = False


def run(source: str):
    scanner = Scanner(source)
    tokens = scanner.scan_tokens()

    parser = Parser(tokens)
    expression = parser.parse()

    if gHadError:
        return

    gInterpreter.interpret(expression)


## error handling


def token_error(token: Token, message: str):
    if token.type == TokenType.EOF:
        report(token.line, " at end", message)
    else:
        report(token.line, f" at '{token.lexeme}'", message)


def error(line: int, message: str):
    report(line, "", message)


def report(line: int, where: str, message: str):
    print(f"[line {line}] Error {where}: {message}", file=sys.stderr)
    gHadError = True


def runtime_error(error: RuntimeException):
    print(f"{error.message} \n[line {error.token.line}]")
    gHadRuntimeError = True


if __name__ == "__main__":
    main()
