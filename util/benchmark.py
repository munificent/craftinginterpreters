#!/usr/bin/env python3

from __future__ import print_function

from os.path import join
from subprocess import Popen, PIPE
import sys


def color_text(text, color):
  """Converts text to a string and wraps it in the ANSI escape sequence for
  color, if supported."""

  # No ANSI escapes on Windows.
  if sys.platform == 'win32':
    return str(text)

  return color + str(text) + '\033[0m'


def green(text):  return color_text(text, '\033[32m')
def pink(text):   return color_text(text, '\033[91m')
def red(text):    return color_text(text, '\033[31m')
def yellow(text): return color_text(text, '\033[33m')


def print_line(line=None):
  # Erase the line.
  print('\033[2K', end='')
  # Move the cursor to the beginning.
  print('\r', end='')
  if line:
    print(line, end='')
    sys.stdout.flush()


def run_trial(benchmark):
  """Runs the benchmark once and returns the elapsed time."""
  args = ['build/clox', join('test', 'benchmark', benchmark + '.lox')]
  proc = Popen(args, stdin=PIPE, stdout=PIPE, stderr=PIPE)

  out, err = proc.communicate()

  out = out.decode("utf-8").replace('\r\n', '\n')

  # Remove the trailing last empty line.
  out_lines = out.split('\n')
  if out_lines[-1] == '':
    del out_lines[-1]

  # The benchmark should print the elapsed time last.
  return float(out_lines[-1])


def run_benchmark(benchmark):
  trial = 1
  best = 9999

  while True:
    elapsed = run_trial(benchmark)
    if elapsed < best:
      best = elapsed
    print_line("trial {0}   time {1:.4}s   best {2:.4}s".format(trial, elapsed, best))
    trial += 1


if len(sys.argv) != 2:
  print('Usage: benchmark.py <benchmark>')
  sys.exit(1)

if not run_benchmark(sys.argv[1]):
  sys.exit(1)
