import sys
import readline

from pylox.scanner import Scanner

HadError = False


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
    if HadError:
        sys.exit(65)


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
    for token in scanner.scan_tokens():
        print(token)


## error handling


def error(line: int, message: str):
    report(line, "", message)


def report(line: int, where: str, message: str):
    print(f"[line {line}] Error {where}: {message}", file=sys.stderr)
    HadError = True


if __name__ == "__main__":
    main()
