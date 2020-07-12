import 'dart:io';

import 'book.dart';
import 'code_tag.dart';
import 'location.dart';
import 'page.dart';

final _blockPattern = RegExp(
    r"^/\* ([A-Z][A-Za-z\s]+) ([-a-z0-9]+) < ([A-Z][A-Za-z\s]+) ([-a-z0-9]+)$");
final _blockSnippetPattern = RegExp(r"^/\* < ([-a-z0-9]+)$");
final _beginSnippetPattern = RegExp(r"^//> ([-a-z0-9]+)$");
final _endSnippetPattern = RegExp(r"^//< ([-a-z0-9]+)$");
final _beginChapterPattern = RegExp(r"^//> ([A-Z][A-Za-z\s]+) ([-a-z0-9]+)$");
final _endChapterPattern = RegExp(r"^//< ([A-Z][A-Za-z\s]+) ([-a-z0-9]+)$");

// Hacky regexes that matches various declarations.
final _constructorPattern = RegExp(r"^  ([A-Z][a-z]\w+)\(");
final _functionPattern = RegExp(r"(\w+)>*\*? (\w+)\(([^)]*)");
final _modulePattern = RegExp(r"^(\w+) (\w+);");
final _structPattern = RegExp(r"^struct (s\w+)? {$");
final _typePattern =
    RegExp(r"(public )?(abstract )?(class|enum|interface) ([A-Z]\w+)");
final _namedTypedefPattern = RegExp(r"^typedef (enum|struct|union) (\w+) {$");
final _unnamedTypedefPattern = RegExp(r"^typedef (enum|struct|union) {$");
final _typedefNamePattern = RegExp(r"^\} (\w+);$");

/// Reserved words that can appear like a return type in a function declaration
/// but shouldn't be treated as one.
const _keywords = {"new", "return", "throw"};

class SourceFileParser {
  final Book _book;
  final String _path;
  final SourceFile _file;
  final List<String> _lines;
  final List<_ParseState> _states = [];

  Location _unnamedTypedef;

  Location _location;
  Location _locationBeforeBlock;

  SourceFileParser(this._book, this._path, String relative)
      : _file = SourceFile(relative),
        _lines = File(_path).readAsLinesSync() {
    _location = Location(null, "file", _file.nicePath);
  }

  SourceFile parse() {
//  line_num = 1
//  handled = False
//
//  def error(message):
//    print("Error: {} line {}: {}".format(relative, line_num, message),
//        file=sys.stderr)
//    source_code.errors[state.start.chapter].append(
//        "{} line {}: {}".format(relative, line_num, message))
//
    // Split the source file into lines.
//    printed_file = False
//    line_num = 1
    for (var i = 0; i < _lines.length; i++) {
      var line = _lines[i].trimRight();
//      handled = False
//
//      # Report any lines that are too long.
//      trimmed = re.sub(r'// \[([-a-z0-9]+)\]', '', line)
//      if len(trimmed) > 72 and not '/*' in trimmed:
//        if not printed_file:
//          print("Long line in {}:".format(file.path))
//          printed_file = True
//        print("{0:4} ({1:2} chars): {2}".format(line_num, len(trimmed), trimmed))
//

      _updateLocationBefore(line, i);

      if (!_updateState(line)) {
        var sourceLine =
            SourceLine(line, _location, _currentState.start, _currentState.end);
        _file.lines.add(sourceLine);
      }

      _updateLocationAfter(line);
//
//      line_num += 1
    }

//    # ".parent.parent" because there is always the top "null" state.
//    if state.parent != None and state.parent.parent != None:
//      print("{}: Ended with more than one state on the stack.".format(relative),
//          file=sys.stderr)
//      s = state
//      while s.parent != None:
//        print("  {}".format(s.start), file=sys.stderr)
//        s = s.parent
//      sys.exit(1)
//

    // TODO: Validate that we don't define two snippets with the same chapter
    // and number. A snippet may end up in disjoint lines in the final output
    // because a later snippet is inserted in it, but it shouldn't be explicitly
    // authored that way.
    return _file;
  }

  /// Keep track of the current location where the parser is in the source file.
  void _updateLocationBefore(String line, int lineIndex) {
    // See if we reached a new function or method declaration.
    var match = _functionPattern.firstMatch(line);
    if (match != null &&
        !line.contains("#define") &&
        !_keywords.contains(match[1])) {
      // Hack. Don't get caught by comments or string literals.
      if (!line.contains("//") && !line.contains('"')) {
        var isFunctionDeclaration = line.endsWith(";");

        // Hack: Handle multi-line declarations.
        if (line.endsWith(",") && _lines[lineIndex + 1].endsWith(";")) {
          isFunctionDeclaration = true;
        }

        _location = Location(_location,
            _file.language == "java" ? "method" : "function", match[2],
            signature: match[3],
            isFunctionDeclaration: isFunctionDeclaration);
        return;
      }
    }

    match = _constructorPattern.firstMatch(line);
    if (match != null) {
      _location = Location(_location, "constructor", match[1]);
      return;
    }

    match = _typePattern.firstMatch(line);
    if (match != null) {
      // Hack. Don't get caught by comments or string literals.
      if (!line.contains("//") && !line.contains('"')) {
        var kind = match[3];
        var name = match[4];
        _location = Location(_location, kind, name);
      }
      return;
    }

    match = _structPattern.firstMatch(line);
    if (match != null) {
      _location = Location(_location, "struct", match[1]);
      return;
    }

    match = _namedTypedefPattern.firstMatch(line);
    if (match != null) {
      _location = Location(_location, match[1], match[2]);
      return;
    }

    match = _unnamedTypedefPattern.firstMatch(line);
    if (match != null) {
      // We don't know the name of the typedef yet.
      _location = Location(_location, match[1], null);
      _unnamedTypedef = _location;
      return;
    }

    match = _modulePattern.firstMatch(line);
    if (match != null) {
      _location = Location(_location, "variable", match[1]);
      return;
    }
  }

