#include "typechecker.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../lib/hashmap/hashmap.h"
#include "parser.h"
#include "utils.h"

static int entry_compare(const void* a, const void* b, void* udata) {
    const BaseEnvEntry* entry_a = a;
    const BaseEnvEntry* entry_b = b;
    return strcmp(entry_a->key, entry_b->key);
}

static uint64_t entry_hash(const void* item, uint64_t seed0, uint64_t seed1) {
    const BaseEnvEntry* entry = item;
    return hashmap_sip(entry->key, strlen(entry->key), seed0, seed1);
}

static int dynamic_entry_compare(const void* a, const void* b, void* udata) {
    const DynamicEnvEntry* entry_a = a;
    const DynamicEnvEntry* entry_b = b;
    return strcmp(entry_a->key, entry_b->key);
}

static uint64_t dynamic_entry_hash(const void* item, uint64_t seed0, ui]\nt64_t seed1) {
    const DynamicEnvEntry* entry = item;
    return hashmap_sip(entry->key, strlen(entry->key), seed0, seed1);
}

static int expr_len(Expression* expr) {
    int len = 0;
    Expression* curr = expr;
    while (curr != NULL) {
        curr = curr->next;
        len++;
    }
    return len;
}

static int tup_len(TupleT* expr) {
    int len = 0;
    TupleT* curr = expr;
    while (curr != NULL) {
        curr = curr->next;
        len++;
    }
    return len;
}

static bool same_type(Type* a, Type* b);

static bool same_TupleT(TupleT* a, TupleT* b) {
    while (a != NULL && b != NULL) {
        if (!same_type(a->type, b->type)) {
            return false;
        }
        a = a->next;
        b = b->next;
    }
    return a == NULL && b == NULL;
}

static bool same_type(Type* a, Type* b) {
    if (a->kind != b->kind) {
        return false;
    }
    switch (a->kind) {
        case LitType:
            return a->type.literal == b->type.literal;
        case DefType:
            return strcmp(a->type.defined, b->type.defined);
        case TupType:
            return same_TupleT(a->type.tuple, b->type.tuple);
        case FunType:
            return same_TupleT(a->type.func.args, b->type.func.args) &&
                   same_type(a->type.func.return_type,
                             b->type.func.return_type);
    }
}

static Environment get_empty_env();
static Environment get_base_env();
static Type* get_symbol_type(Expression* expr, Environment* env);
static Type* get_literal_type(Expression* expr);

static void free_env(Environment* env) {
    hashmap_free(env->base_env);
    hashmap_free(env->dynamic_env);
    free(env);
}

static TupleT* construct_tuple_type(int count, Type* types, ...) {
    va_list arg_ptr;
    va_start(arg_ptr, types);

    TupleT* tup_type = malloc(sizeof(TupleT));
    TupleT* curr = tup_type;
    curr->type = types;
    for (int i = 0; i < count - 1; i++) {
        curr->next = malloc(sizeof(TupleT));
        curr = curr->next;
        Type* arg = va_arg(arg_ptr, Type*);
        curr->type = arg;
    }
    curr->next = NULL;
    va_end(arg_ptr);
    return tup_type;
}

static Type* construct_function_type(TupleT* args, Type* return_type) {
    Type* func = malloc(sizeof(Type));
    func->kind = FunType;
    func->type.func = (Function){.args = args, .return_type = return_type};
    return func;
}

Type* get_type(Expression* expr, Environment* env) {
    bool created_env = false;
    if (env == NULL) {
        env = malloc(sizeof(Environment));
        *env = get_base_env();
        created_env = true;
    }

    Type* type;
    if (expr->data.kind == AtomData) {
        if (expr->data.type.atom.kind == SymbolAtom) {
            type = get_symbol_type(expr, env);
        } else {
            type = get_literal_type(expr);
        }
    } else {
        type = get_type(expr->data.type.expr, env);
    }

    if (expr->prev == NULL) {
        // we should be a function, operating on the rest of our args
        assert(type->kind == FunType);
        TupleT* args = type->type.func.args;
        Type* return_type = type->type.func.return_type;

        // typecheck our arguments
        TupleT* curr_arg = args;
        Expression* curr_expr = expr->next;
        while (curr_arg != NULL && curr_expr != NULL) {
            if (curr_arg->type != NULL) {
                Type* curr_expr_type = get_type(curr_expr, env);
                if (!same_type(curr_arg->type, curr_expr_type)) {
                    printf("type mismatch, expected:\n");
                    print_type(curr_arg->type);
                    printf("\nfound:\n");
                    print_type(curr_expr_type);
                    printf("\n");
                    abort();
                }
            }
            curr_expr = curr_expr->next;
            curr_arg = curr_arg->next;
        }
        if (curr_arg == NULL && curr_expr != NULL) {
            printf("extra args\n");
            abort();
        }
        if (curr_arg != NULL && curr_expr == NULL) {
            printf("missing args\n");
            abort();
        }
        // TODO: free args tuple
        return return_type;
    } else {
        return type;
    }

    if (created_env) {
        free_env(env);
    }
}

static Type* lit(LiteralKind literal_kind) {
    Type* literal = malloc(sizeof(Type));
    *literal = (Type){.kind = LitType, .type.literal = literal_kind};
    return literal;
}

static Type* get_if_type(Expression* expr, Environment* env) {
    Type* if_body_type = get_type(expr->next->next, env);
    return construct_function_type(
        construct_tuple_type(3, lit(BoolLit), if_body_type, if_body_type),
        if_body_type);
}

