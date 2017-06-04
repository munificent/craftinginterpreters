#!/usr/bin/env python3

from __future__ import print_function

from collections import defaultdict
from os import listdir
from os.path import abspath, basename, dirname, isdir, isfile, join, realpath, relpath, splitext
import re
from subprocess import Popen, PIPE
import sys

# Runs the tests.
REPO_DIR = dirname(dirname(realpath(__file__)))

OUTPUT_EXPECT = re.compile(r'// expect: ?(.*)')
ERROR_EXPECT = re.compile(r'// (Error.*)')
ERROR_LINE_EXPECT = re.compile(r'// \[((java|c) )?line (\d+)\] (Error.*)')
RUNTIME_ERROR_EXPECT = re.compile(r'// expect runtime error: (.+)')
SYNTAX_ERROR_RE = re.compile(r'\[.*line (\d+)\] (Error.+)')
STACK_TRACE_RE = re.compile(r'\[line (\d+)\]')
NONTEST_RE = re.compile(r'// nontest')

passed = 0
failed = 0
num_skipped = 0
expectations = 0

interpreter = None
filter_path = None

INTERPRETERS = {}
C_SUITES = []
JAVA_SUITES = []


class Interpreter:
  def __init__(self, name, language, args, tests):
    self.name = name
    self.language = language
    self.args = args
    self.tests = tests


def c_interpreter(name, tests):
  if name == 'clox':
    path = 'build/cloxd'
  else:
    path = 'build/' + name

  INTERPRETERS[name] = Interpreter(name, 'c', [path], tests)
  C_SUITES.append(name)


def java_interpreter(name, tests):
  if name == 'jlox':
    dir = 'build/java'
  else:
    dir = 'build/gen/' + name

  INTERPRETERS[name] = Interpreter(name, 'java',
      ['java', '-cp', dir, 'com.craftinginterpreters.lox.Lox'], tests)
  JAVA_SUITES.append(name)


java_interpreter('jlox', {
  'test': 'pass',

  # These are just for earlier chapters.
  'test/scanning': 'skip',
  'test/expressions': 'skip',

  # No hardcoded limits in jlox.
  'test/limit/loop_too_large.lox': 'skip',
  'test/limit/too_many_constants.lox': 'skip',
  'test/limit/too_many_locals.lox': 'skip',
  'test/limit/too_many_upvalues.lox': 'skip',

  # Rely on JVM for stack overflow checking.
  'test/limit/stack_overflow.lox': 'skip',
})

java_interpreter('chap04_scanning', {
  # No interpreter yet.
  'test': 'skip',

  'test/scanning': 'pass'
})

# No test for chapter 5. It just has a hardcoded main() in AstPrinter.

java_interpreter('chap06_parsing', {
  # No real interpreter yet.
  'test': 'skip',

  'test/expressions/parse.lox': 'pass'
})

java_interpreter('chap07_evaluating', {
  # No real interpreter yet.
  'test': 'skip',

  'test/expressions/evaluate.lox': 'pass'
})

java_interpreter('chap08_statements', {
  'test': 'pass',

  # These are just for earlier chapters.
  'test/scanning': 'skip',
  'test/expressions': 'skip',

  # No hardcoded limits in jlox.
  'test/limit/loop_too_large.lox': 'skip',
  'test/limit/reuse_constants.lox': 'skip',
  'test/limit/too_many_constants.lox': 'skip',
  'test/limit/too_many_locals.lox': 'skip',
  'test/limit/too_many_upvalues.lox': 'skip',

  # Rely on JVM for stack overflow checking.
  'test/limit/stack_overflow.lox': 'skip',

  # No control flow.
  'test/block/empty.lox': 'skip',
  'test/for': 'skip',
  'test/if': 'skip',
  'test/logical_operator': 'skip',
  'test/while': 'skip',
  'test/variable/unreached_undefined.lox': 'skip',

  # No functions.
  'test/call': 'skip',
  'test/closure': 'skip',
  'test/function': 'skip',
  'test/operator/not.lox': 'skip',
  'test/return': 'skip',
  'test/unexpected_character.lox': 'skip',
  
  'test/regression': 'skip',

  # Broken because we haven't fixed it yet by detecting the error.
  'test/return/at_top_level.lox': 'skip',
  'test/variable/use_local_in_initializer.lox': 'skip',

  # No resolution.
  'test/closure/assign_to_shadowed_later.lox': 'skip',
  'test/function/local_mutual_recursion.lox': 'skip',
  'test/variable/collide_with_parameter.lox': 'skip',
  'test/variable/duplicate_local.lox': 'skip',
  'test/variable/duplicate_parameter.lox': 'skip',
  'test/variable/early_bound.lox': 'skip',

  # No classes.
  'test/assignment/to_this.lox': 'skip',
  'test/call/object.lox': 'skip',
  'test/class': 'skip',
  'test/closure/close_over_method_parameter.lox': 'skip',
  'test/constructor': 'skip',
  'test/field': 'skip',
  'test/inheritance': 'skip',
  'test/method': 'skip',
  'test/number/decimal_point_at_eof.lox': 'skip',
  'test/number/trailing_dot.lox': 'skip',
  'test/operator/equals_class.lox': 'skip',
  'test/operator/not_class.lox': 'skip',
  'test/super': 'skip',
  'test/this': 'skip',
  'test/return/in_method.lox': 'skip',
  'test/variable/local_from_method.lox': 'skip',
})

