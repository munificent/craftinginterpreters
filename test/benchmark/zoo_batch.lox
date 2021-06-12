class Zoo {
  init() {
    this.aarvark  = 1;
    this.baboon   = 1;
    this.cat      = 1;
    this.donkey   = 1;
    this.elephant = 1;
    this.fox      = 1;
  }
  ant()    { return this.aarvark; }
  banana() { return this.baboon; }
  tuna()   { return this.cat; }
  hay()    { return this.donkey; }
  grass()  { return this.elephant; }
  mouse()  { return this.fox; }
}

var zoo = Zoo();
var sum = 0;
var start = clock();
var batch = 0;
while (clock() - start < 10) {
  for (var i = 0; i < 10000; i = i + 1) {
    sum = sum + zoo.ant()
              + zoo.banana()
              + zoo.tuna()
              + zoo.hay()
              + zoo.grass()
              + zoo.mouse();
  }
  batch = batch + 1;
}

print sum;
print batch;
print clock() - start;
