1.  Python 3 allows comparing all of the various number types with each other,
    except for complex numbers. Booleans (True and False) are a subclass of
    int and work like 1 and 0 for comparison.

    Strings can be compared with each other and are ordered lexicographically.
    Likewise other sequences.

    Comparing sets is defined in terms of subsets and supersets, so that, for
    example `{1, 2} < {1, 2, 3}`. This isn't a total order since many pairs of
    sets are neither subsets nor supersets of each other.

    I think it would be reasonable to extend Lox to support comparing strings
    with each other. I wouldn't support comparing other built in types, nor
    mixing them. Allowing `"1" < 2` is a recipe for confusion.

2.  Replace the Token.PLUS case with:

    ```java
    case PLUS:
      if (left instanceof String || right instanceof String) {
        return stringify(left) + stringify(right);
      }

      if (left instanceof Double && right instanceof Double) {
        return (double)left + (double)right;
      }

      throw new RuntimeError(expr.operator,
          "Operands must be two numbers or two strings.");
      ```

3.  It returns Infinity, -Infinity, or NaN based on sign of the dividend. Given
    that Lox is a high level scripting language, I think it would be better to
    raise a runtime error to let the user know something got weird. That's what
    Python and Ruby do.

    On the other hand, given that Lox gives the user no way to catch and
    handle runtime errors, not throwing one might be more flexible.