java_interpreter('chap09_control', {
  'test': 'pass',

  # These are just for earlier chapters.
  'test/scanning': 'skip',
  'test/expressions': 'skip',

  # No hardcoded limits in jlox.
  'test/limit/loop_too_large.lox': 'skip',
  'test/limit/reuse_constants.lox': 'skip',
  'test/limit/too_many_constants.lox': 'skip',
  'test/limit/too_many_locals.lox': 'skip',
  'test/limit/too_many_upvalues.lox': 'skip',

  # Rely on JVM for stack overflow checking.
  'test/limit/stack_overflow.lox': 'skip',

  # No functions.
  'test/call': 'skip',
  'test/closure': 'skip',
  'test/for/closure_in_body.lox': 'skip',
  'test/for/return_closure.lox': 'skip',
  'test/for/return_inside.lox': 'skip',
  'test/for/syntax.lox': 'skip',
  'test/function': 'skip',
  'test/operator/not.lox': 'skip',
  'test/return': 'skip',
  'test/unexpected_character.lox': 'skip',
  'test/while/closure_in_body.lox': 'skip',
  'test/while/return_closure.lox': 'skip',
  'test/while/return_inside.lox': 'skip',

  # Broken because we haven't fixed it yet by detecting the error.
  'test/return/at_top_level.lox': 'skip',
  'test/variable/use_local_in_initializer.lox': 'skip',

  # No resolution.
  'test/closure/assign_to_shadowed_later.lox': 'skip',
  'test/function/local_mutual_recursion.lox': 'skip',
  'test/variable/collide_with_parameter.lox': 'skip',
  'test/variable/duplicate_local.lox': 'skip',
  'test/variable/duplicate_parameter.lox': 'skip',
  'test/variable/early_bound.lox': 'skip',

  # No classes.
  'test/assignment/to_this.lox': 'skip',
  'test/call/object.lox': 'skip',
  'test/class': 'skip',
  'test/closure/close_over_method_parameter.lox': 'skip',
  'test/constructor': 'skip',
  'test/field': 'skip',
  'test/inheritance': 'skip',
  'test/method': 'skip',
  'test/number/decimal_point_at_eof.lox': 'skip',
  'test/number/trailing_dot.lox': 'skip',
  'test/operator/equals_class.lox': 'skip',
  'test/operator/not_class.lox': 'skip',
  'test/super': 'skip',
  'test/this': 'skip',
  'test/return/in_method.lox': 'skip',
  'test/variable/local_from_method.lox': 'skip',
})

