# Embr
Making a programming language, because I can, I guess

Not sure what to call this thing yet, for now, maybe Embr?

In Embr, everything is an expression. Why? Cause it's super easy to make a language that way, and it parses nicely.

## Syntax
```
if <expr> { <expr> } else { <expr> }
# i.e. if (a) == (5) { call_function_1(a) } else { call_function_2(a) }

fn <args> -> <output_type> { <expr> }
# i.e. fn (a : int, b : int) -> int { (a) + (b) }

let <identifier> = <expr>; <expr>
# i.e.
# let x = 5; 
# let res = if (num_customers) == (10) { 3 } else { 15 };
# let func_1 = fn (a : bool) -> int { if a { 1 } else { 0 } }; ...

(<expr>) <operator> (<expr>)
#i.e. (5) + (5)

<operator> (<expr>)
#i.e. !have_enough_tokens -10

<literal>
#i.e. 5 "hello world" true

<variable>
# this would be getting the value of a variable 
#i.e this_is_a_variable

<variable> <args>
# this would be calling a variable (if the variable contains a function)
#i.e. x(5, 10)
```
Note that all functions are closures (that is, they can refer to any bindings made above them).

## Cool, how do I write and run code?
just use ./run.sh "<path/to/your/file>". It'll print the result of your program.

## Error messages?
No. Also the typechecker is nonexistent, stuff will just break at runtime. So have fun with that.

## Why is your language so bad?
Jeez. Cut me some slack, I wrote all this code in like a few days...
Also, it's bad cause I have no idea what I'm doing.