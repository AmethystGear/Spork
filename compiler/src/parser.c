#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "../lib/sds/sds.h"
#include "lexer.h"
#include "parser.h"
#include "utils.h"

static Type *parse_type(ParserPointer *curr);
typedef void *(*DelimParser)(ParserPointer *curr);
static TupleKind get_tuple_kind(DelimParser parser, ParserPointer *curr, Token *delim);
void free_expr(Expr *expr);

static ParserPointer parser_ptr_pop(ParserPointer ptr, Token **token)
{
    Token *t;
    ParserPointer next = ptr;
    if (ptr.token_ptr < cvector_size(ptr.tokens))
    {
        t = &ptr.tokens[ptr.token_ptr];
        next.token_ptr += 1;
    }
    else
    {
        t = NULL;
    }
    if (token)
    {
        *token = t;
    }
    return next;
}

static bool match_token(Token *t, Token match)
{
    if (!t)
    {
        return false;
    }
    Token token = *t;
    if (token.token_kind != match.token_kind)
    {
        return false;
    }
    switch (token.token_kind)
    {
    case KwTok:
        return token.token_type.kw == match.token_type.kw || match.token_type.kw == -1;
    case SymbTok:
        return token.token_type.symb == match.token_type.symb || match.token_type.symb == -1;
    case IdentTok:
        return match.token_type.ident == NULL || strcmp(token.token_type.ident, match.token_type.ident) == 0;
    default:
        return true;
    }
}

static Token create_match_token(TokenKind token_kind, int kind, char *ident)
{
    Token tok = {.token_kind = token_kind};
    switch (token_kind)
    {
    case KwTok:
        tok.token_type.kw = kind;
        break;
    case SymbTok:
        tok.token_type.symb = kind;
        break;
    case IdentTok:
        tok.token_type.ident = ident;
        break;
    case LitTok:
        break;
    }
    return tok;
}

static bool get_match(ParserPointer *ptr, Token *match_tokens, int n_tokens, cvector_vector_type(Token) * matched_tokens)
{
    if (matched_tokens)
    {
        *matched_tokens = NULL;
    }
    ParserPointer new_ptr = *ptr;
    for (int i = 0; i < n_tokens; i++)
    {
        Token *next_token;
        new_ptr = parser_ptr_pop(new_ptr, &next_token);
        if (!next_token || !match_token(next_token, match_tokens[i]))
        {
            return false;
        }
        else if (matched_tokens)
        {
            cvector_push_back(*matched_tokens, *next_token);
        }
    }
    *ptr = new_ptr;
    return true;
}

static Expr *parse_let(ParserPointer *curr)
{
    cvector_vector_type(Token) matched_tokens = NULL;
    Token match_tokens[] = {
        create_match_token(KwTok, LetKw, NULL),
        create_match_token(IdentTok, -1, NULL),
        create_match_token(SymbTok, AssignSym, NULL),
    };
    if (!get_match(curr, match_tokens, ARRAY_LEN(match_tokens), &matched_tokens))
    {
        compiler_error("'let' must be followed by an identifier and the '=' symbol");
    }
    Expr *equal_expr = parse_expr(curr);
    Token match_terminator[] = {create_match_token(SymbTok, TermSym, NULL)};
    if (!get_match(curr, match_terminator, ARRAY_LEN(match_terminator), NULL))
    {
        compiler_error("let expression must be terminated with a ';' symbol");
    }
    Expr *next_expr = parse_expr(curr);

    Let let_expr = {
        .variable = matched_tokens[1].token_type.ident,
        .equal_expr = equal_expr,
        .next_expr = next_expr,
    };

    cvector_free(matched_tokens);

    Expr *expr = malloc(sizeof(Expr));
    expr->expr_kind = LetExpr;
    expr->expr_type.let = let_expr;
    return expr;
}

static Expr *parse_literal(ParserPointer *curr)
{
    Token *next_token;
    *curr = parser_ptr_pop(*curr, &next_token);
    Expr *expr = malloc(sizeof(Expr));
    expr->expr_kind = LiteralExpr;
    expr->expr_type.literal = next_token->token_type.lit;
    return expr;
}

