#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "lexer.h"
#include "../lib/sds/sds.h"
#include "../lib/cvector.h"
#include "macros.h"

typedef char *(*Matcher)(char *match_string, char *text_ptr);

bool match_symbol(char **text_ptr, Symbol *symb);

// exactly match provided string to text
// if exact match, return advanced text_ptr, otherwise return original text_ptr.
char *match_exact(char *match_string, char *text_ptr)
{
    size_t match_len = strlen(match_string);
    for (int i = 0; i < match_len; i++)
    {
        if (match_string[i] != text_ptr[i])
        {
            return text_ptr;
        }
    }
    return text_ptr + match_len;
}

// given a provided matcher and an array of strings, find the first match to the text.
// out parameters: reference to text_ptr, match_index.
// return if there is any match.
bool match_first(Matcher matcher, char *strings[], int n_strings, char **text_ptr, int *match_index)
{
    for (int i = 0; i < n_strings; i++)
    {
        char *new_text_ptr = matcher(strings[i], *text_ptr);
        if (new_text_ptr != *text_ptr)
        {
            *text_ptr = new_text_ptr;
            *match_index = i;
            return true;
        }
    }
    return false;
}

// confirms if the end of the token is properly terminated.
// this means the string must be '\0', whitespace, or a symbol.
bool proper_token_termination(char *after_token_end)
{
    Symbol _symbol;
    return *after_token_end == '\0' || isspace(*after_token_end) || match_symbol(&after_token_end, &_symbol);
}

// exactly match provided string to text, and confirm that there is no continuation of the token
// after it ends. (new symbol token start, end of string, or whitespace).
// if exact match, return advanced text_ptr, otherwise return original text_ptr.
char *match_exact_confirm_terminate(char *match_string, char *text_ptr)
{
    char *new_text_ptr = match_exact(match_string, text_ptr);
    if (new_text_ptr == text_ptr)
    {
        return text_ptr;
    }
    return proper_token_termination(new_text_ptr) ? new_text_ptr : text_ptr;
}

//
bool match_type(Matcher matcher, char *strings[], int n_strings, char **text_ptr, void *type)
{
    int match_index = -1;
    bool matched = match_first(matcher, strings, n_strings, text_ptr, &match_index);
    if (matched)
    {
        *(int *)type = match_index;
    }
    return matched;
}

// array of all the string symbols.
// see Symbol in lexer.h to find what each of these map to.
char *symbols[] = {
    "(",
    ")",
    "{",
    "}",
    ":",
    ",",
    ";",
    "==",
    "=",
    "->",
    "+",
    "-",
    "*",
    "/",
    "&&",
    "||",
    "!",
};

// match any symbol.
// out parameter: punc - will contain matched symbol if any symbol matches.
// return true if there is any match else false.
bool match_symbol(char **text_ptr, Symbol *symb)
{
    return match_type(match_exact, symbols, ARRAY_LEN(symbols), text_ptr, (void *)symb);
}

// array of all the string keywords.
// see Keyword in lexer.h to find what each of these map to.
char *keywords[] = {
    "fn",
    "let",
    "if",
    "else",
    "type",
    "struct",
};

// match any keyword. checks for proper token termination (see `proper_token_termination`).
// out parameters:
//     kw - will contain matched keyword if any keyword matches.
//     text_ptr - is updated if there is a match.
// return true if there is any match else false.
bool match_kw(char **text_ptr, KeyWord *kw)
{
    return match_type(match_exact_confirm_terminate, keywords, ARRAY_LEN(keywords), text_ptr, (void *)kw);
}

// match a boolean literal. checks for proper token termination (see `proper_token_termination`).
// out parameters:
//     lit - will contain matched boolean if "true" or "false" matches.
//     text_ptr - is updated if there is a match.
// return true if there is any match else false.
bool match_bool(char **text_ptr, Literal *lit)
{
    char *boolean[] = {"true", "false"};
    bool boolean_value[] = {true, false};
    int match_index = -1;
    bool matched = match_first(match_exact_confirm_terminate, boolean, ARRAY_LEN(boolean), text_ptr, &match_index);
    if (matched)
    {
        lit->literal_kind = BoolLit;
        lit->literal_type.Bool = boolean_value[match_index];
    }
    return matched;
}

// match an integer literal. checks for proper token termination (see `proper_token_termination`).
// out parameters:
//     lit - will contain matched integer if the following text is only digits.
//     text_ptr - is updated if there is a match.
// return true if there is any match else false.
bool match_int(char **text_ptr, Literal *lit)
{
    char *curr = *text_ptr;
    while (isdigit(*curr))
    {
        curr++;
    }
    bool matched = curr != *text_ptr && proper_token_termination(curr);
    if (matched)
    {
        lit->literal_kind = IntLit;
        char tmp = *curr;
        *curr = '\0';
        lit->literal_type.Int = atoi(*text_ptr);
        *curr = tmp;
        *text_ptr = curr;
    }
    return matched;
}

