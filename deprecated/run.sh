#! /bin/sh
ocamlopt -o spork_interpreter files.ml lexer.ml parser.ml interpreter.ml
./spork_interpreter "$1"
rm spork_interpreter
