import 'package:charcode/ascii.dart';
import 'package:string_scanner/string_scanner.dart';

/// Takes a string of source code and returns a block of HTML with spans for
/// syntax highlighting.
///
/// Wraps the result in a <pre> tag with the given [preClass].
String formatCode(String language, int length, List<String> lines,
    [String preClass]) {
  return Highlighter(language, length)._highlight(lines, preClass);
}

class Highlighter {
  final int _lineLength;
  final StringBuffer _buffer = StringBuffer();
  StringScanner _scanner;
  final Language _language;

  Highlighter(String language, this._lineLength)
      : _language =
            _languages[language] ?? (throw "Unknown language '$language'.");

  String _highlight(List<String> lines, [String preClass]) {
    _buffer.write("<pre");
    if (preClass != null) _buffer.write(' class="$preClass"');
    _buffer.write(">");

    // TODO: If we change build to not pass this output through the Markdown
    // parser, then revisit this.
    // Hack. Markdown seems to discard leading and trailing newlines, so we'll
    // add them back ourselves.
    var leadingNewlines = 0;
    while (leadingNewlines < lines.length &&
        lines[leadingNewlines].trim().isEmpty) {
      leadingNewlines++;
      _buffer.write("<br>");
    }

    var trailingNewlines = 0;
    while (trailingNewlines < lines.length - leadingNewlines &&
        lines[lines.length - trailingNewlines - 1].trim().isEmpty) {
      trailingNewlines++;
    }

    // TODO: Temp hack. Munge it some to match the old build output.
    _buffer.write("<span></span>");

    for (var i = leadingNewlines; i < lines.length - trailingNewlines; i++) {
      _scanLine(lines[i]);
    }

    for (var i = 0; i < trailingNewlines; i++) {
      _buffer.write("<br>");
    }

    _buffer.write("</pre>");
    return _buffer.toString();
  }

  void _scanLine(String line) {
    if (line.trim().isEmpty) {
      _buffer.writeln();
      return;
    }

    // TODO: Reuse scanner for all lines?
    _scanner = StringScanner(line.padRight(_lineLength, " "));

    while (!_scanner.isDone) {
      var found = false;
      for (var rule in _language.rules) {
        if (rule.apply(this)) {
          found = true;
          break;
        }
      }

      if (!found) _writeChar(_scanner.readChar());
    }

    _buffer.writeln();
  }

  void _token(String type, [String text]) {
    text ??= _scanner.lastMatch[0];
    _buffer.write('<span class="$type">');
    _write(text);
    _buffer.write('</span>');
  }

  void _write(String string) {
    for (var i = 0; i < string.length; i++) {
      _writeChar(string.codeUnitAt(i));
    }
  }

  void _writeChar(int char) {
    switch (char) {
      case $less_than:
        _buffer.write("&lt;");
        break;
      case $greater_than:
        _buffer.write("&gt;");
        break;
      case $single_quote:
        _buffer.write("&#39;");
        break;
      case $double_quote:
        _buffer.write("&quot;");
        break;
      case $ampersand:
        _buffer.write("&amp;");
        break;
      default:
        _buffer.writeCharCode(char);
    }
  }
}

/// Defines the syntax rules for a single programming language.
class Language {
  final Map<String, String> words = {};
  final List<Rule> rules;

  Language(
      {String keywords,
      String constants,
      String names,
      Map<String, String> other,
      List<Rule> rules})
      // TODO: Allow omitting rules for languages that aren't supported yet.
      : rules = rules ?? const [] {
    keywordType(String wordList, String type) {
      if (wordList == null) return;
      for (var word in wordList.split(" ")) {
        words[word] = type;
      }
    }

    keywordType(keywords, "k");
    keywordType(constants, "kc");
    keywordType(names, "nb");
    if (other != null) words.addAll(other);
  }
}

abstract class Rule {
  final RegExp pattern;

  factory Rule(String pattern, String tokenType) =>
      SimpleRule(pattern, tokenType);

  factory Rule.capture(String pattern, List<String> tokenTypes) =>
      CaptureRule(pattern, tokenTypes);

  Rule._(String pattern) : pattern = RegExp(pattern);

  bool apply(Highlighter highlighter) {
    if (!highlighter._scanner.scan(pattern)) return false;
    applyRule(highlighter);
    return true;
  }

  void applyRule(Highlighter highlighter);
}

