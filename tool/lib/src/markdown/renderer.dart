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

  String _block;

  /// Since asides are pulled out of the main text flow, we want a <p>
  /// immediately after </aside> to become <p-next> if there was a <p> before
  /// the aside. This tracks that.
  bool _wasParagraphBeforeAside = false;
  bool _isPending = false;

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
      _wasParagraphBeforeAside = _block == "p" || _block == "p-next";
      _endBlock();
      _buffer.write(text);
      _block = "aside";
      _isPending = false;
    } else if (text.startsWith("</aside>")) {
      _endBlock();
      if (_wasParagraphBeforeAside) {
        _block = "p-next";
        _isPending = true;
      }
    } else if (text.startsWith("<location")) {
      _endBlock();
      _buffer.write(text);
    } else if (text.startsWith("<cite>")) {
      // End the <quote> block.
      _endBlock();
      _buffer.writeln(text);
    } else if (text.startsWith("<div") || text.startsWith("</div>")) {
      // Don't wrap the challenge and design note divs in paragraphs.
      _buffer.writeln(text);
    } else if (text.startsWith("<img")) {
      var match = _imagePathPattern.firstMatch(text);
      if (match != null) {
        _buffer.write("<image>${match[1]}</image>");
      } else {
        print("Could not parse img tag:\n$text");
        _buffer.write("<image>$text</image>");
      }
    } else {
      _flushPending();
      _buffer.write(text);
    }
  }

  bool visitElementBefore(Element element) {
    // TODO: Handle <pre> and <blockquote> tags inside asides.

    var tag = element.tag;
    switch (tag) {
      case "p":
        if (_block == null) _startBlock("p");
        return true;

      case "blockquote":
        _startBlock("quote");
        return true;

      case "pre":
        var wasAside = _block == "aside" || _block == "aside-next";
        _endBlock();
        if (wasAside) {
          tag = "aside-$tag";
          _block = "aside-pre";
        }
        break;

      case "h2":
      case "h3":
      case "li":
      case "ol":
      case "ul":
        // Block-level tags.
        _endBlock();
        break;

      case "a":
      case "code":
      case "em":
      case "small":
      case "strong":
        // Inline tags.
        if (_block == "aside" || _block == "aside-next") {
          tag = "aside-$tag";
        }
        break;

      default:
        print("unexpected open tag $tag");
        return true;
    }

    _buffer.write('<$tag');

    for (var entry in element.attributes.entries) {
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
        _endBlock();
        return;
      case "p":
        if (_block != null) {
          var nextBlock = _block;
          if (!nextBlock.endsWith("-next")) nextBlock += "-next";
          _endBlock();
          _startBlock(nextBlock);
        }
        return;

      case "h2":
      case "h3":
      case "li":
      case "ol":
      case "ul":
        // Block-level tags that don't contain newlines.
        _buffer.writeln('</$tag>');
      break;

      case "pre":
        if (_block == "aside-pre") {
          _endBlock();
        } else {
          _buffer.write('</$tag>');
        }
        break;

      case "a":
      case "code":
      case "em":
      case "small":
      case "strong":
        // Inline tags.
        if (_block == "aside" || _block == "aside-next") {
          tag = "aside-$tag";
        }
        _buffer.write('</$tag>');
        break;

      default:
        throw "unexpected close tag $tag";
    }
  }

  void _startBlock(String tag) {
    // Should not already be in a block.
    assert(_block == null);

    _block = tag;
    _isPending = true;
  }

  void _endBlock() {
    // May not be in a block if we exited it early (for example because of a
    // <cite> in a <blockquote>.
    if (_block == null) return;

    if (!_isPending) _buffer.writeln("</$_block>");
    _block = null;
  }

  void _flushPending() {
    if (_block == null || !_isPending) return;

    _buffer.write('<$_block>');
    _isPending = false;
  }
}
