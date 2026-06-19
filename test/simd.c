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
    PUTS("The take_till_one_of parser works...");

    CPC_TAKE_TILL_ONE_OF(p_take_till_semicol_or_comma, ";,")

    CpcResult result = p_take_till_semicol_or_comma(NULL, cpc_slice_from_cstr("token,rest"));

    ASSERT_OUT_SLICE_EQ(result, "token");
    ASSERT_REST_EQ(result, ",rest");

    PUTS("The take_till_one_of parser returns empty when the first byte matches...");

    CPC_TAKE_TILL_ONE_OF(p_take_till_comma, ",")

    CpcResult result2 = p_take_till_comma(NULL, cpc_slice_from_cstr(",rest"));

    ASSERT_OUT_SLICE_EQ(result2, "");
    ASSERT_REST_EQ(result2, ",rest");

    PUTS("The take_till_one_of parser consumes the whole input when it hits eof...");

    CPC_TAKE_TILL_ONE_OF(p_take_till_semicol, ";")

    CpcResult result3 = p_take_till_semicol(NULL, cpc_slice_from_cstr("token"));

    ASSERT_OUT_SLICE_EQ(result3, "token");
    ASSERT_REST_EMPTY(result3);
  }

  {
    PUTS("The span_quoted parser works with doubled quotes...");

    CPC_TAKE_QUOTED_BY(p_span_dquoted, '"')
    CPC_TAKE_QUOTED_BY(p_span_squoted, '\'')

    CpcResult result = p_span_dquoted(NULL, cpc_slice_from_cstr("\"a\"\"b\",rest"));

    ASSERT_OUT_SLICE_EQ(result, "\"a\"\"b\"");
    ASSERT_REST_EQ(result, ",rest");

    PUTS("The span_quoted parser works with plain quoted text...");

    CpcResult result_plain = p_span_dquoted(NULL, cpc_slice_from_cstr("\"abcdefgh\",rest"));

    ASSERT_OUT_SLICE_EQ(result_plain, "\"abcdefgh\"");
    ASSERT_REST_EQ(result_plain, ",rest");

    PUTS("The span_quoted parser works with an empty quoted span...");

    CpcResult result_empty_span = p_span_dquoted(NULL, cpc_slice_from_cstr("\"\",rest"));

    ASSERT_OUT_SLICE_EQ(result_empty_span, "\"\"");
    ASSERT_REST_EQ(result_empty_span, ",rest");

    PUTS("The span_quoted parser fails if the opening quote is missing...");

    CpcResult result2 = p_span_dquoted(NULL, cpc_slice_from_cstr("plain"));

    ASSERT(!result2.ok);
    ASSERT(STRCMP(result2.err, "p_span_dquoted: missing quote") == 0);

    PUTS("The span_quoted parser fails on empty input...");

    CpcResult result_empty = p_span_dquoted(NULL, cpc_slice_from_cstr(""));

    ASSERT(!result_empty.ok);
    ASSERT(STRCMP(result_empty.err, "p_span_dquoted: missing quote") == 0);

    PUTS("The span_quoted parser fails if the closing quote is missing...");

    CpcResult result3 = p_span_dquoted(NULL, cpc_slice_from_cstr("\"unterminated"));

    ASSERT(!result3.ok);
    ASSERT(STRCMP(result3.err, "p_span_dquoted: missing quote") == 0);

    PUTS("The span_quoted parser fails on a single quote char...");

    CpcResult result_single_quote = p_span_dquoted(NULL, cpc_slice_from_cstr("\""));

    ASSERT(!result_single_quote.ok);
    ASSERT(STRCMP(result_single_quote.err, "p_span_dquoted: missing quote") == 0);

    PUTS("The span_quoted parser fails if input ends after a doubled quote...");

    CpcResult result4 = p_span_dquoted(NULL, cpc_slice_from_cstr("\"abcde\"\""));

    ASSERT(!result4.ok);
    ASSERT(STRCMP(result4.err, "p_span_dquoted: missing quote") == 0);
    ASSERT_REST_EQ(result4, "\"abcde\"\"");

    PUTS("The span_quoted parser works with single quotes...");

    CpcResult result_single = p_span_squoted(NULL, cpc_slice_from_cstr("'abcdefgh',rest"));

    ASSERT_OUT_SLICE_EQ(result_single, "'abcdefgh'");
    ASSERT_REST_EQ(result_single, ",rest");

    PUTS("The span_quoted parser works with doubled single quotes...");

    CpcResult result_single_escaped =
        p_span_squoted(NULL, cpc_slice_from_cstr("'abcd''efg''hi',rest"));

    ASSERT_OUT_SLICE_EQ(result_single_escaped, "'abcd''efg''hi'");
    ASSERT_REST_EQ(result_single_escaped, ",rest");
  }

  return EXIT_SUCCESS;
}
