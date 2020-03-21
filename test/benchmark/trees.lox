class Tree {
  init(depth) {
    this.depth = depth;
    if (depth > 0) {
      this.a = Tree(depth - 1);
      this.b = Tree(depth - 1);
      this.c = Tree(depth - 1);
      this.d = Tree(depth - 1);
      this.e = Tree(depth - 1);
    }
  }

  walk() {
    if (this.depth == 0) return 0;
    return this.depth 
        + this.a.walk()
        + this.b.walk()
        + this.c.walk()
        + this.d.walk()
        + this.e.walk();
  }
}

var tree = Tree(8);
var start = clock();
for (var i = 0; i < 100; i = i + 1) {
  if (tree.walk() != 122068) print "Error";
}
print clock() - start;
