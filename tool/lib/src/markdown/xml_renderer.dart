import 'package:markdown/markdown.dart';

class XmlRenderer implements NodeVisitor {
  static final _imagePathPattern = RegExp(r'"([^"]+.png)"');

  final List<Paragraph> _paragraphs = [];

  final List<Inline> _inlineStack = [];

  _ChapterSection _section = _ChapterSection.main;

  /// What kind of text we're in: "p", "aside", "ordered", etc.
  String _region;

  String render(List<Node> nodes) {
    for (final node in nodes) {
      node.accept(this);
    }

    var buffer = StringBuffer();
    buffer.writeln("<chapter>");

    var firstParagraph = true;
    var firstAside = true;

    for (var paragraph in _paragraphs) {
      var isFirst = true;
      switch (paragraph.tag) {
        case "p":
          isFirst = firstParagraph;
          break;
        case "aside":
          isFirst = firstAside;
          break;
      }

      paragraph.prettyPrint(buffer, isFirst);

      switch (paragraph.tag) {
        case "p":
          firstParagraph = false;
          firstAside = true;
          break;

        case "aside":
          firstAside = false;
          break;

        case "aside-image":
        case "aside-pre":
          firstAside = true;
          break;

        case "":
        case "heading":
        case "heading-challenges":
        case "heading-design":
        case "image":
        case "list":
        case "list-pre":
        case "ordered":
        case "pre":
        case "quote":
        case "subheading":
        case "unordered":
          firstParagraph = true;
          firstAside = true;
          break;

        default:
          print("unhandled tag ${paragraph.tag} in calculating next");
          break;
      }
    }

    buffer.writeln("</chapter>");
    return buffer.toString();
  }

  void visitText(Text node) {
    var text = node.text;

    text = text
        .replaceAll("&eacute;", "&#233;")
        .replaceAll("&ensp;", "&#8194;")
        .replaceAll("&ldquo;", "&#8220;")
        .replaceAll("&nbsp;", "&#160;")
        .replaceAll("&rdquo;", "&#8221;")
        .replaceAll("&rsquo;", "&#8217;")
        .replaceAll("&rarr;", "&#8594;")
        .replaceAll("&sect;", "&#167;")
        .replaceAll("&thinsp;", "&#8201;")
        .replaceAll("&times;", "&#215;")
        .replaceAll("<br>", "<br/>");

    // Since aside tags appear in the Markdown as literal HTML, they are parsed
    // as text, not Markdown elements.
    if (text.startsWith("<aside")) {
      _startRegion("aside");
    } else if (text.startsWith("</aside>")) {
      _endRegion();
    } else if (text.startsWith("<img")) {
      var imagePath = _imagePathPattern.firstMatch(text)[1];

      // Put main column images in their own paragraph.
      if (_region == null) {
        _paragraphs.add(Paragraph("image"));
      } else if (_region == "aside") {
        _paragraphs.add(Paragraph("aside-image"));
      } else if (_region == "unordered") {
        // Inline three-color GC icons. Do nothing to leave image inline.
      } else {
        print("Image in unexpected region $_region.");
      }

      _paragraphs.last.addText(imagePath);
    } else if (text.startsWith("<div") || text.startsWith("</div>")) {
      // Discard the challenge and design note divs.
    } else if (text.startsWith("<cite>") ||
        text.startsWith("<location-file>")) {
      // Include block-level XML content as-is.
      _endRegion();
      _paragraphs.add(Paragraph.xml(text));
    } else if (_inlineStack.isNotEmpty) {
      _inlineStack.last.text += text;
    } else {
      // If we've changed regions, start a new paragraph.
      if (_region != null && _paragraphs.last.tag != _region) {
        _paragraphs.add(Paragraph(_region));
      }
      _paragraphs.last.addText(text);
    }
  }