static Expr *parse_type_def(ParserPointer *curr)
{
    *curr = parser_ptr_pop(*curr, NULL); // skip 'type'
    Expr *expr = malloc(sizeof(Expr));
    TypeDef type_def = {.type = parse_type(curr)};
    expr->expr_kind = TypeDefExpr;
    expr->expr_type.type_def = type_def;
    return expr;
}

static Expr *parse_if_else(ParserPointer *curr)
{
    Token *token;
    *curr = parser_ptr_pop(*curr, &token); // skip 'if'
    Expr *condition = parse_expr(curr);
    *curr = parser_ptr_pop(*curr, &token);
    if (!match_token(token, create_match_token(SymbTok, LeftScopeSym, NULL)))
    {
        compiler_error("expected '{' after if condition");
    }
    Expr *if_body = parse_expr(curr);
    Token match_tokens[] = {
        create_match_token(SymbTok, RightScopeSym, NULL),
        create_match_token(KwTok, ElseKw, NULL),
        create_match_token(SymbTok, LeftScopeSym, NULL),
    };
    if (!get_match(curr, match_tokens, ARRAY_LEN(match_tokens), NULL))
    {
        compiler_error("expected '} else {' after if body");
    }
    Expr *else_body = parse_expr(curr);
    *curr = parser_ptr_pop(*curr, &token);
    if (!match_token(token, create_match_token(SymbTok, RightScopeSym, NULL)))
    {
        compiler_error("expected '}' after else body");
    }

    IfElse if_else = {
        .condition = condition,
        .else_body = else_body,
        .if_body = if_body,
    };

    Expr *expr = malloc(sizeof(Expr));
    expr->expr_kind = IfElseExpr;
    expr->expr_type.if_else = if_else;
    return expr;
}

static cvector_vector_type(void *) parse_delimited(DelimParser parser, Token delim, Token terminator, ParserPointer *curr)
{
    cvector_vector_type(void *) elems = NULL;
    Token *next_token;
    parser_ptr_pop(*curr, &next_token);
    while (!match_token(next_token, terminator))
    {
        cvector_push_back(elems, parser(curr));
        *curr = parser_ptr_pop(*curr, &next_token);
        if (!match_token(next_token, delim) && !match_token(next_token, terminator))
        {
            compiler_error("missing delimiter");
        }
        Token *after_next_token;
        parser_ptr_pop(*curr, &after_next_token);
        if (match_token(after_next_token, terminator))
        {
            *curr = parser_ptr_pop(*curr, &next_token);
            break;
        }
    }
    return elems;
}

Expr *parse_expr(ParserPointer *curr);
static void *parse_expr_wrapper(ParserPointer *curr)
{
    return parse_expr(curr);
}

static void free_expr_ptr(void *elem)
{
    Expr **expr = elem;
    free_expr(*expr);
}

static Expr *parse_tuple_construct(ParserPointer *curr)
{
    *curr = parser_ptr_pop(*curr, NULL); // skip '('

    Token delim;
    TupleKind tuple_kind = get_tuple_kind(parse_expr_wrapper, curr, &delim);
    Token right_paren = create_match_token(SymbTok, RightParenSym, NULL);
    cvector_vector_type(void *) elems = parse_delimited(parse_expr_wrapper, delim, right_paren, curr);

    Tuple tuple = {.exprs = NULL, .tuple_kind = tuple_kind};
    cvector_set_elem_destructor(tuple.exprs, free_expr_ptr);
    for (int i = 0; i < cvector_size(elems); i++)
    {
        cvector_push_back(tuple.exprs, elems[i]);
    }
    cvector_free(elems);

    Expr *expr = malloc(sizeof(Expr));
    expr->expr_kind = TupleExpr;
    expr->expr_type.tuple = tuple;
    return expr;
}

static void *parse_name_expr_pair(ParserPointer *curr)
{
    cvector_vector_type(Token) matched_tokens = NULL;
    Token match_tokens[] = {
        create_match_token(IdentTok, -1, NULL),
        create_match_token(SymbTok, TypeSpecSym, NULL),
    };
    if (!get_match(curr, match_tokens, ARRAY_LEN(match_tokens), &matched_tokens))
    {
        compiler_error("expected 'identifier' : <type>");
    }

    NameExprPair *name_type = malloc(sizeof(NameExprPair));
    name_type->name = matched_tokens[0].token_type.ident;
    name_type->expr = parse_expr(curr);
    cvector_free(matched_tokens);
    return name_type;
}

