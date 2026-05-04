#ifndef CPARSEC_H_INCLUDED

#define CPARSEC_H_INCLUDED

#include <stdbool.h>

// The basic element of the parser are string slices to allow for zero-copy parsing
typedef struct {
  const char *ptr;
  size_t      len;
} CpcSlice;

static inline CpcSlice slice_sub(CpcSlice s, size_t start, size_t len) {
  return (CpcSlice){.ptr = s.ptr + start, .len = len};
}

static inline CpcSlice slice_from_cstr(const char *s) {
  size_t n = 0;
  while (s[n] != '\0')
    n++;
  return (CpcSlice){.ptr = s, .len = n};
}

// TODO no error reporting
// returns the accepted output slice and the rest of the string as another slice
typedef struct {
  bool         ok; // we don't have an Either type on C so we just use a bool
  CpcSlice out;
  CpcSlice rest;
} CpcResult;

typedef struct CParsec CParsec; // needed to pass compilation below

typedef union {
  CpcSlice str;
  struct {
    const CParsec *x, *y;
  } two;
} CpcCtx;

// The structure has a "virtual method" to support composing parsers and keeping the dispatch uniform
struct CParsec {
  CpcResult (*fn)(const CParsec *self, CpcSlice input);
  CpcCtx    ctx;
};

// to call the virtual method
#define CPC_VCALL(p, input) p->fn(p, input)

static inline CpcResult cpc_parse(const CParsec *p, const char *input) {
  return CPC_VCALL(p, slice_from_cstr(input));
}

// This is more like Parsec `string'`, which doesn't consume the matching prefix.
// We do this to avoid having a `try` function and working better with `alt`
CParsec cpc_string(const char *s);

// `alt` for "alternative" is the equivalent of Parsec `<|>`
CParsec cpc_alt(const CParsec *a, const CParsec *b);

// `right` is the equivalent of Haskell's Applicative right sequencing `*>`
CParsec cpc_right(const CParsec *x, const CParsec *y);

// `left` is the equivalent of Haskell's Applicative left sequencing `<*`
CParsec cpc_left(const CParsec *x, const CParsec *y);

#endif /* CPARSEC_H_INCLUDED */

#ifdef CPARSEC_IMPLEMENTATION

static CpcResult string_fn(const CParsec *self, CpcSlice input) {
  const CpcSlice slice = self->ctx.str;

  if (input.len < slice.len) // short circuit a too short string
    return (CpcResult){.ok=false, .rest = input};

  for (size_t i = 0; i < slice.len; ++i)
    if (input.ptr[i] != slice.ptr[i]) // mismatch
      return (CpcResult){.ok=false, .rest = input};

  return (CpcResult){
    .ok   = true,
    .out  = slice_sub(input, 0, slice.len),
    .rest = slice_sub(input, slice.len, input.len - slice.len)
  };
}

CParsec cpc_string(const char *s) {
  return (CParsec){
    .fn = string_fn
  , .ctx = (CpcCtx){.str = slice_from_cstr(s)}
  };
}

static CpcResult alt_fn(const CParsec *self, CpcSlice input) {
  CpcResult res = CPC_VCALL(self->ctx.two.x, input);
  if (res.ok)
    return res;
  else
    return CPC_VCALL(self->ctx.two.y, input);
}

CParsec cpc_alt(const CParsec *x, const CParsec *y) {
  return (CParsec){
    .fn = alt_fn
  , .ctx = (CpcCtx){.two = {.x = x, .y = y}}
  };
}

static CpcResult right_fn(const CParsec *self, CpcSlice input) {
  CpcResult res = CPC_VCALL(self->ctx.two.x, input);
  if (!res.ok)
    return res;
  else
    return CPC_VCALL(self->ctx.two.y, res.rest);
}

CParsec cpc_right(const CParsec *x, const CParsec *y) {
  return (CParsec){
    .fn = right_fn
  , .ctx = (CpcCtx){.two = {.x = x, .y = y}}
  };
}

static CpcResult left_fn(const CParsec *self, CpcSlice input) {
  CpcResult res1 = CPC_VCALL(self->ctx.two.x, input);
  if (!res1.ok) return res1;

  CpcResult res2 = CPC_VCALL(self->ctx.two.y, res1.rest);
  if (!res2.ok) return res2;

  return (CpcResult){.ok = true, .out = res1.out, .rest = res2.rest};
}

CParsec cpc_left(const CParsec *x, const CParsec *y) {
  return (CParsec){
    .fn = left_fn
  , .ctx = (CpcCtx){.two = {.x = x, .y = y}}
  };
}

#endif /* CPARSEC_IMPLEMENTATION */
