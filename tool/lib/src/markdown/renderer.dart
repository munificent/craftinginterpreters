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

class XmlRenderer implements NodeVisitor {
  static final _imagePathPattern = RegExp(r'"([^"]+.png)"');

  final StringBuffer _buffer = StringBuffer();

  _ChapterSection _section = _ChapterSection.main;

  final List<String> _tags = [];

  bool _isPending = false;

  String _paragraphTag;
  String _lastParagraphTag;
  String _lastSidebarTag;
  bool _isNextParagraph = false;

  String render(List<Node> nodes) {
    for (final node in nodes) {
      node.accept(this);
    }

    _buffer.writeln();
    return _buffer.toString();
  }

  void visitText(Text node) {
    var text = node.text;

    // Since aside tags appear in the Markdown as literal HTML, they are parsed
    // as text, not Markdown elements.
    if (text.startsWith("<aside")) {
      _pushTag("aside");
    } else if (text.startsWith("</aside>")) {
      _popTag();
    } else if (text.startsWith("<cite>")) {
      // End the <quote> block and push placeholder <cite> tag. This way when
      // the blockquote is popped, there is a tag to pop.
      _switchTag("cite");
      _buffer.writeln(text);
    } else if (text.startsWith("<div") || text.startsWith("</div>")) {
      // Discard the challenge and design note divs.
    } else if (text.startsWith("<img")) {
      // Reset the paragraph are a main column image so that we don't indent
      // after an image.
      if (!_isInContext("aside")) _resetParagraph();

      var match = _imagePathPattern.firstMatch(text);
      _buffer.writeln("<image>${match[1]}</image>");
    } else {
      if (_flushParagraph()) {
        // Just started a block level tag, so discard any leading whitespace.
        _buffer.write(text.trimLeft());
      } else {
        _buffer.write(text);
      }
    }
  }

  bool visitElementBefore(Element element) {
    // TODO: Handle <blockquote> tags inside asides.

    var tag = element.tag;
    switch (tag) {
      case "p":
        _startParagraph();
        return true;

      case "blockquote":
        _pushTag("quote");
        return true;

      case "pre":
        tag = _preTag(tag);
        _resetParagraph();
        break;

      case "h2":
        _resetParagraph();
        var text = element.textContent;
        if (text == "Challenges") {
          _buffer.writeln('<h2-challenge>Challenges</h2-challenge>');
          _section = _ChapterSection.challenges;
          return false;
        } else if (text.contains("Design Note")) {
          _buffer.writeln('<h2-design>$text</h2-design>');
          _section = _ChapterSection.designNote;
          return false;
        }
        break;

      case "h3":
        _resetParagraph();
        break;

      case "li":
        // Start a new paragraph so that we don't use `-next` for the
        // first paragraph in the next list item.
        _resetParagraph();
        return true;

      case "ol":
        _pushTag("ordered");
        return true;

      case "ul":
        _pushTag("bullet");
        return true;

      case "a":
      case "code":
      case "em":
      case "small":
      case "strong":
        // Inline tags.
        tag = _inlineTag(tag);
        break;

      default:
        print("unexpected open tag $tag");
        return true;
    }

    _buffer.write('<$tag');

    for (var entry in element.attributes.entries) {
      // Skip links because InDesign tries to follow them.
      // TODO: How should I handle <a> tags?
      if (entry.key == "href") continue;
      _buffer.write(' ${entry.key}="${entry.value}"');
    }

    if (element.isEmpty) {
      _buffer.write(' />');
    } else {
      _buffer.write('>');
    }
    return !element.isEmpty;
  }

  void visitElementAfter(Element element) {
    var tag = element.tag;

    switch (tag) {
      case "blockquote":
      case "ol":
      case "ul":
        _popTag();
        return;

      case "p":
        _endParagraph();
        return;

      case "li":
        // Ignore.
        break;

      case "h2":
      case "h3":
        // Block-level tags that don't contain newlines.
        _buffer.writeln('</$tag>');
        break;

      case "pre":
        tag = _preTag(tag);
        _buffer.write('</$tag>');
        break;

      case "a":
      case "code":
      case "em":
      case "small":
      case "strong":
        // Inline tags.
        tag = _inlineTag(tag);
        _buffer.write('</$tag>');
        break;

      default:
        throw "unexpected close tag $tag";
    }
  }

  /// The XML [tag] to use for inline tags given the current context.
  String _inlineTag(String tag) {
    if (_isInContext("quote")) {
      return "quote-$tag";
    } else if (_isInContext("aside")) {
      return "aside-$tag";
    } else {
      return tag;
    }
  }

  /// The XML [tag] to use for <pre> tags given the current context.
  String _preTag(String tag) {
    // TODO: Different tag for pre in challenge?
    if (_isInContext("aside")) {
      return "aside-$tag";
    } else if (_isInContext("ordered") || _isInContext("bullet")) {
      return "list-$tag";
    } else {
      return tag;
    }
  }

  void _pushTag(String tag) {
    _tags.add(tag);
  }

  void _switchTag(String tag) {
    _popTag();
    _pushTag(tag);
  }

  void _popTag() {
    _tags.removeLast();
  }

  void _startParagraph() {
    if (_isInContext("quote")) {
      _paragraphTag = "quote";
    } else if (_isInContext("aside")) {
      _paragraphTag = "aside";
    } else if (_isInContext("ordered")) {
      if (_section == _ChapterSection.challenges) {
        _paragraphTag = "challenge";
      } else {
        _paragraphTag = "ordered";
      }
    } else if (_isInContext("bullet")) {
      _paragraphTag = "bullet";
    } else {
      _paragraphTag = "p";
    }

    if (_isSidebarTag(_paragraphTag)) {
      _isNextParagraph = _paragraphTag == _lastSidebarTag;
    } else {
      _isNextParagraph = _paragraphTag == _lastParagraphTag;

      // We are back into the main column, so the next sidebar will be new.
      _lastSidebarTag = null;
    }

    _isPending = true;
  }

  void _endParagraph() {
    if (!_isPending) {
      var tag = _paragraphTag;
      if (_isNextParagraph) tag += "-next";
      _buffer.writeln('</$tag>');
    }
    _isPending = false;

    if (_isSidebarTag(_paragraphTag)) {
      _lastSidebarTag = _paragraphTag;
    } else {
      _lastParagraphTag = _paragraphTag;
    }

    _paragraphTag = null;
  }

  bool _flushParagraph() {
    if (!_isPending) return false;

    var tag = _paragraphTag;
    if (_isNextParagraph) tag += "-next";
    _buffer.write('<$tag>');

    _isPending = false;
    return true;
  }

  void _resetParagraph() {
    if (_paragraphTag != null) _endParagraph();
    _lastParagraphTag = null;
    _lastSidebarTag = null;
  }

  /// Whether [tag] is a paragraph tag that appears in the sidebar (like an
  /// aside) or in the main column.
  bool _isSidebarTag(String tag) {
    return const {"aside"}.contains(tag);
  }

  bool _isInContext(String tag1, [String tag2]) {
    if (_tags.isEmpty) return false;
    if (_tags.last != tag1) return false;
    if (tag2 != null) {
      if (_tags.length < 2) return false;
      if (_tags[_tags.length - 2] != tag2) return false;
    }
    return true;
  }
}

enum _ChapterSection {
  main,
  challenges,
  designNote,
}
