all: parser.o lexer.o sds.o
	clang parser.o lexer.o sds.o -o spork_compiler

sds.o: lib/sds/sds.c
	clang -c lib/sds/sds.c

parser.o: src/parser.c
	clang -c src/parser.c

lexer.o: src/lexer.c 
	clang -c src/lexer.c

clean:
	rm *.o spork_compiler