import 'dart:io';

import 'page.dart';
import 'snippet_tag.dart';
import 'source_code.dart';

//BLOCK_PATTERN = re.compile(
//    r'/\* ([A-Z][A-Za-z\s]+) ([-a-z0-9]+) < ([A-Z][A-Za-z\s]+) ([-a-z0-9]+)')
//BLOCK_SNIPPET_PATTERN = re.compile(r'/\* < ([-a-z0-9]+)')
final _beginSnippetPattern = RegExp(r"^//> ([-a-z0-9]+)$");
final _endSnippetPattern = RegExp(r"^//< ([-a-z0-9]+)$");
final _beginChapterPattern = RegExp(r"^//> ([A-Z][A-Za-z\s]+) ([-a-z0-9]+)$");
final _endChapterPattern = RegExp(r"^//< ([A-Z][A-Za-z\s]+) ([-a-z0-9]+)$");
//
//# Hacky regexes that matches a function, method or constructor declaration.
//CONSTRUCTOR_PATTERN = re.compile(r'^  ([A-Z][a-z]\w+)\(')
//FUNCTION_PATTERN = re.compile(r'(\w+)>*\*? (\w+)\(([^)]*)')
//MODULE_PATTERN = re.compile(r'^(\w+) (\w+);')
//STRUCT_PATTERN = re.compile(r'struct (s\w+)? {')
//TYPE_PATTERN = re.compile(r'(public )?(abstract )?(class|enum|interface) ([A-Z]\w+)')
//TYPEDEF_PATTERN = re.compile(r'typedef (enum|struct|union)( \w+)? {')
//TYPEDEF_NAME_PATTERN = re.compile(r'\} (\w+);')
//
//# Reserved words that can appear like a return type in a function declaration
//# but shouldn't be treated as one.
//KEYWORDS = ['new', 'return', 'throw']

class SourceFileParser {
  final String _path;
  final SourceFile _file;
  final List<_ParseState> _states = [];

  Location _currentLocation;

  SourceFileParser(this._path, String relative) : _file = SourceFile(relative) {
    _currentLocation = Location(null, "file", _file.nicePath);
  }

