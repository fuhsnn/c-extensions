#include "test.h"

struct S {
  long a,b,c;
};

int coro1({%}, int m) {
  int i = 1;
  _Yield i * (m += 2);

  i *= 10;
  _Yield i * (m += 2);

  i *= 10;
  return i * (m += 2);
}

struct S struct_rtn({%}, int i, int j, int k) {
  _Yield (struct S){i, j, k};
  _Yield (struct S){j, k, i};
  return (struct S){k, i, j};
}


int yield_in_stmt_expr({%}, int i) {
  return
    ({ int j = i * 3; _Yield j; j; }) +
    ({ int j = i * 20; _Yield j; j; }) +
    ({ int j = i * 100; _Yield j; j; });
}

static void kill_regs(void) {
  __asm (
    "xor %esi, %esi;"
    "xor %edi, %edi;"
    "xor %eax, %eax;"
    "xor %ecx, %ecx;"
    "xor %edx, %edx;"
    "xor %r8d, %r8d;"
    "xor %r9d, %r9d;"
    "xor %r10d, %r10d;"
    "xor %r11d, %r11d;"
  );
}

int main(void) {
  {
    _Ctxof(coro1) *jp = malloc(sizeof(_Ctxof(coro1)));
    _Ctxof(coro1) jv;

    ASSERT(3, coro1({jp}, 1));
    ASSERT(50, coro1({jp}, ...));
    ASSERT(4, jv..coro1(2));
    ASSERT(700, coro1({jp}, ...));
    ASSERT(60, jv..coro1(...));
    ASSERT(800, jv..coro1(...));

    free(jp);
  }
  {
    _Ctxof(struct_rtn) ctx;
    auto s0 = struct_rtn({&ctx}, 3, 5, 7);
    auto s1 = struct_rtn({&ctx}, ...);
    auto s2 = struct_rtn({&ctx}, ...);
    ASSERT(357, s0.a * 100 + s0.b * 10 + s0.c);
    ASSERT(573, s1.a * 100 + s1.b * 10 + s1.c);
    ASSERT(735, s2.a * 100 + s2.b * 10 + s2.c);
  }
  {
    _Ctxof(yield_in_stmt_expr) ctx;
    int i = 0;
    i = yield_in_stmt_expr({&ctx}, 3) * 10; kill_regs();
    i += yield_in_stmt_expr({&ctx}, ...) * 10; kill_regs();
    i += yield_in_stmt_expr({&ctx}, ...) * 10; kill_regs();
    ASSERT(369, yield_in_stmt_expr({&ctx}, ...));
    ASSERT(3690, i);
  }
}
