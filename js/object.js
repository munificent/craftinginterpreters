"use strict";

// TODO: Keep track of its VoxClass.
function VoxObject() {
  this.fields = {};
}

function VoxFunction(parameters, body, closure) {
  VoxObject.call(this);
  this.parameters = parameters;
  this.body = body;
  this.closure = closure;
}

VoxFunction.prototype = Object.create(VoxObject.prototype);

function VoxClass(constructor, methods) {
  VoxObject.call(this);
  this.constructor = constructor;
  this.methods = methods;
}

// TODO: Inherit from VoxFunction?
VoxClass.prototype = Object.create(VoxObject.prototype);

exports.VoxObject = VoxObject;
exports.VoxClass = VoxClass;
exports.VoxFunction = VoxFunction;
