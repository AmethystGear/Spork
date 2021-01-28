from copy import deepcopy
from enum import Enum, auto
from functools import partial
from charstream import CharStream

# all the different kinds of tokens
class TokenType(Enum):
    PUNC = "punctuation"
    LIT = "literal"
    KEYWORD = "keyword"
    IDENT = "identifier"
    OPERATOR = "operator"

# attempts to match the char input to the provided matching fn
# if a match is found, returns a new Token with the provided type, 
# and removes all whitespace after the token
# if a match is not found, returns None and resets the CharStream 
# as if the fn was never run.
def match(token_type, fn, char_stream: CharStream):
    state = char_stream.get_state()
    matching = fn(char_stream)
    if matching is None:
        char_stream.reset_to(state)
        return None
    else:
        return Token(token_type, matching)

# contains token type and the actual value of the token
class Token():
    def __init__(self, token_type: TokenType, value: str):
        self.token_type = token_type
        self.value = value

    def __repr__(self):
        return str(self.token_type) + ", '" + self.value + "'"

#####################################################
# defining the functions we use to recognize tokens #
#####################################################

# match an exact sequence of characters
def parse_exact(val: str, char_stream: CharStream, valid_post_tokens=[], require_whitespace=False):
    val_stream = CharStream(val)
    while not val_stream.eof() and not char_stream.eof() and val_stream.peek() == char_stream.peek():
        val_stream.next()
        char_stream.next()

    if not val_stream.eof():
        return None
    elif char_stream.eof() or char_stream.peek().isspace():
        return val
    elif valid_post_tokens:
        found_valid_token = False
        state = char_stream.get_state()
        for token in valid_post_tokens:
            if token(char_stream) is not None:
                found_valid_token = True
                break

        char_stream.reset_to(state)
        return val if found_valid_token else None
    else:
        return None if require_whitespace else val

# match a number literal
def parse_number(valid_post_tokens, char_stream: CharStream):
    num = ''
    while not char_stream.eof() and char_stream.peek().isdigit():
        num += char_stream.next()

    if num == '':
        return None

    if char_stream.peek().isspace():
        return num

    found_valid_token = False
    state = char_stream.get_state()
    for token in valid_post_tokens:
        if token(char_stream) is not None:
            found_valid_token = True
            break
    
    char_stream.reset_to(state)

    return None if not found_valid_token and valid_post_tokens else num

# match a boolean literal
def parse_boolean(valid_post_tokens, char_stream: CharStream):
    if char_stream.peek() == 't':
        return parse_exact('true', char_stream, valid_post_tokens=valid_post_tokens)
    elif char_stream.peek() == 'f':
        return parse_exact('false', char_stream, valid_post_tokens=valid_post_tokens)
    else:
        return None

# match a string literal
def parse_string(char_stream: CharStream):
    string = ''
    if char_stream.peek() != '"':
        return None
    
    string += char_stream.next()
    escape = False
    while not char_stream.eof():
        char = char_stream.next()
        string += char
        if char == '\\':
            escape = not escape
        elif char == '"' and not escape:
            break
        else:
            escape = False

    return string if string[0] == '"' and string[-1] == '"' else None
        
# match an identifier
def parse_identifier(banned_tokens, char_stream: CharStream):
    ident = ''
    if char_stream.peek().isdigit():
        return None

    while not char_stream.eof() and (char_stream.peek().isalnum() or char_stream.peek() == '_'):
        ident += char_stream.next()

    state = char_stream.get_state()
    for banned in banned_tokens:      
        if (banned(CharStream(ident))):
            return None
        
        char_stream.reset_to(state)

    return None if ident == '' else ident

#####################################################

##########################################
# defining all the valid kinds of tokens #
##########################################

