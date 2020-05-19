import 'page.dart';

// TODO: Move to separate file? Rename to "CodeTag"?
class SnippetTag implements Comparable<SnippetTag> {
  final Page chapter;
  final String name;
  final int index;

  factory SnippetTag(Page chapter, String name, int index) {
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
