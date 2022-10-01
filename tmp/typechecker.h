#ifndef SPORK_TYPE_CHECKER_H_
#define SPORK_TYPE_CHECKER_H_
#include "literal.h"
#include "parser.h"
#include "../lib/hashmap/hashmap.h"

typedef struct Type Type;

typedef struct TupleT TupleT;
typedef struct TupleT {
    TupleT* next;
    Type* type;
} TupleT;

typedef struct Function {
    TupleT* args;
    Type* return_type;
} Function;

typedef union TypeType {
    TupleT* tuple;
    LiteralKind literal;
    Function func;
    Symbol defined;
} TypeType;

typedef enum TypeKind { TupType, LitType, DefType, FunType } TypeKind;

typedef struct Type {
    TypeKind kind;
    TypeType type;
} Type;

typedef struct Environment {
    struct hashmap* base_env;
    struct hashmap* dynamic_env;
} Environment;

typedef Type* (*TypeDeterminer)(Expression* expr, Environment* env);

typedef struct BaseEnvEntry {
    char* key;
    TypeDeterminer value;
} BaseEnvEntry;

typedef struct DynamicEnvEntry {
    char* key;
    Type* value;
} DynamicEnvEntry;

Type* get_type(Expression* expr, Environment* env);
#endif