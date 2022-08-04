#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "../lib/sds/sds.h"
#include "lexer.h"
#include "parser.h"
#include "utils.h"

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
    if (!match_token(next_token, create_match_token(LitTok, -1, NULL)))
    {
        compiler_bug("trying to parse a literal from a token that isn't a literal");
    }
    Expr *expr = malloc(sizeof(Expr));
    expr->expr_kind = LiteralExpr;
    expr->expr_type.literal = next_token->token_type.lit;
    return expr;
}

typedef void *(*DelimParser)(ParserPointer *curr);
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
            break;
        }
    }
    *curr = parser_ptr_pop(*curr, &next_token);
    return elems;
}

static Type *parse_type(ParserPointer *curr);
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

    Token delim = create_match_token(SymbTok, DelimSym, NULL);
    Token right_paren = create_match_token(SymbTok, RightParenSym, NULL);
    cvector_vector_type(void *) elems = parse_delimited(parse_name_type_pair, delim, right_paren, curr);

    NamedTuple named_tuple = NULL;
    cvector_set_elem_destructor(named_tuple, free_name_type_pair_ptr);
    for (int i = 0; i < cvector_size(elems); i++)
    {
        cvector_push_back(named_tuple, elems[i]);
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
    Token delim = create_match_token(SymbTok, DelimSym, NULL);
    Token right_paren = create_match_token(SymbTok, RightParenSym, NULL);
    cvector_vector_type(void *) elems = parse_delimited(parse_type_wrapper, delim, right_paren, curr);

    Tuple tuple = NULL;
    cvector_set_elem_destructor(tuple, free_type_ptr);
    for (int i = 0; i < cvector_size(elems); i++)
    {
        cvector_push_back(tuple, elems[i]);
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
        create_match_token(IdentTok, -1, "int"),
        create_match_token(IdentTok, -1, "bool"),
        create_match_token(IdentTok, -1, "float"),
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
    compiler_error("cannot parse expression");
    return NULL;
}

void free_type(Type *type)
{
    if (type->type_kind == TupleType)
    {
        cvector_free(type->type_type.tuple);
    }
    if (type->type_kind == NamedTupleType)
    {
        cvector_free(type->type_type.named_tuple);
    }
    free(type);
}

void free_expr(Expr *expr)
{
    if (expr->expr_kind == LetExpr)
    {
        free_expr(expr->expr_type.let.equal_expr);
        free_expr(expr->expr_type.let.next_expr);
    }
    free(expr);
}

// TESTS
void test_parse_let()
{
    char program_string[] = "let x = \"hello world\"; 5";
    cvector_vector_type(Token) tokens = tokenize(program_string);
    cvector_set_elem_destructor(tokens, free_token);

    ParserPointer start = {.token_ptr = 0, .tokens = tokens};
    Expr *program = parse_expr(&start);

    // verify that our parse tree representation is correct
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

void test_parse_type()
{
    char type_string[] = "(int, string, (thing, bool, float),)";
    cvector_vector_type(Token) tokens = tokenize(type_string);
    ParserPointer start = {.token_ptr = 0, .tokens = tokens};

    Type *type = parse_type(&start);

    // verify that our type representation is correct
    assert(type->type_kind == TupleType);
    assert(type->type_type.tuple[0]->type_kind == LitType);
    assert(type->type_type.tuple[0]->type_type.literal == IntLit);
    assert(type->type_type.tuple[1]->type_kind == LitType);
    assert(type->type_type.tuple[1]->type_type.literal == StringLit);
    assert(type->type_type.tuple[2]->type_kind == TupleType);
    assert(type->type_type.tuple[2]->type_type.tuple[0]->type_kind == DefType);
    assert(strcmp(type->type_type.tuple[2]->type_type.tuple[0]->type_type.defined, "thing") == 0);
    assert(type->type_type.tuple[2]->type_type.tuple[1]->type_kind == LitType);
    assert(type->type_type.tuple[2]->type_type.tuple[1]->type_type.literal == BoolLit);
    assert(type->type_type.tuple[2]->type_type.tuple[2]->type_kind == LitType);
    assert(type->type_type.tuple[2]->type_type.tuple[2]->type_type.literal == FloatLit);

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
    assert(strcmp(type->type_type.named_tuple[0]->name, "x") == 0);
    assert(type->type_type.named_tuple[0]->type->type_kind == LitType);
    assert(type->type_type.named_tuple[0]->type->type_type.literal == IntLit);
    assert(strcmp(type->type_type.named_tuple[1]->name, "y") == 0);
    assert(type->type_type.named_tuple[1]->type->type_kind == LitType);
    assert(type->type_type.named_tuple[1]->type->type_type.literal == IntLit);

    free_type(type);
    cvector_free(tokens);
}