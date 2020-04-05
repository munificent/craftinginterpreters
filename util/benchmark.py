#!./util/env/bin/python3

from os.path import join
from subprocess import Popen, PIPE
import sys


def run_trial(interpreter, benchmark):
  """Runs the benchmark once and returns the elapsed time."""
  args = [interpreter, join('test', 'benchmark', benchmark + '.lox')]
  proc = Popen(args, stdin=PIPE, stdout=PIPE, stderr=PIPE)

  out, err = proc.communicate()

  out = out.decode("utf-8").replace('\r\n', '\n')

  # Remove the trailing last empty line.
  out_lines = out.split('\n')
  if out_lines[-1] == '':
    del out_lines[-1]

  # The benchmark should print the elapsed time last.
  return float(out_lines[-1])


def run_comparison(interpreters, benchmark):
  trial = 1
  best = {}
  for interpreter in interpreters:
    best[interpreter] = 9999

  while True:
    for interpreter in interpreters:
      elapsed = run_trial(interpreter, benchmark)
      if elapsed < best[interpreter]:
        best[interpreter] = elapsed

    best_time = 999
    worst_time = 0
    for interpreter in interpreters:
      if best[interpreter] < best_time:
        best_time = best[interpreter]
        best_interpreter = interpreter
      if best[interpreter] > worst_time:
        worst_time = best[interpreter]

    # Turn the time measurement into an effort measurement in units where 1
    # "work" is just the total thing the benchmark does.
    worst_work = 1.0 / worst_time

    print("trial #{0}".format(trial))
    for interpreter in interpreters:
      if interpreter == best_interpreter:
        best_work = 1.0 / best[interpreter]
        work_ratio = best_work / worst_work
        faster = 100 * (work_ratio - 1.0)
        suffix = "{0:0.4f}% faster".format(faster)
      else:
        ratio = best[interpreter] / best_time
        suffix = "{0:0.4}x time of best".format(ratio)
      print("  {0}   best {1:0.4}s  {2}".format(
          interpreter, best[interpreter], suffix))

    trial += 1


def run_benchmark(interpreter, benchmark):
  trial = 1
  best = 9999

  while True:
    elapsed = run_trial(interpreter, benchmark)
    if elapsed < best:
      best = elapsed

    print("trial #{0}  {1}   best {2:0.4}s".format(trial, interpreter, best))
    trial += 1


interpreters = ['build/clox']
if len(sys.argv) == 2:
  benchmark = sys.argv[1]
elif len(sys.argv) > 2:
  interpreters = sys.argv[1:-1]
  benchmark = sys.argv[-1]
else:
  print('Usage: benchmark.py [interpreters...] <benchmark>')
  sys.exit(1)

if len(interpreters) > 1:
  run_comparison(interpreters, benchmark)
else:
  run_benchmark(interpreters[0], benchmark)
