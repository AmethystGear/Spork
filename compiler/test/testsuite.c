#include <stdio.h>
#include "../src/lexer.h"
#include "../src/parser.h"
#define TEST(test_fn) test_fn(); printf("\033[32mpassed: "); printf(#test_fn); printf("\033[0m\n");

void lexer_testsuite() {
    TEST(test_lex_literals)
}

void parser_testsuite() {
    TEST(test_parse_let)
    TEST(test_parse_type)
    TEST(test_parse_named_type)
}

int main() {
    lexer_testsuite();
    parser_testsuite();
    printf("passed all tests!\n");
}