#include <stdio.h>
#include "../src/lexer.h"
#include "../src/parser.h"
#define TEST(test_fn) test_fn(); printf("\033[32mpassed: "); printf(#test_fn); printf("\033[0m\n");

void lexer_testsuite() {
    TEST(test_lex_literals)
    TEST(test_lex_keywords)
}

void parser_testsuite() {
    TEST(test_parse_let)
    TEST(test_parse_type)
    TEST(test_parse_named_type)
    TEST(test_parse_variant_type)
    TEST(test_parse_named_variant_type)
    TEST(test_parse_type_def)
    TEST(test_parse_if_else)
}

int main() {
    TEST(lexer_testsuite)
    TEST(parser_testsuite)
}
