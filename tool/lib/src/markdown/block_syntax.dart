import 'package:markdown/markdown.dart';

import '../page.dart';

/// Parses atx-style headers like `## Header` and gives them the book's special
/// handling:
///
/// - Generates anchor links.
/// - Includes the section numbers.
class BookHeaderSyntax extends BlockSyntax {
  /// Leading `#` define atx-style headers.
  static final _headerPattern = RegExp(r'^(#{1,6}) (.*)$');

  final Page _page;
  final bool _isXml;

  RegExp get pattern => _headerPattern;

  BookHeaderSyntax(this._page, {bool xml = false}) : _isXml = xml;

  Node parse(BlockParser parser) {
    var header = _page.headers[parser.current];
    parser.advance();

    if (_isXml) {
      return Element("h${header.level}", [UnparsedContent(header.name)]);
    }

    var number = "";
    if (!header.isSpecial) {
      number = "${_page.numberString}&#8202;.&#8202;${header.headerIndex}";
      if (header.subheaderIndex != null) {
        number += "&#8202;.&#8202;${header.subheaderIndex}";
      }
    }

    var link = Element("a", [
      if (!header.isSpecial) Element("small", [Text(number)]),
      UnparsedContent(header.name)
    ]);
    link.attributes["href"] = "#${header.anchor}";
    link.attributes["name"] = header.anchor;

    return Element("h${header.level}", [link]);
  }
}
