import 'dart:io';

import 'package:path/path.dart' as p;
import 'package:pool/pool.dart';

import 'package:tool/src/book.dart';
import 'package:tool/src/code_tag.dart';
import 'package:tool/src/page.dart';
import 'package:tool/src/split_chapter.dart';
import 'package:tool/src/term.dart' as term;

/// Tests that various snippets in the middle of chapters can be compiled without
/// error. Ensures that, as much as possible, we have a working program at
/// multiple points throughout the chapter.

// TODO: Do this for Java chapters.

var _chapterTags = <String, List<String>>{
  "Chunks of Bytecode": [
    "free-array",
    "main-include-chunk",
    "simple-instruction",
    "add-constant",
    "return-after-operand",
  ],
  "A Virtual Machine": [
    "main-include-vm",
    "vm-include-debug",
    "print-return",
    "main-negate",
  ],
  "Scanning on Demand": [
    "init-scanner",
    "error-token",
    "advance",
    "match",
    "newline",
    "peek-next",
    "string",
    "number",
    "identifier-type",
    "check-keyword",
  ],
  "Compiling Expressions": [
    "expression",
    "forward-declarations",
    "precedence-body",
    "infix",
    "define-debug-print-code",
    "dump-chunk"
  ],
  "Types of Values": [
    "op-arithmetic",
    "print-value",
    "disassemble-not",
    "values-equal",
  ],
  "Strings": [
    // "as-string",
    // We could get things working earlier by moving the "Operations on Strings"
    // section before "Strings".
    "value-include-object",
    "vm-include-object-memory",
  ],
  "Hash Tables": [
    "free-table",
    "hash-string",
    "table-add-all",
    "table-get",
    "table-delete",
    "resize-increment-count",
  ],
  "Global Variables": [
    "disassemble-print",
    "disassemble-pop",
    "synchronize",
    "define-global-op",
    "disassemble-define-global",
    "disassemble-get-global",
    "disassemble-set-global",
  ],
  "Local Variables": [
    "local-struct",
    "compiler",
    "end-scope",
    "add-local",
    "too-many-locals",
    "pop-locals",
    "interpret-set-local",
  ],
  "Jumping Back and Forth": [
    "jump-if-false-op",
    "compile-else",
    "jump-op",
    "pop-end",
    "jump-instruction",
    "and",
    "or",
    "while-statement",
    "loop-op",
    "disassemble-loop",
    "for-statement",
  ],
  "Calls and Functions": [
    "as-function",
    "function-type-enum",
    "init-compiler",
    "init-function-slot",
    "return-function",
    "disassemble-end",
    "runtime-error-temp",
    "compile-function",
    "init-function-name",
    "call",
    "interpret",
    "disassemble-call",
    "return-statement",
    "runtime-error-stack",
    "return-from-script",
    "print-native",
    "define-native",
    "vm-include-time"
  ],
  "Closures": [
    "obj-closure",
    "new-closure-h",
    "print-closure",
    "closure-op",
    "disassemble-closure",
    "interpret-closure",
    "runtime-error-function",
    "interpret",
    "upvalue-struct",
    "resolve-upvalue-recurse",
    "capture-upvalues",
    "debug-include-object",
    "obj-upvalue",
    "new-upvalue-h",
    "print-upvalue",
    "upvalue-fields",
    "allocate-upvalue-array",
    "init-upvalue-fields",
    "free-upvalues",
    "capture-upvalue",
    "interpret-get-upvalue",
    "interpret-set-upvalue",
    "is-captured-field",
    "init-is-captured",
    "init-zero-local-is-captured",
    "mark-local-captured",
    "close-upvalue-op",
    "disassemble-close-upvalue",
    "next-field",
    "init-next",
    "open-upvalues-field",
    "init-open-upvalues",
    "look-for-existing-upvalue",
    "insert-upvalue-in-list",
    "closed-field",
    "init-closed",
    "return-close-upvalues",
  ],
  "Garbage Collection": [
    "collect-garbage-h",
    "collect-garbage",
    "define-stress-gc",
    "call-collect",
    "define-log-gc",
    "debug-log-includes",
    "log-before-collect",
    "log-after-collect",
    "debug-log-allocate",
    "log-free-object",
    "mark-value-h",
    "mark-object-h",
    "is-marked-field",
    "init-is-marked",
    "log-mark-object",
    "mark-table-h",
    "mark-table",
    "mark-closures",
    "mark-open-upvalues",
    "memory-include-compiler",
    "compiler-include-memory",
    "vm-gray-stack",
    "init-gray-stack",
    "free-gray-stack",
    "blacken-closure",
    "log-blacken-object",
    "check-is-marked",
    "sweep",
    "unmark",
    "table-remove-white-h",
    "table-remove-white",
    "vm-fields",
    "init-gc-fields",
    "updated-bytes-allocated",
    "collect-on-next",
    "heap-grow-factor",
    "log-before-size",
    "log-collected-amount",
    "chunk-include-vm",
    "push-string",
    "pop-string",
    "concatenate-peek",
    "concatenate-pop",
  ],
  "Classes and Instances": [
    "obj-class",
    "print-class",
    "class-op",
    "disassemble-class",
    "interpret-class",
    "object-include-table",
    "print-instance",
    "call-class",
    "property-ops",
    "disassemble-property-ops",
    "interpret-get-property",
    "get-undefined",
    "get-not-instance",
    "interpret-set-property",
    "set-not-instance",
  ],
  "Methods and Initializers": [
    "class-methods",
    "init-methods",
    "free-methods",
    "mark-methods",
    "method-op",
    "disassemble-method",
    "define-method",
    "obj-bound-method",
    "print-bound-method",
    "bind-method",
    "call-bound-method",
    "this",
    "slot-zero",
    "method-type-enum",
    "method-type",
    "store-receiver",
    "class-compiler-struct",
    "create-class-compiler",
    "pop-enclosing",
    "this-outside-class",
    "vm-init-string",
    "init-init-string",
    "mark-init-string",
    "null-init-string",
    "clear-init-string",
    "initializer-type-enum",
    "return-this",
    "return-from-init",
    "invoke-op",
    "invoke-instruction",
    "invoke-from-class",
    "invoke-field",
  ],
  "Superclasses": [
    "inherit-op",
    "disassemble-inherit",
    "interpret-inherit",
    "inherit-non-class",
    "synthetic-token",
    "has-superclass",
    "init-has-superclass",
    "set-has-superclass",
    "get-super-op",
    "disassemble-get-super",
    "interpret-get-super",
    "super-invoke-op",
    "disassemble-super-invoke",
    "interpret-super-invoke",
  ],
  "Optimization": [
    "initial-index",
    "next-index",
    "adjust-alloc",
    "adjust-init",
    "re-hash",
    "adjust-free",
    "table-set-grow",
    "init-capacity-mask",
    "add-all-loop",
    "find-string-index",
    "find-string-next",
    "mark-table",
    "remove-white",
    "free-table",
    "define-nan-boxing",
    "end-values-equal",
  ],
};

