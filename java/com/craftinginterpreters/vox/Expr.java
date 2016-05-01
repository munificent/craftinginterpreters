package com.craftinginterpreters.vox;

import java.util.List;

abstract class Expr {
  interface Visitor<R, C> {
    R visitAssignExpr(Assign expr, C context);
    R visitBinaryExpr(Binary expr, C context);
    R visitCallExpr(Call expr, C context);
    R visitGroupingExpr(Grouping expr, C context);
    R visitLiteralExpr(Literal expr, C context);
    R visitLogicalExpr(Logical expr, C context);
    R visitPropertyExpr(Property expr, C context);
    R visitSuperExpr(Super expr, C context);
    R visitThisExpr(This expr, C context);
    R visitUnaryExpr(Unary expr, C context);
    R visitVariableExpr(Variable expr, C context);
  }

  static class Assign extends Expr {
    Assign(Expr object, Token name, Expr value) {
      this.object = object;
      this.name = name;
      this.value = value;
    }

    <R, C> R accept(Visitor<R, C> visitor, C context) {
      return visitor.visitAssignExpr(this, context);
    }

    final Expr object;
    final Token name;
    final Expr value;
  }

  static class Binary extends Expr {
    Binary(Expr left, Token operator, Expr right) {
      this.left = left;
      this.operator = operator;
      this.right = right;
    }

    <R, C> R accept(Visitor<R, C> visitor, C context) {
      return visitor.visitBinaryExpr(this, context);
    }

    final Expr left;
    final Token operator;
    final Expr right;
  }

  static class Call extends Expr {
    Call(Expr callee, Token paren, List<Expr> arguments) {
      this.callee = callee;
      this.paren = paren;
      this.arguments = arguments;
    }

    <R, C> R accept(Visitor<R, C> visitor, C context) {
      return visitor.visitCallExpr(this, context);
    }

    final Expr callee;
    final Token paren;
    final List<Expr> arguments;
  }

  static class Grouping extends Expr {
    Grouping(Expr expression) {
      this.expression = expression;
    }

    <R, C> R accept(Visitor<R, C> visitor, C context) {
      return visitor.visitGroupingExpr(this, context);
    }

    final Expr expression;
  }

  static class Literal extends Expr {
    Literal(Object value) {
      this.value = value;
    }

    <R, C> R accept(Visitor<R, C> visitor, C context) {
      return visitor.visitLiteralExpr(this, context);
    }

    final Object value;
  }

  static class Logical extends Expr {
    Logical(Expr left, Token operator, Expr right) {
      this.left = left;
      this.operator = operator;
      this.right = right;
    }

    <R, C> R accept(Visitor<R, C> visitor, C context) {
      return visitor.visitLogicalExpr(this, context);
    }

    final Expr left;
    final Token operator;
    final Expr right;
  }

  static class Property extends Expr {
    Property(Expr object, Token name) {
      this.object = object;
      this.name = name;
    }

    <R, C> R accept(Visitor<R, C> visitor, C context) {
      return visitor.visitPropertyExpr(this, context);
    }

    final Expr object;
    final Token name;
  }

  static class Super extends Expr {
    Super(Token method) {
      this.method = method;
    }

    <R, C> R accept(Visitor<R, C> visitor, C context) {
      return visitor.visitSuperExpr(this, context);
    }

    final Token method;
  }

  static class This extends Expr {
    This(Token name) {
      this.name = name;
    }

    <R, C> R accept(Visitor<R, C> visitor, C context) {
      return visitor.visitThisExpr(this, context);
    }

    final Token name;
  }

  static class Unary extends Expr {
    Unary(Token operator, Expr right) {
      this.operator = operator;
      this.right = right;
    }

    <R, C> R accept(Visitor<R, C> visitor, C context) {
      return visitor.visitUnaryExpr(this, context);
    }

    final Token operator;
    final Expr right;
  }

  static class Variable extends Expr {
    Variable(Token name) {
      this.name = name;
    }

    <R, C> R accept(Visitor<R, C> visitor, C context) {
      return visitor.visitVariableExpr(this, context);
    }

    final Token name;
  }

  abstract <R, C> R accept(Visitor<R, C> visitor, C context);
}
