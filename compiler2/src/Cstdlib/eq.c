#include <stdbool.h>
#include "../../lib/sds/sds.h"

bool Eq_Int(int a, int b) {
    return a == b;
}

bool Eq_String(sds a, sds b) {
    return strcmp(a, b) == 0;
}