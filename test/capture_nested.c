#include "test.h"

static int call_fnptr(int(*p)(int)) {
   return p(7);
}

static int no_capture(void) {
  int no_capture(int j) {
    return j;
  }

  ASSERT(3, no_capture(3));
  ASSERT(7, call_fnptr(&no_capture));
  return 1;
}

static int capture(void) {
  static int v_static;
  extern int v_extern;

  int a = 0, b = 0;
  int f({a, &b}) {
    a = 3;
    b = 5;
    v_static = 7;
    v_extern = 9;
    return a * 1000 + b * 100 + v_static * 10 + v_extern;
  }
  _Ctxof(f) ctx;
  __builtin_capture_init(ctx);

  ASSERT(3579, f({&ctx}));
  ASSERT(0, a);
  ASSERT(5, b);
  ASSERT(7, v_static);
  ASSERT(9, v_extern);
  return 1;
}

int v_extern;

static int name_shadowing(void) {
  int name = 7;
  {
    int name({&}, char(*p)[name], int name) {
      return sizeof(*p) * 10 + (++name);
    }
    _Ctxof(name) ctx;
    __builtin_capture_init(ctx);
    ASSERT(74, name({&ctx}, (void*)0, 3));
  }
  return 1;
}

static int multiple_context() {
  int a = 3, b = 5;
  int f({&, b}) {
    return a++ * 10 + b++;
  }
  _Ctxof(f) ctx_v;
  _Ctxof(f) *ctx_p = malloc(sizeof(_Ctxof(f)));
  __builtin_capture_init(ctx_v);
  __builtin_capture_init(*ctx_p);
  ASSERT(35, f({ctx_p}));
  ASSERT(45, f({&ctx_v}));
  ASSERT(56, f({&ctx_v}));
  ASSERT(66, f({ctx_p}));
  ASSERT(77, ctx_v..f());
  ASSERT(87, (*ctx_p)..f());
  free(ctx_p);
  return 1;
}

static int doubly_nesting() {
  int a = 1;

  int f1({a}, int i) {
    a += i;

    int f2({&a}, int j) {
      return a += j;
    }
    _Ctxof(f2) f2ctx;
    __builtin_capture_init(f2ctx);
    ASSERT(111, f2({&f2ctx}, 10));
    return a += i;
  }
  _Ctxof(f1) f1ctx;
  __builtin_capture_init(f1ctx);
  ASSERT(211, f1({&f1ctx}, 100));
  ASSERT(1, a);
  return 1;
}

static int recursive() {
  int arr[9];

  int fibo({&arr}, int i) {
    int val;
    if (i < 2)
      val = i;
    else
      val = fibo({}, i - 1) + fibo({}, i - 2);
    arr[i] = val;
    return val;
  }
  _Ctxof(fibo) ctx;
  __builtin_capture_init(ctx);
  ASSERT(13, fibo({&ctx}, 7));
  ASSERT(8, arr[6]);
  ASSERT(5, arr[5]);
  ASSERT(3, arr[4]);
  ASSERT(2, arr[3]);
  ASSERT(1, arr[2]);
  ASSERT(1, arr[1]);
  ASSERT(0, arr[0]);
  return 1;
}

static int capture_another() {
  char a = 3;
  long b = 5;

  int f1({&a, b}) {
    return a * 10 + b;
  }

  _Ctxof(f1) ctx1;
  __builtin_capture_init(ctx1);

  int f2({&ctx1}) {
    return (ctx1)..f1();
  }

  _Ctxof(f2) ctx2;
  __builtin_capture_init(ctx2);

  a = 7;
  ASSERT(75, f2({&ctx2}));
  return 1;
}

int main() {
  ASSERT(1, no_capture());
  ASSERT(1, capture());
  ASSERT(1, name_shadowing());
  ASSERT(1, multiple_context());
  ASSERT(1, doubly_nesting());
  ASSERT(1, recursive());
  ASSERT(1, capture_another());
}