static Type* get_tuple_type(Expression* expr, Environment* env) {
    Expression* curr = expr->next;
    Type* tuple_type = malloc(sizeof(Type));
    tuple_type->kind = TupType;
    if (curr == NULL) {
        tuple_type->type.tuple = NULL;
        return construct_function_type(NULL, tuple_type);
    }

    tuple_type->type.tuple = malloc(sizeof(TupleT));
    TupleT* curr_tup = tuple_type->type.tuple;
    while (curr != NULL) {
        curr_tup->type = get_type(curr, env);
        curr = curr->next;
        if (curr != NULL) {
            curr_tup->next = malloc(sizeof(TupleT));
            curr_tup = curr_tup->next;
        } else {
            curr_tup->next = NULL;
        }
    }
    // TODO: make copy of tuple_type->type.tuple
    // (otherwise when we free it, we'll break the typechecker)
    return construct_function_type(tuple_type->type.tuple, tuple_type);
}

static Type* get_literal_type(Expression* expr) {
    assert(expr->data.kind == AtomData);
    assert(expr->data.type.atom.kind == LiteralAtom);
    Type* literal_type = malloc(sizeof(Type));
    literal_type->kind = LitType;
    literal_type->type.literal = expr->data.type.atom.type.literal.kind;
    return literal_type;
}

static Environment* clone_env(Environment* env) {
    Environment* cloned_env = malloc(sizeof(Environment));
    *cloned_env = get_empty_env();

    void* item;
    size_t i;

    i = 0;
    while (hashmap_iter(env->base_env, &i, &item)) {
        BaseEnvEntry* base_item = item;
        hashmap_set(cloned_env->base_env, base_item);
    }

    i = 0;
    while (hashmap_iter(env->dynamic_env, &i, &item)) {
        DynamicEnvEntry* dyn_item = item;
        hashmap_set(cloned_env->dynamic_env, dyn_item);
    }
    return cloned_env;
}

static Type* get_let_type(Expression* expr, Environment* env) {
    assert(expr_len(expr) == 4);
    DynamicEnvEntry new_entry = {.key = expr->next->data.type.atom.type.symbol,
                                 .value = get_type(expr->next->next, env)};

    Environment* cloned_env = clone_env(env);
    hashmap_set(cloned_env->dynamic_env, &new_entry);
    Type* return_type = get_type(expr->next->next->next, cloned_env);
    free_env(cloned_env);

    return construct_function_type(construct_tuple_type(3, NULL, NULL, NULL),
                                   return_type);
}

static Type* get_symbol_type(Expression* expr, Environment* env) {
    Symbol symbol = expr->data.type.atom.type.symbol;
    BaseEnvEntry base_key = {.key = symbol};
    DynamicEnvEntry dyn_key = {.key = symbol};
    BaseEnvEntry* base_entry = hashmap_get(env->base_env, &base_key);
    DynamicEnvEntry* dyn_entry = hashmap_get(env->dynamic_env, &dyn_key);
    if (base_entry != NULL) {
        TypeDeterminer type_determiner = base_entry->value;
        return type_determiner(expr, env);
    } else if (dyn_entry != NULL) {
        Type* type = dyn_entry->value;
        return type;
    } else {
        fprintf(stderr, "type error: symbol \"%s\" is not defined\n", symbol);
        abort();
    }
}

static Environment get_empty_env() {
    return (Environment){
        .base_env = hashmap_new(sizeof(BaseEnvEntry), 0, 0, 0, entry_hash,
                                entry_compare, NULL, NULL),
        .dynamic_env =
            hashmap_new(sizeof(DynamicEnvEntry), 0, 0, 0, dynamic_entry_hash,
                        dynamic_entry_compare, NULL, NULL)};
}

static Environment get_base_env() {
    Environment env = get_empty_env();
    hashmap_set(env.base_env,
                &(BaseEnvEntry){.key = "if", .value = get_if_type});
    hashmap_set(env.base_env,
                &(BaseEnvEntry){.key = "let", .value = get_let_type});
    hashmap_set(env.base_env,
                &(BaseEnvEntry){.key = ".", .value = get_tuple_type});

    Type* binaryIntOp = construct_function_type(
        construct_tuple_type(2, lit(IntLit), lit(IntLit)), lit(IntLit));

    hashmap_set(env.dynamic_env,
                &(DynamicEnvEntry){.key = "+", .value = binaryIntOp});
    hashmap_set(env.dynamic_env,
                &(DynamicEnvEntry){.key = "-", .value = binaryIntOp});
    hashmap_set(env.dynamic_env,
                &(DynamicEnvEntry){.key = "*", .value = binaryIntOp});
    hashmap_set(env.dynamic_env,
                &(DynamicEnvEntry){.key = "/", .value = binaryIntOp});

    Type* binaryIntBoolOp = construct_function_type(
        construct_tuple_type(2, lit(IntLit), lit(IntLit)), lit(BoolLit));

    hashmap_set(env.dynamic_env,
                &(DynamicEnvEntry){.key = "==", .value = binaryIntBoolOp});
    hashmap_set(env.dynamic_env,
                &(DynamicEnvEntry){.key = ">", .value = binaryIntBoolOp});
    hashmap_set(env.dynamic_env,
                &(DynamicEnvEntry){.key = "<", .value = binaryIntBoolOp});
    return env;
}