#include "typechecker.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../lib/hashmap/hashmap.h"
#include "parser.h"
#include "utils.h"


// every Expr has a Type (the Type of what it evaluates to)
// (let x y) x