java_interpreter('chap10_functions', {
  'test': 'pass',

  # These are just for earlier chapters.
  'test/scanning': 'skip',
  'test/expressions': 'skip',

  # No hardcoded limits in jlox.
  'test/limit/loop_too_large.lox': 'skip',
  'test/limit/too_many_constants.lox': 'skip',
  'test/limit/too_many_locals.lox': 'skip',
  'test/limit/too_many_upvalues.lox': 'skip',

  # Rely on JVM for stack overflow checking.
  'test/limit/stack_overflow.lox': 'skip',

  # Broken because we haven't fixed it yet by detecting the error.
  'test/return/at_top_level.lox': 'skip',
  'test/variable/use_local_in_initializer.lox': 'skip',

  # No resolution.
  'test/closure/assign_to_shadowed_later.lox': 'skip',
  'test/function/local_mutual_recursion.lox': 'skip',
  'test/variable/collide_with_parameter.lox': 'skip',
  'test/variable/duplicate_local.lox': 'skip',
  'test/variable/duplicate_parameter.lox': 'skip',
  'test/variable/early_bound.lox': 'skip',

  # No classes.
  'test/assignment/to_this.lox': 'skip',
  'test/call/object.lox': 'skip',
  'test/class': 'skip',
  'test/closure/close_over_method_parameter.lox': 'skip',
  'test/constructor': 'skip',
  'test/field': 'skip',
  'test/inheritance': 'skip',
  'test/method': 'skip',
  'test/number/decimal_point_at_eof.lox': 'skip',
  'test/number/trailing_dot.lox': 'skip',
  'test/operator/equals_class.lox': 'skip',
  'test/operator/not_class.lox': 'skip',
  'test/super': 'skip',
  'test/this': 'skip',
  'test/return/in_method.lox': 'skip',
  'test/variable/local_from_method.lox': 'skip',
})

java_interpreter('chap11_resolving', {
  'test': 'pass',

  # These are just for earlier chapters.
  'test/scanning': 'skip',
  'test/expressions': 'skip',

  # No hardcoded limits in jlox.
  'test/limit/loop_too_large.lox': 'skip',
  'test/limit/too_many_constants.lox': 'skip',
  'test/limit/too_many_locals.lox': 'skip',
  'test/limit/too_many_upvalues.lox': 'skip',

  # Rely on JVM for stack overflow checking.
  'test/limit/stack_overflow.lox': 'skip',

  # No classes.
  'test/assignment/to_this.lox': 'skip',
  'test/call/object.lox': 'skip',
  'test/class': 'skip',
  'test/closure/close_over_method_parameter.lox': 'skip',
  'test/constructor': 'skip',
  'test/field': 'skip',
  'test/inheritance': 'skip',
  'test/method': 'skip',
  'test/number/decimal_point_at_eof.lox': 'skip',
  'test/number/trailing_dot.lox': 'skip',
  'test/operator/equals_class.lox': 'skip',
  'test/operator/not_class.lox': 'skip',
  'test/super': 'skip',
  'test/this': 'skip',
  'test/return/in_method.lox': 'skip',
  'test/variable/local_from_method.lox': 'skip',
})

java_interpreter('chap12_classes', {
  'test': 'pass',

  # These are just for earlier chapters.
  'test/scanning': 'skip',
  'test/expressions': 'skip',

  # No hardcoded limits in jlox.
  'test/limit/loop_too_large.lox': 'skip',
  'test/limit/too_many_constants.lox': 'skip',
  'test/limit/too_many_locals.lox': 'skip',
  'test/limit/too_many_upvalues.lox': 'skip',

  # Rely on JVM for stack overflow checking.
  'test/limit/stack_overflow.lox': 'skip',

  # No inheritance.
  'test/class/inherited_method.lox': 'skip',
  'test/inheritance': 'skip',
  'test/super': 'skip',
})

java_interpreter('chap13_inheritance', {
  'test': 'pass',

  # These are just for earlier chapters.
  'test/scanning': 'skip',
  'test/expressions': 'skip',

  # No hardcoded limits in jlox.
  'test/limit/loop_too_large.lox': 'skip',
  'test/limit/too_many_constants.lox': 'skip',
  'test/limit/too_many_locals.lox': 'skip',
  'test/limit/too_many_upvalues.lox': 'skip',

  # Rely on JVM for stack overflow checking.
  'test/limit/stack_overflow.lox': 'skip',
})

c_interpreter('clox', {
  'test': 'pass',

  # These are just for earlier chapters.
  'test/scanning': 'skip',
  'test/expressions': 'skip'
})

# TODO: Other chapters.

