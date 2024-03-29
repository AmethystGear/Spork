#include <stdio.h>
#include "../src/escape.h"
#include "../src/literal.h"

#define TEST(test_fn) test_fn(); printf("\033[32mpassed: "); printf(#test_fn); printf("\033[0m\n");

void escape_testsuite() {
    TEST(test_escape)
    TEST(test_unescape)
}

void literal_testsuite() {
    TEST(test_match_int_literal)
    TEST(test_match_float_literal)
    TEST(test_match_bool_literal)
}


int main() {
    TEST(escape_testsuite)
    TEST(literal_testsuite)
}