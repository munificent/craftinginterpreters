import 'book.dart';
import 'code_tag.dart';
import 'location.dart';
import 'text.dart';

/// A snippet of source code that is inserted in the book.
class Snippet {
  final SourceFile file;
  final CodeTag tag;

  Location get location => _location;
  Location _location;

  int _firstLine;
  int _lastLine;

  Location get precedingLocation => _ensureContext()._precedingLocation;
  Location _precedingLocation;

  /// If the snippet replaces a line with the same line but with a trailing
  /// comma, this is that line (with the comma).
  String get addedComma => _ensureContext()._addedComma;
  String _addedComma;

  final List<String> added = [];
  final List<String> removed = [];

  bool _hasContext = false;

  final List<String> _contextBefore = [];
  List<String> get contextBefore => _ensureContext()._contextBefore;

  final List<String> _contextAfter = [];
  List<String> get contextAfter => _ensureContext()._contextAfter;

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

    if (contextBefore.isEmpty && contextAfter.isEmpty) {
      // No lines around the snippet, it must be a new file.
      result.add("create new file");
    } else if (contextBefore.isEmpty) {
      // No lines before the snippet, it must be at the beginning.
      result.add("add to top of file");
    } else {
      var html = location.toHtml(precedingLocation, removed);
      if (html != null) result.add(html);
    }

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

  /// Lazily calculate the surrounding context information for this snippet.
  Snippet _ensureContext() {
    if (_hasContext) return this;

    // TODO: Should only need to grab as many preceding lines as the tag
    // requests and until we find a preceding location, but changing the `<= 5`
    // causes some locations to become wrong. Figure out why.

    // Get the preceding lines.
    for (var i = _firstLine - 1; i >= 0 && _contextBefore.length <= 5; i--) {
      var line = file.lines[i];
      if (line.isPresent(tag)) {
        _contextBefore.insert(0, line.text);

        // Store the most precise preceding location we find.
        if (_precedingLocation == null ||
            line.location.depth > _precedingLocation.depth) {
          _precedingLocation = line.location;
        }
      }
    }

    // Get the following lines.
    for (var i = _lastLine + 1;
        i < file.lines.length && _contextAfter.length < tag.afterCount;
        i++) {
      var line = file.lines[i];
      if (line.isPresent(tag)) _contextAfter.add(line.text);
    }

    // Find line changes that just add a trailing comma.
    if (added.isNotEmpty &&
        removed.isNotEmpty &&
        added.first == "${removed.last},") {
      _addedComma = added.first;
      added.removeAt(0);
      removed.removeLast();
    }

    _hasContext = true;
    return this;
  }
}
