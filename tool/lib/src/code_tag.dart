import 'page.dart';

class CodeTag with Ordering<CodeTag> implements Comparable<CodeTag> {
  final Page chapter;
  final String name;

  /// The zero-based index of the tag in the order that it appears on the page.
  final int _index;

  /// Number of preceding lines of context to show.
  final int beforeCount;

  /// Number of trailing lines of context to show.
  final int afterCount;

  /// Whether to show location information.
  final bool showLocation;

  factory CodeTag(Page chapter, String name, int index, int beforeCount,
      int afterCount, bool showLocation) {
    // Hackish. Always want "not-yet" to be the last tag even if it appears
    // before a real tag. That ensures we can push it for other tags that have
    // been named.
    if (name == "not-yet") index = 9999;

    return CodeTag._(
        chapter, name, index, beforeCount, afterCount, showLocation);
  }

  CodeTag._(this.chapter, this.name, this._index, this.beforeCount,
      this.afterCount, this.showLocation);

  /// Gets the name of the directory used for this tag when the code is split
  /// at this tag's snippet.
  String get directory {
    var index = _index.toString().padLeft(2, "0");
    return "$index-$name";
  }

  int compareTo(CodeTag other) {
    if (chapter.ordinal != other.chapter.ordinal) {
      return chapter.ordinal.compareTo(other.chapter.ordinal);
    }

    return _index.compareTo(other._index);
  }

  String toString() => "Tag(${chapter.ordinal}|$_index: $chapter $name)";
}

/// Implements the comparison operators in terms of [compareTo()].
mixin Ordering<T> implements Comparable<T> {
  bool operator <(T other) => compareTo(other) < 0;
  bool operator <=(T other) => compareTo(other) <= 0;
  bool operator >(T other) => compareTo(other) > 0;
  bool operator >=(T other) => compareTo(other) >= 0;
}