c_interpreter('chap21_global', {
  'test': 'pass',

  # These are just for earlier chapters.
  'test/scanning': 'skip',
  'test/expressions': 'skip',

  # No control flow.
  'test/block/empty.lox': 'skip',
  'test/for': 'skip',
  'test/if': 'skip',
  'test/limit/loop_too_large.lox': 'skip',
  'test/logical_operator': 'skip',
  'test/variable/unreached_undefined.lox': 'skip',
  'test/while': 'skip',

  # No blocks.
  'test/assignment/local.lox': 'skip',
  'test/variable/in_middle_of_block.lox': 'skip',
  'test/variable/in_nested_block.lox': 'skip',
  'test/variable/scope_reuse_in_different_blocks.lox': 'skip',
  'test/variable/shadow_and_local.lox': 'skip',
  'test/variable/undefined_local.lox': 'skip',

  # No local variables.
  'test/block/scope.lox': 'skip',
  'test/variable/duplicate_local.lox': 'skip',
  'test/variable/shadow_global.lox': 'skip',
  'test/variable/shadow_local.lox': 'skip',
  'test/variable/use_local_in_initializer.lox': 'skip',

  # No functions.
  'test/call': 'skip',
  'test/closure': 'skip',
  'test/function': 'skip',
  'test/limit/reuse_constants.lox': 'skip',
  'test/limit/stack_overflow.lox': 'skip',
  'test/limit/too_many_constants.lox': 'skip',
  'test/limit/too_many_locals.lox': 'skip',
  'test/limit/too_many_upvalues.lox': 'skip',
  'test/return': 'skip',
  'test/unexpected_character.lox': 'skip',
  'test/variable/collide_with_parameter.lox': 'skip',
  'test/variable/duplicate_parameter.lox': 'skip',
  'test/variable/early_bound.lox': 'skip',

  # No classes.
  'test/assignment/to_this.lox': 'skip',
  'test/class': 'skip',
  'test/constructor': 'skip',
  'test/field': 'skip',
  'test/inheritance': 'skip',
  'test/method': 'skip',
  'test/number/decimal_point_at_eof.lox': 'skip',
  'test/number/trailing_dot.lox': 'skip',
  'test/operator/equals_class.lox': 'skip',
  'test/operator/not.lox': 'skip',
  'test/operator/not_class.lox': 'skip',
  'test/super': 'skip',
  'test/this': 'skip',
  'test/variable/local_from_method.lox': 'skip',
})

c_interpreter('chap22_local', {
  'test': 'pass',

  # These are just for earlier chapters.
  'test/scanning': 'skip',
  'test/expressions': 'skip',

  # No control flow.
  'test/block/empty.lox': 'skip',
  'test/for': 'skip',
  'test/if': 'skip',
  'test/limit/loop_too_large.lox': 'skip',
  'test/logical_operator': 'skip',
  'test/variable/unreached_undefined.lox': 'skip',
  'test/while': 'skip',

  # No functions.
  'test/call': 'skip',
  'test/closure': 'skip',
  'test/function': 'skip',
  'test/limit/reuse_constants.lox': 'skip',
  'test/limit/stack_overflow.lox': 'skip',
  'test/limit/too_many_constants.lox': 'skip',
  'test/limit/too_many_locals.lox': 'skip',
  'test/limit/too_many_upvalues.lox': 'skip',
  'test/return': 'skip',
  'test/unexpected_character.lox': 'skip',
  'test/variable/collide_with_parameter.lox': 'skip',
  'test/variable/duplicate_parameter.lox': 'skip',
  'test/variable/early_bound.lox': 'skip',

  # No classes.
  'test/assignment/to_this.lox': 'skip',
  'test/class': 'skip',
  'test/constructor': 'skip',
  'test/field': 'skip',
  'test/inheritance': 'skip',
  'test/method': 'skip',
  'test/number/decimal_point_at_eof.lox': 'skip',
  'test/number/trailing_dot.lox': 'skip',
  'test/operator/equals_class.lox': 'skip',
  'test/operator/not.lox': 'skip',
  'test/operator/not_class.lox': 'skip',
  'test/super': 'skip',
  'test/this': 'skip',
  'test/variable/local_from_method.lox': 'skip',
})

