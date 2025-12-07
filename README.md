This is a collection of extensions to C programming language implemented as patches to the [slimcc](https://github.com/fuhsnn/slimcc) compiler, done mostly for fun, may not be well thought out. They're kept in [separate branches](https://github.com/fuhsnn/c-extensions/branches) for better view-ability and to keep upstream codebase flexible. For building the compiler please refer to [upstream readme](https://github.com/fuhsnn/slimcc?tab=readme-ov-file#building-and-using). Examples may need `-std=c23` to build because slimcc hasn't default to that.

 - [`struct _Compat(ident) {...}`](#compat-ident)
 - [`for LoopName (...)`](#alt-named-loops-syntax)
 - [`[[gate(allowlist)]]`](#attr-gate)
 - [`_Match_type()`](#match-type)
 - [`_Match_int()`](#match-int)

<a name="compat-ident"></a>
## [Compatibility identifiers](https://github.com/fuhsnn/c-extensions/tree/compat-ident)

It's a common idiom to do generic types with macros like:
```C
#define span(T) struct span_##T { ssize_t N; T* data; }
```
However even with C23's tag compatibility rule, it's still not possible to use pointer types directly with this pattern (because the `*` cannot be macro-pasted with `span_##T`).

`_Compat(ident)` is a simplified twist of the [N3332 _Record proposal](https://thephd.dev/_vendor/future_cxx/papers/C%20-%20_Record%20types.html), it appears in place of struct tag and cannot be mixed. Types declared with `_Compat` remain tag-less to the existing type system, only during compatibility checks, two types with the same `ident` count as having the same tag according to C23's rule.
```C
#include <stddef.h>
#define vector(T) struct _Compat(vector) { T *ptr; size_t sz; }
#define array(T) struct _Compat(array) { T *ptr; size_t sz; }

// struct;                 // meaningless and invalid
// struct _Compat(vector); // meaningless and invalid just like above

// compatible: _Compat ident same, layout same
static_assert(1 == _Generic(vector(int *), default: 0, vector(typeof(&(int){})): 1));

// not compatible: _Compat ident same, layout differ
static_assert(0 == _Generic(vector(int *), default: 0, vector(long *): 1));

// not compatible: _Compat ident differ, layout same
static_assert(0 == _Generic(vector(int *), default: 0, array(int *): 1));
```
Runnable example:
```C
#include <stdio.h>

#def result(T)
struct _Compat(result_v0) {
  T val;
  bool has_val;
}
#enddef

result(int *) fn(int *p) {
  if (p == nullptr)
    return (result(int*)){.has_val = false};
  else
    return (result(int*)){p, true};
}

int main(void) {
  if (auto e = fn(nullptr); e.has_val)
    printf("e1 val %d\n", *e.val);
  else
    printf("e1 no val\n");

  if (auto e = fn(&(int){3}); e.has_val)
    printf("e2 val %d\n", *e.val);
  else
    printf("e2 no val\n");
}
```

<a name="alt-named-loops-syntax"></a>
## [Alternative C2Y named loops syntax](https://github.com/fuhsnn/c-extensions/tree/alt-named-loops-syntax)

An implementation of the [n3377 proposal to WG14](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3377.pdf) because I'm sympathetic to it.

It's done on top of the label-based syntax, so those still work. `do while` loops are implemented differently because slimcc's current structure is too single-pass-y to support it elegantly, the name is positioned after `do` and only work for curly braced blocks.

Runnable example:
```C
#include <stdio.h>

int main(void) {
  int i = 0;
  do DW1 {
    For_lab2:
    for For_lab1 (int j = 0; j < 10; j++) {
        switch S1 (j) {
        case 3:
            break;
        case 2:
            continue For_lab1;
        case 5:
            break S1;
        default:
            continue;
        }
        if ((i % j) == 0) {
            printf("%d\n",i);
            break For_lab2;
        }
    }
    while W1 (i++ > 25)
        break DW1;
  } while (1);
}
```

<a name="attr-gate"></a>
## [Block attribute `[[gate(allowlist)]]`](https://github.com/fuhsnn/c-extensions/tree/attr-gate)

The attribute act as a guarded gate that only allow identifiers listed in `allowlist` to be referenced from inside the following block. It can be applied to normal blocks, secondary-blocks, statement-expressions as well as function bodies.
```C
int foo, bar, baz;
[[slimcc::gate(foo, baz)]] {
  foo += baz;
  bar += 2; // error, bar isn't in allowlist, it's an undefined variable here.
}
```
Its intended use is to make complex function-like macros less leaky, like:
```C
// #def and #enddef from N3531 proposal
#def Psuedo_template_fn(Tr, T1, T2, arg1, arg2)
  ({
    typeof(T1) v1 = (arg1);
    typeof(T2) v2 = (arg2);

    (typeof(Tr)) [[slimcc::gate(v1, v2, exit)]] ({
      /* only v1, v2 and exit() are usable in this statement-expression,
        just like regular functions, stricter in fact */
    });
  })
#enddef
```
Compilable example, also try it with gcc/clang to see the difference:
```C
#include <stdlib.h>

#ifdef __has_c_attribute
#if __has_c_attribute(slimcc::gate)
#define GATE(allow_list...) [[slimcc::gate(allow_list)]]
#endif
#endif

#ifndef GATE
#define GATE(...)
#endif

struct Linked {
  struct Linked *next;
};

#define Macro_w_typo(_x)          \
  do GATE(_x, free) {             \
    __auto_type nxt = (_x)->next; \
    free(x /*typo*/);             \
    _x = nxt;                     \
  } while(0)

void fn(struct Linked *x, struct Linked *y) {
  Macro_w_typo(y); // error: "x" undefined instead of silently using same named var
}

int psuedo_pure_func(int i) GATE(i) { // applicable to function body

  fn(NULL, NULL); // error: fn unusable here since it's not in allow list

  int g = GATE(i)({ i - 1; }); // also applicable to statement-expression
  return g - 1;
}

int main(void) {
  return psuedo_pure_func(2);
}
```

<a name="match-type"></a>
## [`_Match_type(type/expr, type:(expr), ...)`](https://github.com/fuhsnn/c-extensions/tree/match-type-int)

As a response to Simon Tatham's [Workarounds for C11 _Generic](https://www.chiark.greenend.org.uk/~sgtatham/quasiblog/c11-generic/), this is a version of `_Generic` that chooses from parentheses-balanced tokens without semantic checking until the selection is made. The result still needs to be a complete expression, more complex stuff can be done with statement-expression.

Runnable example:
```C
#include <stdio.h>
#include <string.h>

struct MyStringBuffer {
  char *data;
  size_t length;
};

// #def and #enddef from N3531 proposal
#def string_length(x)
  _Match_type(x,
    char *                  : (strlen(x)),
    struct MyStringBuffer * : ((x)->length)
  )
#enddef

int main() {
  struct MyStringBuffer buf = {"sliced", 3};
  printf("size %d\n", (int)string_length(&buf));

  printf("size %d\n", (int)string_length("foobar"));
}
```

<a name="match-int"></a>
## [`_Match_int(const_expr, const_expr:(expr), ...)`](https://github.com/fuhsnn/c-extensions/tree/match-type-int)

Like `_Match_type` but chooses from integer constant expressions instead, in other words, compile-time expression selection with pattern-matching syntax. It also tries to be less sloppy by requiring case numbers to be the same integer type without implicit conversion.

Runnable example:
```C
#include <stdio.h>

int main() {
  puts(_Match_int(__STDC_VERSION__,
    202311L : ( "C23" ),
    201710L : ( "C17" ),
    201112L : ( "C11" ),
    199901L : ( "C99" ),
    0L : ( paren-balanced invalid syntax )
  ));
}
```
