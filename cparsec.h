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

typedef enum { CPC_NOTHING, CPC_SLICE, CPC_LIST, CPC_PTR } CpcValueKind;

typedef struct {
  CpcValueKind kind;

  union {
    CpcSlice slice;
    struct {
      size_t start;
      size_t len;
    } list; // The list is a range in the arena [start..len]
    void *ptr;
  } as;
} CpcValue;

// Dedicated bump arena for CpcValues
typedef struct {
  CpcValue *items; // storage items
  size_t   cap;    // capacity
  size_t   offset; // bump pointer offset [0..cap]
} CpcArena;

// the signature is like this because both the arena and the items have to be declared separately to allow static or stack allocation
static inline void cpc_arena_init(CpcArena *a, CpcValue *items, size_t cap) {
  *a = (CpcArena){.items = items, .cap = cap, .offset = 0};
}

typedef enum { CPC_OK = 1, CPC_ERR, CPC_ERR_NO_LIST, CPC_ERR_NO_ARENA, CPC_ERR_ARENA_FULL } CpcResKind;

// TODO no error reporting
// returns the accepted output value and the rest of the string as another slice
typedef struct {
  CpcResKind kind;
  CpcValue   out;
  CpcSlice   rest;
} CpcResult;

typedef struct CParsec CParsec; // needed to pass compilation below

// we need the data pointer so we can pass stack or heap allocated values,
// otherwise only static values can be passed
typedef CpcResult (*FmapFn)(CpcArena *arena, const CpcValue *v, CpcSlice rest, void *data);

typedef union {
  CpcSlice str;
  struct {
    const CParsec *x, *y;
  } two;
  struct {
    const CParsec *x;
    FmapFn fn;
    void *data;
  } fmap;
} CpcCtx;

// The structure has a "virtual method" to support composing parsers and keeping the dispatch uniform
struct CParsec {
  CpcResult (*fn)(const CParsec *self, CpcArena *A, CpcSlice input);
  CpcCtx    ctx;
};

static inline bool cpc_is_ok(CpcResult res) {
  return res.kind == CPC_OK;
}

static inline CpcResult cpc_res_ok(CpcValue out, CpcSlice rest) {
  return (CpcResult){.out = out, .rest = rest, .kind = CPC_OK};
}

static inline CpcResult cpc_res_err(CpcSlice rest, CpcResKind kind) {
  return (CpcResult){.out = (CpcValue){.kind = CPC_NOTHING}, .rest = rest, .kind = kind};
}

static inline CpcValue cpc_val_slice(CpcSlice s) {
  return (CpcValue){.kind = CPC_SLICE, .as.slice = s};
}

static inline CpcValue cpc_val_list(CpcArena *A) {
  return (CpcValue){.kind = CPC_LIST, .as.list = {.start = A->offset, .len = 0}};
}

static inline CpcValue cpc_val_ptr(void *p) {
  return (CpcValue){.kind = CPC_PTR, .as.ptr = p};
}

static inline bool cpc_is_list(const CpcValue *v) {
  return v && v->kind == CPC_LIST;
}

static inline CpcResKind cpc_val_list_push(CpcArena *A, CpcValue *list, CpcValue x) {
  if (!A)
    return CPC_ERR_NO_ARENA;
  if (!cpc_is_list(list))
    return CPC_ERR_NO_LIST;
  if (A->offset >= A->cap)
    return CPC_ERR_ARENA_FULL;

  A->items[A->offset++] = x;
  list->as.list.len++;

  return CPC_OK;
}

static inline const CpcValue *cpc_val_list_at(const CpcArena *A, const CpcValue *list, size_t i) {
  if (!A || !cpc_is_list(list) || i >= list->as.list.len) return NULL;
  return &A->items[list->as.list.start + i];
}

// to call the virtual method
#define CPC_VCALL(p, A, input) p->fn(p, A, input)

