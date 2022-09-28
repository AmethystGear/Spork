#ifndef SPORK_PARSER_H_
#define SPORK_PARSER_H_
#include "lexer.h"
#include "../lib/cvector.h"

typedef struct Expr Expr;
typedef struct Type Type;

typedef struct NameTypePair
{
    Identifier name;
    Type *type;
} NameTypePair;

typedef enum TypeKind
{
    DefType,
    LitType,
    TupleType,
    NamedTupleType
} TypeKind;

typedef enum TupleKind
{
    Product,
    Sum,
} TupleKind;

typedef struct TupleT
{
    cvector_vector_type(Type *) tuple;
    TupleKind tuple_kind;
} TupleT;

typedef struct NamedTupleT
{
    cvector_vector_type(NameTypePair *) named_tuple;
    TupleKind tuple_kind;
} NamedTupleT;

typedef union TypeType
{
    Identifier defined;
    LiteralKind literal;
    TupleT tuple;
    NamedTupleT named_tuple;
} TypeType;

typedef struct Type
{
    TypeKind type_kind;
    TypeType type_type;
} Type;

typedef struct Let
{
    Identifier variable;
    Expr *equal_expr;
    Expr *next_expr;
} Let;

typedef struct FnDef
{
    NamedTupleT arguments;
    Type *return_type;
    Expr *body;
} FnDef;

typedef struct TypeDef
{
    Type *type;
} TypeDef;

typedef struct IfElse
{
    Expr *condition;
    Expr *if_body;
    Expr *else_body;
} IfElse;

typedef struct Var
{
    Identifier *var_name;
} Var;

typedef struct NameExprPair
{
    Identifier name;
    Expr* expr;
} NameExprPair;

typedef struct Tuple {
    cvector_vector_type(Expr*) exprs;
    TupleKind tuple_kind;
} Tuple;

typedef struct NamedTuple {
    cvector_vector_type(NameExprPair*) exprs;
    TupleKind tuple_kind;
} NamedTuple;


typedef struct FnCall
{
    Identifier *fn_name;
    NamedTuple params;
} FnCall;

typedef union
{
    Literal literal;
    Let let;
    FnDef fn_def;
    TypeDef type_def;
    IfElse if_else;
    Var var;
    FnCall fn_call;
    Tuple tuple;
} ExprType;

typedef enum
{
    LiteralExpr,
    LetExpr,
    FnDefExpr,
    TypeDefExpr,
    IfElseExpr,
    VarExpr,
    FnCallExpr,
    TupleExpr
} ExprKind;

struct Expr
{
    ExprType expr_type;
    ExprKind expr_kind;
};

typedef struct
{
    cvector_vector_type(Token) tokens;
    int token_ptr;
} ParserPointer;

Expr *parse_expr(ParserPointer *curr);

// tests
void test_parse_named_type();
void test_parse_type();
void test_parse_let();
void test_parse_variant_type();
void test_parse_named_variant_type();
void test_parse_type_def();
void test_parse_if_else();

#endif