#! /bin/sh
ocamlopt -o embr_interpreter files.ml lexer.ml parser.ml interpreter.ml
./embr_interpreter "$1"
rm embr_interpreter