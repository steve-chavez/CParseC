#ifndef BASIC_H_INCLUDED
#define BASIC_H_INCLUDED

// parsers used across tests
CPC_STRING(p_begin, "BEGIN")

CPC_STRING(p_semicol, ";")

CPC_STRING(p_a, "A")
CPC_STRING(p_b, "B")

static inline bool is_a(char c) {
  return c == 'a';
}

static inline bool is_space(char c) {
  return c == ' ';
}

CPC_TAKE_WHILE(p_is_space, is_space)

int cpc_basic_test_run(void) {
  {
    PUTS("The string parser succeeds...");

    CpcResult result = p_begin(NULL, cpc_slice_from_cstr("BEGIN leftovers"));
    ASSERT_OUT_SLICE_EQ(result, "BEGIN");
    ASSERT_REST_EQ(result, " leftovers");
  }

  {
    PUTS("The string parser fails if there's a mismatch...");

    CpcResult result = p_begin(NULL, cpc_slice_from_cstr("unknown leftovers"));
    ASSERT_OUT_NOTHING(result);
    ASSERT_REST_EQ(result, "unknown leftovers");
    ASSERT_ERR_EQ(result, "mismatch");
  }

  {
    PUTS("The string parser fails if the input is too short...");

    CpcResult result = p_begin(NULL, cpc_slice_from_cstr("a"));
    ASSERT_OUT_NOTHING(result);
    ASSERT_REST_EQ(result, "a");
    ASSERT_ERR_EQ(result, "mismatch");
  }

  {
    PUTS("The string parser can be labeled...");

    CPC_STRING_LABEL(p_begin_l, "BEGIN", "this is wrong")

    CpcResult result = p_begin_l(NULL, cpc_slice_from_cstr("a"));
    ASSERT_OUT_NOTHING(result);
    ASSERT_REST_EQ(result, "a");
    ASSERT_ERR_EQ(result, "this is wrong");
  }

  {
    PUTS("The one_of parser works...");

    CPC_ONE_OF(p_vowel, "aeiou")
    CPC_ONE_OF_LABEL(p_vowel_l, "aeiou", "expected vowel")

    CpcResult result = p_vowel(NULL, cpc_slice_from_cstr("apple"));

    ASSERT_OUT_SLICE_EQ(result, "a");
    ASSERT_REST_EQ(result, "pple");

    CpcResult result2 = p_vowel(NULL, cpc_slice_from_cstr("banana"));

    ASSERT(!result2.ok);
    ASSERT_ERR_EQ(result2, "none matched");

    CpcResult result3 = p_vowel_l(NULL, cpc_slice_from_cstr("banana"));

    ASSERT(!result3.ok);
    ASSERT_ERR_EQ(result3, "expected vowel");
  }

  {
    PUTS("The alternative parser works...");

    CPC_STRING(p_end, "END")
    CPC_ALT(p_combined, p_begin, p_end)

    CpcResult result = p_combined(NULL, cpc_slice_from_cstr("END leftovers"));
    ASSERT_OUT_SLICE_EQ(result, "END");
    ASSERT_REST_EQ(result, " leftovers");
  }

  {
    PUTS("The right parser works...");

    CPC_STRING(p_val, "value=")
    CPC_STRING(p_num, "12345")
    CPC_RIGHT(p_valnum, p_val, p_num)

    CpcResult result = p_valnum(NULL, cpc_slice_from_cstr("value=12345"));
    ASSERT_OUT_SLICE_EQ(result, "12345");
    ASSERT_REST_EMPTY(result);
  }

  {
    PUTS("The left parser works...");

    CPC_STRING(p_sel, "select 1")
    CPC_LEFT(p_stmt, p_sel, p_semicol)

    CpcResult result = p_stmt(NULL, cpc_slice_from_cstr("select 1;"));
    ASSERT_OUT_SLICE_EQ(result, "select 1");
    ASSERT_REST_EMPTY(result);
  }

  {
    PUTS("The between parser works...");

    CPC_STRING(p_lparen, "(")
    CPC_STRING(p_rparen, ")")
    CPC_STRING(p_abc, "abc")
    CPC_BETWEEN(p_paren_abc, p_lparen, p_abc, p_rparen)

    CpcResult result = p_paren_abc(NULL, cpc_slice_from_cstr("(abc)rest"));

    ASSERT_OUT_SLICE_EQ(result, "abc");
    ASSERT_REST_EQ(result, "rest");
  }

  {
    CpcValue arena_storage[2] = {0};
    CpcArena arena;
    typedef struct {
      char x;
      char y;
    } Pair;
    Pair pair = {0};
    cpc_arena_init(&arena, arena_storage, sizeof(arena_storage) / sizeof(arena_storage[0]), &pair);

    PUTS("The apply + map parser works...");

    CpcResult to_pair(CpcArena * A, const CpcValue *v, CpcSlice rest) {
      Pair *pair = A->user;
      pair->x    = cpc_val_list_at(A, v, 0)->as.slice.ptr[0];
      pair->y    = cpc_val_list_at(A, v, 1)->as.slice.ptr[0];

      return cpc_res_ok(cpc_val_ptr(pair), rest);
    }

    CPC_APPLY(p_ab, p_a, p_b);
    CPC_MAP(p_mapped_ab, p_ab, to_pair);

    CpcResult result = p_mapped_ab(&arena, cpc_slice_from_cstr("AB"));

    ASSERT(result.ok);
    ASSERT(pair.x == 'A');
    ASSERT(pair.y == 'B');
    ASSERT_REST_EMPTY(result);
  }

  {
    PUTS("The takewhile parser succeeds...");

    CPC_TAKE_WHILE(p_only_a, is_a)

    CpcResult result = p_only_a(NULL, cpc_slice_from_cstr("aaaaaaaaaabbbbb"));

    ASSERT_OUT_SLICE_EQ(result, "aaaaaaaaaa");
    ASSERT_REST_EQ(result, "bbbbb");

    PUTS("The takewhile parser never fails...");

    CpcResult result2 = p_only_a(NULL, cpc_slice_from_cstr("aabbbbbaaaa"));

    ASSERT_OUT_SLICE_EQ(result2, "aa");
    ASSERT_REST_EQ(result2, "bbbbbaaaa");

    PUTS("The takewhile parser never fails, it returns empty string if the "
         "pred returns false at "
         "first char...");

    CpcResult result1 = p_only_a(NULL, cpc_slice_from_cstr("bbbbbaaaa"));

    ASSERT_OUT_SLICE_EQ(result1, "");
    ASSERT_REST_EQ(result1, "bbbbbaaaa");
  }

  {
    PUTS("The takewhile1 parser succeeds...");

    CPC_TAKE_WHILE_1(p_at_least_1_a, is_a)

    CpcResult result = p_at_least_1_a(NULL, cpc_slice_from_cstr("abb"));

    ASSERT_OUT_SLICE_EQ(result, "a");
    ASSERT_REST_EQ(result, "bb");

    PUTS("The takewhile1 parser does fail...");

    CpcResult result2 = p_at_least_1_a(NULL, cpc_slice_from_cstr("bba"));

    ASSERT_OUT_NOTHING(result2);
    ASSERT_ERR_EQ(result2, "too few");
    ASSERT_REST_EQ(result2, "bba");

    PUTS("The takewhile1 parser fails on empty input...");

    CpcResult result3 = p_at_least_1_a(NULL, cpc_slice_from_cstr(""));

    ASSERT_OUT_NOTHING(result3);
    ASSERT_ERR_EQ(result3, "too few");
    ASSERT_REST_EMPTY(result3);

    PUTS("The takewhile1 parser can be labeled...");

    CPC_TAKE_WHILE_1_LABEL(p_at_least_1_a_l, is_a, "expected at least one a")

    CpcResult result4 = p_at_least_1_a_l(NULL, cpc_slice_from_cstr("bba"));

    ASSERT(!result4.ok);
    ASSERT_ERR_EQ(result4, "expected at least one a");
  }

  {
    PUTS("The many parser succeeds...");

    CpcValue arena_storage[10] = {0};
    CpcArena arena;
    cpc_arena_init(&arena, arena_storage, sizeof(arena_storage) / sizeof(arena_storage[0]), NULL);

    CPC_MANY(p_many_a, p_a)

    {
      CpcResult result = p_many_a(&arena, cpc_slice_from_cstr("AAAAb"));

      ASSERT(result.ok);
      ASSERT(cpc_is_list(&result.out));
      ASSERT(result.out.as.list.len == 4);
      for (size_t i = 0; i < result.out.as.list.len; i++) {
        CpcSlice slice_ = cpc_val_list_at(&arena, &result.out, i)->as.slice;
        ASSERT(STRNCMP(slice_.ptr, "A", slice_.len) == 0);
      }
      ASSERT_REST_EQ(result, "b");
    }

    PUTS("The many parser doesn't fail if it doesn't consume any input...");

    {
      CpcResult result = p_many_a(&arena, cpc_slice_from_cstr("aaaab"));

      ASSERT(result.ok);
      ASSERT(cpc_is_list(&result.out));
      ASSERT(result.out.as.list.len == 0);
      ASSERT_REST_EQ(result, "aaaab");
    }

    PUTS("The many parser will always finish...");

    {
      CPC_MANY(p_inf_many, p_is_space);

      cpc_arena_reset(&arena);

      CpcResult result = p_inf_many(&arena, cpc_slice_from_cstr("anything"));

      ASSERT_ERR_EQ(result, "no progress");
    }

    PUTS("The many parser will fail if the arena doesn't have enough "
         "capacity...");

    {
      CpcResult result = p_many_a(&arena, cpc_slice_from_cstr("AAAAAAAAAAAAA"));

      ASSERT(!result.ok);
      ASSERT_ERR_EQ(result, "arena surpassed");
    }
  }

  {
    PUTS("The many1 parser succeeds...");

    CpcValue arena_storage[10] = {0};
    CpcArena arena;
    cpc_arena_init(&arena, arena_storage, sizeof(arena_storage) / sizeof(arena_storage[0]), NULL);

    CPC_MANY_1(p_many_1_a, p_a)
    CPC_MANY_1_LABEL(p_many_1_a_l, p_a, "expected one or more As")

    {
      CpcResult result = p_many_1_a(&arena, cpc_slice_from_cstr("AAAb"));

      ASSERT(result.ok);
      ASSERT(cpc_is_list(&result.out));
      ASSERT(result.out.as.list.len == 3);
      for (size_t i = 0; i < result.out.as.list.len; i++) {
        CpcSlice slice_ = cpc_val_list_at(&arena, &result.out, i)->as.slice;
        ASSERT(STRNCMP(slice_.ptr, "A", slice_.len) == 0);
      }
      ASSERT_REST_EQ(result, "b");
    }

    PUTS("The many1 parser fails...");

    {
      CpcResult result = p_many_1_a(&arena, cpc_slice_from_cstr("bAAAAb"));

      ASSERT(!result.ok);
      ASSERT_REST_EQ(result, "bAAAAb");
      ASSERT_ERR_EQ(result, "too few");
    }

    PUTS("The many1 parser will fail if the arena doesn't have enough "
         "capacity...");

    {
      CpcResult result = p_many_1_a(&arena, cpc_slice_from_cstr("AAAAAAAAAAAAAA"));

      ASSERT(!result.ok);
      ASSERT_ERR_EQ(result, "arena surpassed");
    }

    {
      PUTS("The many1 parser can be labeled...");

      CpcResult result = p_many_1_a_l(&arena, cpc_slice_from_cstr("bAAAAb"));

      ASSERT(!result.ok);
      ASSERT_ERR_EQ(result, "expected one or more As");
    }
  }

  {
    CpcValue arena_storage[10] = {0};
    CpcArena arena;
    cpc_arena_init(&arena, arena_storage, sizeof(arena_storage) / sizeof(arena_storage[0]), NULL);

    CPC_MANY_TILL(p_many_a_till_semicol, p_a, p_semicol)

    {
      PUTS("The manytill parser succeeds...");

      CpcResult result = p_many_a_till_semicol(&arena, cpc_slice_from_cstr("AAAAAAAAA;"));

      ASSERT(result.ok);
      ASSERT(cpc_is_list(&result.out));
      ASSERT(result.out.as.list.len == 9);
      for (size_t i = 0; i < result.out.as.list.len; i++) {
        CpcSlice slice_ = cpc_val_list_at(&arena, &result.out, i)->as.slice;
        ASSERT(STRNCMP(slice_.ptr, "A", slice_.len) == 0);
      }
      ASSERT_REST_EMPTY(result);
    }

    {
      PUTS("The manytill parser fails...");

      CpcResult result = p_many_a_till_semicol(&arena, cpc_slice_from_cstr("bb"));

      ASSERT(!result.ok);
      ASSERT_REST_EQ(result, "bb");
    }

    {
      PUTS("The manytill parser will always finish...");

      CPC_MANY_TILL(p_inf_many_till, p_is_space, p_b)

      CpcResult result = p_inf_many_till(&arena, cpc_slice_from_cstr("abc"));

      ASSERT(!result.ok);
      ASSERT_ERR_EQ(result, "no progress");
    }

    {
      PUTS("The manytill parser will fail if the arena doesn't have enough "
           "capacity...");

      CpcResult result = p_many_a_till_semicol(&arena, cpc_slice_from_cstr("AAAAAAAAAAA;"));

      ASSERT(!result.ok);
      ASSERT_ERR_EQ(result, "arena surpassed");
    }
  }

  {
    PUTS("The taketill parser succeeds...");

    CPC_TAKE_TILL(p_till_b, is_a)

    CpcResult result = p_till_b(NULL, cpc_slice_from_cstr("bbbbaaaa"));

    ASSERT_OUT_SLICE_EQ(result, "bbbb");
    ASSERT_REST_EQ(result, "aaaa");

    PUTS("The taketill parser returns empty when the predicate matches at the first char...");

    CpcResult result2 = p_till_b(NULL, cpc_slice_from_cstr("aaaa"));

    ASSERT_OUT_SLICE_EQ(result2, "");
    ASSERT_REST_EQ(result2, "aaaa");

    PUTS("The taketill parser consumes the whole input when it hits eof...");

    CpcResult result3 = p_till_b(NULL, cpc_slice_from_cstr("bbbb"));

    ASSERT_OUT_SLICE_EQ(result3, "bbbb");
    ASSERT_REST_EMPTY(result3);
  }

  {
    CpcValue arena_storage[6] = {0};
    CpcArena arena;
    cpc_arena_init(&arena, arena_storage, sizeof(arena_storage) / sizeof(arena_storage[0]), NULL);

    CPC_SEP_BY(p_A_sep_by_space, p_a, p_is_space)

    {
      PUTS("The sepby parser succeeds...");

      CpcResult result = p_A_sep_by_space(&arena, cpc_slice_from_cstr("A A A A B"));

      ASSERT(result.ok);
      ASSERT(cpc_is_list(&result.out));
      ASSERT(result.out.as.list.len == 4);
      for (size_t i = 0; i < result.out.as.list.len; i++) {
        CpcSlice slice_ = cpc_val_list_at(&arena, &result.out, i)->as.slice;
        ASSERT(STRNCMP(slice_.ptr, "A", slice_.len) == 0);
      }
      ASSERT_REST_EQ(result, " B");
    }

    {
      PUTS("The sepby parser doesn't fail if it doesn't consume any input...");

      CpcResult result = p_A_sep_by_space(&arena, cpc_slice_from_cstr("B B B"));

      ASSERT(result.ok);
      ASSERT(cpc_is_list(&result.out));
      ASSERT(result.out.as.list.len == 0);
      ASSERT_REST_EQ(result, "B B B");
    }

    {
      PUTS("The sepby parser will fail if the arena doesn't have enough "
           "capacity...");

      CpcResult result = p_A_sep_by_space(&arena, cpc_slice_from_cstr("A A A A A A A A"));

      ASSERT(!result.ok);
      ASSERT_ERR_EQ(result, "arena surpassed");
    }

    {
      PUTS("The sepby parser will always finish...");

      CPC_SEP_BY(p_inf_sep_by, p_is_space, p_is_space)

      cpc_arena_reset(&arena);

      CpcResult result = p_inf_sep_by(&arena, cpc_slice_from_cstr("abc"));

      ASSERT(!result.ok);
      ASSERT_ERR_EQ(result, "no progress");
    }
  }

  {
    CpcValue arena_storage[6] = {0};
    CpcArena arena;
    cpc_arena_init(&arena, arena_storage, sizeof(arena_storage) / sizeof(arena_storage[0]), NULL);

    CPC_SEP_BY_1(p_A_sep_by_1_space, p_a, p_is_space)
    CPC_SEP_BY_1_LABEL(p_A_sep_by_1_space_l, p_a, p_is_space, "expected one or more items")

    {
      PUTS("The sepby1 parser succeeds...");

      CpcResult result = p_A_sep_by_1_space(&arena, cpc_slice_from_cstr("A A A A B"));

      ASSERT(result.ok);
      ASSERT(cpc_is_list(&result.out));
      ASSERT(result.out.as.list.len == 4);
      for (size_t i = 0; i < result.out.as.list.len; i++) {
        CpcSlice slice_ = cpc_val_list_at(&arena, &result.out, i)->as.slice;
        ASSERT(STRNCMP(slice_.ptr, "A", slice_.len) == 0);
      }
      ASSERT_REST_EQ(result, " B");
    }

    {
      PUTS("The sepby1 parser fails...");

      CpcResult result = p_A_sep_by_1_space(&arena, cpc_slice_from_cstr("B A A"));

      ASSERT(!result.ok);
      ASSERT_REST_EQ(result, "B A A");
      ASSERT_ERR_EQ(result, "too few");
    }

    {
      PUTS("The sepby1 parser will always finish...");

      bool is_comma(char c) {
        return c == ',';
      }
      CPC_TAKE_WHILE(p_take_while_comma, is_comma)

      bool is_not_comma(char c) {
        return c != ',';
      }
      CPC_TAKE_WHILE(p_take_while_not_comma, is_not_comma)

      CPC_SEP_BY_1(p_inf_sep_by_1, p_take_while_comma, p_take_while_not_comma)

      CpcResult result = p_inf_sep_by_1(&arena, cpc_slice_from_cstr(","));

      ASSERT_ERR_EQ(result, "no progress");
    }

    {
      PUTS("The sepby1 parser can be labeled...");

      CpcResult result = p_A_sep_by_1_space_l(&arena, cpc_slice_from_cstr("B A A"));

      ASSERT(!result.ok);
      ASSERT_ERR_EQ(result, "expected one or more items");
    }
  }

  {
    PUTS("The end of line parser succeeds on CRLF...");

    CpcResult result = CPC_END_OF_LINE_(NULL, cpc_slice_from_cstr("\r\nrest"));

    ASSERT_OUT_SLICE_EQ(result, "\r\n");
    ASSERT_REST_EQ(result, "rest");
  }

  {
    PUTS("The end of line parser succeeds on LF...");

    CpcResult result = CPC_END_OF_LINE_(NULL, cpc_slice_from_cstr("\nrest"));

    ASSERT_OUT_SLICE_EQ(result, "\n");
    ASSERT_REST_EQ(result, "rest");
  }

  {
    PUTS("The end of line parser fails on non-newline input...");

    CpcResult result = CPC_END_OF_LINE_(NULL, cpc_slice_from_cstr("A"));

    ASSERT(!result.ok);
    ASSERT_REST_EQ(result, "A");
    ASSERT_ERR_EQ(result, "expected newline");
  }

  {
    PUTS("The end of line parser can be labeled...");

    CPC_END_OF_LINE_LABEL(p_eol_l, "bad line ending")

    CpcResult result = p_eol_l(NULL, cpc_slice_from_cstr("A"));

    ASSERT(!result.ok);
    ASSERT_ERR_EQ(result, "bad line ending");
  }

  {
    PUTS("The eof parser succeeds...");

    CpcResult result = CPC_EOF_(NULL, cpc_slice_from_cstr(""));

    ASSERT(result.ok);
    ASSERT_REST_EMPTY(result);
  }

  {
    PUTS("The eof parser fails...");

    CpcResult result = CPC_EOF_(NULL, cpc_slice_from_cstr("A"));

    ASSERT(!result.ok);
    ASSERT_REST_EQ(result, "A");
    ASSERT_ERR_EQ(result, "expected eof");
  }

  {
    PUTS("The named eof parser works...");

    CPC_EOF(p_eof)

    CpcResult result = p_eof(NULL, cpc_slice_from_cstr(""));

    ASSERT(result.ok);
    ASSERT_REST_EMPTY(result);
  }

  {
    PUTS("The eof parser can be labeled...");

    CPC_EOF_LABEL(p_eof_l, "missing eof")

    CpcResult result = p_eof_l(NULL, cpc_slice_from_cstr("A"));

    ASSERT(!result.ok);
    ASSERT_ERR_EQ(result, "missing eof");
  }

  {

    {
      PUTS("The pure parser works...");

      // This is the same as Haskell's `string "\"\"" >> return "\""`
      CPC_STRING(p_ddquote, "\"\"")
      CPC_PURE(p_dquote_, cpc_val_slice(cpc_slice_from_cstr("\"")))
      CPC_RIGHT(p_dquote, p_ddquote, p_dquote_)

      CpcResult result = p_dquote(NULL, cpc_slice_from_cstr("\"\"abc"));

      ASSERT_OUT_SLICE_EQ(result, "\"");
      ASSERT_REST_EQ(result, "abc");
    }
  }

  {
    PUTS("The match parser works...");

    CPC_STRING(p_token, "token")
    CPC_LEFT(p_token_semicol, p_token, p_semicol)
    CPC_MATCH(p_match_token_semicol, p_token_semicol)

    CpcResult plain = p_token_semicol(NULL, cpc_slice_from_cstr("token;rest"));

    ASSERT_OUT_SLICE_EQ(plain, "token");
    ASSERT_REST_EQ(plain, "rest");

    CpcResult result = p_match_token_semicol(NULL, cpc_slice_from_cstr("token;rest"));

    ASSERT_OUT_SLICE_EQ(result, "token;");
    ASSERT_REST_EQ(result, "rest");
  }

  {
    PUTS("The match parser restores arena state...");

    CpcValue arena_storage[2] = {0};
    CpcArena arena;
    cpc_arena_init(&arena, arena_storage, sizeof(arena_storage) / sizeof(arena_storage[0]), NULL);

    CPC_APPLY(p_ab_for_match, p_a, p_b)
    CPC_MATCH(p_match_ab, p_ab_for_match)

    CpcResult result1 = p_match_ab(&arena, cpc_slice_from_cstr("ABrest"));

    ASSERT_OUT_SLICE_EQ(result1, "AB");
    ASSERT_REST_EQ(result1, "rest");
    ASSERT(arena.offset == 0);

    CpcResult result2 = p_match_ab(&arena, cpc_slice_from_cstr("ABrest"));

    ASSERT_OUT_SLICE_EQ(result2, "AB");
    ASSERT(arena.offset == 0);
  }

  {
    PUTS("The any parser succeeds...");

    CpcResult result = CPC_ANY_(NULL, cpc_slice_from_cstr("Arest"));

    ASSERT_OUT_SLICE_EQ(result, "A");
    ASSERT_REST_EQ(result, "rest");
  }

  {
    PUTS("The any parser fails on eof...");

    CpcResult result = CPC_ANY_(NULL, cpc_slice_from_cstr(""));

    ASSERT(!result.ok);
    ASSERT_REST_EMPTY(result);
    ASSERT_ERR_EQ(result, "eof");
  }

  {
    PUTS("The any parser can be labeled...");

    CPC_ANY_LABEL(p_any_l, "expected any char")

    CpcResult result = p_any_l(NULL, cpc_slice_from_cstr(""));

    ASSERT(!result.ok);
    ASSERT_REST_EMPTY(result);
    ASSERT_ERR_EQ(result, "expected any char");
  }

  return 0;
}

#endif
