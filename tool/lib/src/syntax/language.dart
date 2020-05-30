import 'rule.dart';

/// Defines the syntax rules for a single programming language.
class Language {
  final Map<String, String> words = {};
  final List<Rule> rules;

  Language(
      {String keywords,
      String types,
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
    keywordType(types, "t");
  }
}