  SourceFile parse() {
//  line_num = 1
//  state = ParseState(None, None)
//  handled = False
//
//  location_before_block = None
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
    for (var line in File(_path).readAsLinesSync()) {
      line = line.trimRight();
//      handled = False
//
      _parseLine(line);
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

//  # TODO: Validate that we don't define two snippets with the same chapter and
//  # number. A snippet may end up in disjoint lines in the final output because
//  # a later snippet is inserted in it, but it shouldn't be explicitly authored
//  # that way.
    return _file;
  }

  void _parseLine(String line) {
//      # Report any lines that are too long.
//      trimmed = re.sub(r'// \[([-a-z0-9]+)\]', '', line)
//      if len(trimmed) > 72 and not '/*' in trimmed:
//        if not printed_file:
//          print("Long line in {}:".format(file.path))
//          printed_file = True
//        print("{0:4} ({1:2} chars): {2}".format(line_num, len(trimmed), trimmed))
//
//      # See if we reached a new function or method declaration.
//      match = FUNCTION_PATTERN.search(line)
//      is_function_declaration = False
//      if match and "#define" not in line and match.group(1) not in KEYWORDS:
//        # Hack. Don't get caught by comments or string literals.
//        if '//' not in line and '"' not in line:
//          _currentLocation = Location(
//              _currentLocation,
//              'method' if file.path.endswith('.java') else 'function',
//              match.group(2),
//              match.group(3))
//          # TODO: What about declarations with aside comments:
//          #   void foo(); // [wat]
//          is_function_declaration = line.endswith(';')
//
//          # Hack: Handle multi-line declarations.
//          if line.endswith(',') and lines[line_num].endswith(';'):
//            is_function_declaration = True
//
//      match = CONSTRUCTOR_PATTERN.match(line)
//      if (match != null) {
//        _currentLocation = Location(_currentLocation,
//                                    'constructor', match.group(1))
//
//      match = TYPE_PATTERN.search(line)
//      if (match != null) {
//        # Hack. Don't get caught by comments or string literals.
//        if '//' not in line and '"' not in line:
//          _currentLocation = Location(_currentLocation,
//                                      match.group(3), match.group(4))
//
//      match = STRUCT_PATTERN.match(line)
//      if (match != null) {
//        _currentLocation = Location(_currentLocation, 'struct', match.group(1))
//
//      match = TYPEDEF_PATTERN.match(line)
//      if (match != null) {
//        # We don't know the name of the typedef.
//        _currentLocation = Location(_currentLocation, match.group(1), '???')
//
//      match = MODULE_PATTERN.match(line)
//      if (match != null) {
//        _currentLocation = Location(_currentLocation, 'variable',
//                                    match.group(1))
//
//      match = BLOCK_PATTERN.match(line)
//      if (match != null) {
//        push(match.group(1), match.group(2), match.group(3), match.group(4))
//        location_before_block = _currentLocation
//
//      match = BLOCK_SNIPPET_PATTERN.match(line)
//      if (match != null) {
//        name = match.group(1)
//        push(state.start.chapter, state.start.name, state.start.chapter, name)
//        location_before_block = _currentLocation
//
//      if line.strip() == '*/' and state.end:
//        _currentLocation = location_before_block
//        pop()
//
    var match = _beginSnippetPattern.firstMatch(line);
    if (match != null) {
      var name = match.group(1);
      // TODO: Temp.
      SnippetTag tag;
//        var tag = source_code.find_snippet_tag(state.start.chapter, name);
//        if tag < state.start:
//          error("Can't push earlier snippet {} from {}.".format(name, state.start.name))
//        elif tag == state.start:
//          error("Can't push to same snippet {}.".format(name))
      _push(_currentState.start.chapter, name);
      return;
    }

    match = _endSnippetPattern.firstMatch(line);
    if (match != null) {
      var name = match.group(1);
//        if name != state.start.name:
//          error("Expecting to pop {} but got {}.".format(state.start.name, name))
//        if state.parent.start.chapter == None:
//          error('Cannot pop last state {}.'.format(state.start))
      _pop();
      return;
    }

    match = _beginChapterPattern.firstMatch(line);
    if (match != null) {
      var chapter = match.group(1);
      var name = match.group(2);

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
      _push(Page.find(chapter), name);
      return;
    }

    match = _endChapterPattern.firstMatch(line);
    if (match != null) {
      var chapter = match.group(1);
      var name = match.group(2);
//        if chapter != state.start.chapter or name != state.start.name:
//          error('Expecting to pop "{} {}" but got "{} {}".'.format(
//              state.start.chapter, state.start.name, chapter, name))
//        if state.start.chapter == None:
//          error('Cannot pop last state "{}".'.format(state.start))
      _pop();
      return;
    }
//
//      if not handled:
//        if not state.start:
//          error("No snippet in effect.".format(relative))
//
    var sourceLine = SourceLine(
        line, _currentLocation, _currentState.start, _currentState.end);
    _file.lines.add(sourceLine);
//
//      match = TYPEDEF_NAME_PATTERN.match(line)
//      if (match != null) {
//        # Now we know the typedef name.
//        _currentLocation.name = match.group(1)
//        _currentLocation = _currentLocation.parent
//
//      # Use "startswith" to include lines like "} [aside-marker]".
//      # TODO: Hacky. Generalize?
//      if line.startswith('}'):
//        _currentLocation = _currentLocation.pop_to_depth(0)
//      elif line.startswith('  }'):
//        _currentLocation = _currentLocation.pop_to_depth(1)
//      elif line.startswith('    }'):
//        _currentLocation = _currentLocation.pop_to_depth(2)
//
//      # If we reached a function declaration, not a definition, then it's done
//      # after one line.
//      if is_function_declaration:
//        _currentLocation = _currentLocation.parent
//
//      # Module variables are only a single line.
//      if _currentLocation.kind == 'variable':
//        _currentLocation = _currentLocation.parent
//
//      # Hack. There is a one-line class in Parser.java.
//      if 'class ParseError' in line:
//        _currentLocation = _currentLocation.parent
  }

  _ParseState get _currentState => _states.last;

  void _push(Page chapter, String name, [Page endChapter, String endName]) {
//    print("push ${chapter.title} $name");
    var start = chapter.findSnippetTag(name);
    SnippetTag end;

    if (endChapter != null) {
      end = endChapter.findSnippetTag(endName);
    }

    _states.add(_ParseState(start, end));
  }

  void _pop() {
//    print("pop ${_currentState.start.chapter.title}");
    _states.removeLast();
  }
}

class _ParseState {
  // TODO: Types?
  final SnippetTag start;
  final SnippetTag end;

  _ParseState(this.start, [this.end]);

  String toString() {
    if (end != null) return "state $start -> $end";
    return "state $start";
  }
}
