#!/usr/bin/env python

from __future__ import print_function

from collections import defaultdict
from os import listdir
from os.path import abspath, basename, dirname, isdir, isfile, join, realpath, relpath, splitext
import re
from subprocess import Popen, PIPE
import sys

# Runs the tests.
REPO_DIR = dirname(dirname(realpath(__file__)))
JS_INTERPRETER = join(REPO_DIR, 'js', 'vox.js')
JAVA_INTERPRETER = join(REPO_DIR, 'jvox')

EXPECT_PATTERN = re.compile(r'// expect: ?(.*)')
EXPECT_ERROR_PATTERN = re.compile(r'// expect error(?! line)')
EXPECT_ERROR_LINE_PATTERN = re.compile(r'// expect error line (\d+)')
EXPECT_RUNTIME_ERROR_PATTERN = re.compile(r'// expect runtime error: (.+)')
ERROR_PATTERN = re.compile(r'\[.*line (\d+)\] Error')
STACK_TRACE_PATTERN = re.compile(r'\[line (\d+)\]')
SKIP_PATTERN = re.compile(r'// skip: (.*)')
NONTEST_PATTERN = re.compile(r'// nontest')

passed = 0
failed = 0
num_skipped = 0
skipped = defaultdict(int)
expectations = 0


class Test:
  def __init__(self, path):
    self.path = path
    self.output = []
    self.compile_errors = set()
    self.runtime_error_line = 0
    self.runtime_error_message = None
    self.exit_code = 0
    self.failures = []


  def parse(self):
    global num_skipped
    global skipped
    global expectations

    line_num = 1
    with open(self.path, 'r') as file:
      for line in file:
        match = EXPECT_PATTERN.search(line)
        if match:
          self.output.append((match.group(1), line_num))
          expectations += 1

        match = EXPECT_ERROR_PATTERN.search(line)
        if match:
          self.compile_errors.add(line_num)

          # If we expect a compile error, it should exit with EX_DATAERR.
          self.exit_code = 65
          expectations += 1

        match = EXPECT_ERROR_LINE_PATTERN.search(line)
        if match:
          self.compile_errors.add(int(match.group(1)))

          # If we expect a compile error, it should exit with EX_DATAERR.
          self.exit_code = 65
          expectations += 1

        match = EXPECT_RUNTIME_ERROR_PATTERN.search(line)
        if match:
          self.runtime_error_line = line_num
          self.runtime_error_message = match.group(1)
          # If we expect a runtime error, it should exit with EX_SOFTWARE.
          self.exit_code = 70
          expectations += 1

        match = SKIP_PATTERN.search(line)
        if match:
          num_skipped += 1
          skipped[match.group(1)] += 1
          return False

        match = NONTEST_PATTERN.search(line)
        if match:
          # Not a test file at all, so ignore it.
          return False

        line_num += 1


    # If we got here, it's a valid test.
    return True


  def run(self, app, type):
    # Invoke the interpreter and run the test.
    test_arg = self.path
    proc = Popen([app, test_arg], stdin=PIPE, stdout=PIPE, stderr=PIPE)

    out, err = proc.communicate()
    self.validate(type == "example", proc.returncode, out, err)


  def validate(self, is_example, exit_code, out, err):
    if self.compile_errors and self.runtime_error_message:
      self.fail("Test error: Cannot expect both compile and runtime errors.")
      return

    try:
      out = out.decode("utf-8").replace('\r\n', '\n')
      err = err.decode("utf-8").replace('\r\n', '\n')
    except:
      self.fail('Error decoding output.')

    error_lines = err.split('\n')

    # Validate that an expected runtime error occurred.
    if self.runtime_error_message:
      self.validate_runtime_error(error_lines)
    else:
      self.validate_compile_errors(error_lines)

    self.validate_exit_code(exit_code, error_lines)

    # Ignore output from examples.
    if is_example: return

    self.validate_output(out)


  def validate_runtime_error(self, error_lines):
    if len(error_lines) < 2:
      self.fail('Expected runtime error "{0}" and got none.',
          self.runtime_error_message)
      return

    # Skip any compile errors. This can happen if there is a compile error in
    # a module loaded by the module being tested.
    line = 0
    while ERROR_PATTERN.search(error_lines[line]):
      line += 1

    if error_lines[line] != self.runtime_error_message:
      self.fail('Expected runtime error "{0}" and got:',
          self.runtime_error_message)
      self.fail(error_lines[line])

    # Make sure the stack trace has the right line. Skip over any lines that
    # come from builtin libraries.
    match = False
    stack_lines = error_lines[line + 1:]
    for stack_line in stack_lines:
      match = STACK_TRACE_PATTERN.search(stack_line)
      if match: break

    if not match:
      self.fail('Expected stack trace and got:')
      for stack_line in stack_lines:
        self.fail(stack_line)
    else:
      stack_line = int(match.group(1))
      if stack_line != self.runtime_error_line:
        self.fail('Expected runtime error on line {0} but was on line {1}.',
            self.runtime_error_line, stack_line)


  def validate_compile_errors(self, error_lines):
    # Validate that every compile error was expected.
    found_errors = set()
    for line in error_lines:
      match = ERROR_PATTERN.search(line)
      if match:
        error_line = float(match.group(1))
        if error_line in self.compile_errors:
          found_errors.add(error_line)
        else:
          self.fail('Unexpected error:')
          self.fail(line)
      elif line != '':
        self.fail('Unexpected output on stderr:')
        self.fail(line)

    # Validate that every expected error occurred.
    for line in self.compile_errors - found_errors:
      self.fail('Missing expected error on line {0}.', line)


  def validate_exit_code(self, exit_code, error_lines):
    if exit_code == self.exit_code: return

    self.fail('Expected return code {0} and got {1}. Stderr:',
        self.exit_code, exit_code)
    self.failures += error_lines


  def validate_output(self, out):
    # Remove the trailing last empty line.
    out_lines = out.split('\n')
    if out_lines[-1] == '':
      del out_lines[-1]

    index = 0
    for line in out_lines:
      if sys.version_info < (3, 0):
        line = line.encode('utf-8')

      if index >= len(self.output):
        self.fail('Got output "{0}" when none was expected.', line)
      elif self.output[index][0] != line:
        self.fail('Expected output "{0}" on line {1} and got "{2}".',
            self.output[index][0], self.output[index][1], line)
      index += 1

    while index < len(self.output):
      self.fail('Missing expected output "{0}" on line {1}.',
          self.output[index][0], self.output[index][1])
      index += 1


  def fail(self, message, *args):
    if args:
      message = message.format(*args)
    self.failures.append(message)


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


