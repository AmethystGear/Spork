# Spork
A lisp-inspired language, that aims to be statically typed.

# To Run Programs
build the spork interpreter first:
```
python build.py all
```
then you can run a program with:
```
./compiler_spork <name of the program>
```
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
(let <variable_name> <expr>) <expr>
(fn (<param0> <param1> ...) <fn_body>)
(if <condition_expr> <if_body_expr> <else_body_expr>)
```
Currently, these are the only "special forms, everything else is a function.


Some arithmetic functions are built into the language. (==, +, -, /, *). For now, these only work on integers. No iteration has been implemented yet, so you must use recursion to have looping behavior.