// match a float literal. checks for proper token termination (see `proper_token_termination`).
// out parameters:
//     lit - will contain matched float if the following text is only digits (and a single decimal point).
//     text_ptr - is updated if there is a match.
// return true if there is any match else false.
bool match_float(char **text_ptr, Literal *lit)
{
    char *curr = *text_ptr;
    bool has_decimal = false;
    while (isdigit(*curr) || *curr == '.')
    {
        if (*curr == '.')
        {
            if (has_decimal)
            {
                return false;
            }
            else
            {
                has_decimal = true;
            }
        }
        curr++;
    }
    // if there either is no decimal, or we end on the decimal point,
    // this isn't a valid float.
    if (!has_decimal || curr != *text_ptr || *(curr - 1) == '.' || !proper_token_termination(curr))
    {
        return false;
    }

    lit->literal_kind = FloatLit;
    char tmp = *curr;
    *curr = '\0';
    lit->literal_type.Float = atof(*text_ptr);
    *curr = tmp;
    *text_ptr = curr;
    return true;
}

bool match_string(char **text_ptr, Literal *lit)
{
    char *curr = *text_ptr;
    if (*curr != '"')
    {
        return false;
    }
    curr++;
    while (*curr != '"' || *(curr - 1) == '\\')
    {
        if (*curr == '\n')
        {
            return false;
        }
        curr++;
    }

    bool matched = proper_token_termination(curr + 1);
    if (matched)
    {
        lit->literal_kind = StringLit;
        *curr = '\0';
        lit->literal_type.String = sdsnew((*text_ptr) + 1);
        *curr = '"';
        *text_ptr = curr + 1;
    }
    return matched;
}

typedef bool (*LiteralMatcher)(char **text_ptr, Literal *lit);
bool match_literal(char **text_ptr, Literal *lit)
{
    LiteralMatcher literal_matchers[] = {
        match_bool,
        match_int,
        match_float,
        match_string,
    };

    for (int i = 0; i < ARRAY_LEN(literal_matchers); i++)
    {
        if (literal_matchers[i](text_ptr, lit))
        {
            return true;
        }
    }
    return false;
}

bool match_ident(char **text_ptr, Identifier *ident)
{
    char *curr = *text_ptr;
    while (!proper_token_termination(curr))
    {
        curr++;
    }
    bool matched = curr != *text_ptr;
    if (matched)
    {
        char tmp = *curr;
        *curr = '\0';
        *ident = sdsnew(*text_ptr);
        *curr = tmp;
        *text_ptr = curr;
    }
    return matched;
}

bool match_token(char **text_ptr, Token *token)
{
    Symbol symbol;
    if (match_symbol(text_ptr, &symbol))
    {
        token->token_kind = SymbTok;
        token->token_type.symb = symbol;
        return true;
    }

    Literal literal;
    if (match_literal(text_ptr, &literal))
    {
        token->token_kind = LitTok;
        token->token_type.lit = literal;
        return true;
    }

    KeyWord kw;
    if (match_kw(text_ptr, &kw))
    {
        token->token_kind = KwTok;
        token->token_type.kw = kw;
        return true;
    }

    Identifier ident;
    if (match_ident(text_ptr, &ident))
    {
        token->token_kind = IdentTok;
        token->token_type.ident = ident;
        return true;
    }
    return false;
}

void free_token(void *elem)
{
    if (elem == NULL)
    {
        return;
    }
    Token *token = elem;
    if (token->token_kind == LitTok && token->token_type.lit.literal_kind == StringLit)
    {
        sdsfree(token->token_type.lit.literal_type.String);
    }
    if (token->token_kind == IdentTok)
    {
        sdsfree(token->token_type.ident);
    }
}

cvector_vector_type(Token) tokenize(char *s)
{
    int len = strlen(s);
    char *text_ptr = s;

    cvector_vector_type(Token) tokens = NULL;
    cvector_set_elem_destructor(tokens, free_token);
    while ((text_ptr - s) < len)
    {
        while (isspace(*text_ptr))
            text_ptr++;

        Token token;
        if (!match_token(&text_ptr, &token))
        {
            fprintf(stderr, "error parsing token\n");
            abort();
        }
        cvector_push_back(tokens, token);

        while (isspace(*text_ptr))
            text_ptr++;
    }
    return tokens;
}

sds symbol_to_string(void *symbol)
{
    return sdsnew(symbols[*(int *)symbol]);
}

sds keyword_to_string(void *kw)
{
    return sdsnew(keywords[*(int *)kw]);
}

sds ident_to_string(void *ident)
{
    return sdsdup(*(sds *)ident);
}

sds literal_to_string(void *lit)
{
    Literal *l = (Literal *)lit;
    switch (l->literal_kind)
    {
    case BoolLit:
        return sdsnew(l->literal_type.Bool ? "bool true" : "bool false");
    case IntLit:
        return sdscatprintf(sdsempty(), "int %d", l->literal_type.Int);
    case FloatLit:
        return sdscatprintf(sdsempty(), "float %e", l->literal_type.Float);
    case StringLit:
        return sdscatfmt(sdsempty(), "string \"%S\"", l->literal_type.String);
    };
    return sdsnew("unknown literal");
}

typedef sds (*TokenStringifier)(void *);
sds token_to_string(Token *token)
{
    char *token_kinds[] = {
        "keyword",
        "symbol",
        "literal",
        "identifier",
    };

    TokenStringifier stringifiers[] = {
        keyword_to_string,
        symbol_to_string,
        literal_to_string,
        ident_to_string,
    };

    void *token_types[] = {
        &token->token_type.kw,
        &token->token_type.symb,
        &token->token_type.lit,
        &token->token_type.ident,
    };

    sds stringified = stringifiers[token->token_kind](token_types[token->token_kind]);
    sds token_str = sdscatfmt(sdsempty(), "%s %S", token_kinds[token->token_kind], stringified);
    sdsfree(stringified);
    return token_str;
}

