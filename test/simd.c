#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hosted.h"

#define CPC_USE_STRING_H
#include "assertions.h"
#include "cparsec.h"

int main(void) {
  {
    {
      PUTS("The take_till_one_of parser works...");

      CPC_TAKE_TILL_ONE_OF(p_take_till_semicol_or_comma, ";,")

      CpcResult result =
          CPC_PARSE(p_take_till_semicol_or_comma, cpc_slice_from_cstr("token,rest"), NULL);

      ASSERT_OUT_SLICE_EQ(result, "token");
      ASSERT_REST_EQ(result, ",rest");
    }

    {
      PUTS("The take_till_one_of parser returns empty when the first byte matches...");

      CPC_TAKE_TILL_ONE_OF(p_take_till_comma, ",")

      CpcResult result = CPC_PARSE(p_take_till_comma, cpc_slice_from_cstr(",rest"), NULL);

      ASSERT_OUT_SLICE_EQ(result, "");
      ASSERT_REST_EQ(result, ",rest");
    }

    {
      PUTS("The take_till_one_of parser consumes the whole input when it hits eof...");

      CPC_TAKE_TILL_ONE_OF(p_take_till_semicol, ";")

      CpcResult result = CPC_PARSE(p_take_till_semicol, cpc_slice_from_cstr("token"), NULL);

      ASSERT_OUT_SLICE_EQ(result, "token");
      ASSERT_REST_EMPTY(result);
    }
  }

  {
    CPC_TAKE_QUOTED(p_span_dquoted, '"', '"')

    {
      PUTS("The take_quoted parser works with doubled quotes...");

      CpcResult result = CPC_PARSE(p_span_dquoted, cpc_slice_from_cstr("\"a\"\"b\",rest"), NULL);

      ASSERT_OUT_SLICE_EQ(result, "\"a\"\"b\"");
      ASSERT_REST_EQ(result, ",rest");
    }

    {
      PUTS("The take_quoted parser works with plain quoted text...");

      CpcResult result = CPC_PARSE(p_span_dquoted, cpc_slice_from_cstr("\"abcdefgh\",rest"), NULL);

      ASSERT_OUT_SLICE_EQ(result, "\"abcdefgh\"");
      ASSERT_REST_EQ(result, ",rest");
    }

    {
      PUTS("The take_quoted parser works with an empty quoted span...");

      CpcResult result = CPC_PARSE(p_span_dquoted, cpc_slice_from_cstr("\"\",rest"), NULL);

      ASSERT_OUT_SLICE_EQ(result, "\"\"");
      ASSERT_REST_EQ(result, ",rest");
    }

    {
      PUTS("The take_quoted parser fails if the opening quote is missing...");

      CpcResult result = CPC_PARSE(p_span_dquoted, cpc_slice_from_cstr("plain"), NULL);

      ASSERT_OUT_NOTHING(result);
      ASSERT_ERR_EQ(result, "missing quote");
    }

    {
      PUTS("The take_quoted parser fails on empty input...");

      CpcResult result = CPC_PARSE(p_span_dquoted, cpc_slice_from_cstr(""), NULL);

      ASSERT_OUT_NOTHING(result);
      ASSERT_ERR_EQ(result, "missing quote");
    }

    {
      PUTS("The take_quoted parser fails if the closing quote is missing...");

      CpcResult result = CPC_PARSE(p_span_dquoted, cpc_slice_from_cstr("\"unterminated"), NULL);

      ASSERT_OUT_NOTHING(result);
      ASSERT_ERR_EQ(result, "missing quote");
    }

    {
      PUTS("The take_quoted parser fails on a single quote char...");

      CpcResult result = CPC_PARSE(p_span_dquoted, cpc_slice_from_cstr("\""), NULL);

      ASSERT_OUT_NOTHING(result);
      ASSERT_ERR_EQ(result, "missing quote");
    }

    {
      PUTS("The take_quoted parser fails if input ends after a doubled quote...");

      CpcResult result = CPC_PARSE(p_span_dquoted, cpc_slice_from_cstr("\"abcde\"\""), NULL);

      ASSERT_OUT_NOTHING(result);
      ASSERT_ERR_EQ(result, "missing quote");
      ASSERT_REST_EQ(result, "\"abcde\"\"");
    }

    CPC_TAKE_QUOTED(p_span_squoted, '\'', '\'')

    {
      PUTS("The take_quoted parser works with single quotes...");

      CpcResult result = CPC_PARSE(p_span_squoted, cpc_slice_from_cstr("'abcdefgh',rest"), NULL);

      ASSERT_OUT_SLICE_EQ(result, "'abcdefgh'");
      ASSERT_REST_EQ(result, ",rest");
    }

    {
      PUTS("The take_quoted parser works with doubled single quotes...");

      CpcResult result =
          CPC_PARSE(p_span_squoted, cpc_slice_from_cstr("'abcd''efg''hi',rest"), NULL);

      ASSERT_OUT_SLICE_EQ(result, "'abcd''efg''hi'");
      ASSERT_REST_EQ(result, ",rest");
    }

    CPC_TAKE_QUOTED(p_span_bsquoted, '\'', '\\')

    {
      PUTS("The take_quoted parser works with backslash-escaped quotes...");

      CpcResult result = CPC_PARSE(p_span_bsquoted, cpc_slice_from_cstr("'abc\\'def',rest"), NULL);

      ASSERT_OUT_SLICE_EQ(result, "'abc\\'def'");
      ASSERT_REST_EQ(result, ",rest");
    }

    {
      PUTS("The take_quoted parser treats a quote after odd backslashes as escaped...");

      CpcResult result =
          CPC_PARSE(p_span_bsquoted, cpc_slice_from_cstr("'abc\\\\\\'def',rest"), NULL);

      ASSERT_OUT_SLICE_EQ(result, "'abc\\\\\\'def'");
      ASSERT_REST_EQ(result, ",rest");
    }

    {
      PUTS("The take_quoted parser treats a quote after even backslashes as unescaped...");

      CpcResult result =
          CPC_PARSE(p_span_bsquoted, cpc_slice_from_cstr("'abc\\\\'def',rest"), NULL);

      ASSERT_OUT_SLICE_EQ(result, "'abc\\\\'");
      ASSERT_REST_EQ(result, "def',rest");
    }

    {
      PUTS("The take_quoted parser can read runtime data...");

      typedef struct {
        char quote;
        char escape;
      } QuotedCtx;

      CpcValue  arena_storage[8] = {0};
      CpcArena  arena;
      QuotedCtx ctx = {.quote = '\'', .escape = '\\'};
      cpc_arena_init(&arena, arena_storage, sizeof(arena_storage) / sizeof(arena_storage[0]), &ctx);

      CPC_TAKE_QUOTED(p_span_ctx_bsquoted, CPC_USER(QuotedCtx, quote), CPC_USER(QuotedCtx, escape))
      CpcResult result =
          CPC_PARSE(p_span_ctx_bsquoted, cpc_slice_from_cstr("'abc\\'def',rest"), &arena);

      ASSERT_OUT_SLICE_EQ(result, "'abc\\'def'");
      ASSERT_REST_EQ(result, ",rest");
    }

    {
      PUTS("The take_quoted parser can be labeled...");

      CPC_TAKE_QUOTED(p_span_dquoted_l_, '"', '"')
      CPC_LABEL(p_span_dquoted_l, p_span_dquoted_l_, "expected quoted field")

      CpcResult result = CPC_PARSE(p_span_dquoted_l, cpc_slice_from_cstr("plain"), NULL);

      ASSERT_OUT_NOTHING(result);
      ASSERT_ERR_EQ(result, "expected quoted field");
    }
  }

  return EXIT_SUCCESS;
}
