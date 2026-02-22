#ifndef ZPARSEC_H_INCLUDED

#define ZPARSEC_H_INCLUDED

#include <stdbool.h>

// The basic element of the parser are string slices to allow for zero-copy parsing
typedef struct {
  const char *ptr;
  size_t      len;
} ZParsecSlice;

ZParsecSlice slice_sub(ZParsecSlice s, size_t start, size_t len);
ZParsecSlice slice_from_cstr(const char *s);

#endif /* ZPARSEC_H_INCLUDED */

#ifdef ZPARSEC_IMPLEMENTATION

ZParsecSlice slice_sub(ZParsecSlice s, size_t start, size_t len) {
  return (ZParsecSlice){.ptr = s.ptr + start, .len = len};
}

ZParsecSlice slice_from_cstr(const char *s) {
  size_t n = 0;
  while (s[n] != '\0')
    n++;
  return (ZParsecSlice){.ptr = s, .len = n};
}


#endif /* ZPARSEC_IMPLEMENTATION */