static Expr *parse_named_tuple_construct(ParserPointer *curr)
{
    *curr = parser_ptr_pop(*curr, NULL); // skip '('

    Token delim;
    TupleKind tuple_kind = get_tuple_kind(parse_name_expr_pair, curr, &delim);
    Token right_paren = create_match_token(SymbTok, RightParenSym, NULL);
    cvector_vector_type(void *) elems = parse_delimited(parse_name_expr_pair, delim, right_paren, curr);

    Tuple tuple = {.exprs = NULL, .tuple_kind = tuple_kind};
    cvector_set_elem_destructor(tuple.exprs, free_expr_ptr);
    for (int i = 0; i < cvector_size(elems); i++)
    {
        cvector_push_back(tuple.exprs, elems[i]);
    }
    cvector_free(elems);

    Expr *expr = malloc(sizeof(Expr));
    expr->expr_kind = TupleExpr;
    expr->expr_type.tuple = tuple;
    return expr;
}

static TupleKind get_tuple_kind(DelimParser parser, ParserPointer *curr, Token *delim)
{
    Token *next_token;
    ParserPointer ptr = *curr;
    parser(&ptr);
    parser_ptr_pop(ptr, &next_token);

    Token sum_delim = create_match_token(SymbTok, DelimSym, NULL);
    Token product_delim = create_match_token(SymbTok, BarSym, NULL);

    if (match_token(next_token, product_delim))
    {
        *delim = product_delim;
        return Product;
    }
    else
    {
        *delim = sum_delim;
        return Sum;
    }
}

static void *parse_name_type_pair(ParserPointer *curr)
{
    cvector_vector_type(Token) matched_tokens = NULL;
    Token match_tokens[] = {
        create_match_token(IdentTok, -1, NULL),
        create_match_token(SymbTok, TypeSpecSym, NULL),
    };
    if (!get_match(curr, match_tokens, ARRAY_LEN(match_tokens), &matched_tokens))
    {
        compiler_error("expected 'identifier' : <type>");
    }

    NameTypePair *name_type = malloc(sizeof(NameTypePair));
    name_type->name = matched_tokens[0].token_type.ident;
    name_type->type = parse_type(curr);
    cvector_free(matched_tokens);
    return name_type;
}

void free_type(Type *type);
static void free_name_type_pair_ptr(void *elem)
{
    NameTypePair **name_type_pair = elem;
    free_type((*name_type_pair)->type);
    free(*name_type_pair);
}

static Type *parse_named_tuple_type(ParserPointer *curr)
{
    // first, verify that we have a '(', 'identifier' ':' sequence
    Token match_tokens[] = {
        create_match_token(SymbTok, LeftParenSym, NULL),
        create_match_token(IdentTok, -1, NULL),
        create_match_token(SymbTok, TypeSpecSym, NULL),
    };
    ParserPointer ptr = *curr;
    if (!get_match(&ptr, match_tokens, ARRAY_LEN(match_tokens), NULL))
    {
        return NULL;
    }
    *curr = parser_ptr_pop(*curr, NULL); // skip left paren

    Token delim;
    TupleKind tuple_kind = get_tuple_kind(parse_name_type_pair, curr, &delim);
    Token right_paren = create_match_token(SymbTok, RightParenSym, NULL);
    cvector_vector_type(void *) elems = parse_delimited(parse_name_type_pair, delim, right_paren, curr);

    NamedTupleT named_tuple = {.named_tuple = NULL, .tuple_kind = tuple_kind};
    cvector_set_elem_destructor(named_tuple.named_tuple, free_name_type_pair_ptr);
    for (int i = 0; i < cvector_size(elems); i++)
    {
        cvector_push_back(named_tuple.named_tuple, elems[i]);
    }
    cvector_free(elems);

    Type *type = malloc(sizeof(Type));
    type->type_kind = NamedTupleType;
    type->type_type.named_tuple = named_tuple;
    return type;
}

static void *parse_type_wrapper(ParserPointer *curr)
{
    return parse_type(curr);
}

static void free_type_ptr(void *elem)
{
    Type **type = elem;
    free_type(*type);
}