  bool visitElementBefore(Element element) {
    // TODO: Handle <blockquote> tags inside asides.

    var tag = element.tag;
    switch (tag) {
      case "p":
        _startParagraph();
        break;

      case "blockquote":
        _startRegion("quote");
        break;

      case "h2":
        var text = element.textContent;
        if (text == "Challenges") {
          _section = _ChapterSection.challenges;
          _startRegion("heading-challenges");
        } else if (text.contains("Design Note")) {
          _section = _ChapterSection.designNote;
          _startRegion("heading-design");
        } else {
          _startRegion("heading");
        }
        break;

      case "h3":
        _startRegion("subheading");
        break;

      case "ol":
        _startRegion("ordered");
        break;

      case "pre":
        if (_region == "aside") {
          _startRegion("aside-pre");
        } else if (_region == "ordered" ||
            _region == "unordered" ||
            _region == "list") {
          _startRegion("list-pre");
        } else if (_region != null) {
          print("pre in $_region");
        } else {
          _startRegion("pre");
        }
        break;

      case "ul":
        _startRegion("unordered");
        break;

      case "li":
        // Discard. There will be <p> tags nested within.
        break;

      case "a":
        // TODO: What do we want to do with links? Highlight them somehow so
        // that I decide if the surrounding text needs tweaking?
        break;

      case "code":
      case "em":
      case "small":
      case "strong":
        // Inline tags.

        // If we're in an inline tags already, flatten them by emitting inline
        // segments for any text they have. Leave them on the stack so that
        // they get resumed when the nested inline tags end.
        var tagParts = [tag];
        for (var i = 0; i < _inlineStack.length; i++) {
          var inline = _inlineStack[i];
          if (inline.text.isNotEmpty) {
            _paragraphs.last.contents.add(inline);
            _inlineStack[i] = Inline.tag(inline.tag);
          }

          tagParts.add(inline.tag);
        }

        tagParts.sort();
        // Make a tag name that includes all nested tags. We'll define separate
        // styles for each combination.
        _inlineStack.add(Inline.tag(tagParts.join("-")));
        break;

      default:
        print("unexpected open tag $tag");
    }

    return !element.isEmpty;
  }

  void visitElementAfter(Element element) {
    var tag = element.tag;
    switch (tag) {
      case "blockquote":
      case "h2":
      case "h3":
      case "ol":
      case "ul":
        _endRegion();
        break;

      case "pre":
        switch (_region) {
          case "aside-pre":
            _startRegion("aside");
            break;
          case "list-pre":
            _startRegion("list");
            break;
          default:
            _endRegion();
        }
        break;

      case "a":
      case "li":
      case "p":
        // Nothing to do.
        break;

      case "code":
      case "em":
      case "small":
      case "strong":
        // Inline tags.
        var inline = _inlineStack.removeLast();
        _paragraphs.last.contents.add(inline);
        break;

      default:
        print("unexpected close tag $tag");
    }
  }

  void _startRegion(String region) {
    _region = region;
  }

  void _endRegion() {
    _region = null;
  }

  void _startParagraph() {
    _paragraphs.add(Paragraph(_region ?? "p"));
  }
}

enum _ChapterSection {
  main,
  challenges,
  designNote,
}

/// A paragraph-level tag that contains text and inline tags.
class Paragraph {
  /// The tag for this paragraph of the empty string if it contains literal XML.
  final String tag;

  final List<Inline> contents = [];

  /// Instantiates a [tag] Element with [children].
  Paragraph(this.tag);

  Paragraph.xml(String text) : tag = "" {
    contents.add(Inline.text(text));
  }

  void addText(String text) {
    // Discard any leading whitespace at the beginning of list items.
    if (tag == "ordered" || tag == "unordered" && contents.isEmpty) {
      text = text.trimLeft();
    }

    contents.add(Inline.text(text));
  }

  void prettyPrint(StringBuffer buffer, bool firstParagraph) {
    if (contents.isEmpty) return;

    if (tag == "") {
      // Literal XML like from code snippet.
      for (var inline in contents) {
        inline.prettyPrint(buffer, tag);
      }
      buffer.writeln();
      return;
    }

    var fullTag = tag;
    if (!firstParagraph && fullTag != "aside-image") fullTag += "-next";

    buffer.write("<$fullTag>");
    for (var inline in contents) {
      inline.prettyPrint(buffer, tag);
    }
    buffer.write("</$fullTag>");
    buffer.writeln();
  }
}

/// An inline tag or plain text.
class Inline {
  /// The paragraph types that use normal prose text so don't need their inline
  /// tags prefixed.
  static const _proseParagraphs = {"p", "list", "ordered", "unordered"};

  /// The tag name if this is an inline tag or the empty string if it is text.
  final String tag;

  String text;

  Inline.tag(this.tag) : text = "";

  Inline.text(this.text) : tag = "";

  bool get isText => tag == "";

  void prettyPrint(StringBuffer buffer, String paragraph) {
    if (tag == "") {
      buffer.write(text);
      return;
    }

    var fullTag = tag;
    if (!_proseParagraphs.contains(paragraph)) fullTag = "$paragraph-$tag";
    buffer.write("<$fullTag>$text</$fullTag>");
  }
}
