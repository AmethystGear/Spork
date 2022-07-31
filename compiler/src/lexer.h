#ifndef SPORK_LEXER_H_
#define SPORK_LEXER_H_
#include <stdbool.h>
#include "../lib/sds/sds.h"
#include "../lib/cvector.h"

typedef enum KeyWord
{
    FnKw,
    LetKw,
    IfKw,
    ElseKw,
    TypeKw,
    StructKw,
} KeyWord;

typedef enum
{
    LeftParenSym,
    RightParenSym,
    LeftScopeSym,
    RightScopeSym,
    TypeSpecSym,
    DelimSym,
    TermSym,
    EqSym,
    AssignSym,
    IntoSym,
    AddSym,
    SubSym,
    MulSym,
    DivSym,
    AndSym,
    OrSym,
    NotSym,
} Symbol;

typedef enum LiteralKind
{
    BoolLit,
    IntLit,
    FloatLit,
    StringLit,
} LiteralKind;

typedef union
{
    bool Bool;
    int Int;
    double Float;
    sds String;
} LiteralType;

typedef struct
{
    LiteralType literal_type;
    LiteralKind literal_kind;
} Literal;

typedef sds Identifier;

typedef enum TokenKind
{
    KwTok,
    SymbTok,
    LitTok,
    IdentTok,
} TokenKind;

typedef union TokenType
{
    KeyWord kw;
    Literal lit;
    Symbol symb;
    Identifier ident;
} TokenType;

typedef struct Token
{
    TokenType token_type;
    TokenKind token_kind;
    int line;
    int col;
} Token;

cvector_vector_type(Token) tokenize(char *s);
void free_token(void *elem);
sds token_to_string(Token *token);
#endif