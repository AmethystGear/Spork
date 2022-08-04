#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "lexer.h"

// prints error in red text, adds newline, and exits
void fail(const char *error)
{
    fprintf(stderr, "\033[0;31m");
    fprintf(stderr, "%s", error);
    fprintf(stderr, "\n");
    fprintf(stderr, "\033[0m");
    exit(1);
}

// print "compiler bug: ", then fail with the provided error
void compiler_bug(const char *error)
{
    fprintf(stderr, "compiler bug: ");
    fail(error);
}

// print "error: ", then fail with the provided error
void compiler_error(const char *error)
{
    fprintf(stderr, "error: ");
    fail(error);
}

// print the provided list of tokens
void print_tokens(cvector_vector_type(Token) tokens)
{
    for (int i = 0; i < cvector_size(tokens); i++)
    {
        sds token_stringified = token_to_string(&tokens[i]);
        printf("%s\n", token_stringified);
        sdsfree(token_stringified);
    }
}
