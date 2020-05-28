import 'dart:io';

import 'package:glob/glob.dart';
import 'package:path/path.dart' as p;
import 'package:pool/pool.dart';

import 'package:tool/src/book.dart';
import 'package:tool/src/code_tag.dart';
import 'package:tool/src/page.dart';
import 'package:tool/src/source_file_parser.dart';

/// Don't do too many file operations at once or we risk running out of file
/// descriptors.
var _filePool = Pool(200);

Future<void> splitChapter(Book book, Page chapter, [CodeTag tag]) async {
  var futures = <Future<void>>[];

  for (var file in Glob("${chapter.language}/**.{c,h,java}").listSync()) {
    futures.add(_splitSourceFile(book, chapter, file.path, tag));
  }

  await Future.wait(futures);
}

Future<void> _splitSourceFile(Book book, Page chapter, String sourcePath,
    [CodeTag tag]) async {
  var relative = p.relative(sourcePath, from: chapter.language);

  // Don't split the generated files.
  if (relative == "com/craftinginterpreters/lox/Expr.java") return;
  if (relative == "com/craftinginterpreters/lox/Stmt.java") return;

  var package = chapter.shortName;
  if (tag != null) {
    package = p.join("snippets", package, tag.directory);
  }

  // If we're generating the split for an entire chapter, include all its
  // snippets.
  tag ??= book.lastSnippet(chapter).tag;

  var outputFile = File(p.join("gen", package, relative));

  var resource = await _filePool.request();
  try {
    var output = _generateSourceFile(book, chapter, sourcePath, tag);
    if (output.isNotEmpty) {
      // Don't overwrite the file if it didn't change, so the makefile doesn't
      // think it was touched.
      if (await outputFile.exists()) {
        var previous = await outputFile.readAsString();
        if (previous == output) return;
      }

      // Write the changed output.
      await Directory(p.dirname(outputFile.path)).create(recursive: true);
      await outputFile.writeAsString(output);
    } else {
      // Remove it since it's supposed to be nonexistent.
      if (await outputFile.exists()) await outputFile.delete();
    }
  } finally {
    resource.release();
  }
}

/// Gets the code for [sourceFilePath] as it appears at [tag] of [chapter].
String _generateSourceFile(
    Book book, Page chapter, String sourcePath, CodeTag tag) {
  var shortPath = p.relative(sourcePath, from: chapter.language);
  var sourceFile = SourceFileParser(book, sourcePath, shortPath).parse();

  var buffer = StringBuffer();
  for (var line in sourceFile.lines) {
    if (line.isPresent(tag)) {
      // Hack. In generate_ast.java, we split up a parameter list among
      // multiple chapters, which leads to hanging commas in some cases.
      // Remove them.
      if (line.text.trim().startsWith(")")) {
        var text = buffer.toString();
        if (text.endsWith(",\n")) {
          buffer.clear();
          buffer.writeln(text.substring(0, text.length - 2));
        }
      }

      buffer.writeln(line.text);
    }
  }

  return buffer.toString();
}
