#ifndef SPORK_ESCAPE_H_
#define SPORK_ESCAPE_H_
#include "../lib/sds/sds.h"

sds escape(char *string);
sds unescape(char *string);

// TESTS
void test_escape();
void test_unescape();
#endif