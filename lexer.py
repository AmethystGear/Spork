import re
from enum import Enum, auto
from functools import partial
from charstream import CharStream
from time import sleep

# all the different kinds of tokens
class TokenType(Enum):
    PUNC = "punctuation"
    NUM = "number"
    KEYWORD = "keyword"
    IDENT = "identifier"
    OPERATOR = "operator"

# attempts to match the char input to the provided matching fn
# if a match is found, returns a new Token with the provided type, 
# and removes all whitespace after the token
# if a match is not found, returns None and resets the CharStream.
def match(token_type, fn, char_stream: CharStream):
    pos = char_stream.pos
    col = char_stream.col
    line = char_stream.line

    matching = fn(char_stream)
    if matching is None:
        char_stream.pos = pos
        char_stream.col = col
        char_stream.line = line
        return None
    else:
        return Token(token_type, matching)

# store token's type and the actual value of the token
class Token():
    def __init__(self, token_type: TokenType, value: str):
        self.token_type = token_type
        self.value = value

    def __repr__(self):
        return str(self.token_type) + ", '" + self.value + "'"

# if the CharStream exactly matches val, return val, else return None
# if whitespace is required, also return None if there is no whitespace after the token.
def exact(val: str, require_whitespace_after: bool, char_stream: CharStream):
    val_stream = CharStream(val)
    while val_stream.peek() == char_stream.peek():
        val_stream.next()
        char_stream.next()

    if require_whitespace_after and not char_stream.peek().isspace():
        return None

    return val if val_stream.eof() else None

# if the CharStream matches a number, return that number, otherwise return None
def number(char_stream: CharStream):
    num = ''
    if char_stream.peek() == '-':
        num += '-'

    while not char_stream.eof() and char_stream.peek().isdigit():
        num += char_stream.next()

    return None if num == '' or num == '-' else num

# return a valid identifier if it exists, otherwise return None
def identifier(char_stream: CharStream):
    ident = ''
    while not char_stream.eof() and (char_stream.peek().isalnum() or char_stream.peek() == '_'):
        ident += char_stream.next()

    return None if ident == '' else ident

# all the different types of tokens that we can match on
left_paren = partial(match, TokenType.PUNC, partial(exact, '(', False))
right_paren = partial(match, TokenType.PUNC, partial(exact, ')', False))
left_bracket = partial(match, TokenType.PUNC, partial(exact, '{', False))
right_bracket = partial(match, TokenType.PUNC, partial(exact, '}', False))
delim = partial(match, TokenType.PUNC, partial(exact, ',', False))
terminator = partial(match, TokenType.PUNC, partial(exact, ';', False))
type_specifier = partial(match, TokenType.PUNC, partial(exact, ':', False))

eq = partial(match, TokenType.KEYWORD, partial(exact, '=', False))
convert = partial(match, TokenType.KEYWORD, partial(exact, '->', False))
let = partial(match, TokenType.KEYWORD, partial(exact, 'let', True))

num = partial(match, TokenType.NUM, number)

ident = partial(match, TokenType.IDENT, identifier)

# put all the above tokens into a list so we can loop over them
token_types = [left_paren, right_paren, left_bracket, \
               right_bracket, delim, terminator, type_specifier, \
               eq, convert, let, num, ident]

# generate a stream of tokens from the stream of characters
class TokenStream:
    def __init__(self, char_stream: CharStream):
        self.char_stream = char_stream
        self.current = None

    def next(self) -> Token:
        # handle eof case
        if self.eof():
            self.current = None
            return None

        # skip whitespace
        while self.char_stream.peek().isspace():
            self.char_stream.next()
            if self.eof():
                self.current = None
                return None

        # skip comments
        if self.char_stream.peek() == '#':
            while self.char_stream.next() != '\n':
                pass

            return self.next()
        
        token = None
        for token_type in token_types:
            token = token_type(self.char_stream)
            if token is not None:
                break

        if token is None:
            self.char_stream.err("can't handle token at: ")

        # skip whitespace again
        while not self.eof() and self.char_stream.peek().isspace():
            self.char_stream.next()

        self.current = token
        return token


    def peek(self) -> Token:
        return current

    def eof(self) -> bool:
        return self.char_stream.eof()


# test code that only runs if we're running the lexer directly
if __name__ == "__main__":
    import sys
    file = sys.argv[1]
    with open(file, 'r') as f:
        s = CharStream(f.read())
        t = TokenStream(s)
        while not t.eof():
            print(t.next())            