static Type *parse_tuple_type(ParserPointer *curr)
{
    Token *next_token;
    ParserPointer next = parser_ptr_pop(*curr, &next_token);
    if (!match_token(next_token, create_match_token(SymbTok, LeftParenSym, NULL)))
    {
        return NULL;
    }
    *curr = next;

    Token delim;
    TupleKind tuple_kind = get_tuple_kind(parse_type_wrapper, curr, &delim);
    Token right_paren = create_match_token(SymbTok, RightParenSym, NULL);
    cvector_vector_type(void *) elems = parse_delimited(parse_type_wrapper, delim, right_paren, curr);

    TupleT tuple = {.tuple = NULL, .tuple_kind = tuple_kind};
    cvector_set_elem_destructor(tuple.tuple, free_type_ptr);
    for (int i = 0; i < cvector_size(elems); i++)
    {
        cvector_push_back(tuple.tuple, elems[i]);
    }
    cvector_free(elems);

    Type *type = malloc(sizeof(Type));
    type->type_kind = TupleType;
    type->type_type.tuple = tuple;
    return type;
}

static Type *parse_literal_type(ParserPointer *curr)
{
    Token literal_types[] = {
        create_match_token(IdentTok, -1, "float"),
        create_match_token(IdentTok, -1, "int"),
        create_match_token(IdentTok, -1, "bool"),
        create_match_token(IdentTok, -1, "string"),
    };
    Token *next_token;
    ParserPointer next = parser_ptr_pop(*curr, &next_token);
    for (int i = 0; i < ARRAY_LEN(literal_types); i++)
    {
        if (match_token(next_token, literal_types[i]))
        {
            Type *type = malloc(sizeof(Type));
            type->type_kind = LitType;
            type->type_type.literal = i;
            *curr = next;
            return type;
        }
    }
    return NULL;
}

static Type *parse_defined_type(ParserPointer *curr)
{
    Token *next_token;
    ParserPointer next = parser_ptr_pop(*curr, &next_token);
    if (!match_token(next_token, create_match_token(IdentTok, -1, NULL)))
    {
        return NULL;
    }
    Type *type = malloc(sizeof(Type));
    type->type_kind = DefType;
    type->type_type.defined = next_token->token_type.ident;
    *curr = next;
    return type;
}

typedef Type *(*TypeParser)(ParserPointer *curr);
static Type *parse_type(ParserPointer *curr)
{
    TypeParser type_parsers[] = {
        parse_named_tuple_type,
        parse_tuple_type,
        parse_literal_type,
        parse_defined_type,
    };

    for (int i = 0; i < ARRAY_LEN(type_parsers); i++)
    {
        Type *type = type_parsers[i](curr);
        if (type)
        {
            return type;
        }
    }
    compiler_error("can't parse type");
    return NULL;
}

Expr *parse_expr(ParserPointer *curr)
{
    Token *token;
    parser_ptr_pop(*curr, &token);
    if (!token)
    {
        compiler_error("reached EOF while parsing expression");
    }
    if (match_token(token, create_match_token(LitTok, -1, NULL)))
    {
        return parse_literal(curr);
    }
    if (match_token(token, create_match_token(KwTok, LetKw, NULL)))
    {
        return parse_let(curr);
    }
    if (match_token(token, create_match_token(KwTok, TypeKw, NULL)))
    {
        return parse_type_def(curr);
    }
    if (match_token(token, create_match_token(KwTok, IfKw, NULL)))
    {
        return parse_if_else(curr);
    }
    if (match_token(token, create_match_token(KwTok, FnKw, NULL)))
    {
        return parse_fn_def(curr);
    }
    compiler_error("cannot parse expression");
    return NULL;
}

