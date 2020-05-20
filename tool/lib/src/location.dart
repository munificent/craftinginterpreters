/// The context in which a line of code appears. The chain of types and
/// functions it's in.
class Location {
  final Location parent;
  final String kind;
  final String name;
  final String signature;

  /// If [kind] is "method" or "function" then this tracks where we are
  /// declaring or defining the function.
  final bool isFunctionDeclaration;

  Location(this.parent, this.kind, this.name,
      {this.signature, this.isFunctionDeclaration = false});

  bool get isFile => kind == "file";

  bool get isFunction =>
      const {"constructor", "function", "method"}.contains(kind);

  int get depth {
    var current = this;
    var result = 0;
    while (current != null) {
      result++;
      current = current.parent;
    }
    return result;
  }

  String toString() {
    var result = "$kind $name";
    if (signature != null) result += "($signature)";
    if (parent != null) result = "$parent > $result";
    return result;
  }

  /// Generates a string of HTML that describes a snippet at this location,
  /// when following the [preceding] location.
  String toHtml(Location preceding, List<String> removed) {
    // Note: The order of these is highly significant.
    if (kind == "class" && parent?.kind == "class") {
      return "nest inside class <em>${parent.name}</em>";
    }

    if (isFunction && preceding == this) {
      //  Hack. There's one place where we add a new overload and that shouldn't
      //  be treated as in the same function. But we can't always look at the
      //  signature because there's another place where a change signature would
      //  confuse the build script. So just check for the one-off case here.
      if (name == "resolve" && signature == "Expr expr") {
        return "add after <em>${preceding.name}</em>(${preceding.signature})";
      }

      // We're still inside a function.
      return "in <em>$name</em>()";
    }

    if (isFunction && removed.isNotEmpty) {
      // Hack. We don't appear to be in the middle of a function, but we are
      // replacing lines, so assume we're replacing the entire function.
      return "$kind <em>$name</em>()";
    }

    if (parent == preceding && !preceding.isFile) {
      // We're nested inside a type.
      return "in ${preceding.kind} <em>${preceding.name}</em>";
    }

    if (preceding == this && !isFile) {
      // We're still inside a type.
      return "in $kind <em>$name</em>";
    }

    if (preceding.isFunction) {
      // We aren't inside a function, but we do know the preceding one.
      return "add after <em>${preceding.name}</em>()";
    }

    if (!preceding.isFile) {
      // We aren't inside any function, but we do know what we follow.
      return "add after ${preceding.kind} <em>${preceding.name}</em>";
    }

    // TODO: How do we get here?
    print("No location for $this");
    return null;
  }

  bool operator ==(Object other) {
    // Note: Signature is deliberately not considered part of equality. There's
    // a case in calls-and-functions where the signature of a function changes
    // and it confuses the build script if we treat the signatures as
    // significant.
    return other is Location && kind == other.kind && name == other.name;
  }

  int get hashCode => kind.hashCode ^ name.hashCode;

  /// Discard as many children as needed to get to [depth] parents.
  Location popToDepth(int depth) {
    var current = this;
    var locations = <Location>[];
    while (current != null) {
      locations.add(current);
      current = current.parent;
    }

    // If we are already shallower, there is nothing to pop.
    if (locations.length < depth + 1) return this;

    return locations[locations.length - depth - 1];
  }
}