# punctuation
left_paren = partial(match, TokenType.PUNC, partial(parse_exact, '('))
right_paren = partial(match, TokenType.PUNC, partial(parse_exact, ')'))
left_bracket = partial(match, TokenType.PUNC, partial(parse_exact, '{'))
right_bracket = partial(match, TokenType.PUNC, partial(parse_exact, '}'))
left_sq_bracket = partial(match, TokenType.PUNC, partial(parse_exact, '['))
right_sq_bracket = partial(match, TokenType.PUNC, partial(parse_exact, ']'))
delim = partial(match, TokenType.PUNC, partial(parse_exact, ','))
terminator = partial(match, TokenType.PUNC, partial(parse_exact, ';'))
type_specifier = partial(match, TokenType.PUNC, partial(parse_exact, ':'))
period = partial(match, TokenType.PUNC, partial(parse_exact, '.'))
punctuation = [left_paren, right_paren, left_bracket, right_bracket, left_sq_bracket, right_sq_bracket, delim, terminator, type_specifier, period]

# operators
eq = partial(match, TokenType.OPERATOR, partial(parse_exact, '='))
into = partial(match, TokenType.OPERATOR, partial(parse_exact, '->'))
add = partial(match, TokenType.OPERATOR, partial(parse_exact, '+'))
sub = partial(match, TokenType.OPERATOR, partial(parse_exact, '-'))
power = partial(match, TokenType.OPERATOR, partial(parse_exact, '**'))
mul = partial(match, TokenType.OPERATOR, partial(parse_exact, '*'))
div = partial(match, TokenType.OPERATOR, partial(parse_exact, '/'))
_not = partial(match, TokenType.OPERATOR, partial(parse_exact, '!'))
_and = partial(match, TokenType.OPERATOR, partial(parse_exact, '&&'))
_or = partial(match, TokenType.OPERATOR, partial(parse_exact, '||'))
ops = [eq, into, add, sub, power, mul, div, _not, _and, _or]

# keywords 
let = partial(match, TokenType.KEYWORD, partial(parse_exact, 'let', require_whitespace=True))
struct = partial(match, TokenType.KEYWORD, partial(parse_exact, 'struct', valid_post_tokens=[left_bracket], require_whitespace=True))
interface = partial(match, TokenType.KEYWORD, partial(parse_exact, 'interface',valid_post_tokens=[left_bracket],  require_whitespace=True))
_self = partial(match, TokenType.KEYWORD, partial(parse_exact, 'Self', valid_post_tokens=[right_paren, delim], require_whitespace=True))
_if = partial(match, TokenType.KEYWORD, partial(parse_exact, 'if', valid_post_tokens=[left_bracket], require_whitespace=True))
_else = partial(match, TokenType.KEYWORD, partial(parse_exact, 'else', valid_post_tokens=[left_bracket], require_whitespace=True))
fn = partial(match, TokenType.KEYWORD, partial(parse_exact, 'fn', valid_post_tokens=[right_paren], require_whitespace=True))
keywords = [let, struct, interface, _self, fn]

# literals
num = partial(match, TokenType.LIT, partial(parse_number, punctuation + ops))
boolean = partial(match, TokenType.LIT, partial(parse_boolean, punctuation + ops))
string = partial(match, TokenType.LIT, parse_string)
literals = [num, boolean, string]

# identifiers
ident = partial(match, TokenType.IDENT, partial(parse_identifier, keywords + literals))

##############################################

# this holds all the token types so that we can iterate over all of them
token_types = punctuation + ops + keywords + literals + [ident]

# returns a stream of tokens
class TokenStream:
    def __init__(self, char_stream: CharStream):
        self.char_stream = char_stream
        self.current = None

    # read in the next token and return it
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
            token = token_type(char_stream=self.char_stream)
            if token is not None:
                break

        if token is None:
            self.char_stream.err("can't handle token")

        # skip whitespace again
        while not self.eof() and self.char_stream.peek().isspace():
            self.char_stream.next()

        self.current = token
        return token

    # return the current token
    def peek(self) -> Token:
        if self.current is None:
            self.next()

        return self.current

    # return if we're at the end of the file or not
    def eof(self) -> bool:
        return self.char_stream.eof()

    def err(self, msg):
        self.char_stream.err(msg)
        
# testing this for 
if __name__ == "__main__":
    import sys
    file = sys.argv[1]
    with open(file, 'r') as f:
        s = CharStream(f.read())
        t = TokenStream(s)
        while not t.eof():
            print(t.next())            
