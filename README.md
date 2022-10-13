# Spork
A lisp-inspired language, that aims to be statically typed.

# Dependencies
you will need `clang` and `ld` on your machine to compile and link the spork interpreter.

# To Run Programs
build the spork interpreter first:
```
python build.py all
```
then you can run a program with:
```
./compiler_spork <name of the program>
```

if you ever need a clean build of the spork interpreter:

```
python build.py clean && python build.py all
```

check out the example_programs folder for some sample spork programs.

# Writing Spork Programs
Spork syntax is extremely simple, taking most of it's inspiration from lisp. In general, the syntax is as follows:
```
(<function_name> <arg0> <arg1> <arg2> ...)
```
as an example:
```
(+ 1 2)
```
This would return 3.


In spork, there is no 'main' function. The code is just run as provided, and the final value of the computation is printed to the console. In the case of the program `(+ 1 2)`, if that was the entire program, then 3 would be printed to the console.


There are a few "special forms" in Spork. They are as follows:
```
(let <variable_name> <expr>)
(fn (<param0> <param1> ...) <fn_body>)
(if <condition_expr> <if_body_expr> <else_body_expr>)
```
Currently, these are the only "special forms", everything else is a function.


Some arithmetic functions are built into the language. (==, +, -, /, *). For now, these only work on integers. No iteration has been implemented yet, so you must use recursion to have looping behavior.


# Chaining
There is one exception to the lisp-like syntax, and it is called 'chaining':

```
(let x 5);
(let z 10);
(print "the sum is ");
(+ x z)
```
-- prints "the sum is 15"

essentially, this allows us to write a series of expressions, where the intermediate expressions don't return any meaningful values, and the final expression will return the value of the whole expression. This allows us to avoid nesting a ton of let expressions and creating unnecessary indentation, when the code is "intended" to be read linearly (i.e. step 1, step 2, etc.)


This feature is mostly intended for things that cause side effects, like `let` or `print`, things that change the lexical environment or do I/O, because generally humans think about side effects (like variables and I/0) linearly - do x, do y, do z. In essence, these are like statements in a procedural language, while maintaining the flexibility of expression-based syntax. 
