#ifndef SPORK_PARSER_H_
#define SPORK_PARSER_H_
#include "lexer.h"
#include "../lib/cvector.h"

typedef struct Expr Expr;
typedef struct LangType LangType;

typedef struct
{
    Identifier param_name;
    LangType *type;
} Param;

typedef struct
{
    Identifier variable;
    Expr *equal_expr;
    Expr *next_expr;
} Let;

typedef struct
{
    cvector_vector_type(Param) params;
    LangType *return_type;
    Expr *body;
} FnDef;

typedef struct
{
    cvector_vector_type(Param) params;
} StructDef;

typedef struct
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

struct Expr {
    ExprType expr_type;
    ExprKind expr_kind;
};

typedef struct
{
    cvector_vector_type(Token) tokens;
    int token_ptr;
} ParserPointer;

Expr* parse_expr(ParserPointer start, ParserPointer *end);

#endif