/// Parses a single regex and outputs the entire matched text as a single token
/// with the given [tokenType].
class SimpleRule extends Rule {
  final String tokenType;

  SimpleRule(String pattern, this.tokenType) : super._(pattern);

  void applyRule(Highlighter highlighter) {
    highlighter._token(tokenType);
  }
}

/// Parses a single regex where each capture group has a corresponding token
/// type. If the type is `""` for some group, the matched string text is output
/// as plain text.
class CaptureRule extends Rule {
  final List<String> tokenTypes;

  CaptureRule(String pattern, this.tokenTypes) : super._(pattern);

  void applyRule(Highlighter highlighter) {
    var match = highlighter._scanner.lastMatch;
    for (var i = 0; i < tokenTypes.length; i++) {
      var type = tokenTypes[i];
      if (type.isNotEmpty) {
        highlighter._token(type, match[i + 1]);
      } else {
        highlighter._write(match[i + 1]);
      }
    }
  }
}

/// Parses string literals and the escape codes inside them.
class StringRule extends Rule {
  static final _escapePattern = RegExp(r"\\.");

  StringRule() : super._('"');

  void applyRule(Highlighter highlighter) {
    var scanner = highlighter._scanner;
    var start = scanner.position - 1;

    while (!scanner.isDone) {
      if (scanner.scan(_escapePattern)) {
        if (scanner.position > start) {
          // TODO: Something smarter than "-2" if we need multi-character
          // escapes.
          highlighter._token(
              "s", scanner.substring(start, scanner.position - 2));
        }
        highlighter._token("se");
        start = scanner.position;
      } else if (scanner.scanChar($double_quote)) {
        highlighter._token("s", scanner.substring(start, scanner.position));
        return;
      } else {
        scanner.position++;
      }
    }

    // Error: Unterminated string.
    highlighter._token("err", scanner.substring(start, scanner.position));
  }
}

/// Parses an identifier and resolves keywords for their token type.
class IdentifierRule extends Rule {
  IdentifierRule() : super._(r"[a-zA-Z_][a-zA-Z0-9_]*");

  void applyRule(Highlighter highlighter) {
    var identifier = highlighter._scanner.lastMatch[0];
    var type = highlighter._language.words[identifier] ?? "n";

    // Capitalized identifiers are treated specially in Lox.
    // TODO: Do something less hacky.
    if (highlighter._language == _lox &&
        identifier.codeUnitAt(0) >= $A &&
        identifier.codeUnitAt(0) <= $Z) {
      type = "nc";
    }

    highlighter._token(type);
  }
}

// TODO: This is pretty hacky. The Pygments lexers for C and Java handle
// colons differently. Probably need to fork this.
/// Parses an identifier followed by a colon and treats it as a label or a
/// keyword followed by a ":" as needed.
class LabelRule extends Rule {
  LabelRule() : super._(r"([a-zA-Z_][a-zA-Z0-9_]*)(\s*)(:)");

  void applyRule(Highlighter highlighter) {
    var name = highlighter._scanner.lastMatch[1];
    var space = highlighter._scanner.lastMatch[2];
    var colon = highlighter._scanner.lastMatch[3];
    highlighter._token(highlighter._language.words[name] ?? "nl", name);
    // TODO: Allowing space here means that this incorrectly parses an
    // identifier before a conditional operator as a label name. Pygments
    // does this wrong so this matches that. Once we aren't trying to match
    // exactly, remove the (\s*) from the regex to fix that.
    highlighter._write(space);
    highlighter._token("o", colon);
  }
}

final _c = Language(
  keywords: "bool break case char const continue default do double else enum "
      "extern FILE for goto if inline int return size_t sizeof static struct "
      "switch typedef uint16_t uint32_t uint64_t uint8_t uintptr_t union "
      "va_list void while",
  names: "false NULL true",
  rules: [
    // Include.
    Rule.capture(r"(#include)(\s+)(.*)", ["cp", "", "cpf"]),

    // Preprocessor with comment.
    Rule.capture(r"(#.*?)(//.*)", ["cp", "c1"]),

    // Preprocessor.
    Rule(r"#.*", "cp"),

    LabelRule(),

    ..._commonRules,
    _characterRule,
    _cOperatorRule,
  ],
);

