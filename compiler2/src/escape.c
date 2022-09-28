#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "../lib/sds/sds.h"
#include "literal.h"
#include "parser.h"
#include "utils.h"

/**
 * @brief this contains the character and then escape char to translate to so
 * "a\a" means that if we see an 'a' following a '\', we can translate to '\a'
 */
static const char* const ESCAPES[] = {"a\a", "b\b", "f\f",  "n\n", "r\r",
                                      "t\t", "v\v", "\"\"", "\\\\"};

/**
 * @brief append char to provided sds string
 *
 * @param string reference to sds string to append char to
 * @param c char to append
 */
static void sdscatchar(sds* string, char c) {
    *string = sdscat(*string, (char[]){c, '\0'});
}

/**
 * @brief convert a string to the unescaped string (i.e. convert "\t" to a tab
 * character, etc.)
 *
 * @param string
 * @return sds unescaped string. Caller must free with sdsfree.
 */
sds unescape(char* string) {
    sds unescaped_string = sdsempty();
    char* string_ptr = string;
    while (*string_ptr != '\0') {
        if (*string_ptr == '\\') {
            bool found_escape = false;
            for (int i = 0; i < ARRAY_LEN(ESCAPES); i++) {
                if (*(string_ptr + 1) == ESCAPES[i][0]) {
                    sdscatchar(&unescaped_string, ESCAPES[i][1]);
                    found_escape = true;
                    break;
                }
            }
            if (!found_escape) {
                syntax_error("invalid escape sequence in string");
            } else {
                string_ptr += 2;
            }
        } else {
            sdscatchar(&unescaped_string, *string_ptr);
            string_ptr++;
        }
    }
    return unescaped_string;
}

/**
 * @brief convert a string to the escaped string (i.e. convert a tab character
 * to "\t", etc.)
 *
 * @param string
 * @return sds escaped string. Caller must free with sdsfree.
 */
sds escape(char* string) {
    sds escaped_string = sdsempty();
    char* string_ptr = string;
    while (*string_ptr != '\0') {
        bool found_escape = false;
        for (int i = 0; i < ARRAY_LEN(ESCAPES); i++) {
            if (*string_ptr == ESCAPES[i][1]) {
                sdscatchar(&escaped_string, '\\');
                sdscatchar(&escaped_string, ESCAPES[i][0]);
                found_escape = true;
                break;
            }
        }

        if (!found_escape) {
            sdscatchar(&escaped_string, *string_ptr);
        }

        string_ptr++;
    }
    return escaped_string;
}

// TESTS
static char* STRING = "\a\b\f\n\r\t\v\"\\";
static char* ESCAPED_STRING = "\\a\\b\\f\\n\\r\\t\\v\\\"\\\\";

void test_escape() {
    sds escaped = escape(STRING);
    assert(strcmp(escaped, ESCAPED_STRING) == 0);
}

void test_unescape() {
    sds unescaped = unescape(ESCAPED_STRING);
    assert(strcmp(unescaped, STRING) == 0);
}