static inline CpcResult cpc_parse(CpcArena *A, const CParsec *p, const char *input) {
  return CPC_VCALL(p, A, slice_from_cstr(input));
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

// `apply` is the equivalent of Haskell's Applicative sequential application `<*>
// It produces a list of 2 elements, but this can be mapped to another struct
CParsec cpc_apply(const CParsec *x, const CParsec *y);

// `fmap` is the equivalent of Haskell's `<$>`
CParsec cpc_fmap(const CParsec *x, FmapFn fn, void *data);

#endif /* CPARSEC_H_INCLUDED */

#ifdef CPARSEC_IMPLEMENTATION

static CpcResult string_fn(const CParsec *self, __attribute__((unused)) CpcArena *A, CpcSlice input) {
  const CpcSlice slice = self->ctx.str;

  if (input.len < slice.len) // short circuit a too short string
    return cpc_res_err(input, CPC_ERR);

  for (size_t i = 0; i < slice.len; ++i)
    if (input.ptr[i] != slice.ptr[i]) // mismatch
      return cpc_res_err(input, CPC_ERR);

  return cpc_res_ok(cpc_val_slice(slice_sub(input, 0, slice.len)), slice_sub(input, slice.len, input.len - slice.len));
}

CParsec cpc_string(const char *s) {
  return (CParsec){
    .fn = string_fn
  , .ctx = (CpcCtx){.str = slice_from_cstr(s)}
  };
}

static CpcResult alt_fn(const CParsec *self, CpcArena *A, CpcSlice input) {
  CpcResult res = CPC_VCALL(self->ctx.two.x, A, input);
  if (cpc_is_ok(res))
    return res;
  else
    return CPC_VCALL(self->ctx.two.y, A, input);
}

CParsec cpc_alt(const CParsec *x, const CParsec *y) {
  return (CParsec){
    .fn = alt_fn
  , .ctx = (CpcCtx){.two = {.x = x, .y = y}}
  };
}

static CpcResult right_fn(const CParsec *self, CpcArena *A, CpcSlice input) {
  CpcResult res = CPC_VCALL(self->ctx.two.x, A, input);
  if (!cpc_is_ok(res))
    return res;
  else
    return CPC_VCALL(self->ctx.two.y, A, res.rest);
}

CParsec cpc_right(const CParsec *x, const CParsec *y) {
  return (CParsec){
    .fn = right_fn
  , .ctx = (CpcCtx){.two = {.x = x, .y = y}}
  };
}

static CpcResult left_fn(const CParsec *self, CpcArena *A, CpcSlice input) {
  CpcResult res1 = CPC_VCALL(self->ctx.two.x, A, input);
  if (!cpc_is_ok(res1)) return res1;

  CpcResult res2 = CPC_VCALL(self->ctx.two.y, A, res1.rest);
  if (!cpc_is_ok(res2)) return res2;

  return cpc_res_ok(res1.out, res2.rest);
}

CParsec cpc_left(const CParsec *x, const CParsec *y) {
  return (CParsec){
    .fn = left_fn
  , .ctx = (CpcCtx){.two = {.x = x, .y = y}}
  };
}

static CpcResult apply_fn(const CParsec *self, CpcArena *A, CpcSlice input) {
  CpcResult  r1 = CPC_VCALL(self->ctx.two.x, A, input);
  if (!cpc_is_ok(r1)) return r1;
  CpcResult  r2 = CPC_VCALL(self->ctx.two.y, A, r1.rest);
  if (!cpc_is_ok(r2)) return r2;

  CpcValue out = cpc_val_list(A);

  CpcResKind resk = cpc_val_list_push(A, &out, r1.out);
  if (resk != CPC_OK)
    return cpc_res_err(r1.rest, resk);

  resk = cpc_val_list_push(A, &out, r2.out);
  if (resk != CPC_OK)
    return cpc_res_err(r1.rest, resk);

  return cpc_res_ok(out, r2.rest);
}

CParsec cpc_apply(const CParsec *x, const CParsec *y) {
  return (CParsec){
    .fn = apply_fn
  , .ctx = (CpcCtx){.two = {.x = x, .y = y}}
  };
}

static CpcResult fmap_fn(const CParsec *self, CpcArena *A, CpcSlice input) {
  CpcResult r = CPC_VCALL(self->ctx.fmap.x, A, input);
  return self->ctx.fmap.fn(A, &r.out, r.rest, self->ctx.fmap.data);
}

CParsec cpc_fmap(const CParsec *x, FmapFn fn, void *data) {
  return (CParsec){
    .fn = fmap_fn
  , .ctx = (CpcCtx){.fmap = {.x = x, .fn = fn, .data = data}}
  };
}

#endif /* CPARSEC_IMPLEMENTATION */
