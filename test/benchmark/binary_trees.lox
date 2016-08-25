class Tree {
  init(item, depth) {
    this.item = item;
    this.depth = depth;
    if (depth > 0) {
      var item2 = item + item;
      depth = depth - 1;
      this.left = Tree(item2 - 1, depth);
      this.right = Tree(item2, depth);
    } else {
      this.left = nil;
      this.right = nil;
    }
  }

  check() {
    if (this.left == nil) {
      return this.item;
    }

    return this.item + this.left.check() - this.right.check();
  }
}

var minDepth = 4;
var maxDepth = 12;
var stretchDepth = maxDepth + 1;

var start = clock();

print "stretch tree of depth:";
print stretchDepth;
print "check:";
print Tree(0, stretchDepth).check();

var longLivedTree = Tree(0, maxDepth);

// iterations = 2 ** maxDepth
var iterations = 1;
var d = 0;
while (d < maxDepth) {
  iterations = iterations * 2;
  d = d + 1;
}

var depth = minDepth;
while (depth < stretchDepth) {
  var check = 0;
  var i = 1;
  while (i <= iterations) {
    check = check + Tree(i, depth).check() + Tree(-i, depth).check();
    i = i + 1;
  }

  print "num trees:";
  print iterations * 2;
  print "depth:";
  print depth;
  print "check:";
  print check;

  iterations = iterations / 4;
  depth = depth + 2;
}

print "long lived tree of depth:";
print maxDepth;
print "check:";
print longLivedTree.check();
print "elapsed:";
print clock() - start;
