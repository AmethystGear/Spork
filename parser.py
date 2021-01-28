from charstream import CharStream
from lexer import TokenStream, TokenType, Token

# NOTE: need to do two passes, one to create the AST and another to check types

BUILTIN_TYPES = ['int', 'str', 'bool']

# TODO: add float support
def parse_literal(token_stream: TokenStream):
    token = token_stream.peek()
    token_stream.next()
    if token.token_type != TokenType.LIT:
        raise Exception('bug in the parser')

    if token.value[0] == '"':
        return {
            'name' : 'str',
            'value' : token.value[1:-1]
        }

    if token.value[0].isdigit():
        return {
            'name' : 'int',
            'value' : int(token.value)
        }

    if token.value[0] == 't' or token.value[0] == 'f':
        return {
            'name' : 'bool',
            'value' : token.value == 'true'
        }

    token_stream.err('cannot handle literal!')

def parse_if(token_stream: TokenStream):
    if token_stream.peek().value != 'if':
        raise Exception('bug in the parser')

    token_stream.next()

    cond = parse_expression(token_stream)

    if token_stream.next().value != '{':
        token_stream.err('expected "{" after condition')

    token_stream.next()

    then = parse_expression(token_stream)

    if token_stream.peek().value != "}":
        token_stream.err('no closing bracket on if expression!')
    
    if token_stream.next().value != 'else':
        token_stream.err('expected else')

    if token_stream.next().value != '{':
        token_stream.err('expected "{" after else')

    token_stream.next()

    _else = parse_expression(token_stream)

    if token_stream.peek().value != '}':
        token_stream.err('no closing bracket on else expression!')

    return {
        'name' : 'if',
        'cond' : cond,
        'then' : then,
        'else' : _else
    }

def parse_let(token_stream: TokenStream):
    if token_stream.peek().value != 'let':
        raise Exception('bug in the parser')

    identifier = token_stream.next()
    if identifier.token_type != TokenType.IDENT:
        token_stream.err('expected identifier after let')
    
    if token_stream.next().value != '=':
        token_stream.err('expected "=" after identifier')

    token_stream.next()

    expr = parse_expression(token_stream)

    if token_stream.next().value != ';':
        token_stream.err('all let statements must end with a ";"')

    return {
        'name' : 'let',
        'var_name' : identifier.value,
        'expr' : expr
    }

def parse_parameter(token_stream: TokenStream):
    identifier = token_stream.peek()
    if identifier.token_type != TokenType.IDENT:
        token_stream.err('expected identifier')

    if token_stream.next().value != ':':
        token_stream.err('expected type specifier')

    token_stream.next()

    _type = parse_type(token_stream)

    return {
        'name' : 'param',
        'param_name' : identifier.value,
        'type' : _type
    }

def parse_type(token_stream: TokenStream):
    if token_stream.peek().value == '(':
        token_stream.next()

        _type = []
        while token_stream.peek().value != ')':
            _type.append(parse_type(token_stream))

        token_stream.next()

        return _type
    else:
        identifier = token_stream.peek()
        print(identifier)
        if identifier.token_type != TokenType.IDENT:
            token_stream.err('expected type name')

        if token_stream.next().value == ',':
            token_stream.next()

        return identifier.value

def parse_fn(token_stream: TokenStream):
    if token_stream.peek().value != 'fn':
        raise Exception('bug in the parser')

    if token_stream.next().value != '(':
        token_stream.err('expected "(" to begin params')

    token_stream.next()

    params = []
    first = True
    while token_stream.peek().value != ')':
        if not first and token_stream.next().value != ',':
            token_stream.err('expected delimiter')

        params.append(parse_parameter(token_stream))
        first = False

    if token_stream.next().value != '->':
        token_stream.err('expected "->" after params')

    token_stream.next()

    _type = parse_type(token_stream)

    if token_stream.peek().value != '{':
        token_stream.err('expected "{"')

    token_stream.next()

    expr = parse_expression(token_stream)

    if token_stream.next().value != '}':
        token_stream.err('expected "}"')

    return {
        'name' : 'fn',
        'params' : params,
        'return_type' : _type,
        'body' : expr
    }

def parse_expression(token_stream: TokenStream):
    token = token_stream.peek()
    if token.token_type == TokenType.LIT:
        return parse_literal(token_stream)
    elif token.value == 'let':
        return parse_let(token_stream)
    elif token.value == 'if':
        return parse_if(token_stream)
    elif token.value == 'fn':
        return parse_fn(token_stream)
    elif token.token_type == TokenType.IDENT:
        return {
            'name' : 'ident',
            'val' : token.value
        }
    else:
        print(token.value)
        token_stream.err('cannot handle token')

def parse_program(token_stream: TokenStream):
    expressions = []
    while not token_stream.eof():
        expressions.append(parse_expression(token_stream))

    return expressions


if __name__ == "__main__":
    import sys
    file = sys.argv[1]
    with open(file, 'r') as f:
        s = CharStream(f.read())
        t = TokenStream(s)
        print(parse_program(t))