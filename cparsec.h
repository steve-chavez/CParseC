#ifndef CPARSEC_H_INCLUDED
#define CPARSEC_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>

#ifdef CPC_USE_MEMCHR
#  include <string.h>
#endif

// The basic element of the parser are string slices to enable zero-copy parsing
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
  CpcValue *items;  // storage items
  size_t    cap;    // capacity
  size_t    offset; // bump pointer offset [0..cap]
  void     *user;   // user pointer data for use on the fmap, we don't handle its
                    // lifecycle
} CpcArena;

// the signature is like this because both the arena and the items have to be
// declared separately to allow static or stack allocation
static inline void cpc_arena_init(CpcArena *a, CpcValue *items, size_t cap, void *user) {
  *a = (CpcArena){.items = items, .cap = cap, .offset = 0, .user = user};
}

// reset the arena offset
static inline void cpc_arena_reset(CpcArena *a) {
  a->offset = 0;
}

// returns the accepted output value and the rest of the string as another slice
typedef struct {
  bool        ok;
  CpcValue    out;
  CpcSlice    rest;
  const char *err;
} CpcResult;

static inline CpcResult cpc_res_ok(CpcValue out, CpcSlice rest) {
  return (CpcResult){.out = out, .rest = rest, .ok = true};
}

static inline CpcResult cpc_res_err(CpcSlice rest, const char *err) {
  return (CpcResult){.out = (CpcValue){.kind = CPC_NOTHING}, .rest = rest, .ok = false, .err = err};
}

static inline CpcValue cpc_val_slice(CpcSlice s) {
  return (CpcValue){.kind = CPC_SLICE, .as.slice = s};
}

static inline CpcValue cpc_val_list(CpcArena *A) {
  return (CpcValue){.kind = CPC_LIST, .as.list = {.start = A->offset, .len = 0}};
}

static inline CpcValue cpc_val_nothing(void) {
  return (CpcValue){.kind = CPC_NOTHING};
}

static inline CpcValue cpc_val_ptr(void *p) {
  return (CpcValue){.kind = CPC_PTR, .as.ptr = p};
}

static inline bool cpc_is_list(const CpcValue *v) {
  return v && v->kind == CPC_LIST;
}

static inline bool cpc_is_slice(const CpcValue *v) {
  return v && v->kind == CPC_SLICE;
}

static inline bool cpc_is_nothing(const CpcValue *v) {
  return v && v->kind == CPC_NOTHING;
}

static inline bool cpc_val_list_push(CpcArena *A, CpcValue *list, CpcValue x) {
  // TODO differentiate between these errors
  if (!A) return false;
  if (!cpc_is_list(list)) return false;
  if (A->offset >= A->cap) return false;

  A->items[A->offset++] = x;
  list->as.list.len++;

  return true;
}

static inline const CpcValue *cpc_val_list_at(const CpcArena *A, const CpcValue *list, size_t i) {
  if (!A || !cpc_is_list(list) || i >= list->as.list.len) return NULL;
  return &A->items[list->as.list.start + i];
}

// If the current slice is the same as the previous slice, no progress was made.
// This condition ensures all the parsers never cause infinite loops, which is a
// problem in Haskell Parsec. See
// https://hackage.haskell.org/package/attoparsec-0.14.4/docs/Data-Attoparsec-ByteString.html#v:takeWhile
static inline bool cpc_no_progress_made(const CpcSlice cur, const CpcSlice prev) {
  return (cur.ptr == prev.ptr && cur.len == prev.len);
}

// Note that GCC unused really means "may be unused", some parsers do use the
// arena (and no error is thrown) and others don't use it.
#define CPC_DEFINE_PARSER(name) CpcResult name(__attribute__((unused)) CpcArena *A, CpcSlice input)

// This is more like Parsec `string'`, which doesn't consume the matching
// prefix. We do this to avoid having a `try` function and working better with
// `alt`
#define ___CPC_STRING(name, lit, err)                                                              \
  CPC_DEFINE_PARSER(name) {                                                                        \
    const CpcSlice slice = {.ptr = (lit), .len = sizeof(lit) - 1};                                 \
                                                                                                   \
    if (input.len < slice.len) return cpc_res_err(input, (err));                                   \
                                                                                                   \
    for (size_t i = 0; i < slice.len; ++i)                                                         \
      if (input.ptr[i] != slice.ptr[i]) return cpc_res_err(input, (err));                          \
                                                                                                   \
    return cpc_res_ok(cpc_val_slice(cpc_slice_sub(input, 0, slice.len)),                           \
                      cpc_slice_sub(input, slice.len, input.len - slice.len));                     \
  }

