#ifndef SPORK_LITERAL_H_
#define SPORK_LITERAL_H_
#include <stdbool.h>
#include "../lib/sds/sds.h"

typedef enum LiteralKind {
    IntLit,
    FloatLit,
    StringLit,
    BoolLit,
    InvalidLit
} LiteralKind;

typedef union LiteralType {
    long Int;
    double Float;
    sds String;
    bool Bool;
} LiteralType;

typedef struct Literal {
    LiteralKind kind;
    LiteralType type;
} Literal;

Literal match_literal(sds token);
void free_literal(Literal literal);

// TEST
void test_match_int_literal();
void test_match_float_literal();
void test_match_bool_literal();
void test_match_string_literal();
#endif