c_interpreter('chap23_jumping', {
  'test': 'pass',

  # These are just for earlier chapters.
  'test/scanning': 'skip',
  'test/expressions': 'skip',

  # No functions.
  'test/call': 'skip',
  'test/closure': 'skip',
  'test/for/closure_in_body.lox': 'skip',
  'test/for/return_closure.lox': 'skip',
  'test/for/return_inside.lox': 'skip',
  'test/for/syntax.lox': 'skip',
  'test/function': 'skip',
  'test/limit/reuse_constants.lox': 'skip',
  'test/limit/stack_overflow.lox': 'skip',
  'test/limit/too_many_constants.lox': 'skip',
  'test/limit/too_many_locals.lox': 'skip',
  'test/limit/too_many_upvalues.lox': 'skip',
  'test/return': 'skip',
  'test/unexpected_character.lox': 'skip',
  'test/variable/collide_with_parameter.lox': 'skip',
  'test/variable/duplicate_parameter.lox': 'skip',
  'test/variable/early_bound.lox': 'skip',
  'test/while/closure_in_body.lox': 'skip',
  'test/while/return_closure.lox': 'skip',
  'test/while/return_inside.lox': 'skip',

  # No classes.
  'test/assignment/to_this.lox': 'skip',
  'test/class': 'skip',
  'test/constructor': 'skip',
  'test/field': 'skip',
  'test/inheritance': 'skip',
  'test/method': 'skip',
  'test/number/decimal_point_at_eof.lox': 'skip',
  'test/number/trailing_dot.lox': 'skip',
  'test/operator/equals_class.lox': 'skip',
  'test/operator/not.lox': 'skip',
  'test/operator/not_class.lox': 'skip',
  'test/super': 'skip',
  'test/this': 'skip',
  'test/variable/local_from_method.lox': 'skip',
})

c_interpreter('chap24_calls', {
  'test': 'pass',

  # These are just for earlier chapters.
  'test/scanning': 'skip',
  'test/expressions': 'skip',

  # No closures.
  'test/closure': 'skip',
  'test/for/closure_in_body.lox': 'skip',
  'test/for/return_closure.lox': 'skip',
  'test/function/local_recursion.lox': 'skip',
  'test/limit/too_many_upvalues.lox': 'skip',
  'test/while/closure_in_body.lox': 'skip',
  'test/while/return_closure.lox': 'skip',

  # No classes.
  'test/assignment/to_this.lox': 'skip',
  'test/call/object.lox': 'skip',
  'test/class': 'skip',
  'test/constructor': 'skip',
  'test/field': 'skip',
  'test/inheritance': 'skip',
  'test/method': 'skip',
  'test/number/decimal_point_at_eof.lox': 'skip',
  'test/number/trailing_dot.lox': 'skip',
  'test/operator/equals_class.lox': 'skip',
  'test/operator/not.lox': 'skip',
  'test/operator/not_class.lox': 'skip',
  'test/return/in_method.lox': 'skip',
  'test/super': 'skip',
  'test/this': 'skip',
  'test/variable/local_from_method.lox': 'skip',
})

c_interpreter('chap25_closures', {
  'test': 'pass',

  # These are just for earlier chapters.
  'test/scanning': 'skip',
  'test/expressions': 'skip',

  # No classes.
  'test/assignment/to_this.lox': 'skip',
  'test/call/object.lox': 'skip',
  'test/class': 'skip',
  'test/closure/close_over_method_parameter.lox': 'skip',
  'test/constructor': 'skip',
  'test/field': 'skip',
  'test/inheritance': 'skip',
  'test/method': 'skip',
  'test/number/decimal_point_at_eof.lox': 'skip',
  'test/number/trailing_dot.lox': 'skip',
  'test/operator/equals_class.lox': 'skip',
  'test/operator/not.lox': 'skip',
  'test/operator/not_class.lox': 'skip',
  'test/return/in_method.lox': 'skip',
  'test/super': 'skip',
  'test/this': 'skip',
  'test/variable/local_from_method.lox': 'skip',
})

