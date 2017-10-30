from pygments import highlight
from pygments.lexers import PythonLexer
from pygments.formatters import HtmlFormatter

from pygments.lexer import RegexLexer
from pygments.token import *

class LoxLexer(RegexLexer):
  name = 'Lox'
  aliases = ['lox']
  filenames = ['*.lox']

  tokens = {
    'root': [
      # Whitespace.
      (r'\s+', Text),

      # Comments.
      (r'//.*?\n', Comment.Single),

      # Numbers
      (r'\d+\.\d+', Number.Float),
      (r'\d+', Number.Integer),

      # Strings.
      (r'"[^"]*"', String),

      # Reserved words.
      (r'\b(and|class|else|for|fun|if|or|print|return|super|var|while)\b', Keyword),
      (r'\b(false|nil|this|true)\b', Name.Builtin),

      # Identifiers.
      (r'[a-z_][a-zA-Z_0-9]*', Name),
      (r'[A-Z][a-zA-Z_0-9]*', Name.Variable.Global),

      # Operators and punctuators.
      (r'\+|-|\*|/|!=?|<=?|>=?|==?', Operator),
      (r'[(),.;{}]', Punctuation),
    ],
  }