void free_type(Type *type)
{
    if (type->type_kind == TupleType)
    {
        cvector_free(type->type_type.tuple.tuple);
    }
    if (type->type_kind == NamedTupleType)
    {
        cvector_free(type->type_type.named_tuple.named_tuple);
    }
    free(type);
}
/*
    LiteralExpr,
    LetExpr,
    FnDefExpr,
    TypeDefExpr,
    IfElseExpr,
    VarExpr,
    FnCallExpr,
    TupleExpr
*/
void free_expr(Expr *expr)
{
    // LiteralExpr and VarExpr don't need to be freed here, since we just
    // take the token value. So when we free the token vector,
    // all LiteralExprs/VarExprs will be freed.
    if (expr->expr_kind == LetExpr)
    {
        free_expr(expr->expr_type.let.equal_expr);
        free_expr(expr->expr_type.let.next_expr);
    }
    if (expr->expr_kind == FnDefExpr)
    {
        Type *type = malloc(sizeof(Type));
        type->type_kind = NamedTupleType;
        type->type_type.named_tuple = expr->expr_type.fn_def.arguments;
        free_type(type);
        free_expr(expr->expr_type.fn_def.body);
        free_type(expr->expr_type.fn_def.return_type);
    }
    if (expr->expr_kind == TypeDefExpr)
    {
        free_type(expr->expr_type.type_def.type);
    }
    if (expr->expr_kind == IfElseExpr)
    {
        free_expr(expr->expr_type.if_else.condition);
        free_expr(expr->expr_type.if_else.if_body);
        free_expr(expr->expr_type.if_else.else_body);
    }
    if (expr->expr_kind == FnCallExpr)
    {
        Type *type = malloc(sizeof(Type));
        free_expr(expr->expr_type.fn_call.params);
    }
        
    free(expr);
}

// TESTS
void test_parse_let()
{
    char program_string[] = "let x = \"hello world\"; 5";
    cvector_vector_type(Token) tokens = tokenize(program_string);

    ParserPointer start = {.token_ptr = 0, .tokens = tokens};
    Expr *program = parse_expr(&start);

    assert(program->expr_kind == LetExpr);
    assert(strcmp(program->expr_type.let.variable, "x") == 0);
    assert(program->expr_type.let.equal_expr->expr_kind == LiteralExpr);
    assert(program->expr_type.let.equal_expr->expr_type.literal.literal_kind == StringLit);
    assert(strcmp(program->expr_type.let.equal_expr->expr_type.literal.literal_type.String, "hello world") == 0);
    assert(program->expr_type.let.next_expr->expr_kind == LiteralExpr);
    assert(program->expr_type.let.next_expr->expr_type.literal.literal_kind == IntLit);
    assert(program->expr_type.let.next_expr->expr_type.literal.literal_type.Int == 5);

    free_expr(program);
    cvector_free(tokens);
}

void test_parse_if_else()
{
    char program_string[] = "if true { 0 } else { 1 }";
    cvector_vector_type(Token) tokens = tokenize(program_string);

    ParserPointer start = {.token_ptr = 0, .tokens = tokens};
    Expr *program = parse_expr(&start);

    assert(program->expr_kind == IfElseExpr);
    assert(program->expr_type.if_else.condition->expr_kind == LiteralExpr);
    assert(program->expr_type.if_else.condition->expr_type.literal.literal_kind == BoolLit);
    assert(program->expr_type.if_else.condition->expr_type.literal.literal_type.Bool);

    assert(program->expr_type.if_else.if_body->expr_kind == LiteralExpr);
    assert(program->expr_type.if_else.if_body->expr_type.literal.literal_kind == IntLit);
    assert(program->expr_type.if_else.if_body->expr_type.literal.literal_type.Int == 0);

    assert(program->expr_type.if_else.else_body->expr_kind == LiteralExpr);
    assert(program->expr_type.if_else.else_body->expr_type.literal.literal_kind == IntLit);
    assert(program->expr_type.if_else.else_body->expr_type.literal.literal_type.Int == 1);
}

void test_parse_type()
{
    char type_string[] = "(int, string, (thing, bool, float),)";
    cvector_vector_type(Token) tokens = tokenize(type_string);
    ParserPointer start = {.token_ptr = 0, .tokens = tokens};

    Type *type = parse_type(&start);

    // verify that our type representation is correct
    assert(type->type_kind == TupleType);
    assert(type->type_type.tuple.tuple_kind == Sum);
    assert(type->type_type.tuple.tuple[0]->type_kind == LitType);
    assert(type->type_type.tuple.tuple[0]->type_type.literal == IntLit);
    assert(type->type_type.tuple.tuple[1]->type_kind == LitType);
    assert(type->type_type.tuple.tuple[1]->type_type.literal == StringLit);
    assert(type->type_type.tuple.tuple[2]->type_kind == TupleType);
    assert(type->type_type.tuple.tuple[2]->type_type.tuple.tuple[0]->type_kind == DefType);
    assert(strcmp(type->type_type.tuple.tuple[2]->type_type.tuple.tuple[0]->type_type.defined, "thing") == 0);
    assert(type->type_type.tuple.tuple[2]->type_type.tuple.tuple[1]->type_kind == LitType);
    assert(type->type_type.tuple.tuple[2]->type_type.tuple.tuple[1]->type_type.literal == BoolLit);
    assert(type->type_type.tuple.tuple[2]->type_type.tuple.tuple[2]->type_kind == LitType);
    assert(type->type_type.tuple.tuple[2]->type_type.tuple.tuple[2]->type_type.literal == FloatLit);

    free_type(type);
    cvector_free(tokens);
}

