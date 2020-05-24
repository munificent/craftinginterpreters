/// Utilities for printing to the terminal.
import 'dart:io';

final _cyan = _ansi('\u001b[36m');
final _green = _ansi('\u001b[32m');
final _magenta = _ansi('\u001b[35m');
final _red = _ansi('\u001b[31m');
final _yellow = _ansi('\u001b[33m');
final _blue = _ansi('\u001b[34m');
final _gray = _ansi('\u001b[1;30m');
final _none = _ansi('\u001b[0m');
final _resetColor = _ansi('\u001b[39m');
final _bold = _ansi('\u001b[1m');

String gray(Object message) => "$_gray$message$_none";
String green(Object message) => "$_green$message$_resetColor";
String magenta(Object message) => "$_magenta$message$_resetColor";
String red(Object message) => "$_red$message$_resetColor";
String yellow(Object message) => "$_yellow$message$_resetColor";

void clearLine() {
  if (_allowAnsi) {
    stdout.write("\u001b[2K\r");
  } else {
    print("");
  }
}

bool get _allowAnsi =>
    !Platform.isWindows && stdioType(stdout) == StdioType.terminal;

String _ansi(String special, [String fallback = '']) =>
    _allowAnsi ? special : fallback;
