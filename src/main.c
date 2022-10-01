#include <stdio.h>
#include "parser.h"
//#include "typechecker.h"
#include "utils.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "provide one file to compile\n");
        abort();
    }
    char *program = read_file_to_string(argv[1]);
    if (program == NULL) {
        fprintf(stderr, "file %s does not exist", argv[1]);
    }

    Expression *expr = parse_expr(&program);
    print_expr(expr); printf("\n");

    //Type* type = get_type(expr, NULL);
    //print_type(type); printf("\n");

    //eval(expr, NULL);
    free_expr(expr);
}
