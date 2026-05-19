#ifndef CPARSEC_H_INCLUDED

#define CPARSEC_H_INCLUDED

#include <stdbool.h>

// The basic element of the parser are string slices to allow for zero-copy parsing
typedef struct {
  const char *ptr;
  size_t      len;
} CpcSlice;

static inline CpcSlice cpc_slice_sub(CpcSlice s, size_t start, size_t len) {
  return (CpcSlice){.ptr = s.ptr + start, .len = len};
}

static inline CpcSlice cpc_slice_from_cstr(const char *s) {
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
  void     *user;  // user pointer data for use on the fmap, we don't handle its lifecycle
} CpcArena;

// the signature is like this because both the arena and the items have to be declared separately to allow static or stack allocation
static inline void cpc_arena_init(CpcArena *a, CpcValue *items, size_t cap, void *user) {
  *a = (CpcArena){.items = items, .cap = cap, .offset = 0, .user = user};
}

typedef enum { CPC_OK = 1, CPC_ERR, CPC_ERR_NO_LIST, CPC_ERR_NO_ARENA, CPC_ERR_ARENA_FULL } CpcResKind;

// TODO no error reporting
// returns the accepted output value and the rest of the string as another slice
typedef struct {
  CpcResKind kind;
  CpcValue   out;
  CpcSlice   rest;
} CpcResult;

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

#define CPC_DEFINE_PARSER(name)                                                                \
  static inline CpcResult name(CpcArena *A, CpcSlice input)

// This is more like Parsec `string'`, which doesn't consume the matching prefix.
// We do this to avoid having a `try` function and working better with `alt`
#define CPC_STRING(name, lit)                                                                                                   \
CPC_DEFINE_PARSER(name) {                                                                                                       \
  (void)A;                                                                                                                      \
  const CpcSlice slice = {.ptr = (lit), .len = sizeof(lit) - 1};                                                                \
                                                                                                                                \
  if (input.len < slice.len) /* short circuit a too short string */                                                             \
    return cpc_res_err(input, CPC_ERR);                                                                                         \
                                                                                                                                \
  for (size_t i = 0; i < slice.len; ++i)                                                                                        \
    if (input.ptr[i] != slice.ptr[i]) /* mismatch */                                                                            \
      return cpc_res_err(input, CPC_ERR);                                                                                       \
                                                                                                                                \
  return cpc_res_ok(cpc_val_slice(cpc_slice_sub(input, 0, slice.len)), cpc_slice_sub(input, slice.len, input.len - slice.len)); \
}

// `alt` for "alternative" is the equivalent of Parsec `<|>`
#define CPC_ALT(name, x, y)      \
CPC_DEFINE_PARSER(name) {        \
  CpcResult res = (x)(A, input); \
  if (cpc_is_ok(res))            \
    return res;                  \
  else                           \
    return (y)(A, input);        \
}

// `right` is the equivalent of Haskell's Applicative right sequencing `*>`
#define CPC_RIGHT(name, x, y)    \
CPC_DEFINE_PARSER(name) {        \
  CpcResult res = (x)(A, input); \
  if (!cpc_is_ok(res))           \
    return res;                  \
  else                           \
    return (y)(A, res.rest);     \
}

// `left` is the equivalent of Haskell's Applicative left sequencing `<*`
#define CPC_LEFT(name, x, y)              \
CPC_DEFINE_PARSER(name) {                 \
  CpcResult res1 = (x)(A, input);         \
  if (!cpc_is_ok(res1)) return res1;      \
                                          \
  CpcResult res2 = (y)(A, res1.rest);     \
  if (!cpc_is_ok(res2)) return res2;      \
                                          \
  return cpc_res_ok(res1.out, res2.rest); \
}

// `apply` is the equivalent of Haskell's Applicative sequential application `<*>
// It produces a list of 2 elements, but this can be mapped to another struct
#define CPC_APPLY(name, x, y)                           \
CPC_DEFINE_PARSER(name) {                               \
  CpcResult  r1 = (x)(A, input);                        \
  if (!cpc_is_ok(r1)) return r1;                        \
  CpcResult  r2 = (y)(A, r1.rest);                      \
  if (!cpc_is_ok(r2)) return r2;                        \
                                                        \
  CpcValue out = cpc_val_list(A);                       \
                                                        \
  CpcResKind resk = cpc_val_list_push(A, &out, r1.out); \
  if (resk != CPC_OK)                                   \
    return cpc_res_err(r1.rest, resk);                  \
                                                        \
  resk = cpc_val_list_push(A, &out, r2.out);            \
  if (resk != CPC_OK)                                   \
    return cpc_res_err(r1.rest, resk);                  \
                                                        \
  return cpc_res_ok(out, r2.rest);                      \
}

// `fmap` is the equivalent of Haskell's `<$>`
#define CPC_FMAP(name, x, fn)     \
CPC_DEFINE_PARSER(name) {         \
  CpcResult r = x(A, input);      \
  return (fn)(A, &r.out, r.rest); \
}

#endif /* CPARSEC_H_INCLUDED */

#ifdef CPARSEC_IMPLEMENTATION

#endif /* CPARSEC_IMPLEMENTATION */
