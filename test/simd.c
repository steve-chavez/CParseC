#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hosted.h"

#define CPC_USE_MEMCHR
#include "assertions.h"
#include "cparsec.h"

int main(void) {
  {
    {
      PUTS("The take_till_one_of parser works...");

      CPC_TAKE_TILL_ONE_OF(p_take_till_semicol_or_comma, ";,")

      CpcResult result = p_take_till_semicol_or_comma(NULL, cpc_slice_from_cstr("token,rest"));

      ASSERT_OUT_SLICE_EQ(result, "token");
      ASSERT_REST_EQ(result, ",rest");
    }

    {
      PUTS("The take_till_one_of parser returns empty when the first byte matches...");

      CPC_TAKE_TILL_ONE_OF(p_take_till_comma, ",")

      CpcResult result = p_take_till_comma(NULL, cpc_slice_from_cstr(",rest"));

      ASSERT_OUT_SLICE_EQ(result, "");
      ASSERT_REST_EQ(result, ",rest");
    }

    {
      PUTS("The take_till_one_of parser consumes the whole input when it hits eof...");

      CPC_TAKE_TILL_ONE_OF(p_take_till_semicol, ";")

      CpcResult result = p_take_till_semicol(NULL, cpc_slice_from_cstr("token"));

      ASSERT_OUT_SLICE_EQ(result, "token");
      ASSERT_REST_EMPTY(result);
    }
  }

  {
    CPC_TAKE_QUOTED_BY(p_span_dquoted, '"')

    {
      PUTS("The take_quoted parser works with doubled quotes...");

      CpcResult result = p_span_dquoted(NULL, cpc_slice_from_cstr("\"a\"\"b\",rest"));

      ASSERT_OUT_SLICE_EQ(result, "\"a\"\"b\"");
      ASSERT_REST_EQ(result, ",rest");
    }

    {
      PUTS("The take_quoted parser works with plain quoted text...");

      CpcResult result = p_span_dquoted(NULL, cpc_slice_from_cstr("\"abcdefgh\",rest"));

      ASSERT_OUT_SLICE_EQ(result, "\"abcdefgh\"");
      ASSERT_REST_EQ(result, ",rest");
    }

    {
      PUTS("The take_quoted parser works with an empty quoted span...");

      CpcResult result = p_span_dquoted(NULL, cpc_slice_from_cstr("\"\",rest"));

      ASSERT_OUT_SLICE_EQ(result, "\"\"");
      ASSERT_REST_EQ(result, ",rest");
    }

    {
      PUTS("The take_quoted parser fails if the opening quote is missing...");

      CpcResult result = p_span_dquoted(NULL, cpc_slice_from_cstr("plain"));

      ASSERT_OUT_NOTHING(result);
      ASSERT_ERR_EQ(result, "missing quote");
    }

    {
      PUTS("The take_quoted parser fails on empty input...");

      CpcResult result = p_span_dquoted(NULL, cpc_slice_from_cstr(""));

      ASSERT_OUT_NOTHING(result);
      ASSERT_ERR_EQ(result, "missing quote");
    }

    {
      PUTS("The take_quoted parser fails if the closing quote is missing...");

      CpcResult result = p_span_dquoted(NULL, cpc_slice_from_cstr("\"unterminated"));

      ASSERT_OUT_NOTHING(result);
      ASSERT_ERR_EQ(result, "missing quote");
    }

    {
      PUTS("The take_quoted parser fails on a single quote char...");

      CpcResult result = p_span_dquoted(NULL, cpc_slice_from_cstr("\""));

      ASSERT_OUT_NOTHING(result);
      ASSERT_ERR_EQ(result, "missing quote");
    }

    {
      PUTS("The take_quoted parser fails if input ends after a doubled quote...");

      CpcResult result = p_span_dquoted(NULL, cpc_slice_from_cstr("\"abcde\"\""));

      ASSERT_OUT_NOTHING(result);
      ASSERT_ERR_EQ(result, "missing quote");
      ASSERT_REST_EQ(result, "\"abcde\"\"");
    }

    CPC_TAKE_QUOTED_BY(p_span_squoted, '\'')

    {
      PUTS("The take_quoted parser works with single quotes...");

      CpcResult result = p_span_squoted(NULL, cpc_slice_from_cstr("'abcdefgh',rest"));

      ASSERT_OUT_SLICE_EQ(result, "'abcdefgh'");
      ASSERT_REST_EQ(result, ",rest");
    }

    {
      PUTS("The take_quoted parser works with doubled single quotes...");

      CpcResult result = p_span_squoted(NULL, cpc_slice_from_cstr("'abcd''efg''hi',rest"));

      ASSERT_OUT_SLICE_EQ(result, "'abcd''efg''hi'");
      ASSERT_REST_EQ(result, ",rest");
    }
  }

  return EXIT_SUCCESS;
}
