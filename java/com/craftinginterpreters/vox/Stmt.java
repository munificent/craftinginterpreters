package com.craftinginterpreters.vox;

import java.util.List;

abstract class Stmt {
  interface Visitor<R, C> {
    R visitBlockStmt(Block stmt, C context);
    R visitClassStmt(Class stmt, C context);
    R visitExpressionStmt(Expression stmt, C context);
    R visitForStmt(For stmt, C context);
    R visitFunctionStmt(Function stmt, C context);
    R visitIfStmt(If stmt, C context);
    R visitReturnStmt(Return stmt, C context);
    R visitVarStmt(Var stmt, C context);
    R visitWhileStmt(While stmt, C context);
  }

  static class Block extends Stmt {
    Block(List<Stmt> statements) {
      this.statements = statements;
    }

    <R, C> R accept(Visitor<R, C> visitor, C context) {
      return visitor.visitBlockStmt(this, context);
    }

    final List<Stmt> statements;
  }

  static class Class extends Stmt {
    Class(Token name, Expr superclass, List<Stmt.Function> methods) {
      this.name = name;
      this.superclass = superclass;
      this.methods = methods;
    }

    <R, C> R accept(Visitor<R, C> visitor, C context) {
      return visitor.visitClassStmt(this, context);
    }

    final Token name;
    final Expr superclass;
    final List<Stmt.Function> methods;
  }

  static class Expression extends Stmt {
    Expression(Expr expression) {
      this.expression = expression;
    }

    <R, C> R accept(Visitor<R, C> visitor, C context) {
      return visitor.visitExpressionStmt(this, context);
    }

    final Expr expression;
  }

  static class For extends Stmt {
    For(Token name, Expr iterator, Stmt body) {
      this.name = name;
      this.iterator = iterator;
      this.body = body;
    }

    <R, C> R accept(Visitor<R, C> visitor, C context) {
      return visitor.visitForStmt(this, context);
    }

    final Token name;
    final Expr iterator;
    final Stmt body;
  }

  static class Function extends Stmt {
    Function(Token name, List<Token> parameters, Stmt body) {
      this.name = name;
      this.parameters = parameters;
      this.body = body;
    }

    <R, C> R accept(Visitor<R, C> visitor, C context) {
      return visitor.visitFunctionStmt(this, context);
    }

    final Token name;
    final List<Token> parameters;
    final Stmt body;
  }

  static class If extends Stmt {
    If(Expr condition, Stmt thenBranch, Stmt elseBranch) {
      this.condition = condition;
      this.thenBranch = thenBranch;
      this.elseBranch = elseBranch;
    }

    <R, C> R accept(Visitor<R, C> visitor, C context) {
      return visitor.visitIfStmt(this, context);
    }

    final Expr condition;
    final Stmt thenBranch;
    final Stmt elseBranch;
  }

  static class Return extends Stmt {
    Return(Expr value) {
      this.value = value;
    }

    <R, C> R accept(Visitor<R, C> visitor, C context) {
      return visitor.visitReturnStmt(this, context);
    }

    final Expr value;
  }

  static class Var extends Stmt {
    Var(Token name, Expr initializer) {
      this.name = name;
      this.initializer = initializer;
    }

    <R, C> R accept(Visitor<R, C> visitor, C context) {
      return visitor.visitVarStmt(this, context);
    }

    final Token name;
    final Expr initializer;
  }

  static class While extends Stmt {
    While(Expr condition, Stmt body) {
      this.condition = condition;
      this.body = body;
    }

    <R, C> R accept(Visitor<R, C> visitor, C context) {
      return visitor.visitWhileStmt(this, context);
    }

    final Expr condition;
    final Stmt body;
  }

    abstract <R, C> R accept(Visitor<R, C> visitor, C context);
}