final _java = Language(
  keywords: "abstract assert boolean break byte case catch char class const "
      "continue default do double else enum extends final finally float for "
      "goto if implements import instanceof int interface long native new "
      "package private protected public return short static strictfp super "
      "switch synchronized this throw throws transient try void volatile while",
  constants: "false null true",
  rules: [
    // Type declaration.
    Rule.capture(r"(class|enum|interface)(\s+)(\w+)", ["k", "", "nc"]),
    // Import.
    Rule.capture(r"(import)(\s+)(\w+(?:\.\w+)*)(;)", ["k", "", "n", "o"]),
    // Static import.
    Rule.capture(r"(import\s+static?)(\s+)(\w+(?:\.\w+)*(?:\.\*)?)(;)",
        ["k", "", "n", "o"]),
    // Package.
    Rule.capture(r"(package)(\s+)(\w+(?:\.\w+)*)(;)", ["k", "", "n", "o"]),
    // Annotation.
    Rule(r"@[a-zA-Z_][a-zA-Z0-9_]*", "nd"),

    ..._commonRules,
    _characterRule,
    _cOperatorRule,
  ],
);

final _lox = Language(
  keywords: "and class else fun for if or print return super var while",
  names: "false nil this true",
  rules: [
    ..._commonRules,
    // Lox has fewer operator characters.
    Rule(r"[(){}[\]!+\-/*;.,=<>]+", "o"),

    // TODO: Only used because we use "lox" for EBNF snippets. Remove this and
    // create a separate grammar language.
    _characterRule,

    // Other operators are errors. (This shows up when using Lox for EBNF
    // snippets.)
    // TODO: Make a separate language for EBNF and stop using "err".
    Rule(r"[|&?']+", "err"),
  ],
);

final _ruby = Language(
  keywords: "__LINE__ _ENCODING__ __FILE__ BEGIN END alias and begin break "
      "case class def defined? do else elsif end ensure for if in module next "
      "nil not or redo rescue retry return self super then undef unless until "
      "when while yield",
  names: "lambda puts",
  other: {
    // TODO: Remove these and use an existing type when not trying to match
    // old output.
    "false": "kp",
    "true": "kp",
  },
  rules: [
    ..._commonRules,
    _cOperatorRule,
  ],
);

final _languages = {
  "c": _c,
  // TODO: Add C++ support.
  "c++": Language(),
  "java": _java,
  // TODO: Add JS support.
  "js": Language(
      keywords: "break case catch class const continue debugger default delete "
          "do else export extends finally for function if import in instanceof "
          "let new return super switch this throw try typeof var void while "
          "with yield",
      rules: [..._commonRules, _cOperatorRule]),
  // TODO: Add Lisp support.
  "lisp": Language(),
  // TODO: Make `this` a keyword? It is in Java.
  "lox": _lox,
  // TODO: Add Lua support.
  "lua": Language(),
  "python": Language(
      keywords: "and as assert break class continue def del elif else except "
          "exec finally for from global if import is lambda not or pass print "
          "raise return try while with yield",
      names: "range",
      other: {
        // TODO: Get rid of this and just make it a keyword.
        "in": "ow",
      },
      rules: [
        ..._commonRules,
        _cOperatorRule,
      ]),
  "ruby": _ruby,
};

/// Matches the operator characters in C-like languages.
// TODO: Allowing a space in here would produce visually identical output but
// collapse to fewer spans in cases like ") + (".
final _cOperatorRule = Rule(r"[(){}[\]!+\-/*:;.,|?=<>&^%]+", "o");

// TODO: Multi-character escapes?
final _characterRule = Rule(r"'\\?.'", "s");

final _commonRules = [
  Rule.capture(r"(\.)([a-zA-Z_][a-zA-Z0-9_]*)", ["o", "n"]), // Attribute.

  StringRule(),

  Rule(r"[0-9]+\.[0-9]+f?", "mf"), // Float.
  Rule(r"0x[0-9a-fA-F]+", "mh"), // Hex integer.
  Rule(r"[0-9]+[Lu]?", "mi"), // Integer.

  Rule(r"//.*", "c1"), // Line comment.

  IdentifierRule(),

  // TODO: Pygments doesn't handle backslashes in multi-line defines, so
  // report the same error here. Remove this when not trying to match
  // that.
  Rule(r"\\", "err"),
  // TODO: Just leave this as plain text once we aren't trying to match
  // Pygments.
  Rule(r"→", "err"),
];
