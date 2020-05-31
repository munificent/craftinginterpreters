import 'rule.dart';

/// Defines the syntax rules for a single programming language.
class Language {
  final Map<String, String> words = {};
  final List<Rule> rules;

  Language({String keywords, String types, List<Rule> this.rules}) {
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
