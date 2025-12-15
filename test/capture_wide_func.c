#include "test.h"

int call_wfn(_Wideof(int(int)) fn) {
  return fn(7);
}

_Wideof(int(int)) build_wfn(int a, int b) {
  int f({=, b}, int c) {
    return c * 100 + b * 10 + a;
  }
  _Ctxof(f) *ctx_p = malloc(sizeof(_Ctxof(f)));
  __builtin_capture_init(*ctx_p);

  a = b = 9;

  _Wideof(f) wfn;
  __builtin_widefn_create(wfn, f, ctx_p);
  return wfn;
}

int main(void) {
  int vfn(void) { return 42; }
  _Wideof(int(void)) w;
  __builtin_widefn_create(w, vfn, nullptr);
  ASSERT(42, w());

  {
    int a = 3;
    int f({&}, int b) {
      return a * 10 + b;
    }
    _Ctxof(f) ctx;
    __builtin_capture_init(ctx);

    _Wideof(int(int)) obj;
    __builtin_widefn_create(obj, f, &ctx);

    ASSERT(37, call_wfn(obj));
  }
  {
    auto wfn = build_wfn(3,5);
    ASSERT(753, wfn(7));

    free(__builtin_widefn_get_ctx(wfn));
    __builtin_widefn_get_ctx(wfn) = NULL;

    ASSERT(0, __builtin_widefn_get_ctx(wfn));
  }
}
