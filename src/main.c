#include <stdio.h>

#include "interpreter.h"
#include "parser.h"
#include "utils.h"

void assert_binary_int_op(Tuple tup) {
    assert(cvector_size(tup.values) == 2);
    assert(tup.values[0].kind == LiteralVal);
    assert(tup.values[1].kind == LiteralVal);
    assert(tup.values[0].type.lit.kind == IntLit);
    assert(tup.values[1].type.lit.kind == IntLit);
}

Val builtin_add(Tuple tup) {
    assert_binary_int_op(tup);
    return (Val){
        .kind = LiteralVal,
        .type.lit = (Literal){.kind = IntLit,
                              .type.Int = tup.values[0].type.lit.type.Int +
                                          tup.values[1].type.lit.type.Int}};
}

Val builtin_sub(Tuple tup) {
    assert_binary_int_op(tup);
    return (Val){
        .kind = LiteralVal,
        .type.lit = (Literal){.kind = IntLit,
                              .type.Int = tup.values[0].type.lit.type.Int -
                                          tup.values[1].type.lit.type.Int}};
}

Val builtin_mul(Tuple tup) {
    assert_binary_int_op(tup);
    return (Val){
        .kind = LiteralVal,
        .type.lit = (Literal){.kind = IntLit,
                              .type.Int = tup.values[0].type.lit.type.Int *
                                          tup.values[1].type.lit.type.Int}};
}

Val builtin_div(Tuple tup) {
    assert_binary_int_op(tup);
    return (Val){
        .kind = LiteralVal,
        .type.lit = (Literal){.kind = IntLit,
                              .type.Int = tup.values[0].type.lit.type.Int /
                                          tup.values[1].type.lit.type.Int}};
}

Val builtin_eq(Tuple tup) {
    assert_binary_int_op(tup);
    return (Val){
        .kind = LiteralVal,
        .type.lit = (Literal){.kind = BoolLit,
                              .type.Bool = tup.values[0].type.lit.type.Int ==
                                           tup.values[1].type.lit.type.Int}};
}

Val builtin_print(Tuple tup) {
    assert(cvector_size(tup.values) == 1);
    assert(tup.values[0].kind == LiteralVal);
    assert(tup.values[0].type.lit.kind == StringLit);
    printf("%s", tup.values[0].type.lit.type.String);
    return (Val){.kind = VoidVal};
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "provide one file to compile\n");
        abort();
    }
    char *program = read_file_to_string(argv[1]);
    if (program == NULL) {
        fprintf(stderr, "file %s does not exist", argv[1]);
    }

    Expression *expr = parse_expr(&program);

    cvector_vector_type(LexicalBinding) env = NULL;
    LexicalBinding binding;
    binding = (LexicalBinding){
        .symbol = "+",
        .boundValue = (Val){.kind = BuiltinFnVal, .type.bfn = builtin_add}};
    cvector_push_back(env, binding);

    binding = (LexicalBinding){
        .symbol = "-",
        .boundValue = (Val){.kind = BuiltinFnVal, .type.bfn = builtin_sub}};
    cvector_push_back(env, binding);

    binding = (LexicalBinding){
        .symbol = "==",
        .boundValue = (Val){.kind = BuiltinFnVal, .type.bfn = builtin_eq}};
    cvector_push_back(env, binding);

    binding = (LexicalBinding){
        .symbol = "*",
        .boundValue = (Val){.kind = BuiltinFnVal, .type.bfn = builtin_mul}};
    cvector_push_back(env, binding);

    binding = (LexicalBinding){
        .symbol = "/",
        .boundValue = (Val){.kind = BuiltinFnVal, .type.bfn = builtin_div}};
    cvector_push_back(env, binding);

    binding = (LexicalBinding){
        .symbol = "print",
        .boundValue = (Val){.kind = BuiltinFnVal, .type.bfn = builtin_print}};
    cvector_push_back(env, binding);



    print_val(eval(expr, &env));
    free_expr(expr);
}
