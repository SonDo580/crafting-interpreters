package tool;

import java.io.IOException;
import java.io.PrintWriter;
import java.util.Arrays;
import java.util.List;

/**
 * Use this to generate the syntax tree classes
 */
public class GenerateAst {
    private static int level = 0; // current indentation level of output
    private static PrintWriter writer;

    public static void main(String[] args) throws IOException {
        if (args.length != 1) {
            System.out.println("Usage: java <GenerateAst.class path> <output directory>");
            System.exit(64);
        }
        String outputDir = args[0];
        defineAst(outputDir, "Expr", Arrays.asList(
                "Binary   : Expr left, Token operator, Expr right",
                "Grouping : Expr expression",
                "Literal  : Object value",
                "Unary    : Token operator, Expr right"));
    }

    private static void defineAst(String outputDir, String baseName, List<String> types) throws IOException {
        String path = outputDir + '/' + baseName + ".java";
        writer = new PrintWriter(path, "UTF-8");

        printLine("package jlox;");
        printLine();
        printLine("import java.util.List;");
        printLine();
        printLine("abstract class " + baseName + " {");
        level++;

        // The Visitor interface
        defineVisitor(baseName, types);

        // The abstract accept() method
        printLine();
        printLine("abstract <R> R accept(Visitor<R> visitor);");

        // Define the AST classes
        for (String type : types) {
            String className = type.split(":")[0].trim();
            String fields = type.split(":")[1].trim();
            defineType(baseName, className, fields);
        }

        level--;
        printLine("}");
        writer.close();
    }

    private static void defineType(String baseName, String className, String fieldList) {
        printLine("static class " + className + " extends " + baseName + " {");
        level++;

        // Constructor
        printLine(className + "(" + fieldList + ") {");
        level++;

        String[] fields = fieldList.split(", ");
        for (String field : fields) {
            // Store parameters in fields
            String name = field.split(" ")[1];
            printLine("this." + name + " = " + name + ";");
        }

        level--;
        printLine("}");

        // Visitor pattern
        // Each subclass implements 'accept'
        // and calls the right visit method
        printLine();
        printLine("@Override");
        printLine("<R> R accept(Visitor<R> visitor) {");
        level++;
        printLine("return visitor.visit" + className + baseName + "(this);");
        level--;
        printLine("}");

        // Fields
        printLine();
        for (String field : fields) {
            printLine("final " + field + ";");
        }

        level--;
        printLine("}");
    }

    private static void defineVisitor(String baseName, List<String> types) {
        printLine("interface Visitor<R> {");
        level++;

        for (String type : types) {
            String typeName = type.split(":")[0].trim();
            printLine("R visit" + typeName + baseName + "(" + typeName + " " + baseName.toLowerCase() + ");");
        }

        level--;
        printLine("}");
    }

    /* Customize writer.println to support indentation */
    private static void printLine() {
        writer.println();
    }

    private static void printLine(String message) {
        writer.print("\t".repeat(level));
        writer.println(message);
    }
}