var _allPassed = true;

Future<void> main(List<String> arguments) async {
  var watch = Stopwatch()..start();
  var book = Book();
  var pool = Pool(Platform.numberOfProcessors);
  var futures = <Future<void>>[];

  for (var chapterName in _chapterTags.keys) {
    var chapter = book.findChapter(chapterName);

    var tags = chapter.codeTags;
    var tagNames = _chapterTags[chapterName];
    if (tagNames.isNotEmpty) {
      tags = tagNames.map((name) => book.findTag(chapter, name));
    } else {
      print("Warning, no in-chapter snippets for '$chapterName'");
    }

    for (var tag in tags) {
      futures
          .add(pool.withResource(() => _compileChapterTag(book, chapter, tag)));
    }
  }

  await Future.wait(futures);

  print("Done in ${watch.elapsedMilliseconds / 1000} seconds");
  if (!_allPassed) exit(1);
}

Future<void> _compileChapterTag(Book book, Page chapter, CodeTag tag) async {
  await splitChapter(book, chapter, tag);

  var buildName = "${chapter.shortName}-${tag.directory}";
  var sourceDir = p.join("gen", "snippets", chapter.shortName, tag.directory);

  var makeArguments = [
    "-f",
    "util/c.make",
    "NAME=$buildName",
    "MODE=release",
    "SOURCE_DIR=$sourceDir",
    "SNIPPET=true"
  ];

  var result = await Process.run("make", makeArguments);
  if (result.exitCode == 0) {
    print("${term.green('PASS')} ${chapter.title} / ${tag.name}");
  } else {
    print("${term.red('FAIL')} ${chapter.title} / ${tag.name}");
    print(result.stdout);
    print(result.stderr);
    print("");
    _allPassed = false;
  }
}
