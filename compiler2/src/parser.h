#ifndef SPORK_PARSER_H_
#define SPORK_PARSER_H_
#include <stdbool.h>
#include <stdlib.h>
#include "../lib/sds/sds.h"
#include "literal.h"

typedef sds Symbol;

typedef enum AtomKind { SymbolAtom, LiteralAtom } AtomKind;

typedef union AtomType {
    Symbol symbol;
    Literal literal;
} AtomType;

typedef struct Atom {
    AtomKind kind;
    AtomType type;
} Atom;

typedef enum DataKind { AtomData, ExprData } DataKind;

typedef struct Expression Expression;

typedef union DataType {
    Atom atom;
    Expression* expr;
} DataType;

typedef struct Data {
    DataType type;
    DataKind kind;
} Data;

typedef struct Expression {
    Expression* prev;
    Expression* next;
    Data data;
} Expression;
void syntax_error(char *message);
Expression *parse_expr(char **text_ptr);
void free_expr(Expression *expr);
#endif