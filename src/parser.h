#ifndef SPORK_PARSER_H_
#define SPORK_PARSER_H_
#include <stdbool.h>
#include <stdlib.h>

#include "../lib/cvector/cvector.h"
#include "../lib/sds/sds.h"
#include "literal.h"

/// @brief a symbol is just a string
typedef sds Symbol;

typedef enum AtomKind { SymbolAtom, LiteralAtom } AtomKind;

typedef union AtomType {
    Symbol symbol;
    Literal literal;
} AtomType;

/**
 * @brief An Atom is a single token. It is either a Symbol or a Literal.
 * Literals are strings (e.g. "abc"), integers (e.g. -9999), booleans (e.g.
 * true, false), and floats (e.g. 3.1415).
 */
typedef struct Atom {
    AtomKind kind;
    AtomType type;
} Atom;

typedef struct Expression Expression;

typedef union ExpressionData {
    Atom atom;
    cvector_vector_type(Expression *) expr;
} ExpressionData;

/**
 * @brief An Expression either contains a tuple of expressions or a single atom.
 * We can determine which by checking the 'atomic' flag.
 */
typedef struct Expression {
    ExpressionData data;
    Expression* chain;
    bool atomic;
} Expression;

void syntax_error(char *message);
Expression *parse_expr(char **text_ptr);
void free_expr(Expression *expr);
#endif