  void _updateLocationAfter(String line) {
    var match = _typedefNamePattern.firstMatch(line);
    if (match != null) {
      // Now we know the typedef name.
      _unnamedTypedef?.name = match[1];
      _unnamedTypedef = null;
      _location = _location.parent;
    }

    // Use "startsWith" to include lines like "} [aside-marker]".
    if (line.startsWith("}")) {
      _location = _location.popToDepth(0);
    } else if (line.startsWith("  }")) {
      _location = _location.popToDepth(1);
    } else if (line.startsWith("    }")) {
      _location = _location.popToDepth(2);
    }

    // If we reached a function declaration, not a definition, then it's done
    // after one line.
    if (_location.isFunctionDeclaration) {
      _location = _location.parent;
    }

    // Module variables are only a single line.
    if (_location.kind == "variable") {
      _location = _location.parent;
    }

    // Hack. There is a one-line class in Parser.java.
    if (line.contains("class ParseError")) {
      _location = _location.parent;
    }
  }

  /// Processes any [line] that changes what snippet the parser is currently in.
  ///
  /// Returns `true` if the line contained a snippet annotation.
  bool _updateState(String line) {
    var match = _blockPattern.firstMatch(line);
    if (match != null) {
      _push(
          startChapter: _book.findChapter(match[1]),
          startName: match[2],
          endChapter: _book.findChapter(match[3]),
          endName: match[4]);
      _locationBeforeBlock = _location;
      return true;
    }

    match = _blockSnippetPattern.firstMatch(line);
    if (match != null) {
      _push(endChapter: _currentState.start.chapter, endName: match[1]);
      _locationBeforeBlock = _location;
      return true;
    }

    if (line.trim() == "*/" && _currentState.end != null) {
      _location = _locationBeforeBlock;
      _pop();
      return true;
    }

    match = _beginSnippetPattern.firstMatch(line);
    if (match != null) {
      var name = match[1];
//        var tag = source_code.find_snippet_tag(state.start.chapter, name);
//        if tag < state.start:
//          error("Can't push earlier snippet {} from {}.".format(name, state.start.name))
//        elif tag == state.start:
//          error("Can't push to same snippet {}.".format(name))
      _push(startName: name);
      return true;
    }

    match = _endSnippetPattern.firstMatch(line);
    if (match != null) {
//      var name = match[1];
//        if name != state.start.name:
//          error("Expecting to pop {} but got {}.".format(state.start.name, name))
//        if state.parent.start.chapter == None:
//          error('Cannot pop last state {}.'.format(state.start))
      _pop();
      return true;
    }

    match = _beginChapterPattern.firstMatch(line);
    if (match != null) {
      var chapter = _book.findChapter(match[1]);
      var name = match[2];

//        if state.start != None:
//          old_chapter = book.chapter_number(state.start.chapter)
//          new_chapter = book.chapter_number(chapter)
//
//          if chapter == state.start.chapter and name == state.start.name:
//            error('Pushing same snippet "{} {}"'.format(chapter, name))
//          if chapter == state.start.chapter:
//            error('Pushing same chapter, just use "//>> {}"'.format(name))
//          if new_chapter < old_chapter:
//            error('Can\'t push earlier chapter "{}" from "{}".'.format(
//                chapter, state.start.chapter))
      _push(startChapter: chapter, startName: name);
      return true;
    }

    match = _endChapterPattern.firstMatch(line);
    if (match != null) {
//      var chapter = match[1];
//      var name = match[2];
//        if chapter != state.start.chapter or name != state.start.name:
//          error('Expecting to pop "{} {}" but got "{} {}".'.format(
//              state.start.chapter, state.start.name, chapter, name))
//        if state.start.chapter == None:
//          error('Cannot pop last state "{}".'.format(state.start))
      _pop();
      return true;
    }

    return false;
  }

  _ParseState get _currentState => _states.last;

  void _push(
      {Page startChapter, String startName, Page endChapter, String endName}) {
    startChapter ??= _currentState.start.chapter;

    CodeTag start;
    if (startName != null) {
      start = startChapter.findCodeTag(startName);
    } else {
      start = _currentState.start;
    }

    CodeTag end;
    if (endChapter != null) {
      end = endChapter.findCodeTag(endName);
    }

    _states.add(_ParseState(start, end));
  }

  void _pop() {
    _states.removeLast();
  }
}

class _ParseState {
  final CodeTag start;
  final CodeTag end;

  _ParseState(this.start, [this.end]);

  String toString() {
    if (end != null) return "_ParseState($start > $end)";
    return "_ParseState($start)";
  }
}
