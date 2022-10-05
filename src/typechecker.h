#ifndef SPORK_TYPE_CHECKER_H_
#define SPORK_TYPE_CHECKER_H_
#include "literal.h"
#include "parser.h"
#include "../lib/hashmap/hashmap.h"
#include "../lib/cvector/cvector.h"
// base types
// function needs to be a base type
// the literals (int, float, bool, string)
// tuple is also a base type (. x y z)
// (let point (. int int))
// Tuple (int int)
typedef enum ValueKind { 
    TupleVal, 
} ValueKind;

typedef union ValueType {
    Literal literal;
    cvector_vector_type(Value*) tuple;

} ValueType;

/**
 * @brief this is the value of the 
 */
typedef struct Value Value;
typedef struct Value {
    ValueKind kind;
    ValueType type;
} Value;


typedef union Type Type;
typedef union Type {
    cvector_vector_type(Type*) tuple;
} Type;

// types as values? how do we even do that?
// (let Vec (fn (len Nat) (elem_type Type))

#endif