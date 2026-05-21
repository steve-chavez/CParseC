#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define CPARSEC_IMPLEMENTATION
#include "cparsec.h"

CPC_STRING(p_begin, "BEGIN");
CPC_STRING(p_end, "END");
CPC_ALT(p_combined, p_begin, p_end);

CPC_STRING(p_val, "value=");
CPC_STRING(p_num, "12345");
CPC_RIGHT(p_valnum, p_val, p_num);

CPC_STRING(p_sel, "select 1");
CPC_STRING(p_semicol, ";");
CPC_LEFT(p_stmt, p_sel, p_semicol);

typedef struct {
  char x;
  char y;
} Pair;

CpcResult to_pair(CpcArena *A, const CpcValue *v, CpcSlice rest){
  Pair *pair = A->user;
  pair->x = cpc_val_list_at(A, v, 0)->as.slice.ptr[0];
  pair->y = cpc_val_list_at(A, v, 1)->as.slice.ptr[0];

  return cpc_res_ok(cpc_val_ptr(pair), rest);
}

CPC_STRING(p_a, "A");
CPC_STRING(p_b, "B");
CPC_APPLY(p_ab, p_a, p_b);
CPC_FMAP(p_mapped_ab, p_ab, to_pair);

static inline bool is_a(char c){
  return c == 'a';
}

CPC_TAKE_WHILE(p_only_a, is_a);
CPC_TAKE_WHILE_1(p_at_least_1_a, is_a);

int main() {
  {
    puts("Succeeds over the whole string...");

    CpcResult result = p_begin(NULL, cpc_slice_from_cstr("BEGIN leftovers"));
    assert(cpc_is_ok(result));
    assert(strncmp(result.out.as.slice.ptr, "BEGIN", result.out.as.slice.len) == 0);
    assert(strncmp(result.rest.ptr, " leftovers", result.rest.len) == 0);
  }

  {
    puts("If a part fails it returns the whole string...");

    CpcResult result = p_begin(NULL, cpc_slice_from_cstr("unknown leftovers"));
    assert(!cpc_is_ok(result));
    assert(result.out.as.slice.len == 0);
    assert(strncmp(result.rest.ptr, "unknown leftovers", result.rest.len) == 0);
  }

  {
    puts("The alternative parser works...");

    CpcResult result = p_combined(NULL, cpc_slice_from_cstr("END leftovers"));
    assert(cpc_is_ok(result));
    assert(strncmp(result.out.as.slice.ptr, "END", result.out.as.slice.len) == 0);
    assert(strncmp(result.rest.ptr, " leftovers", result.rest.len) == 0);
  }

  {
    puts("The right parser works...");

    CpcResult result = p_valnum(NULL, cpc_slice_from_cstr("value=12345"));
    assert(cpc_is_ok(result));
    assert(strncmp(result.out.as.slice.ptr, "12345", result.out.as.slice.len) == 0);
    assert(result.rest.len == 0);
  }

  {
    puts("The left parser works...");

    CpcResult result = p_stmt(NULL, cpc_slice_from_cstr("select 1;"));
    assert(cpc_is_ok(result));
    assert(strncmp(result.out.as.slice.ptr, "select 1", result.out.as.slice.len) == 0);
    assert(result.rest.len == 0);
  }

  {
    CpcValue arena_storage[2];
    CpcArena arena;
    Pair pair = {0};
    cpc_arena_init(&arena, arena_storage, sizeof(arena_storage) / sizeof(arena_storage[0]), &pair);

    puts("The apply + fmap parser works...");

    CpcResult result = p_mapped_ab(&arena, cpc_slice_from_cstr("AB"));

    assert(cpc_is_ok(result));
    assert(pair.x == 'A');
    assert(pair.y == 'B');
    assert(result.rest.len == 0);
  }

  {
    puts("The takewhile parser succeeds...");

    CpcResult result = p_only_a(NULL, cpc_slice_from_cstr("aaaaaaaaaabbbbb"));

    assert(cpc_is_ok(result));
    assert(strncmp(result.out.as.slice.ptr, "aaaaaaaaaa", result.out.as.slice.len) == 0);
    assert(strncmp(result.rest.ptr, "bbbbb", result.rest.len) == 0);

    puts("The takewhile parser never fails...");

    CpcResult result2 = p_only_a(NULL, cpc_slice_from_cstr("aabbbbbaaaa"));

    assert(cpc_is_ok(result2));
    assert(strncmp(result2.out.as.slice.ptr, "aa", result2.out.as.slice.len) == 0);
    assert(strncmp(result2.rest.ptr, "bbbbbaaaa", result2.rest.len) == 0);

    puts("The takewhile parser never fails, it returns empty string if the pred returns false at first char...");

    CpcResult result1 = p_only_a(NULL, cpc_slice_from_cstr("bbbbbaaaa"));

    assert(cpc_is_ok(result1));
    assert(strncmp(result1.out.as.slice.ptr, "", result1.out.as.slice.len) == 0);
    assert(strncmp(result1.rest.ptr, "bbbbbaaaa", result1.rest.len) == 0);
  }

  {
    puts("The takewhile1 parser succeeds...");

    CpcResult result = p_at_least_1_a(NULL, cpc_slice_from_cstr("abb"));

    assert(cpc_is_ok(result));
    assert(strncmp(result.out.as.slice.ptr, "a", result.out.as.slice.len) == 0);
    assert(strncmp(result.rest.ptr, "bb", result.rest.len) == 0);

    puts("The takewhile1 parser does fail...");

    CpcResult result2 = p_at_least_1_a(NULL, cpc_slice_from_cstr("bba"));

    assert(!cpc_is_ok(result2));
    assert(result2.kind == CPC_ERR_TAKE_WHILE_1);
    assert(strncmp(result2.out.as.slice.ptr, "", result2.out.as.slice.len) == 0);
    assert(strncmp(result2.rest.ptr, "bba", result2.rest.len) == 0);

    puts("The takewhile1 parser fails on empty input...");

    CpcResult result3 = p_at_least_1_a(NULL, cpc_slice_from_cstr(""));

    assert(!cpc_is_ok(result3));
    assert(result3.kind == CPC_ERR_TAKE_WHILE_1);
    assert(strncmp(result3.out.as.slice.ptr, "", result3.out.as.slice.len) == 0);
    assert(strncmp(result3.rest.ptr, "", result3.rest.len) == 0);
  }

  return EXIT_SUCCESS;
}
