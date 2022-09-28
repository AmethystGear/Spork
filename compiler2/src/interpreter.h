#ifndef SPORK_INTERPRETER_H_
#define SPORK_INTERPRETER_H_
#include "parser.h"
#include "typechecker.h"

void eval(Expression* expr, Environment* env);
#endif