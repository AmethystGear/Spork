#include <stdio.h>
#include "../src/parser.h"
#define TEST(test_fn) test_fn(); printf("passed: "); printf(#test_fn); printf("\n");

void parser_testsuite() {
    TEST(test_parse_let)
    TEST(test_parse_type)
    TEST(test_parse_named_type)
}

int main() {
    parser_testsuite();
    printf("passed all tests!\n");
}