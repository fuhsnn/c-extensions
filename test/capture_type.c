#include "test.h"

struct S {
  int a;
  struct {
    const int b;
  };
};

int fn({struct S}, int c) {
  return a *100 + b * 10 + c;
}

struct S srtn(int a, int b) {
  return (struct S){.a = a, .b = b};
}

int main(void) {

  struct S s = {.a = 1, .b = 2};
  ASSERT(123, fn({&s}, 3));

  ASSERT(127, s..fn(7));

  ASSERT(456, srtn(4, 5)..fn(6));
}
