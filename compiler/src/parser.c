#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "lexer.h"
#include "parser.h"
#include "macros.h"

ParserPointer parser_ptr_pop(ParserPointer ptr, Token *token)
{
    *token = ptr.tokens[ptr.token_ptr];
    ParserPointer next = {ptr.tokens, ptr.token_ptr + 1};
    return next;
}

bool match_token_(Token token, Token match)
{
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

Token create_match_token(TokenKind token_kind, int kind, sds ident)
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

bool get_match(ParserPointer start, ParserPointer *end, Token *match_tokens, int n_tokens, cvector_vector_type(Token) matched_tokens)
{
    *end = start;
    for (int i = 0; i < n_tokens; i++)
    {
        Token next_token;
        *end = parser_ptr_pop(*end, &next_token);
        if (!match_token_(match_tokens[i], next_token))
        {
            return false;
        }
        else if (matched_tokens != NULL)
        {
            cvector_push_back(matched_tokens, next_token);
        }
    }
    return true;
}

Expr* parse_let(ParserPointer start, ParserPointer *end)
{
    ParserPointer curr;
    cvector_vector_type(Token) matched_tokens = NULL;
    Token match_tokens[] = {
        create_match_token(KwTok, LetKw, NULL),
        create_match_token(IdentTok, -1, NULL),
        create_match_token(SymbTok, EqSym, NULL),
    };
    if (!get_match(start, &curr, match_tokens, ARRAY_LEN(match_tokens), matched_tokens))
    {
        return NULL;
    }
    Expr *equal_expr = parse_expr(curr, &curr);
    if (!equal_expr)
    {
        fprintf(stderr, "could not parse let expression");
        abort();
    }
    Token match_terminator[] = {create_match_token(SymbTok, TermSym, NULL)};
    if (!get_match(start, &curr, match_terminator, ARRAY_LEN(match_terminator), NULL))
    {
        fprintf(stderr, "let expression doesn't have a terminator");
        abort();
    }
    Expr *next_expr = parse_expr(curr, &curr);
    if (!next_expr)
    {
        fprintf(stderr, "could not parse expression after let expression");
        abort();
    }

    Let let_expr = {
        .variable = matched_tokens[1].token_type.ident,
        .equal_expr = equal_expr,
        .next_expr = next_expr,
    };

    Expr *expr = malloc(sizeof(Expr));
    expr->expr_kind = LetExpr;
    expr->expr_type.let = let_expr;
    *end = curr;
    return expr;
}

Expr* parse_literal(ParserPointer start, ParserPointer *end) {
    ParserPointer curr;
    cvector_vector_type(Token) matched_token = NULL;
    Token match_literal[] = {
        create_match_token(LitTok, -1, NULL),
    };
    if (!get_match(start, &curr, match_literal, ARRAY_LEN(match_literal), matched_token))
    {
        return NULL;
    }

    Expr *expr = malloc(sizeof(Expr));
    expr->expr_kind = LiteralExpr;
    expr->expr_type.literal = matched_token[0].token_type.lit;
    *end = curr;
    return expr;
}

Expr* parse_expr(ParserPointer start, ParserPointer *end)
{   
    Token token;
    parser_ptr_pop(start, &token);
    if (match_token_(token, create_match_token(LitTok, -1, NULL))) {
        return parse_literal(start, end);
    }
    if (match_token_(token, create_match_token(KwTok, LetKw, NULL))) {
        return parse_let(start, end);
    }
    return NULL;
}

int main()
{
    char string[] = "let x = \"hello world\"; 5";
    cvector_vector_type(Token) tokens = tokenize(string);
    for (int i = 0; i < cvector_size(tokens); i++)
    {
        sds token_stringified = token_to_string(&tokens[i]);
        printf("%s\n", token_stringified);
        sdsfree(token_stringified);
    }

    ParserPointer start = {.token_ptr = 0, .tokens = tokens};
    ParserPointer end;
    Expr* program = parse_expr(start, &end);

    assert(program->expr_kind == LetExpr);
    assert(strcmp(program->expr_type.let.variable, "x") == 0);
    assert(program->expr_type.let.equal_expr->expr_kind == LiteralExpr);
    assert(program->expr_type.let.equal_expr->expr_type.literal.literal_kind == StringLit);
    assert(strcmp(program->expr_type.let.equal_expr->expr_type.literal.literal_type.String, "hello world") == 0);
    assert(program->expr_type.let.next_expr->expr_kind == LiteralExpr);
    assert(program->expr_type.let.next_expr->expr_type.literal.literal_kind == IntLit);
    assert(program->expr_type.let.next_expr->expr_type.literal.literal_type.Int == 5);

    //cvector_free(tokens);
    return 0;
}