c_interpreter('chap26_garbage', {
  'test': 'pass',

  # These are just for earlier chapters.
  'test/scanning': 'skip',
  'test/expressions': 'skip',

  # No classes.
  'test/assignment/to_this.lox': 'skip',
  'test/call/object.lox': 'skip',
  'test/class': 'skip',
  'test/closure/close_over_method_parameter.lox': 'skip',
  'test/constructor': 'skip',
  'test/field': 'skip',
  'test/inheritance': 'skip',
  'test/method': 'skip',
  'test/number/decimal_point_at_eof.lox': 'skip',
  'test/number/trailing_dot.lox': 'skip',
  'test/operator/equals_class.lox': 'skip',
  'test/operator/not.lox': 'skip',
  'test/operator/not_class.lox': 'skip',
  'test/return/in_method.lox': 'skip',
  'test/super': 'skip',
  'test/this': 'skip',
  'test/variable/local_from_method.lox': 'skip',
})

c_interpreter('chap27_classes', {
  'test': 'pass',

  # These are just for earlier chapters.
  'test/scanning': 'skip',
  'test/expressions': 'skip',

  # No inheritance.
  'test/class/inherited_method.lox': 'skip',
  'test/inheritance': 'skip',
  'test/super': 'skip',

  # No methods.
  'test/assignment/to_this.lox': 'skip',
  'test/class/local_reference_self.lox': 'skip',
  'test/class/reference_self.lox': 'skip',
  'test/closure/close_over_method_parameter.lox': 'skip',
  'test/constructor': 'skip',
  'test/field/method.lox': 'skip',
  'test/field/method_binds_this.lox': 'skip',
  'test/method': 'skip',
  'test/operator/equals_class.lox': 'skip',
  'test/return/in_method.lox': 'skip',
  'test/this': 'skip',
  'test/variable/local_from_method.lox': 'skip',
})

c_interpreter('chap28_methods', {
  'test': 'pass',

  # These are just for earlier chapters.
  'test/scanning': 'skip',
  'test/expressions': 'skip',

  # No inheritance.
  'test/class/inherited_method.lox': 'skip',
  'test/inheritance': 'skip',
  'test/super': 'skip',
})

c_interpreter('chap29_superclasses', {
  'test': 'pass',

  # These are just for earlier chapters.
  'test/scanning': 'skip',
  'test/expressions': 'skip',
})

c_interpreter('chap30_optimization', {
  'test': 'pass',

  # These are just for earlier chapters.
  'test/scanning': 'skip',
  'test/expressions': 'skip',
})

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
    global expectations

    # Get the path components.
    parts = self.path.split('/')
    subpath = ""
    state = None

    # Figure out the state of the test. We don't break out of this loop because
    # we want lines for more specific paths to override more general ones.
    for part in parts:
      if subpath: subpath += '/'
      subpath += part

      if subpath in interpreter.tests:
        state = interpreter.tests[subpath]

    if not state:
      print('Unknown test state for "{}".'.format(self.path))
    if state == 'skip':
      num_skipped += 1
      return False
    # TODO: State for tests that should be run but are expected to fail?

    line_num = 1
    with open(self.path, 'r') as file:
      for line in file:
        match = OUTPUT_EXPECT.search(line)
        if match:
          self.output.append((match.group(1), line_num))
          expectations += 1

        match = ERROR_EXPECT.search(line)
        if match:
          self.compile_errors.add("[{0}] {1}".format(line_num, match.group(1)))

          # If we expect a compile error, it should exit with EX_DATAERR.
          self.exit_code = 65
          expectations += 1

        match = ERROR_LINE_EXPECT.search(line)
        if match:
          # The two interpreters are slightly different in terms of which
          # cascaded errors may appear after an initial compile error because
          # their panic mode recovery is a little different. To handle that,
          # the tests can indicate if an error line should only appear for a
          # certain interpreter.
          language = match.group(2)
          if not language or language == interpreter.language:
            self.compile_errors.add("[{0}] {1}".format(
                match.group(3), match.group(4)))

            # If we expect a compile error, it should exit with EX_DATAERR.
            self.exit_code = 65
            expectations += 1

        match = RUNTIME_ERROR_EXPECT.search(line)
        if match:
          self.runtime_error_line = line_num
          self.runtime_error_message = match.group(1)
          # If we expect a runtime error, it should exit with EX_SOFTWARE.
          self.exit_code = 70
          expectations += 1

        match = NONTEST_RE.search(line)
        if match:
          # Not a test file at all, so ignore it.
          return False

        line_num += 1


    # If we got here, it's a valid test.
    return True


  def run(self):
    # Invoke the interpreter and run the test.
    args = interpreter.args[:]
    args.append(self.path)
    proc = Popen(args, stdin=PIPE, stdout=PIPE, stderr=PIPE)

    out, err = proc.communicate()
    self.validate(proc.returncode, out, err)


  def validate(self, exit_code, out, err):
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
    self.validate_output(out)


  def validate_runtime_error(self, error_lines):
    if len(error_lines) < 2:
      self.fail('Expected runtime error "{0}" and got none.',
          self.runtime_error_message)
      return

    # Skip any compile errors. This can happen if there is a compile error in
    # a module loaded by the module being tested.
    line = 0
    while SYNTAX_ERROR_RE.search(error_lines[line]):
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
      match = STACK_TRACE_RE.search(stack_line)
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
    num_unexpected = 0
    for line in error_lines:
      match = SYNTAX_ERROR_RE.search(line)
      if match:
        error = "[{0}] {1}".format(match.group(1), match.group(2))
        if error in self.compile_errors:
          found_errors.add(error)
        else:
          if num_unexpected < 10:
            self.fail('Unexpected error:')
            self.fail(line)
          num_unexpected += 1
      elif line != '':
        if num_unexpected < 10:
          self.fail('Unexpected output on stderr:')
          self.fail(line)
        num_unexpected += 1

    if num_unexpected > 10:
      self.fail('(truncated ' + str(num_unexpected - 10) + ' more...)')

    # Validate that every expected error occurred.
    for error in self.compile_errors - found_errors:
      self.fail('Missing expected error: {0}', error)


  def validate_exit_code(self, exit_code, error_lines):
    if exit_code == self.exit_code: return

    if len(error_lines) > 10:
      error_lines = error_lines[0:10]
      error_lines.append('(truncated...)')
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
def gray(text):   return color_text(text, '\033[1;30m')


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