#define CPC_STRING(name, lit) ___CPC_STRING(name, lit, "mismatch")
#define CPC_STRING_LABEL(name, lit, label) ___CPC_STRING(name, lit, label)

#define ___CPC_ANY(name, err)                                                                      \
  CPC_DEFINE_PARSER(name) {                                                                        \
    if (input.len == 0) return cpc_res_err(input, (err));                                          \
    return cpc_res_ok(cpc_val_slice(cpc_slice_sub(input, 0, 1)),                                   \
                      cpc_slice_sub(input, 1, input.len - 1));                                     \
  }

// Succeeds if there is at least one character of input. Returns the parsed character.
static inline ___CPC_ANY(CPC_ANY_, "eof")
#define CPC_ANY_LABEL(name, label) ___CPC_ANY(name, label)

#define ___CPC_ONE_OF(name, chars, err)                                                            \
  CPC_DEFINE_PARSER(name) {                                                                        \
    if (input.len == 0) return cpc_res_err(input, (err));                                          \
    for (size_t i = 0; (chars)[i] != '\0'; i++)                                                    \
      if (input.ptr[0] == (chars)[i])                                                              \
        return cpc_res_ok(cpc_val_slice(cpc_slice_sub(input, 0, 1)),                               \
                          cpc_slice_sub(input, 1, input.len - 1));                                 \
    return cpc_res_err(input, (err));                                                              \
  }

// Succeeds if the character is in the supplied string. Returns the parsed character.
#define CPC_ONE_OF(name, chars) ___CPC_ONE_OF(name, chars, "none matched")
#define CPC_ONE_OF_LABEL(name, chars, label) ___CPC_ONE_OF(name, chars, label)

// `alt` for "alternative" is the equivalent of Parsec `<|>`
#define CPC_ALT(name, x, y)                                                                        \
  CPC_DEFINE_PARSER(name) {                                                                        \
    CpcResult res = (x)(A, input);                                                                 \
    if (res.ok)                                                                                    \
      return res;                                                                                  \
    else                                                                                           \
      return (y)(A, input);                                                                        \
  }

// `right` is the equivalent of Haskell's Applicative right sequencing `*>`
#define CPC_RIGHT(name, x, y)                                                                      \
  CPC_DEFINE_PARSER(name) {                                                                        \
    CpcResult res = (x)(A, input);                                                                 \
    if (!res.ok)                                                                                   \
      return res;                                                                                  \
    else                                                                                           \
      return (y)(A, res.rest);                                                                     \
  }

// `left` is the equivalent of Haskell's Applicative left sequencing `<*`
#define CPC_LEFT(name, x, y)                                                                       \
  CPC_DEFINE_PARSER(name) {                                                                        \
    CpcResult res1 = (x)(A, input);                                                                \
    if (!res1.ok) return res1;                                                                     \
                                                                                                   \
    CpcResult res2 = (y)(A, res1.rest);                                                            \
    if (!res2.ok) return res2;                                                                     \
                                                                                                   \
    return cpc_res_ok(res1.out, res2.rest);                                                        \
  }

// `apply` is the equivalent of Haskell's Applicative sequential application
// `<*> It produces a list of 2 elements, but this can be mapped to another
// struct
#define CPC_APPLY(name, x, y)                                                                      \
  CPC_DEFINE_PARSER(name) {                                                                        \
    CpcResult r1 = (x)(A, input);                                                                  \
    if (!r1.ok) return r1;                                                                         \
    CpcResult r2 = (y)(A, r1.rest);                                                                \
    if (!r2.ok) return r2;                                                                         \
                                                                                                   \
    CpcValue out = cpc_val_list(A);                                                                \
                                                                                                   \
    if (!cpc_val_list_push(A, &out, r1.out)) return cpc_res_err(r1.rest, "arena surpassed");       \
                                                                                                   \
    if (!cpc_val_list_push(A, &out, r2.out)) return cpc_res_err(r2.rest, "arena surpassed");       \
                                                                                                   \
    return cpc_res_ok(out, r2.rest);                                                               \
  }

