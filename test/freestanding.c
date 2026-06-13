// This is a "freestanding" test, to ensure cparsec.h doesn't depend on the libc
// or any other dependencies by default

// adapted from https://stackoverflow.com/a/12137265/4692662
static int test_strcmp(const char *s1, const char *s2) {
  while (*s1 != '\0' && *s1 == *s2) {
    s1++;
    s2++;
  }

  return (unsigned char)*s1 - (unsigned char)*s2;
}

// taken from https://stackoverflow.com/a/68800213/4692662
int test_strncmp(const char *s1, const char *s2, int n) {
  for (int i = 0; i < n; i++) {
    int diff = (unsigned char)s1[i] - (unsigned char)s2[i];
    if (diff != 0 || s1[i] == '\0') return diff;
  }
  return 0;
}

#define ASSERT(expr)                                                                               \
  do {                                                                                             \
    if (!(expr)) __builtin_trap();                                                                 \
  } while (0)
#define PUTS(msg) ((void)(msg))
#define STRCMP(s1, s2) test_strcmp((s1), (s2))
#define STRNCMP(s1, s2, n) test_strncmp((s1), (s2), (n))

#include "cparsec.h"

#include "basic.h"