def run_script(path):
  if "benchmark" in path: return

  global passed
  global failed
  global num_skipped

  if (splitext(path)[1] != '.lox'):
    return

  # Check if we are just running a subset of the tests.
  if filter_path:
    this_test = relpath(path, join(REPO_DIR, 'test'))
    if not this_test.startswith(filter_path):
      return

  # Make a nice short path relative to the working directory.

  # Normalize it to use "/" since, among other things, the interpreters expect
  # the argument to use that.
  path = relpath(path).replace("\\", "/")

  # Update the status line.
  print_line('Passed: ' + green(passed) +
             ' Failed: ' + red(failed) +
             ' Skipped: ' + yellow(num_skipped) +
             gray(' (' + path + ')'))

  # Read the test and parse out the expectations.
  test = Test(path)

  if not test.parse():
    # It's a skipped or non-test file.
    return

  test.run()

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


def run_suite(name):
  global interpreter
  global passed
  global failed
  global num_skipped
  global expectations

  interpreter = INTERPRETERS[name]

  passed = 0
  failed = 0
  num_skipped = 0
  expectations = 0

  walk(join(REPO_DIR, 'test'), run_script)
  print_line()

  if failed == 0:
    print('All ' + green(passed) + ' tests passed (' + str(expectations) +
          ' expectations).')
  else:
    print(green(passed) + ' tests passed. ' + red(failed) + ' tests failed.')

  return failed == 0


def run_suites(names):
  any_failed = False
  for name in names:
    print('=== {} ==='.format(name))
    if not run_suite(name):
      any_failed = True

  if any_failed:
    sys.exit(1)


if len(sys.argv) < 2 or len(sys.argv) > 3:
  print('Usage: test.py <interpreter> [filter]')
  sys.exit(1)

if len(sys.argv) == 3:
  filter_path = sys.argv[2]

if sys.argv[1] == 'all':
  run_suites(sorted(INTERPRETERS.keys()))
elif sys.argv[1] == 'c':
  run_suites(C_SUITES)
elif sys.argv[1] == 'java':
  run_suites(JAVA_SUITES)
elif sys.argv[1] not in INTERPRETERS:
  print('Unknown interpreter "{}"'.format(sys.argv[1]))
  sys.exit(1)

else:
  if not run_suite(sys.argv[1]):
    sys.exit(1)
