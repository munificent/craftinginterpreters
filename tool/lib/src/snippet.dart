import 'location.dart';
import 'source_code.dart';

/// A snippet of source code that is inserted in the book.
class Snippet {
  final SourceFile file;
  final String name;

  // TODO: Make final once not being set externally.
  Location location;

  // TODO: Make final?
  Location precedingLocation;

//    # If the snippet replaces a line with the same line but with a trailing
//    # comma, this is that line (with the comma).
//    self.added_comma = None

  final List<String> contextBefore = [];
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

//    if self.removed and self.added:
//      result.append('replace {} line{}'.format(
//          len(self.removed), '' if len(self.removed) == 1 else 's'))
//    elif self.removed and not self.added:
//      result.append('remove {} line{}'.format(
//          len(self.removed), '' if len(self.removed) == 1 else 's'))
//
//    if self.added_comma:
//      result.append('add <em>&ldquo;,&rdquo;</em> to previous line')
    return result;
  }

  String toString() => "${file.nicePath} $name";

//  def dump(self):
//    print(self.name)
//    print("prev: {}".format(self.preceding_location))
//    print("here: {}".format(self.location))
//    for line in self.context_before:
//      print("    {}".format(line))
//    for line in self.removed:
//      print("  - {}".format(line))
//    for line in self.added:
//      print("  + {}".format(line))
//    for line in self.context_after:
//      print("    {}".format(line))
//
}
