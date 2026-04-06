#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define ZPARSEC_IMPLEMENTATION
#include "zparsec.h"

int main() {
  ZParsec pbegin = zparsec_string("BEGIN"),
          pend   = zparsec_string("END"),
          pcombined = zparsec_alt(&pbegin, &pend);

  {
    puts("Succeeds over the whole string...");

    ZParsecResult result = zparse(&pbegin, "BEGIN leftovers");
    assert(result.ok);
    assert(strncmp(result.out.ptr, "BEGIN", result.out.len) == 0);
    assert(strncmp(result.rest.ptr, " leftovers", result.rest.len) == 0);
  }

  {
    puts("If a part fails it returns the whole string...");

    ZParsecResult result = zparse(&pbegin, "unknown leftovers");
    assert(!result.ok);
    assert(result.out.len == 0);
    assert(strncmp(result.rest.ptr, "unknown leftovers", result.rest.len) == 0);
  }

  {
    puts("The alternative parser works...");

    ZParsecResult result = zparse(&pcombined, "END leftovers");
    assert(result.ok);
    assert(strncmp(result.out.ptr, "END", result.out.len) == 0);
    assert(strncmp(result.rest.ptr, " leftovers", result.rest.len) == 0);
  }

  {
    ZParsec p1  = zparsec_string("value="),
            p2  = zparsec_string("12345"),
            p3  = zparsec_right(&p1, &p2);

    puts("The right parser works...");

    ZParsecResult result = zparse(&p3, "value=12345");
    assert(result.ok);
    assert(strncmp(result.out.ptr, "12345", result.out.len) == 0);
    assert(result.rest.len == 0);
  }

  return EXIT_SUCCESS;
}
