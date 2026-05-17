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

    CpcResult result = cpc_parse(NULL, &pbegin, "BEGIN leftovers");
    assert(cpc_is_ok(result));
    assert(strncmp(result.out.as.slice.ptr, "BEGIN", result.out.as.slice.len) == 0);
    assert(strncmp(result.rest.ptr, " leftovers", result.rest.len) == 0);
  }

  {
    puts("If a part fails it returns the whole string...");

    CpcResult result = cpc_parse(NULL, &pbegin, "unknown leftovers");
    assert(!cpc_is_ok(result));
    assert(result.out.as.slice.len == 0);
    assert(strncmp(result.rest.ptr, "unknown leftovers", result.rest.len) == 0);
  }

  {
    puts("The alternative parser works...");

    CpcResult result = cpc_parse(NULL, &pcombined, "END leftovers");
    assert(cpc_is_ok(result));
    assert(strncmp(result.out.as.slice.ptr, "END", result.out.as.slice.len) == 0);
    assert(strncmp(result.rest.ptr, " leftovers", result.rest.len) == 0);
  }

  {
    CParsec p1  = cpc_string("value="),
            p2  = cpc_string("12345"),
            p3  = cpc_right(&p1, &p2);

    puts("The right parser works...");

    CpcResult result = cpc_parse(NULL, &p3, "value=12345");
    assert(cpc_is_ok(result));
    assert(strncmp(result.out.as.slice.ptr, "12345", result.out.as.slice.len) == 0);
    assert(result.rest.len == 0);
  }

  {
    CParsec p1 = cpc_string("select 1"),
            p2 = cpc_string(";"),
            p3 = cpc_left(&p1, &p2);

    puts("The left parser works...");

    CpcResult result = cpc_parse(NULL, &p3, "select 1;");
    assert(cpc_is_ok(result));
    assert(strncmp(result.out.as.slice.ptr, "select 1", result.out.as.slice.len) == 0);
    assert(result.rest.len == 0);
  }

  {
    typedef struct {
      char x;
      char y;
    } Pair;

    CpcResult to_pair(CpcArena *A, const CpcValue *v, CpcSlice rest, void *data){
      Pair *pair = data;
      pair->x = cpc_val_list_at(A, v, 0)->as.slice.ptr[0];
      pair->y = cpc_val_list_at(A, v, 1)->as.slice.ptr[0];

      return cpc_res_ok(cpc_val_ptr(pair), rest);
    }

    CpcValue arena_storage[2];
    CpcArena arena;
    Pair pair = {0};
    cpc_arena_init(&arena, arena_storage, sizeof(arena_storage) / sizeof(arena_storage[0]));

    CParsec p1 = cpc_string("A"),
            p2 = cpc_string("B"),
            p3 = cpc_apply(&p1, &p2),
            p4 = cpc_fmap(&p3, to_pair, &pair);

    puts("The apply + fmap parser works...");

    CpcResult result = cpc_parse(&arena, &p4, "AB");

    assert(cpc_is_ok(result));
    assert(pair.x == 'A');
    assert(pair.y == 'B');
    assert(result.rest.len == 0);
  }

  return EXIT_SUCCESS;
}
