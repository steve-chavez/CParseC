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

    ZParsecResult result = zparse(&pbegin, slice_from_cstr("BEGIN leftovers"));
    assert(result.ok);
    assert(strncmp(result.out.ptr, "BEGIN", result.out.len) == 0);
    assert(strncmp(result.rest.ptr, " leftovers", result.rest.len) == 0);
  }

  {
    puts("If a part fails it returns the whole string...");

    ZParsecResult result = zparse(&pbegin, slice_from_cstr("unknown leftovers"));
    assert(!result.ok);
    assert(result.out.len == 0);
    assert(strncmp(result.rest.ptr, "unknown leftovers", result.rest.len) == 0);
  }

  {
    puts("The alternative parser works...");

    ZParsecResult result = zparse(&pcombined, slice_from_cstr("END leftovers"));
    assert(result.ok);
    assert(strncmp(result.out.ptr, "END", result.out.len) == 0);
    assert(strncmp(result.rest.ptr, " leftovers", result.rest.len) == 0);
  }

  return EXIT_SUCCESS;
}
