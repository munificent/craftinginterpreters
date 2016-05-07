class Base {}

class Derived < Base {
  foo() {
    super.doesNotExist(1); // expect runtime error: Undefined property 'doesNotExist'.
  }
}

Derived().foo();