void test_parse_named_type()
{
    char type_string[] = "(x : int, y : int)";
    cvector_vector_type(Token) tokens = tokenize(type_string);
    ParserPointer start = {.token_ptr = 0, .tokens = tokens};

    Type *type = parse_type(&start);

    assert(type->type_kind == NamedTupleType);
    assert(type->type_type.named_tuple.tuple_kind == Sum);
    assert(strcmp(type->type_type.named_tuple.named_tuple[0]->name, "x") == 0);
    assert(type->type_type.named_tuple.named_tuple[0]->type->type_kind == LitType);
    assert(type->type_type.named_tuple.named_tuple[0]->type->type_type.literal == IntLit);
    assert(strcmp(type->type_type.named_tuple.named_tuple[1]->name, "y") == 0);
    assert(type->type_type.named_tuple.named_tuple[1]->type->type_kind == LitType);
    assert(type->type_type.named_tuple.named_tuple[1]->type->type_type.literal == IntLit);

    free_type(type);
    cvector_free(tokens);
}

void test_parse_variant_type()
{
    char type_string[] = "(int | string)";
    cvector_vector_type(Token) tokens = tokenize(type_string);
    ParserPointer start = {.token_ptr = 0, .tokens = tokens};

    Type *type = parse_type(&start);
    assert(type->type_kind == TupleType);
    assert(type->type_type.tuple.tuple_kind == Product);
    assert(type->type_type.tuple.tuple[0]->type_kind == LitType);
    assert(type->type_type.tuple.tuple[0]->type_type.literal == IntLit);
    assert(type->type_type.tuple.tuple[1]->type_kind == LitType);
    assert(type->type_type.tuple.tuple[1]->type_type.literal == StringLit);

    free_type(type);
    cvector_free(tokens);
}

void test_parse_named_variant_type()
{
    char type_string[] = "(x : int | y : int)";
    cvector_vector_type(Token) tokens = tokenize(type_string);
    ParserPointer start = {.token_ptr = 0, .tokens = tokens};

    Type *type = parse_type(&start);

    assert(type->type_kind == NamedTupleType);
    assert(type->type_type.named_tuple.tuple_kind == Product);
    assert(strcmp(type->type_type.named_tuple.named_tuple[0]->name, "x") == 0);
    assert(type->type_type.named_tuple.named_tuple[0]->type->type_kind == LitType);
    assert(type->type_type.named_tuple.named_tuple[0]->type->type_type.literal == IntLit);
    assert(strcmp(type->type_type.named_tuple.named_tuple[1]->name, "y") == 0);
    assert(type->type_type.named_tuple.named_tuple[1]->type->type_kind == LitType);
    assert(type->type_type.named_tuple.named_tuple[1]->type->type_type.literal == IntLit);

    free_type(type);
    cvector_free(tokens);
}

void test_parse_type_def()
{
    char program_string[] =
        "let Struct = type (x : int, y : int);"
        "let Tuple = type (float, float);"
        "let Variant = type (int | float );"
        "let Enum = type (BetweenZeroAndTen : int | Integer1 : int | Float : float);"
        "let NewType = type int;"
        "let A = type (int); let B = type (int,); let C = type (int|);"
        "let D = type (int|float|); let E = type (x : int|y : int|);"
        "let F = type (x : int, y : int,);"
        "0";

    cvector_vector_type(Token) tokens = tokenize(program_string);

    ParserPointer start = {.token_ptr = 0, .tokens = tokens};
    Expr *program = parse_expr(&start);
    while (program->expr_kind == LetExpr)
    {
        assert(program->expr_type.let.equal_expr->expr_kind == TypeDefExpr);
        program = program->expr_type.let.next_expr;
    }
}
