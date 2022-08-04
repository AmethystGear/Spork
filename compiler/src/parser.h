#ifndef SPORK_PARSER_H_
#define SPORK_PARSER_H_
#include "lexer.h"
#include "../lib/cvector.h"

typedef struct Expr Expr;
typedef struct Type Type;

typedef struct NameTypePair
{
    Identifier name;
    Type* type;
} NameTypePair;

typedef enum TypeKind {
    DefType,
    LitType,
    TupleType,
    NamedTupleType
} TypeKind;

typedef cvector_vector_type(Type *) Tuple;
typedef cvector_vector_type(NameTypePair *) NamedTuple;

typedef union TypeType
{
    Identifier defined;
    LiteralKind literal;
    Tuple tuple;
    NamedTuple named_tuple;
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
    NamedTuple arguments;
    Type *return_type;
    Expr *body;
} FnDef;

typedef struct StructDef
{
   NamedTuple fields;
} StructDef;

typedef struct IfElse
{
    Expr *condition;
    Expr *if_body;
    Expr *else_body;
} IfElse;

typedef struct
{
    Identifier *var_name;
} Var;

typedef struct
{
    Identifier *fn_name;
    cvector_vector_type(Expr) params;
} FnCall;

typedef union
{
    Literal literal;
    Let let;
    FnDef fn_def;
    StructDef struct_def;
    IfElse if_else;
    Var var;
    FnCall fn_call;
} ExprType;

typedef enum
{
    LiteralExpr,
    LetExpr,
    FnDefExpr,
    StructDefExpr,
    IfElseExpr,
    VarExpr,
    FnCallExpr
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

#endif