#include "utils.h"

#include <execinfo.h>
#include <stdio.h>
#include <string.h>

#include "escape.h"
#include "interpreter.h"
#include "literal.h"
#include "parser.h"
//#include "typechecker.h"

/**
 * @brief read a file to a string if the file exists, otherwise, return NULL.
 *
 * @param filename name of file to read
 * @return char* string with contents of file with name filename
 */
char *read_file_to_string(char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        long len = ftell(file);
        fseek(file, 0, SEEK_SET);
        char *buffer = malloc(len + 1);
        fread(buffer, 1, len, file);
        fclose(file);
        buffer[len] = '\0';
        return buffer;
    } else {
        return NULL;
    }
}

/**
 * @brief print readable version of literal
 *
 * @param literal
 */
void print_literal(Literal literal) {
    if (literal.kind == BoolLit) {
        if (literal.type.Bool) {
            printf("true");
        } else {
            printf("false");
        }
    } else if (literal.kind == IntLit) {
        printf("%ld", literal.type.Int);
    } else if (literal.kind == FloatLit) {
        printf("%f", literal.type.Float);
    } else {
        sds escaped = escape(literal.type.String);
        printf("\"%s\"", escaped);
        sdsfree(escaped);
    }
}

void print_atom(Atom atom) {
    if (atom.kind == SymbolAtom) {
        printf("%s", atom.type.symbol);
    } else {
        print_literal(atom.type.literal);
    }
}

void print_val(Val val) {
    switch (val.kind) {
        case BuiltinFnVal:;
            char **symbols = backtrace_symbols(&val.type.bfn, 1);
            printf("%s", *symbols);
            free(symbols);
            break;
        case FnVal:
            printf("fn (");
            for (int i = 0; i < cvector_size(val.type.fn.args); i++) {
                printf(" %s", val.type.fn.args[i]);
            }
            printf(" ) ");
            print_expr(val.type.fn.body);
            break;
        case LiteralVal:
            print_literal(val.type.lit);
            break;
        case TupleVal:
            printf("(");
            for (int i = 0; i < cvector_size(val.type.tup.values); i++) {
                print_val(val.type.tup.values[i]);
                if (i != cvector_size(val.type.tup.values) - 1) {
                    printf(" ");
                }
            }
            printf(")");
            break;
    }
}

/**
 * @brief print readable version of expression
 *
 * @param expr
 */
void print_expr(Expression *expr) {
    if (expr->atomic) {
        print_atom(expr->data.atom);
    } else {
        printf("(");
        for (int i = 0; i < cvector_size(expr->data.expr); i++) {
            if (i != 0) {
                printf(" ");
            }
            print_expr(expr->data.expr[i]);
        }
        printf(")");
    }
}

/**
 * @brief print readable version of type
 *
 * @param type
 */
/*
void print_type(Type *type) {
    if (type == NULL) {
        printf("NULL");
        return;
    }
    if (type->kind == DefType) {
        printf("%s", type->type.defined);
    } else if (type->kind == LitType) {
        char *type_str;
        switch (type->type.literal) {
            case IntLit:
                type_str = "int";
                break;
            case BoolLit:
                type_str = "bool";
                break;
            case FloatLit:
                type_str = "float";
                break;
            case StringLit:
                type_str = "string";
                break;
            case InvalidLit:
                abort();
        }
        printf("%s", type_str);
    } else if (type->kind == TupType) {
        printf("(");
        TupleT *curr = type->type.tuple;
        while (curr != NULL) {
            print_type(curr->type);
            if (curr->next != NULL) {
                printf(" ");
            }
            curr = curr->next;
        }
        printf(")");
    } else if (type->kind == FunType) {
    } else {
        fprintf(stderr,
                "trying to print out type that hasn't been implemented in "
                "print_type()\n");
        abort();
    }
}
*/

/**
 * @brief return a new string that is the same as the old string except any
 * characters that match the provided char c are removed (replaced with '').
 *
 * @param string string to remove characters from
 * @param c character to remove
 * @return char* string with all characters of type c removed, should be freed
 * with `free`
 */
char *remove_char_from_string(char *string, char c) {
    char *src = string;
    char *dest = strdup(string);
    char *output = dest;
    while (*src != '\0') {
        if (*src != c) {
            *dest = *src;
            dest++;
        }
        src++;
    }
    *dest = '\0';
    return output;
}

/**
 * @brief crash with the provided message
 *
 * @param message
 */
void syntax_error(char *message) {
    fprintf(stderr, "syntax error: %s\n", message);
    abort();
}
