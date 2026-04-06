#ifndef ZPARSEC_H_INCLUDED

#define ZPARSEC_H_INCLUDED

#include <stdbool.h>

// The basic element of the parser are string slices to allow for zero-copy parsing
typedef struct {
  const char *ptr;
  size_t      len;
} ZParsecSlice;

static inline ZParsecSlice slice_sub(ZParsecSlice s, size_t start, size_t len) {
  return (ZParsecSlice){.ptr = s.ptr + start, .len = len};
}

static inline ZParsecSlice slice_from_cstr(const char *s) {
  size_t n = 0;
  while (s[n] != '\0')
    n++;
  return (ZParsecSlice){.ptr = s, .len = n};
}

// TODO no error reporting
// returns the accepted output slice and the rest of the string as another slice
typedef struct {
  bool         ok; // we don't have an Either type on C so we just use a bool
  ZParsecSlice out;
  ZParsecSlice rest;
} ZParsecResult;

typedef struct ZParsec ZParsec; // needed to pass compilation below

typedef union {
  ZParsecSlice str;
  struct {
    const ZParsec *x, *y;
  } two;
} ZParsecCtx;

// The structure has a "virtual method" to support composing parsers and keeping the dispatch uniform
struct ZParsec {
  ZParsecResult (*fn)(const ZParsec *self, ZParsecSlice input);
  ZParsecCtx    ctx;
};

// to call the virtual method
#define ZPARSEC_VCALL(p, input) p->fn(p, input)

static inline ZParsecResult zparse(const ZParsec *p, const char *input) {
  return ZPARSEC_VCALL(p, slice_from_cstr(input));
}

// This is more like Parsec `string'`, which doesn't consume the matching prefix.
// We do this to avoid having a `try` function and working better with `alt`
ZParsec zparsec_string(const char *s);

// `alt` for "alternative" is the equivalent of Parsec `<|>`
ZParsec zparsec_alt(const ZParsec *a, const ZParsec *b);

// `right` is the equivalent of Haskell's Applicative right sequencing `*>`
ZParsec zparsec_right(const ZParsec *x, const ZParsec *y);

#endif /* ZPARSEC_H_INCLUDED */

#ifdef ZPARSEC_IMPLEMENTATION

static ZParsecResult string_fn(const ZParsec *self, ZParsecSlice input) {
  const ZParsecSlice slice = self->ctx.str;

  if (input.len < slice.len) // short circuit a too short string
    return (ZParsecResult){.ok=false, .rest = input};

  for (size_t i = 0; i < slice.len; ++i)
    if (input.ptr[i] != slice.ptr[i]) // mismatch
      return (ZParsecResult){.ok=false, .rest = input};

  return (ZParsecResult){
    .ok   = true,
    .out  = slice_sub(input, 0, slice.len),
    .rest = slice_sub(input, slice.len, input.len - slice.len)
  };
}

ZParsec zparsec_string(const char *s) {
  return (ZParsec){
    .fn = string_fn
  , .ctx = (ZParsecCtx){.str = slice_from_cstr(s)}
  };
}

static ZParsecResult alt_fn(const ZParsec *self, ZParsecSlice input) {
  ZParsecResult res = ZPARSEC_VCALL(self->ctx.two.x, input);
  if (res.ok)
    return res;
  else
    return ZPARSEC_VCALL(self->ctx.two.y, input);
}

ZParsec zparsec_alt(const ZParsec *x, const ZParsec *y) {
  return (ZParsec){
    .fn = alt_fn
  , .ctx = (ZParsecCtx){.two = {.x = x, .y = y}}
  };
}

static ZParsecResult right_fn(const ZParsec *self, ZParsecSlice input) {
  ZParsecResult res = ZPARSEC_VCALL(self->ctx.two.x, input);
  if (!res.ok)
    return res;
  else
    return ZPARSEC_VCALL(self->ctx.two.y, res.rest);
}

ZParsec zparsec_right(const ZParsec *x, const ZParsec *y) {
  return (ZParsec){
    .fn = right_fn
  , .ctx = (ZParsecCtx){.two = {.x = x, .y = y}}
  };
}

#endif /* ZPARSEC_IMPLEMENTATION */
