import 'package:markdown/markdown.dart';

/// Custom Markdown to HTML renderer with some tweaks for the output we want.
class HtmlRenderer implements NodeVisitor {
  static const _blockTags = {
    "blockquote",
    "div",
    "h1",
    "h2",
    "h3",
    "h4",
    "h5",
    "h6",
    "hr",
    "li",
    "ol",
    "p",
    "pre",
    "ul",
  };

  StringBuffer buffer;

  final _elementStack = <Element>[];
  String _lastVisitedTag;

  String render(List<Node> nodes) {
    buffer = StringBuffer();

    for (final node in nodes) {
      node.accept(this);
    }

    buffer.writeln();

    return buffer.toString();
  }

  void visitText(Text text) {
    var content = text.text;

    // Put a newline before inline HTML markup for block-level tags.
    if (content.startsWith("<aside") ||
        content.startsWith("</aside") ||
        content.startsWith("<div") ||
        content.startsWith("</div")) {
      buffer.writeln();
    }

    if (const ['p', 'li'].contains(_lastVisitedTag)) {
      content = content.trimLeft();
    }
    buffer.write(content);

    _lastVisitedTag = null;
  }

  bool visitElementBefore(Element element) {
    // Separate block-level elements with newlines.
    if (buffer.isNotEmpty && _blockTags.contains(element.tag)) {
      buffer.writeln();
    }

    buffer.write('<${element.tag}');

    for (var entry in element.attributes.entries) {
      buffer.write(' ${entry.key}="${entry.value}"');
    }

    _lastVisitedTag = element.tag;

    if (element.isEmpty) {
      // Empty element like <hr/>.
      buffer.write(' />');
      if (element.tag == 'br') buffer.write('\n');
      return false;
    } else {
      _elementStack.add(element);
      buffer.write('>');
      return true;
    }
  }

  void visitElementAfter(Element element) {
    assert(identical(_elementStack.last, element));

    if (element.children != null &&
        element.children.isNotEmpty &&
        _blockTags.contains(_lastVisitedTag) &&
        _blockTags.contains(element.tag)) {
      buffer.writeln();
    } else if (element.tag == 'blockquote') {
      buffer.writeln();
    }
    buffer.write('</${element.tag}>');

    _lastVisitedTag = _elementStack.removeLast().tag;
  }
}
