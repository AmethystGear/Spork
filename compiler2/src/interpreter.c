#include "parser.h"
#include "typechecker.h"

void eval(Expression* expression, Environment* env) {
    bool created_env = false;
    if (env == NULL) {
        env = create_env();
        created_env = true;
    }

    

    if (created_env) {
        free_env(env);
    }
}