// `map` is the equivalent of Haskell's `<$>`. Does not fail.
#define CPC_MAP(name, x, fn)                                                                       \
  CPC_DEFINE_PARSER(name) {                                                                        \
    CpcResult r = x(A, input);                                                                     \
    return (fn)(A, &r.out, r.rest);                                                                \
  }

#define ___CPC_TAKE_WHILE(name, pred, validate)                                                    \
  CPC_DEFINE_PARSER(name) {                                                                        \
    size_t i = 0;                                                                                  \
    while (i < input.len && (pred)(input.ptr[i]))                                                  \
      i++;                                                                                         \
    validate;                                                                                      \
    /* return the success from [0..i] and the rest from [i..len-i] */                              \
    return cpc_res_ok(cpc_val_slice(cpc_slice_sub(input, 0, i)),                                   \
                      cpc_slice_sub(input, i, input.len - i));                                     \
  }

// Consume input as long as the predicate returns true, and return the consumed
// input. This parser requires the predicate to succeed on at least one char of
// input: it will fail if the predicate never returns true or if there is no
// input left.
#define CPC_TAKE_WHILE_1(name, pred)                                                               \
  ___CPC_TAKE_WHILE(name, pred, if (i < 1) return cpc_res_err(input, "too few"))
#define CPC_TAKE_WHILE_1_LABEL(name, pred, label)                                                  \
  ___CPC_TAKE_WHILE(name, pred, if (i < 1) return cpc_res_err(input, (label)))

// Consume input as long as the predicate returns true, and return the consumed
// input. This parser does not fail. If the predicate returns false at first
// char, it returns an empty string as the slice. Does not fail.
#define CPC_TAKE_WHILE(name, pred) ___CPC_TAKE_WHILE(name, pred, (void)0)

#define ___CPC_MANY(name, parser, min_count, err_too_few)                                          \
  CPC_DEFINE_PARSER(name) {                                                                        \
    CpcValue out   = cpc_val_list(A);                                                              \
    CpcSlice cur   = input;                                                                        \
    size_t   count = 0;                                                                            \
    size_t   min   = (size_t)(min_count);                                                          \
    /* this loop will always terminate, see below conditions */                                    \
    for (;;) {                                                                                     \
      CpcResult r = (parser)(A, cur);                                                              \
      if (!r.ok) {                                                                                 \
        break;                                                                                     \
      }                                                                                            \
      if (cpc_no_progress_made(r.rest, cur)) return cpc_res_err(input, "no progress");             \
      if (!cpc_val_list_push(A, &out, r.out)) return cpc_res_err(input, "arena surpassed");        \
      cur = r.rest;                                                                                \
      count++;                                                                                     \
    }                                                                                              \
    return count < min ? cpc_res_err(input, (err_too_few)) : cpc_res_ok(out, cur);                 \
  }

// Parses zero or more occurrences of the given parser.
// Unlike the Haskell version this will always terminate, even when paired with
// takewhile. Does not fail.
#define CPC_MANY(name, parser) ___CPC_MANY(name, parser, 0, "too few")

// Parses one or more occurrences of the given parser.
#define CPC_MANY_1(name, parser) ___CPC_MANY(name, parser, 1, "too few")
#define CPC_MANY_1_LABEL(name, parser, label) ___CPC_MANY(name, parser, 1, label)

// Parses zero or more occurrences of parser `item`, until parser `end`
// succeeds. Returns a list of values returned by p. Does not fail.
#define CPC_MANY_TILL(name, item, end)                                                             \
  CPC_DEFINE_PARSER(name) {                                                                        \
    CpcValue out = cpc_val_list(A);                                                                \
    CpcSlice cur = input;                                                                          \
    /* this loop will always terminate, see below conditions */                                    \
    for (;;) {                                                                                     \
      CpcResult rend = (end)(A, cur);                                                              \
      if (rend.ok) return cpc_res_ok(out, rend.rest);                                              \
                                                                                                   \
      CpcResult ritem = (item)(A, cur);                                                            \
      if (!ritem.ok) return ritem;                                                                 \
                                                                                                   \
      if (cpc_no_progress_made(ritem.rest, cur)) return cpc_res_err(input, "no progress");         \
      if (!cpc_val_list_push(A, &out, ritem.out)) return cpc_res_err(input, "arena surpassed");    \
                                                                                                   \
      cur = ritem.rest;                                                                            \
    }                                                                                              \
  }

