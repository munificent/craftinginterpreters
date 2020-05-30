import 'language.dart';
import 'rule.dart';

final languages = {
  "c": c,
  "c++": cpp,
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
  // TODO: Make NULL a constant instead of a keyword?
  keywords: "$_cKeywords false NULL true",
  rules: _cRules,
);

final cpp = Language(
  // TODO: Make NULL a constant instead of a keyword?
  keywords: "$_cKeywords false NULL true",
  types: "vector string",
  rules: _cRules,
);

final java = Language(
  keywords: "abstract assert boolean break byte case catch char class const "
      "continue default do double else enum extends final finally float for "
      "goto if implements import instanceof int interface long native new "
      "package private protected public return short static strictfp super "
      "switch synchronized this throw throws transient try void volatile while",
  constants: "false null true",
  rules: [
    // Type declaration.
    // TODO: Should match enum here too. Or just use identifier capitalization
    // for "nc" and remove this rule.
    Rule.capture(r"(class|interface)(\s+)(\w+)", ["k", "", "nc"]),
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
    "bool break case char const continue default do double else enum "
    "extern FILE for goto if inline int return size_t sizeof static struct "
    "switch typedef uint16_t uint32_t uint64_t uint8_t uintptr_t union "
    "va_list void while";

final _cRules = [
  // Include.
  Rule.capture(r"(#include)(\s+)(.*)", ["cp", "", "cpf"]),

  // Preprocessor with comment.
  Rule.capture(r"(#.*?)(//.*)", ["cp", "c"]),

  // Preprocessor.
  Rule(r"#.*", "cp"),

  LabelRule(),

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
  Rule.capture(r"(\.)([a-zA-Z_][a-zA-Z0-9_]*)", ["o", "n"]), // Attribute.

  StringRule(),

  Rule(r"[0-9]+\.[0-9]+f?", "mf"), // Float.
  Rule(r"0x[0-9a-fA-F]+", "mh"), // Hex integer.
  Rule(r"[0-9]+[Lu]?", "mi"), // Integer.

  Rule(r"//.*", "c"), // Line comment.

  IdentifierRule(),

  // TODO: Pygments doesn't handle backslashes in multi-line defines, so
  // report the same error here. Remove this when not trying to match
  // that.
  Rule(r"\\", "err"),
  // TODO: Just leave this as plain text once we aren't trying to match
  // Pygments.
  Rule(r"→", "err"),
];
