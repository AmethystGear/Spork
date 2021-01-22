import copy 

class CharStreamState:
    def __init__(self, pos, line, col):
        self.pos = pos
        self.line = line
        self.col = col

# makes the whole program into a character stream 
# so we can get each character one at a time
class CharStream:
    def __init__(self, program: str):
        self.stream_state = CharStreamState(0, 1, 0)
        self.program = program

    # return the current character and advance to the next character
    # throw an exception if we're already at the end of the program
    def next(self) -> str:
        if self.eof():
            raise Exception('Cannot call next() when there is nothing left to read in the program')

        curr = self.peek()
        self.stream_state.pos += 1
        if curr == '\n':
            self.stream_state.line += 1
            self.stream_state.col = 0
        else:
            self.stream_state.col += 1
        
        return curr

    # get the current character, or None if there aren't any characters left in the program
    def peek(self) -> str:
        return None if self.eof() else self.program[self.stream_state.pos]

    # return true if we've reached the end of the program, else false
    def eof(self) -> bool:
        return self.stream_state.pos >= len(self.program)

    # fail program compilation at our current position, with a message
    def err(self, msg: str):
        print("Compilation failed at line: " + str(self.stream_state.line) + ", col: " + str(self.stream_state.col))
        print(msg)
        col = self.stream_state.col
        self.stream_state.pos -= self.stream_state.col
        self.stream_state.col = 0
        while self.peek() != '\n':
            print(self.next(), end='')

        print()
        for _ in range(col):
            print(' ', end='')

        print('^')

        quit()

    def get_state(self) -> CharStreamState:
        return copy.deepcopy(self.stream_state)

    def reset_to(self, state: CharStreamState):
        self.stream_state = copy.deepcopy(state)