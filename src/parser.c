#include "parser.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/sds/sds.h"
#include "escape.h"
#include "utils.h"

/**
 * @brief advance text_ptr until the character it points to is not whitespace
 * (or until the end of the string).
 *
 * @param text_ptr
 */
void consume_whitespace(char **text_ptr) {
    char *curr = *text_ptr;
    while (isspace(*curr) && *curr != '\0') {
        curr++;
    }
    *text_ptr = curr;
}

/**
 * @brief return true if c is a parenthesis, otherwise false
 *
 * @param c
 * @return true
 * @return false
 */
bool is_parens(char c) { return c == '(' || c == ')'; }

/**
 * @brief return true if the curr points to the end of a string literal, where
 * start points to the start of the string literal. curr points to the end of
 * the string literal when it points to an unescaped '"' character (the '"'
 * character should not be preceeded by an odd number of backslashes).
 *
 * @param start start of the string literal
 * @param curr current position in the string literal
 * @return true
 * @return false
 */
bool is_end_of_string_literal(char *start, char *curr) {
    if (*curr != '"') {
        return false;
    }
    int num_backslash = 0;
    char *backslash_ptr = curr - 1;
    while (backslash_ptr > start && *backslash_ptr == '\\') {
        num_backslash++;
        backslash_ptr--;
    }
    return num_backslash % 2 == 0;
}

/**
 * @brief parse an atom. An Atom is either a literal (number, bool, string), or
 * a symbol. No symbols may start with backslash (this is so that we can parse
 * strings easier)
 *
 * @param text_ptr
 * @return Atom
 */
Atom parse_atom(char **text_ptr) {
    char *start = *text_ptr;
    char *curr = *text_ptr;
    if (*start == '\\') {
        syntax_error("no symbol can start with '\\'");
    }

    if (*start == '"') {
        curr++;
        while (!is_end_of_string_literal(start, curr) && *curr != '\0') {
            curr++;
        }
        if (*curr == '\0') {
            syntax_error("unclosed string");
        }
        curr++;
    } else {
        while (!isspace(*curr) && !is_parens(*curr) && *curr != '\0') {
            curr++;
        }
    }

    char temp = *curr;
    *curr = '\0';
    sds token = sdsnew(start);
    *curr = temp;
    *text_ptr = curr;

    Atom atom;
    Literal literal = match_literal(token);
    if (literal.kind != InvalidLit) {
        atom.kind = LiteralAtom;
        atom.type.literal = literal;
    } else {
        atom.kind = SymbolAtom;
        atom.type.symbol = sdsdup(token);
    }
    sdsfree(token);
    return atom;
}

/**
 * @brief parse an expression. An expresion is a list containing symbols and
 * other expressions, delimited by parenthesis.
 *
 * @param text_ptr
 * @return Expression* expression. Must be freed by caller with `free_expr`
 */
Expression *parse_expr(char **text_ptr) {
    int len = strlen(*text_ptr);
    char *start = *text_ptr;
    char *curr = *text_ptr;
    consume_whitespace(&curr);

    if ((curr - start) >= len) {
        return NULL;
    }

    Expression *expr = malloc(sizeof(Expression));
    expr->atomic = *curr != '(';
    if (expr->atomic) {
        expr->data.atom = parse_atom(&curr);
    } else {
        curr++;
        expr->data.expr = NULL;
        expr->chain = NULL;
        consume_whitespace(&curr);
        while (*curr != ')') {
            Expression* ex = parse_expr(&curr);
            if (ex == NULL) {
                syntax_error("expected expression as argument, found nothing");
            }
            cvector_push_back(expr->data.expr, ex);
            consume_whitespace(&curr);
            if (*curr == '\0') {
                syntax_error("missing closing paren");
            }
        }
        curr++;
        consume_whitespace(&curr);
        if (*curr == ';') {
            curr++;
            Expression* ex = parse_expr(&curr);
            if (ex == NULL) {
                syntax_error("expected expression after chain: ';', found nothing");
            }
            expr->chain = ex;
        }
    }
    *text_ptr = curr;
    return expr;
}

/**
 * @brief free the provided atom (literal or symbol)
 *
 * @param atom
 */
static void free_atom(Atom atom) {
    if (atom.kind == SymbolAtom) {
        sdsfree(atom.type.symbol);
    } else {
        free_literal(atom.type.literal);
    }
}

/**
 * @brief free the provided expression and all atoms/expressions it contains.
 *
 * @param expr
 */
void free_expr(Expression *expr) {
    if (expr->atomic) {
        free_atom(expr->data.atom);
    } else {
        for (int i = 0; i < cvector_size(expr->data.expr); i++) {
            free_expr(expr->data.expr[i]);
        }
    }
    free(expr);
}