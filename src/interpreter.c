#include "interpreter.h"

#include <stdio.h>

#include "parser.h"
#include "utils.h"

static Val get_binding(Symbol symbol, cvector_vector_type(LexicalBinding) env) {
    for (int i = cvector_size(env) - 1; i >= 0; i--) {
        if (strcmp(env[i].symbol, symbol) == 0) {
            return env[i].boundValue;
        }
    }
    printf("trying to get value of binding that doesn't exist!\n");
    abort();
}

static Symbol get_as_symbol(Expression* expr) {
    return (expr->atomic && expr->data.atom.kind == SymbolAtom)
               ? expr->data.atom.type.symbol
               : NULL;
}

typedef bool (*SpecialFormHandler)(Expression* expr,
                                   cvector_vector_type(LexicalBinding) * env,
                                   Val* val);

typedef struct SpecialForm {
    char* name;
    SpecialFormHandler handler;
} SpecialForm;

static bool handle_comment(Expression* expr,
                           cvector_vector_type(LexicalBinding) * env,
                           Val* val) {
    return false;
}

static bool handle_let(Expression* expr,
                       cvector_vector_type(LexicalBinding) * env, Val* val) {
    assert(cvector_size(expr->data.expr) == 3);
    Symbol variable = get_as_symbol(expr->data.expr[1]);
    assert(variable != NULL);
    LexicalBinding binding = {.symbol = variable,
                              .boundValue = eval(expr->data.expr[2], env)};
    cvector_push_back(*env, binding);
    return false;
}

static bool handle_if(Expression* expr,
                      cvector_vector_type(LexicalBinding) * env, Val* val) {
    assert(cvector_size(expr->data.expr) == 4);
    Val condition = eval(expr->data.expr[1], env);
    assert(condition.kind == LiteralVal && condition.type.lit.kind == BoolLit);
    if (condition.type.lit.type.Bool) {
        *val = eval(expr->data.expr[2], env);
    } else {
        *val = eval(expr->data.expr[3], env);
    }
    return true;
}

static bool handle_fn(Expression* expr,
                      cvector_vector_type(LexicalBinding) * env, Val* val) {
    assert(cvector_size(expr->data.expr) == 3);
    assert(!expr->data.expr[1]->atomic);
    cvector_vector_type(Expression*) param_expr = expr->data.expr[1]->data.expr;
    cvector_vector_type(Symbol) params = NULL;
    for (int i = 0; i < cvector_size(param_expr); i++) {
        assert(param_expr[i]->atomic);
        assert(param_expr[i]->data.atom.kind == SymbolAtom);
        cvector_push_back(params, param_expr[i]->data.atom.type.symbol);
    }
    *val = (Val){.kind = FnVal,
                 .type.fn = (Fn){.args = params, .body = expr->data.expr[2]}};
    return true;
}

SpecialForm special_forms[] = {{.name = "#", .handler = handle_comment},
                               {.name = "let", .handler = handle_let},
                               {.name = "fn", .handler = handle_fn},
                               {.name = "if", .handler = handle_if}};

Val eval(Expression* expr, cvector_vector_type(LexicalBinding) * env) {
    if (expr->atomic) {
        if (expr->data.atom.kind == SymbolAtom) {
            return get_binding(expr->data.atom.type.symbol, *env);
        } else {
            return (Val){.type.lit = expr->data.atom.type.literal,
                         .kind = LiteralVal};
        }
    } else {
        // special forms
        Symbol first_symbol = get_as_symbol(expr->data.expr[0]);
        if (first_symbol) {
            for (int i = 0; i < ARRAY_LEN(special_forms); i++) {
                if (strcmp(special_forms[i].name, first_symbol) == 0) {
                    Val value;
                    if (special_forms[i].handler(expr, env, &value)) {
                        return value;
                    } else if (expr->chain) {
                        return eval(expr->chain, env);
                    } else {
                        return (Val){.kind = VoidVal};
                    }
                }
            }
        }

        // normal functions (includes builtin functions)
        Val fn = eval(expr->data.expr[0], env);
        assert(fn.kind == BuiltinFnVal || fn.kind == FnVal);

        Tuple args = {.values = NULL};
        for (int i = 1; i < cvector_size(expr->data.expr); i++) {
            cvector_push_back(args.values, eval(expr->data.expr[i], env));
        }

        Val fn_return_value;
        if (fn.kind == BuiltinFnVal) {
            fn_return_value = fn.type.bfn(args);
        } else {
            for (int i = 0; i < cvector_size(fn.type.fn.args); i++) {
                LexicalBinding binding = {.symbol = fn.type.fn.args[i],
                                          .boundValue = args.values[i]};
                cvector_push_back(*env, binding);
            }
            fn_return_value = eval(fn.type.fn.body, env);
        }

        if (expr->chain) {
            return eval(expr->chain, env);
        } else {
            return fn_return_value;
        }
    }
}