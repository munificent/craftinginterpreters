import 'book.dart';
import 'code_tag.dart';
import 'location.dart';
import 'text.dart';

/// A snippet of source code that is inserted in the book.
class Snippet {
  final SourceFile file;
  final CodeTag tag;

  Location _location;

  int _firstLine;
  int _lastLine;

  Location get precedingLocation => _precedingLocation;
  Location _precedingLocation;

  /// If the snippet replaces a line with the same line but with a trailing
  /// comma, this is that line (with the comma).
  String get addedComma => _addedComma;
  String _addedComma;

  final List<String> added = [];
  final List<String> removed = [];

  final List<String> contextBefore = [];
  final List<String> contextAfter = [];

  Snippet(this.file, this.tag);

  void addLine(int lineIndex, SourceLine line) {
    if (added.isEmpty) {
      _location = line.location;
      _firstLine = lineIndex;
    }
    added.add(line.text);

    // Assume that we add the removed lines in order.
    _lastLine = lineIndex;
  }

  void removeLine(int lineIndex, SourceLine line) {
    removed.add(line.text);

    // Assume that we add the removed lines in order.
    _lastLine = lineIndex;
  }

  /// Describes where in the file this snippet appears. Returns a list of HTML
  /// strings.
  List<String> get locationDescription {
    var result = ["<em>${file.nicePath}</em>"];

    var html = _location.toHtml(precedingLocation, removed);
    if (html != null) result.add(html);

    if (removed.isNotEmpty && added.isNotEmpty) {
      result.add("replace ${removed.length} line${pluralize(removed)}");
    } else if (removed.isNotEmpty && added.isEmpty) {
      result.add("remove ${removed.length} line${pluralize(removed)}");
    }

    if (addedComma != null) {
      result.add("add <em>&ldquo;,&rdquo;</em> to previous line");
    }

    return result;
  }

  String toString() => "${file.nicePath} ${tag.name}";

  /// Calculate the surrounding context information for this snippet.
  void calculateContext() {
    // TODO: Should only need to grab as many preceding lines as the tag
    // requests and until we find a preceding location, but changing the `<= 5`
    // causes some locations to become wrong. Figure out why.

    // Get the preceding lines.
    for (var i = _firstLine - 1;
        i >= 0 && contextBefore.length < tag.beforeCount;
        i--) {
      var line = file.lines[i];
      if (!line.isPresent(tag)) continue;
      contextBefore.insert(0, line.text);
    }

    // Get the following lines.
    for (var i = _lastLine + 1;
        i < file.lines.length && contextAfter.length < tag.afterCount;
        i++) {
      var line = file.lines[i];
      if (line.isPresent(tag)) contextAfter.add(line.text);
    }

    // Get the preceding location.
    // TODO: This constant is somewhat arbitrary. Come up with a more precise
    // way to track the preceding location.
    int checkedLines = 0;
    for (var i = _firstLine - 1; i >= 0 && checkedLines <= 4; i--) {
      var line = file.lines[i];
      if (!line.isPresent(tag)) continue;
      checkedLines++;

      // Store the most precise preceding location we find.
      if (_precedingLocation == null ||
          line.location.depth > _precedingLocation.depth) {
        _precedingLocation = line.location;
      }
    }

    // Update the current location based on surrounding lines.
    var hasCodeBefore = contextBefore.isNotEmpty;
    var hasCodeAfter = contextAfter.isNotEmpty;
    for (var i = _firstLine - 1; !hasCodeBefore && i >= 0; i--) {
      hasCodeBefore = file.lines[i].isPresent(tag);
    }

    for (var i = _lastLine + 1; !hasCodeAfter && i < file.lines.length; i++) {
      hasCodeAfter = file.lines[i].isPresent(tag);
    }

    if (!hasCodeBefore) {
      _location = Location(null, hasCodeAfter ? "top" : "new", null);
    }

    // Find line changes that just add a trailing comma.
    if (added.isNotEmpty &&
        removed.isNotEmpty &&
        added.first == "${removed.last},") {
      _addedComma = added.first;
      added.removeAt(0);
      removed.removeLast();
    }
  }
}