def walk(dir, callback):
  """
  Walks [dir], and executes [callback] on each file.
  """

  dir = abspath(dir)
  for file in listdir(dir):
    nfile = join(dir, file)
    if isdir(nfile):
      walk(nfile, callback)
    else:
      callback(nfile)


def print_line(line=None):
  # Erase the line.
  print('\033[2K', end='')
  # Move the cursor to the beginning.
  print('\r', end='')
  if line:
    print(line, end='')
    sys.stdout.flush()


def run_script(app, path, type):
  global passed
  global failed
  global num_skipped

  if (splitext(path)[1] != '.vox'):
    return

  # Check if we are just running a subset of the tests.
  if len(sys.argv) == 2:
    this_test = relpath(path, join(REPO_DIR, 'test'))
    if not this_test.startswith(sys.argv[1]):
      return

  # Update the status line.
  print_line('Passed: ' + green(passed) +
             ' Failed: ' + red(failed) +
             ' Skipped: ' + yellow(num_skipped))

  # Make a nice short path relative to the working directory.

  # Normalize it to use "/" since, among other things, the interpreters expect
  # the argument to use that.
  path = relpath(path).replace("\\", "/")

  # Read the test and parse out the expectations.
  test = Test(path)

  if not test.parse():
    # It's a skipped or non-test file.
    return

  test.run(app, type)

  # Display the results.
  if len(test.failures) == 0:
    passed += 1
  else:
    failed += 1
    print_line(red('FAIL') + ': ' + path)
    print('')
    for failure in test.failures:
      print('      ' + pink(failure))
    print('')


def run_test(path):
  run_script(JAVA_INTERPRETER, path, "test")


walk(join(REPO_DIR, 'test'), run_test)

print_line()
if failed == 0:
  print('All ' + green(passed) + ' tests passed (' + str(expectations) +
        ' expectations).')
else:
  print(green(passed) + ' tests passed. ' + red(failed) + ' tests failed.')

for key in sorted(skipped.keys()):
  print('Skipped ' + yellow(skipped[key]) + ' tests: ' + key)

if failed != 0:
  sys.exit(1)
