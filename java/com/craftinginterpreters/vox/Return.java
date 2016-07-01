//>= Functions
package com.craftinginterpreters.vox;

class Return extends RuntimeException {
  final Object value;

  Return(Object value) {
    this.value = value;
  }
}
