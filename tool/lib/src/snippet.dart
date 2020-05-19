import 'source_code.dart';

/// A snippet of source code that is inserted in the book.
class Snippet {
  final SourceFile file;
  final String name;

  // TODO: Make final once not being set externally.
  Location location;

  final List<String> contextBefore = [];
  final List<String> added = [];
  final List<String> removed = [];
  final List<String> contextAfter = [];

  Snippet(this.file, this.name);
//    # If the snippet replaces a line with the same line but with a trailing
//    # comma, this is that line (with the comma).
//    self.added_comma = None
//
//    self.location = None
//    self.preceding_location = None
//
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
//  def describe_location(self):
//    """
//    Describes where in the file this snippet appears. Returns a list of HTML
//    strings.
//    """
//    result = ['<em>{}</em>'.format(self.file.nice_path())]
//
//    if len(self.context_before) == 0 and len(self.context_after) == 0:
//      # No lines around the snippet, it must be a new file.
//      result.append('create new file')
//    elif len(self.context_before) == 0:
//      # No lines before the snippet, it must be at the beginning.
//      result.append('add to top of file')
//    else:
//      location = self.location.to_html(self.preceding_location, self.removed)
//      if location:
//        result.append(location)
//
//    if self.removed and self.added:
//      result.append('replace {} line{}'.format(
//          len(self.removed), '' if len(self.removed) == 1 else 's'))
//    elif self.removed and not self.added:
//      result.append('remove {} line{}'.format(
//          len(self.removed), '' if len(self.removed) == 1 else 's'))
//
//    if self.added_comma:
//      result.append('add <em>&ldquo;,&rdquo;</em> to previous line')
//
//    return result
}
