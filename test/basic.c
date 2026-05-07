#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define CPARSEC_IMPLEMENTATION
#include "cparsec.h"

int main() {
  CParsec pbegin = cpc_string("BEGIN"),
          pend   = cpc_string("END"),
          pcombined = cpc_alt(&pbegin, &pend);

  {
    puts("Succeeds over the whole string...");

    CpcResult result = cpc_parse(&pbegin, "BEGIN leftovers");
    assert(result.ok);
    assert(strncmp(result.out.as.slice.ptr, "BEGIN", result.out.as.slice.len) == 0);
    assert(strncmp(result.rest.ptr, " leftovers", result.rest.len) == 0);
  }

  {
    puts("If a part fails it returns the whole string...");

    CpcResult result = cpc_parse(&pbegin, "unknown leftovers");
    assert(!result.ok);
    assert(result.out.as.slice.len == 0);
    assert(strncmp(result.rest.ptr, "unknown leftovers", result.rest.len) == 0);
  }

  {
    puts("The alternative parser works...");

    CpcResult result = cpc_parse(&pcombined, "END leftovers");
    assert(result.ok);
    assert(strncmp(result.out.as.slice.ptr, "END", result.out.as.slice.len) == 0);
    assert(strncmp(result.rest.ptr, " leftovers", result.rest.len) == 0);
  }

  {
    CParsec p1  = cpc_string("value="),
            p2  = cpc_string("12345"),
            p3  = cpc_right(&p1, &p2);

    puts("The right parser works...");

    CpcResult result = cpc_parse(&p3, "value=12345");
    assert(result.ok);
    assert(strncmp(result.out.as.slice.ptr, "12345", result.out.as.slice.len) == 0);
    assert(result.rest.len == 0);
  }

  {
    CParsec p1 = cpc_string("select 1"),
            p2 = cpc_string(";"),
            p3 = cpc_left(&p1, &p2);

    puts("The left parser works...");

    CpcResult result = cpc_parse(&p3, "select 1;");
    assert(result.ok);
    assert(strncmp(result.out.as.slice.ptr, "select 1", result.out.as.slice.len) == 0);
    assert(result.rest.len == 0);
  }

  return EXIT_SUCCESS;
}
