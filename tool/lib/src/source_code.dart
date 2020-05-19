import 'package:glob/glob.dart';
import 'package:path/path.dart' as p;

import 'book.dart';
import 'snippet.dart';
import 'source_file_parser.dart';

/// All of the source files in the book.
class SourceCode {
  final List<SourceFile> files = [];

  // TODO: Not needed?
//  final Map<Page, Map<String, SnippetTag>> snippetTags = getChapterSnippetTags();

  SourceCode() {
//    # The errors that occurred when parsing the source code, mapped to the
//    # chapter where the error should be displayed.
//    self.errors = {}
//    for chapter in book.CODE_CHAPTERS:
//      self.errors[chapter] = []
  }

  void load() {
    for (var language in ["java", "c"]) {
      for (var file in Glob("$language/**.{c,h,java}").listSync()) {
        var shortPath = p.relative(file.path, from: language);
        files.add(SourceFileParser(file.path, shortPath).parse());
      }
    }

//  return source_code
  }

  // TODO: Move into Page.
  /// Gets the list of snippets that occur in [chapter].
  Map<String, Snippet> findAll(Page chapter) {
    // TODO: Type.
    var snippets = <String, Snippet>{};
//
//    first_lines = {}
//    last_lines = {}
//
    // Create a new snippet for [name] if it doesn't already exist.
    Snippet ensureSnippet(SourceFile file, String name, [int lineNum]) {
      if (!snippets.containsKey(name)) {
        var snippet = Snippet(file, name);
        snippets[name] = snippet;
//        first_lines[snippet] = lineNum
        return snippet;
      }
//
      var snippet = snippets[name];
//      if first_lines[snippet] is None:
//        first_lines[snippet] = lineNum
//      if name != 'not-yet' and name != 'omit' and snippet.file.path != file.path:
//        print('Error: "{} {}" appears in two files, {} and {}.'.format(
//                chapter, name, snippet.file.path, file.path),
//            file=sys.stderr)
//
      return snippet;
    }

    // TODO: If this is slow, could organize directly by snippets in SourceCode.
    // Find the lines added and removed in each snippet.
    for (var file in files) {
      var lineNum = 0;
      for (var line in file.lines) {
        if (line.start.chapter == chapter) {
          var snippet = ensureSnippet(file, line.start.name, lineNum);
          // TODO: Move most of this logic into snippet.
          snippet.added.add(line.text);
//          last_lines[snippet] = lineNum
//
          if (snippet.added.length == 1) {
            snippet.location = line.location;
          }
        }

        if (line.end != null && line.end.chapter == chapter) {
          var snippet = ensureSnippet(file, line.end.name);
          snippet.removed.add(line.text);
        }

        lineNum++;
      }

//    # Find the surrounding context lines and location for each snippet.
//    for name, snippet in snippets.items():
//      current_snippet = self.snippet_tags[chapter][name]
//
//      # Look for preceding lines.
//      i = first_lines[snippet] - 1
//      before = []
//      while i >= 0 and len(before) <= 5:
//        line = snippet.file.lines[i]
//        if line.is_present(current_snippet):
//          before.append(line.text)
//
//          # Store the more precise preceding location we find.
//          if (not snippet.preceding_location or
//              line.location.depth() > snippet.preceding_location.depth()):
//            snippet.preceding_location = line.location
//
//        i -= 1
//      snippet.context_before = before[::-1]
//
//      # Look for following lines.
//      i = last_lines[snippet] + 1
//      after = []
//      while i < len(snippet.file.lines) and len(after) <= 5:
//        line = snippet.file.lines[i]
//        if line.is_present(current_snippet):
//          after.append(line.text)
//        i += 1
//      snippet.context_after = after
//
    }

//    # Find line changes that just add a trailing comma.
//    for name, snippet in snippets.items():
//      if (len(snippet.added) > 0 and
//          len(snippet.removed) > 0 and
//          snippet.added[0] == snippet.removed[-1] + ","):
//        snippet.added_comma = snippet.added[0]
//        del snippet.added[0]
//        del snippet.removed[-1]
//
    return snippets;
  }

//  def split_chapter(self, file, chapter, name):
//    """
//    Gets the code for [file] as it appears at snippet [name] of [chapter].
//    """
//    tag = self.snippet_tags[chapter][name]
//
//    source_file = None
//    for source in self.files:
//      if source.path == file:
//        source_file = source
//        break
//
//    if source_file == None:
//      raise Exception('Could not find file "{}"'.format(file))
//
//    output = ""
//    for line in source_file.lines:
//      if line.is_present(tag):
//        # Hack. In generate_ast.java, we split up a parameter list among
//        # multiple chapters, which leads to hanging commas in some cases.
//        # Remove them.
//        if line.text.strip().startswith(")") and output.endswith(",\n"):
//          output = output[:-2] + "\n"
//
//        output += line.text + "\n"
//    return output
}

