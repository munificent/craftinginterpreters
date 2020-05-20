import 'page.dart';

// TODO: Rename to "CodeTag" or just "Tag"?
class SnippetTag with Ordering<SnippetTag> implements Comparable<SnippetTag> {
  final ChapterPage chapter;
  final String name;
  final int index;

  factory SnippetTag(ChapterPage chapter, String name, int index) {
    // Hackish. Always want "not-yet" to be the last tag even if it appears
    // before a real tag. That ensures we can push it for other tags that have
    // been named.
    if (name == "not-yet") index = 9999;

    return SnippetTag._(chapter, name, index);
  }

  SnippetTag._(this.chapter, this.name, this.index);

  int compareTo(SnippetTag other) {
    if (chapter.chapterIndex != other.chapter.chapterIndex) {
      return chapter.chapterIndex.compareTo(other.chapter.chapterIndex);
    }

    return index.compareTo(other.index);
  }

  String toString() => "Tag(${chapter.chapterIndex}|$index: $chapter $name)";
}

/// Implements the comparison operators in terms of [compareTo()].
mixin Ordering<T> implements Comparable<T> {
  bool operator <(T other) => compareTo(other) < 0;
  bool operator <=(T other) => compareTo(other) <= 0;
  bool operator >(T other) => compareTo(other) > 0;
  bool operator >=(T other) => compareTo(other) >= 0;
}
