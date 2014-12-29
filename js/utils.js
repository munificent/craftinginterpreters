
// Returns true if `c` is an English letter or underscore.
function isAlpha(c) {
  return (c >= "a" && c <= "z") ||
         (c >= "A" && c <= "Z") ||
         c == "_";
}

// Returns true if `c` is an English letter, underscore, or digit.
function isAlphaNumeric(c) {
  return isAlpha(c) || isDigit(c);
}

// Returns true if `c` is a space, newline, or tab.
function isWhitespace(c) {
  return c == " " || c == "\n" || c == "\t";
}

// Returns true if `c` is a digit.
function isDigit(c) {
  return c >= "0" && c <= "9";
}