/// A single source file whose code is included in the book.
class SourceFile {
  final String path;
  final List<SourceLine> lines = [];

  SourceFile(this.path);

  String get language => path.endsWith("java") ? "java" : "c";

  String get nicePath => path.replaceAll("com/craftinginterpreters/", "");
}

/// A line of code in a [SourceFile] and the metadata for it.
class SourceLine {
  final String text;
  final Location location;

  /// The first snippet where this line appears in the book.
  final SnippetTag start;

  /// The last snippet where this line is removed, or null if the line reaches
  /// the end of the book.
  final SnippetTag end;

  SourceLine(this.text, this.location, this.start, this.end);

//  def is_present(self, snippet):
//    """
//    Returns true if this line exists by the time we reach [snippet].
//    """
//    if snippet < self.start:
//      # We haven't reached this line's snippet yet.
//      return False
//
//    if self.end and snippet > self.end:
//      # We are past the snippet where it is removed.
//      return False
//
//    return True
//
//  def __str__(self):
//    result = "{:72} // {}".format(self.text, self.start)
//
//    if self.end:
//      result += " < {}".format(self.end)
//
//    return result + " in {}".format(self.location)
}

/// The context in which a line of code appears. The chain of types and
/// functions it's in.
class Location {
  final Location parent;
  final String kind;
  final String name;
  final String signature;

  Location(this.parent, this.kind, this.name, [this.signature]);

//  def __str__(self):
//    result = self.kind + ' ' + self.name
//    if self.signature:
//      result += "(" + self.signature + ")"
//    if self.parent:
//      result = str(self.parent) + ' > ' + result
//    return result
//
//  def __eq__(self, other):
//    # Note: Signature is deliberately not considered part of equality. There's
//    # a case in calls-and-functions where the signature of a function changes
//    # and it confuses the build script if we treat the signatures as
//    # significant.
//    return other != None and self.kind == other.kind and self.name == other.name
//
//  @property
//  def is_file(self):
//    return self.kind == 'file'
//
//  @property
//  def is_function(self):
//    return self.kind in ['constructor', 'function', 'method']
//
//  def to_html(self, preceding, removed):
//    """
//    Generates a string of HTML that describes a snippet at this location, when
//    following the [preceding] location.
//    """
//
//    # Note: The order of these is highly significant.
//    if self.kind == 'class' and self.parent and self.parent.kind == 'class':
//      return 'nest inside class <em>{}</em>'.format(self.parent.name)
//
//    if self.is_function and preceding == self:
//      # Hack. There's one place where we add a new overload and that shouldn't
//      # be treated as in the same function. But we can't always look at the
//      # signature because there's another place where a change signature would
//      # confuse the build script. So just check for the one-off case here.
//      if self.name == 'resolve' and self.signature == 'Expr expr':
//        return 'add after <em>{}</em>({})'.format(
//            preceding.name, preceding.signature)
//
//      # We're still inside a function.
//      return 'in <em>{}</em>()'.format(self.name)
//
//    if self.is_function and removed:
//      # Hack. We don't appear to be in the middle of a function, but we are
//      # replacing lines, so assume we're replacing the entire function.
//      return '{} <em>{}</em>()'.format(self.kind, self.name)
//
//    if self.parent == preceding and not preceding.is_file:
//      # We're nested inside a type.
//      return 'in {} <em>{}</em>'.format(preceding.kind, preceding.name)
//
//    if preceding == self and not self.is_file:
//      # We're still inside a type.
//      return 'in {} <em>{}</em>'.format(self.kind, self.name)
//
//    if preceding.is_function:
//      # We aren't inside a function, but we do know the preceding one.
//      return 'add after <em>{}</em>()'.format(preceding.name)
//
//    if not preceding.is_file:
//      # We aren't inside any function, but we do know what we follow.
//      return 'add after {} <em>{}</em>'.format(preceding.kind, preceding.name)
//
//    return None
//
//  def depth(self):
//    current = self
//    result = 0
//    while current:
//      result += 1
//      current = current.parent
//
//    return result
//
//  def pop_to_depth(self, depth):
//    """
//    Discard as many children as needed to get to [depth] parents.
//    """
//    current = self
//    locations = []
//    while current:
//      locations.append(current)
//      current = current.parent
//
//    # If we are already shallower, there is nothing to pop.
//    if len(locations) < depth + 1: return self
//
//    return locations[-depth - 1]
}
