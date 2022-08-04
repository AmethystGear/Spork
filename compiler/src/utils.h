#ifndef SPORK_UTILS_H_
#define SPORK_UTILS_H_
#include "lexer.h"

#define ARRAY_LEN(array) (sizeof((array)) / sizeof((array)[0]))

void fail(const char *error);
void compiler_bug(const char *error);
void compiler_error(const char *error);
void print_tokens(cvector_vector_type(Token) tokens);
#endif