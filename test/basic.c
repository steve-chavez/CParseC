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

CPC_MANY(p_many_a, p_a);

static inline bool is_space(char c){
  return c == ' ';
}

CPC_TAKE_WHILE(p_is_space, is_space);
CPC_MANY(p_inf_many, p_is_space);

CPC_MANY_1(p_many_1_a, p_a);

CPC_MANY_TILL(p_many_a_till_semicol, p_a, p_semicol);

CPC_MANY_TILL(p_inf_many_till, p_is_space, p_b);

CPC_SEP_BY(p_A_sep_by_space, p_is_space, p_a);

CPC_SEP_BY(p_inf_sep_by, p_is_space, p_is_space);

CPC_SEP_BY_1(p_A_sep_by_1_space, p_is_space, p_a);

static inline bool is_comma(char c){
  return c == ',';
}
CPC_TAKE_WHILE(p_take_while_comma, is_comma);

static inline bool is_not_comma(char c){
  return c != ',';
}
CPC_TAKE_WHILE(p_take_while_not_comma, is_not_comma);

CPC_SEP_BY_1(p_inf_sep_by_1, p_take_while_not_comma, p_take_while_comma);

CPC_LABEL(p_begin_label, p_begin, "bad bad");

int main() {
  {
    puts("The string parser succeeds...");

    CpcResult result = p_begin(NULL, cpc_slice_from_cstr("BEGIN leftovers"));
    assert(result.ok);
    assert(strncmp(result.out.as.slice.ptr, "BEGIN", result.out.as.slice.len) == 0);
    assert(result.rest.len != 0);
    assert(strncmp(result.rest.ptr, " leftovers", result.rest.len) == 0);
  }

  {
    puts("The string parser fails if there's a mismatch...");

    CpcResult result = p_begin(NULL, cpc_slice_from_cstr("unknown leftovers"));
    assert(!result.ok);
    assert(result.out.as.slice.len == 0);
    assert(result.rest.len != 0);
    assert(strncmp(result.rest.ptr, "unknown leftovers", result.rest.len) == 0);
    assert(strcmp(result.err, "p_begin: mismatch") == 0);
  }

  {
    puts("The string parser fails if the input is too short...");

    CpcResult result = p_begin(NULL, cpc_slice_from_cstr("a"));
    assert(!result.ok);
    assert(result.out.as.slice.len == 0);
    assert(result.rest.len != 0);
    assert(strncmp(result.rest.ptr, "a", result.rest.len) == 0);
    assert(strcmp(result.err, "p_begin: too short") == 0);
  }

  {
    puts("The alternative parser works...");

    CpcResult result = p_combined(NULL, cpc_slice_from_cstr("END leftovers"));
    assert(result.ok);
    assert(strncmp(result.out.as.slice.ptr, "END", result.out.as.slice.len) == 0);
    assert(result.rest.len != 0);
    assert(strncmp(result.rest.ptr, " leftovers", result.rest.len) == 0);
  }

  {
    puts("The right parser works...");

    CpcResult result = p_valnum(NULL, cpc_slice_from_cstr("value=12345"));
    assert(result.ok);
    assert(strncmp(result.out.as.slice.ptr, "12345", result.out.as.slice.len) == 0);
    assert(result.rest.len == 0);
  }

  {
    puts("The left parser works...");

    CpcResult result = p_stmt(NULL, cpc_slice_from_cstr("select 1;"));
    assert(result.ok);
    assert(strncmp(result.out.as.slice.ptr, "select 1", result.out.as.slice.len) == 0);
    assert(result.rest.len == 0);
  }

  {
    CpcValue arena_storage[2] = {0};
    CpcArena arena;
    Pair pair = {0};
    cpc_arena_init(&arena, arena_storage, sizeof(arena_storage) / sizeof(arena_storage[0]), &pair);

    puts("The apply + fmap parser works...");

    CpcResult result = p_mapped_ab(&arena, cpc_slice_from_cstr("AB"));

    assert(result.ok);
    assert(pair.x == 'A');
    assert(pair.y == 'B');
    assert(result.rest.len == 0);
  }

  {
    puts("The takewhile parser succeeds...");

    CpcResult result = p_only_a(NULL, cpc_slice_from_cstr("aaaaaaaaaabbbbb"));

    assert(result.ok);
    assert(strncmp(result.out.as.slice.ptr, "aaaaaaaaaa", result.out.as.slice.len) == 0);
    assert(result.rest.len != 0);
    assert(strncmp(result.rest.ptr, "bbbbb", result.rest.len) == 0);

    puts("The takewhile parser never fails...");

    CpcResult result2 = p_only_a(NULL, cpc_slice_from_cstr("aabbbbbaaaa"));

    assert(result2.ok);
    assert(strncmp(result2.out.as.slice.ptr, "aa", result2.out.as.slice.len) == 0);
    assert(result2.rest.len != 0);
    assert(strncmp(result2.rest.ptr, "bbbbbaaaa", result2.rest.len) == 0);

    puts("The takewhile parser never fails, it returns empty string if the pred returns false at first char...");

    CpcResult result1 = p_only_a(NULL, cpc_slice_from_cstr("bbbbbaaaa"));

    assert(result1.ok);
    assert(strncmp(result1.out.as.slice.ptr, "", result1.out.as.slice.len) == 0);
    assert(result1.rest.len != 0);
    assert(strncmp(result1.rest.ptr, "bbbbbaaaa", result1.rest.len) == 0);
  }

  {
    puts("The takewhile1 parser succeeds...");

    CpcResult result = p_at_least_1_a(NULL, cpc_slice_from_cstr("abb"));

    assert(result.ok);
    assert(strncmp(result.out.as.slice.ptr, "a", result.out.as.slice.len) == 0);
    assert(result.rest.len != 0);
    assert(strncmp(result.rest.ptr, "bb", result.rest.len) == 0);

    puts("The takewhile1 parser does fail...");

    CpcResult result2 = p_at_least_1_a(NULL, cpc_slice_from_cstr("bba"));

    assert(!result2.ok);
    assert(strcmp(result2.err, "p_at_least_1_a: too few") == 0);
    assert(strncmp(result2.out.as.slice.ptr, "", result2.out.as.slice.len) == 0);
    assert(result2.rest.len != 0);
    assert(strncmp(result2.rest.ptr, "bba", result2.rest.len) == 0);

    puts("The takewhile1 parser fails on empty input...");

    CpcResult result3 = p_at_least_1_a(NULL, cpc_slice_from_cstr(""));

    assert(!result3.ok);
    assert(strcmp(result3.err, "p_at_least_1_a: too few") == 0);
    assert(strncmp(result3.out.as.slice.ptr, "", result3.out.as.slice.len) == 0);
    assert(result3.rest.len == 0);
  }

  {
    puts("The many parser succeeds...");

    CpcValue arena_storage[10] = {0};
    CpcArena arena;
    cpc_arena_init(&arena, arena_storage, sizeof(arena_storage) / sizeof(arena_storage[0]), NULL);

    {
      CpcResult result = p_many_a(&arena, cpc_slice_from_cstr("AAAAb"));

      assert(result.ok);
      assert(cpc_is_list(&result.out));
      assert(result.out.as.list.len == 4);
      for(size_t i = 0; i < result.out.as.list.len; i++){
        CpcSlice slice_ = cpc_val_list_at(&arena, &result.out, i)->as.slice;
        assert(strncmp(slice_.ptr, "A", slice_.len) == 0);
      }
      assert(result.rest.len != 0);
      assert(strncmp(result.rest.ptr, "b", result.rest.len) == 0);
    }

    puts("The many parser doesn't fail if it doesn't consume any input...");

    {
      CpcResult result = p_many_a(&arena, cpc_slice_from_cstr("aaaab"));

      assert(result.ok);
      assert(cpc_is_list(&result.out));
      assert(result.out.as.list.len == 0);
      assert(result.rest.len != 0);
      assert(strncmp(result.rest.ptr, "aaaab", result.rest.len) == 0);
    }

    puts("The many parser will always finish...");

    {
      CpcResult result = p_inf_many(NULL, cpc_slice_from_cstr("anything"));

      assert(strcmp(result.err, "p_inf_many: must consume input") == 0);
    }

    puts("The many parser will fail if the arena doesn't have enough capacity...");

    {
      CpcResult result = p_many_a(&arena, cpc_slice_from_cstr("AAAAAAAAAAAAA"));

      assert(!result.ok);
      assert(strcmp(result.err, "p_many_a: arena surpassed") == 0);
    }
  }

  {
    puts("The many1 parser succeeds...");

    CpcValue arena_storage[10] = {0};
    CpcArena arena;
    cpc_arena_init(&arena, arena_storage, sizeof(arena_storage) / sizeof(arena_storage[0]), NULL);

    {
      CpcResult result = p_many_1_a(&arena, cpc_slice_from_cstr("AAAb"));

      assert(result.ok);
      assert(cpc_is_list(&result.out));
      assert(result.out.as.list.len == 3);
      for(size_t i = 0; i < result.out.as.list.len; i++){
        CpcSlice slice_ = cpc_val_list_at(&arena, &result.out, i)->as.slice;
        assert(strncmp(slice_.ptr, "A", slice_.len) == 0);
      }
      assert(result.rest.len != 0);
      assert(strncmp(result.rest.ptr, "b", result.rest.len) == 0);
    }

    puts("The many1 parser fails...");

    {
      CpcResult result = p_many_1_a(&arena, cpc_slice_from_cstr("bAAAAb"));

      assert(!result.ok);
      assert(result.rest.len != 0);
      assert(strncmp(result.rest.ptr, "bAAAAb", result.rest.len) == 0);
      assert(strcmp(result.err, "p_many_1_a: too few") == 0);
    }

    puts("The many1 parser will fail if the arena doesn't have enough capacity...");

    {
      CpcResult result = p_many_1_a(&arena, cpc_slice_from_cstr("AAAAAAAAAAAAAA"));

      assert(!result.ok);
      assert(strcmp(result.err, "p_many_1_a: arena surpassed") == 0);
    }
  }

  {
    CpcValue arena_storage[10] = {0};
    CpcArena arena;
    cpc_arena_init(&arena, arena_storage, sizeof(arena_storage) / sizeof(arena_storage[0]), NULL);

    {
      puts("The manytill parser succeeds...");

      CpcResult result = p_many_a_till_semicol(&arena, cpc_slice_from_cstr("AAAAAAAAA;"));

      assert(result.ok);
      assert(cpc_is_list(&result.out));
      assert(result.out.as.list.len == 9);
      for(size_t i = 0; i < result.out.as.list.len; i++){
        CpcSlice slice_ = cpc_val_list_at(&arena, &result.out, i)->as.slice;
        assert(strncmp(slice_.ptr, "A", slice_.len) == 0);
      }
      assert(result.rest.len == 0);
    }

    {
      puts("The manytill parser fails...");

      CpcResult result = p_many_a_till_semicol(&arena, cpc_slice_from_cstr("bb"));

      assert(!result.ok);
      assert(result.rest.len != 0);
      assert(strncmp(result.rest.ptr, "bb", result.rest.len) == 0);
    }

    {
      puts("The manytill parser will always finish...");

      CpcResult result = p_inf_many_till(&arena, cpc_slice_from_cstr("abc"));

      assert(!result.ok);
      assert(strcmp(result.err, "p_inf_many_till: must consume input") == 0);
    }

    {
      puts("The manytill parser will fail if the arena doesn't have enough capacity...");

      CpcResult result = p_many_a_till_semicol(&arena, cpc_slice_from_cstr("AAAAAAAAAAA;"));

      assert(!result.ok);
      assert(strcmp(result.err, "p_many_a_till_semicol: arena surpassed") == 0);
    }
  }

  {
    CpcValue arena_storage[6] = {0};
    CpcArena arena;
    cpc_arena_init(&arena, arena_storage, sizeof(arena_storage) / sizeof(arena_storage[0]), NULL);

    {
      puts("The sepby parser succeeds...");

      CpcResult result = p_A_sep_by_space(&arena, cpc_slice_from_cstr("A A A A B"));

      assert(result.ok);
      assert(cpc_is_list(&result.out));
      assert(result.out.as.list.len == 4);
      for(size_t i = 0; i < result.out.as.list.len; i++){
        CpcSlice slice_ = cpc_val_list_at(&arena, &result.out, i)->as.slice;
        assert(strncmp(slice_.ptr, "A", slice_.len) == 0);
      }
      assert(result.rest.len != 0);
      assert(strncmp(result.rest.ptr, " B", result.rest.len) == 0);
    }

    {
      puts("The sepby parser doesn't fail if it doesn't consume any input...");

      CpcResult result = p_A_sep_by_space(&arena, cpc_slice_from_cstr("B B B"));

      assert(result.ok);
      assert(cpc_is_list(&result.out));
      assert(result.out.as.list.len == 0);
      assert(result.rest.len != 0);
      assert(strncmp(result.rest.ptr, "B B B", result.rest.len) == 0);
    }

    {
      puts("The sepby parser will fail if the arena doesn't have enough capacity...");

      CpcResult result = p_A_sep_by_space(&arena, cpc_slice_from_cstr("A A A A A A A A"));

      assert(!result.ok);
      assert(strcmp(result.err, "p_A_sep_by_space: arena surpassed") == 0);
    }

    {
      puts("The sepby parser will always finish...");
      cpc_arena_reset(&arena);

      CpcResult result = p_inf_sep_by(&arena, cpc_slice_from_cstr("abc"));

      assert(!result.ok);
      assert(strcmp(result.err, "p_inf_sep_by: must consume input") == 0);
    }
  }

  {
    CpcValue arena_storage[6] = {0};
    CpcArena arena;
    cpc_arena_init(&arena, arena_storage, sizeof(arena_storage) / sizeof(arena_storage[0]), NULL);

    {
      puts("The sepby1 parser succeeds...");

      CpcResult result = p_A_sep_by_1_space(&arena, cpc_slice_from_cstr("A A A A B"));

      assert(result.ok);
      assert(cpc_is_list(&result.out));
      assert(result.out.as.list.len == 4);
      for(size_t i = 0; i < result.out.as.list.len; i++){
        CpcSlice slice_ = cpc_val_list_at(&arena, &result.out, i)->as.slice;
        assert(strncmp(slice_.ptr, "A", slice_.len) == 0);
      }
      assert(result.rest.len != 0);
      assert(strncmp(result.rest.ptr, " B", result.rest.len) == 0);
    }

    {
      puts("The sepby1 parser fails...");

      CpcResult result = p_A_sep_by_1_space(&arena, cpc_slice_from_cstr("B A A"));

      assert(!result.ok);
      assert(result.rest.len != 0);
      assert(strncmp(result.rest.ptr, "B A A", result.rest.len) == 0);
      assert(strcmp(result.err, "p_A_sep_by_1_space: too few") == 0);
    }

    {
      puts("The sepby1 parser will always finish...");

      CpcResult result = p_inf_sep_by_1(&arena, cpc_slice_from_cstr(","));

      assert(strcmp(result.err, "p_inf_sep_by_1: must consume input") == 0);
    }
  }

  {
    puts("The eof parser succeeds...");

    CpcResult result = cpc_parser_eof(NULL, cpc_slice_from_cstr(""));

    assert(result.ok);
    assert(result.rest.len == 0);
  }

  {
    puts("The eof parser fails...");

    CpcResult result = cpc_parser_eof(NULL, cpc_slice_from_cstr("A"));

    assert(!result.ok);
    assert(result.rest.len != 0);
    assert(strncmp(result.rest.ptr, "A", result.rest.len) == 0);
    assert(strcmp(result.err, "cpc_parser_eof: expected eof") == 0);
  }

  {
    puts("The label parser works...");

    CpcResult result = p_begin_label(NULL, cpc_slice_from_cstr("BEGAN"));

    assert(!result.ok);
    assert(result.rest.len != 0);
    assert(strncmp(result.rest.ptr, "BEGAN", result.rest.len) == 0);
    assert(strcmp(result.err, "bad bad") == 0);
  }

  return EXIT_SUCCESS;
}
