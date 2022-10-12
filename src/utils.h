#ifndef SPORK_UTILS_H_
#define SPORK_UTILS_H_

#include "literal.h"
#include "parser.h"
#include "interpreter.h"
//#include "typechecker.h"

#define ARRAY_LEN(array) (sizeof((array)) / sizeof((array)[0]))


char *read_file_to_string(char *filename);
void print_literal(Literal literal);
void print_expr(Expression *expr);
void print_atom(Atom atom);
void print_val(Val val);
char* remove_char_from_string(char* string, char c);
void syntax_error(char *message);
void type_error(char *message);
#endif