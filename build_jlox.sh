#!/bin/bash

# Remove old class files
find jlox -name "*.class" -delete

# Remove generated files
rm -f jlox/lox/Expr.java jlox/lox/Stmt.java

# Compile and run the AST generator
# (generate Expr.java and Stmt.java into 'jlox/lox' folder)
javac jlox/tool/GenerateAst.java
java jlox.tool.GenerateAst jlox/lox

# Compile main application
javac jlox/lox/*.java
