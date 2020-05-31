import 'language.dart';
import 'rule.dart';

final languages = {
  "c": c,
  "c++": cpp,
  "ebnf": ebnf,
  "java": java,
  "js": js,
  "lisp": lisp,
  "lox": lox,
  // TODO: This is just enough for the one line in "scanning". Do more if
  // needed.
  "lua": Language(rules: [..._commonRules, _cOperatorRule]),
  "python": python,
  "ruby": ruby,
};

final c = Language(
  keywords: _cKeywords,
  types: "bool char double FILE int size_t uint16_t uint32_t uint64_t uint8_t "
      "uintptr_t va_list void",
  rules: _cRules,
);

final cpp = Language(
  keywords: _cKeywords,
  types: "vector string",
  rules: _cRules,
);

final ebnf = Language(
  rules: [
    // Color ALL_CAPS terminals like types to make them distinct.
    Rule(r"[A-Z][A-Z0-9_]+", "t"),
    ..._commonRules
  ],
);

final java = Language(
  keywords: "abstract assert break case catch class const continue default do "
      "else enum extends false final finally for goto if implements import "
      "instanceof interface native new null package private protected public "
      "return static strictfp super switch synchronized this throw throws "
      "transient true try volatile while",
  types: "boolean byte char double float int long short void",
  rules: [
    // Import.
    Rule.capture(r"(import)(\s+)(\w+(?:\.\w+)*)(;)", ["k", "", "n", "o"]),
    // Static import.
    Rule.capture(r"(import\s+static?)(\s+)(\w+(?:\.\w+)*(?:\.\*)?)(;)",
        ["k", "", "n", "o"]),
    // Package.
    Rule.capture(r"(package)(\s+)(\w+(?:\.\w+)*)(;)", ["k", "", "n", "o"]),
    // Annotation.
    Rule(r"@[a-zA-Z_][a-zA-Z0-9_]*", "a"),

    // ALL_CAPS constant names are colored like normal identifiers. We give
    // them their own rule so that it matches before the capitalized type name
    // rule.
    Rule(r"[A-Z][A-Z0-9_]+", "n"),

    ..._commonRules,
    _characterRule,
    _cOperatorRule,
  ],
);

final js = Language(
  keywords: "break case catch class const continue debugger default delete do "
      "else export extends finally for function if import in instanceof let "
      "new return super switch this throw try typeof var void while with yield",
  rules: [..._commonRules, _cOperatorRule],
);

final lisp = Language(
  rules: [
    // TODO: Other punctuation characters.
    Rule(r"[a-zA-Z0-9_-]+", "n"),
    Rule(r"[()[\]{}]+", "o"),
  ],
);

final lox = Language(
  keywords: "and class else false fun for if nil or print return super this "
      "true var while",
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

final python = Language(
  keywords: "and as assert break class continue def del elif else except "
      "exec finally for from global if import in is lambda not or pass "
      "print raise range return try while with yield",
  rules: [
    ..._commonRules,
    _cOperatorRule,
  ],
);

final ruby = Language(
  keywords: "__LINE__ _ENCODING__ __FILE__ BEGIN END alias and begin break "
      "case class def defined? do else elsif end ensure false for if in lambda "
      "module next nil not or redo rescue retry return self super then true "
      "undef unless until when while yield",
  rules: [
    ..._commonRules,
    _cOperatorRule,
  ],
);

final _cKeywords =
    "break case const continue default do else enum extern false for goto if "
    "inline return sizeof static struct switch true typedef union while";

final _cRules = [
  // Preprocessor with comment.
  Rule.capture(r"(#.*?)(//.*)", ["a", "c"]),

  // Preprocessor.
  Rule(r"#.*", "a"),

  // ALL_CAPS preprocessor macro use.
  Rule(r"[A-Z][A-Z0-9_]+", "a"),

  ..._commonRules,
  _characterRule,
  _cOperatorRule,
];

/// Matches the operator characters in C-like languages.
// TODO: Allowing a space in here would produce visually identical output but
// collapse to fewer spans in cases like ") + (".
final _cOperatorRule = Rule(r"[(){}[\]!+\-/*:;.,|?=<>&^%]+", "o");

// TODO: Multi-character escapes?
final _characterRule = Rule(r"'\\?.'", "s");

final _commonRules = [
  StringRule(),

  Rule(r"[0-9]+\.[0-9]+f?", "n"), // Float.
  Rule(r"0x[0-9a-fA-F]+", "n"), // Hex integer.
  Rule(r"[0-9]+[Lu]?", "n"), // Integer.

  Rule(r"//.*", "c"), // Line comment.

  // Capitalized type name.
  Rule(r"[A-Z][A-Za-z0-9_]*", "t"),

  // Other identifiers or keywords.
  IdentifierRule(),
];
