import 'location.dart';
import 'source_code.dart';
import 'text.dart';

/// A snippet of source code that is inserted in the book.
class Snippet {
  final SourceFile file;
  final String name;

  // TODO: Make final once not being set externally.
  Location location;

  // TODO: Make final?
  Location precedingLocation;

  final List<String> contextBefore = [];

  /// If the snippet replaces a line with the same line but with a trailing
  /// comma, this is that line (with the comma).
  String addedComma;

  final List<String> added = [];
  final List<String> removed = [];
  final List<String> contextAfter = [];

  Snippet(this.file, this.name);

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

  String toString() => "${file.nicePath} $name";
}
