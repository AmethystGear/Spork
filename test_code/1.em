# code that gets the number of times you can divide an integer
# by two until you get 1.
# nobody would actually implement it like this, this is just to test closures
let y = fn (div : int) -> int {
  let x = fn (a : int, acc : int) -> int {
    if (a) == (1) {
      acc
    } else {
      let divided = (a) / (div);
      let increment = (acc) + (1);
      x(divided, increment)
    }
  };
  let z = fn (a : int) -> int {
    x(a, 0)
  };
  z
};
let test_val = 16;
let f = y(2);
f(test_val)