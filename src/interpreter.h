#ifndef SPORK_INTERPRETER_H_
#define SPORK_INTERPRETER_H_
#include "../lib/cvector/cvector.h"
#include "parser.h"

typedef struct Val Val;

typedef struct Tuple {
    cvector_vector_type(Val) values;
} Tuple;

typedef struct Fn {
    cvector_vector_type(Symbol) args;
    Expression* body;
} Fn;


typedef Val (*BuiltinFn)(Tuple tup);

typedef enum ValKind { LiteralVal, TupleVal, FnVal, BuiltinFnVal } ValKind;
typedef union ValType {
    Literal lit;
    Tuple tup;
    BuiltinFn bfn;
    Fn fn;
} ValType;

typedef struct Val {
    ValKind kind;
    ValType type;
} Val;

typedef struct LexicalBinding {
    Symbol symbol;
    Val boundValue;
} LexicalBinding;

Val eval(Expression* expr, cvector_vector_type(LexicalBinding) *env);
#endif