#define ___CPC_SEP_BY(name, item, sep, first_not_ok)                                               \
  CPC_DEFINE_PARSER(name) {                                                                        \
    CpcValue  out   = cpc_val_list(A);                                                             \
    CpcSlice  cur   = input;                                                                       \
    CpcResult first = (item)(A, cur);                                                              \
    if (!first.ok) {                                                                               \
      return first_not_ok;                                                                         \
    }                                                                                              \
                                                                                                   \
    if (!cpc_val_list_push(A, &out, first.out)) return cpc_res_err(input, "arena surpassed");      \
                                                                                                   \
    cur = first.rest;                                                                              \
    /* this loop will always terminate, see below conditions */                                    \
    for (;;) {                                                                                     \
      CpcSlice  before_sep = cur;                                                                  \
      CpcResult rsep       = (sep)(A, cur);                                                        \
      if (!rsep.ok) {                                                                              \
        break;                                                                                     \
      }                                                                                            \
      CpcResult next = (item)(A, rsep.rest);                                                       \
      if (!next.ok) {                                                                              \
        break;                                                                                     \
      }                                                                                            \
                                                                                                   \
      if (!cpc_val_list_push(A, &out, next.out)) return cpc_res_err(input, "arena surpassed");     \
                                                                                                   \
      cur = next.rest;                                                                             \
      if (cpc_no_progress_made(cur, before_sep)) return cpc_res_err(input, "no progress");         \
    }                                                                                              \
    return cpc_res_ok(out, cur);                                                                   \
  }

// Parses zero or more occurrences of `item`, separated by `sep`.
// Returns a list of values returned by `item`. Does not fail.
#define CPC_SEP_BY(name, item, sep) ___CPC_SEP_BY(name, item, sep, cpc_res_ok(out, input))

// Parses one or more occurrences of `item`, separated by `sep`.
// Returns a list of values returned by `item`.
#define CPC_SEP_BY_1(name, item, sep) ___CPC_SEP_BY(name, item, sep, cpc_res_err(input, "too few"))
#define CPC_SEP_BY_1_LABEL(name, item, sep, label)                                                 \
  ___CPC_SEP_BY(name, item, sep, cpc_res_err(input, (label)))

// Returns a value wrapped in the parser. Does not fail.
#define CPC_PURE(name, value_expr)                                                                 \
  CPC_DEFINE_PARSER(name) {                                                                        \
    return cpc_res_ok((value_expr), input);                                                        \
  }
// Parses open, followed by inner and finally close. Only the value of inner is returned.
#define CPC_BETWEEN(name, open, inner, close)                                                      \
  CPC_DEFINE_PARSER(name) {                                                                        \
    CpcResult ro = (open)(A, input);                                                               \
    if (!ro.ok) return ro;                                                                         \
    CpcResult ri = (inner)(A, ro.rest);                                                            \
    if (!ri.ok) return ri;                                                                         \
    CpcResult rc = (close)(A, ri.rest);                                                            \
    if (!rc.ok) return rc;                                                                         \
    return cpc_res_ok(ri.out, rc.rest);                                                            \
  }

// Run `parser` and tell what substring was matched, like attoparsec `match`
#define CPC_MATCH(name, parser)                                                                    \
  CPC_DEFINE_PARSER(name) {                                                                        \
    /* mark is for restoring the arena state */                                                    \
    size_t    mark = A->offset;                                                                    \
    CpcResult r    = (parser)(A, input);                                                           \
    A->offset      = mark;                                                                         \
    return r.ok ? cpc_res_ok(                                                                      \
                      cpc_val_slice(cpc_slice_sub(input, 0, (size_t)(r.rest.ptr - input.ptr))),    \
                      r.rest)                                                                      \
                : r;                                                                               \
  }

// Consume input as long as the predicate returns false, and return the consumed input.
// This parser does not fail. If the predicate returns false at first
// char, it returns an empty string as the slice.
#define CPC_TAKE_TILL(name, pred)                                                                  \
  CPC_DEFINE_PARSER(name) {                                                                        \
    size_t i = 0;                                                                                  \
    while (i < input.len && !(pred)(input.ptr[i]))                                                 \
      i++;                                                                                         \
    return cpc_res_ok(cpc_val_slice(cpc_slice_sub(input, 0, i)),                                   \
                      cpc_slice_sub(input, i, input.len - i));                                     \
  }

