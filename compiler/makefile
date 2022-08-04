all: parser.o lexer.o sds.o utils.o
	clang parser.o lexer.o sds.o utils.o -o compiler -g -O0

test: parser.o lexer.o sds.o utils.o testsuite.o
	clang parser.o lexer.o sds.o utils.o testsuite.o -o testsuite -g -O0

testsuite.o: test/testsuite.c
	clang -c test/testsuite.c -g -O0

sds.o: lib/sds/sds.c
	clang -c lib/sds/sds.c -g -O0

parser.o: src/parser.c
	clang -c src/parser.c -g -O0

lexer.o: src/lexer.c 
	clang -c src/lexer.c -g -O0

utils.o: src/utils.c
	clang -c src/utils.c -g -O0

clean:
	rm *.o spork_compiler