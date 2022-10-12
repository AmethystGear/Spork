#include "literal.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "escape.h"
#include "utils.h"

/**
 * @brief given the token, return an IntLit Literal containing the int that the
 * token represents if the token is an int literal. If the token is not an int
 * literal, return an InvalidLit. A token is an int literal if it is of the form
 * "-?[0-9|_]+".
 *
 * @param token
 * @return Literal
 */
Literal match_int_literal(sds token) {
    if (strcmp(token, "-") == 0) {
        return (Literal){.kind = InvalidLit};
    }
    char* curr = token;
    if (*curr == '\0') {
        Literal literal;
        literal.kind = InvalidLit;
        return literal;
    }

    if (*curr == '-') {
        curr++;
    }
    while (isdigit(*curr) || *curr == '_') {
        curr++;
    }

    Literal literal;
    if (*curr == '\0') {
        literal.kind = IntLit;
        char* underscores_removed = remove_char_from_string(token, '_');
        literal.type.Int = strtol(underscores_removed, NULL, 10);
        free(underscores_removed);
    } else {
        literal.kind = InvalidLit;
    }
    return literal;
}

/**
 * @brief given the token, return a BoolLit Literal containing the bool that
 * the token represents if the token is an bool literal. If the token is not a
 * bool literal, return an InvalidLit. A token is a bool literal if it is  of
 * the form "true|false"
 *
 * @param token
 * @return Literal
 */
Literal match_bool_literal(sds token) {
    bool token_is_true = strcmp(token, "true") == 0;
    bool token_is_false = strcmp(token, "false") == 0;

    Literal literal;
    if (token_is_true || token_is_false) {
        literal.kind = BoolLit;
        literal.type.Bool = token_is_true;
    } else {
        literal.kind = InvalidLit;
    }
    return literal;
}

/**
 * @brief given the token, return a FloatLit Literal containing the float that
 * the token represents if the token is a float literal. If the token is not a
 * float literal, return an InvalidLit. A token is a float literal if it is of
 * the form "-?[0-9|_]+\.[0-9|_]+"
 *
 * @param token
 * @return Literal
 */
Literal match_float_literal(sds token) {
    char* curr = token;
    if (*curr == '-') {
        curr++;
    }
    bool used_decimal = false;
    while (isdigit(*curr) || *curr == '_' || (!used_decimal && *curr == '.')) {
        if (*curr == '.') {
            used_decimal = true;
        }
        curr++;
    }
    Literal literal;
    if (*curr == '\0' && used_decimal && *(curr - 1) != '.' && *token != '.') {
        literal.kind = FloatLit;
        char* underscores_removed = remove_char_from_string(token, '_');
        literal.type.Float = strtod(underscores_removed, NULL);
        free(underscores_removed);
    } else {
        literal.kind = InvalidLit;
    }
    return literal;
}

/**
 * @brief given the token, return a StringLit Literal containing the string that
 * the token represents if the token is a string literal. If the token is not a
 * string literal, return an InvalidLit. A token is a string literal if it
 * starts with '"'.
 *
 * @param token
 * @return Literal
 */
Literal match_string_literal(sds token) {
    Literal literal;
    if (*token == '"') {
        literal.kind = StringLit;
        token[sdslen(token) - 1] = '\0';
        literal.type.String = unescape(token + 1);
        token[sdslen(token) - 1] = '"';
    } else {
        literal.kind = InvalidLit;
    }
    return literal;
}

/// @brief a function pointer to a function that matches literals from strings.
typedef Literal (*LiteralMatcher)(sds token);

/**
 * @brief try to get a Literal from the provided token. If the token is not any
 * of the literals (bool, int, string, float), return an InvalidLit.
 *
 * @param token
 * @return Literal
 */
Literal match_literal(sds token) {
    LiteralMatcher literal_matchers[] = {
        match_int_literal,
        match_bool_literal,
        match_float_literal,
        match_string_literal,
    };
    for (int i = 0; i < ARRAY_LEN(literal_matchers); i++) {
        Literal literal = literal_matchers[i](token);
        if (literal.kind != InvalidLit) {
            return literal;
        }
    }
    return (Literal){.kind = InvalidLit};
}

void free_literal(Literal literal) {
    if (literal.kind == StringLit) {
        sdsfree(literal.type.String);
    }
}

// TESTS
void test_match_int_literal() {
    Literal lit;
    lit = match_int_literal("100");
    assert(lit.kind == IntLit);
    assert(lit.type.Int == 100);

    // test negative sign
    lit = match_int_literal("-100");
    assert(lit.kind == IntLit);
    assert(lit.type.Int == -100);

    // test underscores
    lit = match_int_literal("__1____0____0______");
    assert(lit.kind == IntLit);
    assert(lit.type.Int == 100);

    // these things aren't floats, so we should get InvalidLit
    char* tests[] = {"-100.0", "", "aaaa"};
    for (int i = 0; i < ARRAY_LEN(tests); i++) {
        lit = match_int_literal(tests[i]);
        assert(lit.kind == InvalidLit);
    }
}

void test_match_float_literal() {
    Literal lit;
    lit = match_float_literal("100.0");
    assert(lit.kind == FloatLit);
    assert(lit.type.Float == 100.0);

    // test negative sign
    lit = match_float_literal("-100.0");
    assert(lit.kind == FloatLit);
    assert(lit.type.Float == -100.0);

    // test underscores
    lit = match_float_literal("__1____0____0___.0___");
    assert(lit.kind == FloatLit);
    assert(lit.type.Float == 100.0);

    // these things aren't floats, so we should get InvalidLit
    char* tests[] = {"100", "", "aaaa"};
    for (int i = 0; i < ARRAY_LEN(tests); i++) {
        lit = match_float_literal(tests[i]);
        assert(lit.kind == InvalidLit);
    }
}

void test_match_bool_literal() {
    Literal lit;
    lit = match_bool_literal("true");
    assert(lit.kind == BoolLit);
    assert(lit.type.Bool);

    lit = match_bool_literal("false");
    assert(lit.kind == BoolLit);
    assert(!lit.type.Bool);

    // these things aren't bools, so we should get InvalidLit
    char* tests[] = {"true__", "false__", "100", "", "aaaa"};
    for (int i = 0; i < ARRAY_LEN(tests); i++) {
        lit = match_bool_literal(tests[i]);
        assert(lit.kind == InvalidLit);
    }
}

void test_match_string_literal() {
    Literal lit;
    lit = match_string_literal("\"\"");
    assert(lit.kind == StringLit);
    assert(strcmp(lit.type.String, "") == 0);

    lit = match_string_literal("\"abcdefg\"");
    assert(lit.kind == StringLit);
    assert(strcmp(lit.type.String, "abcdefg") == 0);
}