#ifdef CPC_USE_MEMCHR
// combination of CPC_TAKE_TILL + CPC_ONE_OF that is SIMD-friendly thanks
// to memchr
#  define CPC_TAKE_TILL_ONE_OF(name, stops)                                                        \
    CPC_DEFINE_PARSER(name) {                                                                      \
      size_t end = input.len;                                                                      \
      for (const char *s = (stops); *s; ++s) {                                                     \
        const char *p = memchr(input.ptr, *s, end);                                                \
        if (p) end = (size_t)(p - input.ptr);                                                      \
      }                                                                                            \
      return cpc_res_ok(cpc_val_slice(cpc_slice_sub(input, 0, end)),                               \
                        cpc_slice_sub(input, end, input.len - end));                               \
    }

// Parses a quoted string, handling doubled quotes as escaped content. Returns a slice.
#  define CPC_TAKE_QUOTED(name, quote)                                                             \
    CPC_DEFINE_PARSER(name) {                                                                      \
      /* rejects anything that is too short (quoted would need at least 3 chars) or does not start \
       * with the quote*/                                                                          \
      if (input.len < 2 || input.ptr[0] != (quote)) return cpc_res_err(input, "missing quote");    \
      /* start after the opening quote */                                                          \
      size_t span = 1;                                                                             \
      while (span < input.len) {                                                                   \
        /* Jump to the next quote candidate instead of scanning char by char */                    \
        const char *p = memchr(input.ptr + span, (quote), input.len - span);                       \
        if (!p) break;                                                                             \
        size_t idx = (size_t)(p - input.ptr);                                                      \
        if ((idx + 1) < input.len && input.ptr[idx + 1] == (quote)) {                              \
          /* A doubled quote is escaped content inside the quoted span */                          \
          span = idx + 2;                                                                          \
          continue;                                                                                \
        }                                                                                          \
        /* A lone quote closes the span, including the opening quote at index 0 */                 \
        span = idx + 1;                                                                            \
        return cpc_res_ok(cpc_val_slice(cpc_slice_sub(input, 0, span)),                            \
                          cpc_slice_sub(input, span, input.len - span));                           \
      }                                                                                            \
      /* err if no closing quote is found */                                                       \
      return cpc_res_err(input, "missing quote");                                                  \
    }
#endif

#define ___CPC_EOF(name, err)                                                                      \
  CPC_DEFINE_PARSER(name) {                                                                        \
    return input.len == 0 ? cpc_res_ok(cpc_val_nothing(), input) : cpc_res_err(input, (err));      \
  }

    // parser that only matches if all the input has been consumed
    static inline ___CPC_EOF(CPC_EOF_, "expected eof")

#define CPC_EOF_LABEL(name, label) ___CPC_EOF(name, label)

#define ___CPC_END_OF_LINE(name, err)                                                              \
  CPC_DEFINE_PARSER(name) {                                                                        \
    if (input.len == 0) return cpc_res_err(input, (err));                                          \
    const char *p = input.ptr;                                                                     \
    if (p[0] == '\r') {                                                                            \
      if (input.len >= 2 && p[1] == '\n')                                                          \
        return cpc_res_ok(cpc_val_slice(cpc_slice_sub(input, 0, 2)),                               \
                          cpc_slice_sub(input, 2, input.len - 2));                                 \
      return cpc_res_ok(cpc_val_slice(cpc_slice_sub(input, 0, 1)),                                 \
                        cpc_slice_sub(input, 1, input.len - 1));                                   \
    }                                                                                              \
    if (p[0] == '\n')                                                                              \
      return cpc_res_ok(cpc_val_slice(cpc_slice_sub(input, 0, 1)),                                 \
                        cpc_slice_sub(input, 1, input.len - 1));                                   \
    return cpc_res_err(input, (err));                                                              \
  }

    // Parses a CRLF (see crlf) or LF (see newline) end-of-line
    static inline ___CPC_END_OF_LINE(CPC_END_OF_LINE_, "expected newline")
#define CPC_END_OF_LINE_LABEL(name, label) ___CPC_END_OF_LINE(name, label)

#endif /* CPARSEC_H_INCLUDED */
