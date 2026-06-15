#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

#define CPC_USE_MEMCHR
#include "cparsec.h"

int main(void) {
  {
    PUTS("The take_till_one_of parser works...");

    CPC_TAKE_TILL_ONE_OF(p_take_till_semicol_or_comma, ";,")

    CpcResult result = p_take_till_semicol_or_comma(NULL, cpc_slice_from_cstr("token,rest"));

    ASSERT(result.ok);
    ASSERT(STRNCMP(result.out.as.slice.ptr, "token", result.out.as.slice.len) == 0);
    ASSERT(result.rest.len != 0);
    ASSERT(STRNCMP(result.rest.ptr, ",rest", result.rest.len) == 0);

    PUTS("The take_till_one_of parser returns empty when the first byte matches...");

    CPC_TAKE_TILL_ONE_OF(p_take_till_comma, ",")

    CpcResult result2 = p_take_till_comma(NULL, cpc_slice_from_cstr(",rest"));

    ASSERT(result2.ok);
    ASSERT(cpc_is_slice(&result2.out));
    ASSERT(result2.out.as.slice.len == 0);
    ASSERT(result2.rest.len != 0);
    ASSERT(STRNCMP(result2.rest.ptr, ",rest", result2.rest.len) == 0);

    PUTS("The take_till_one_of parser consumes the whole input when it hits eof...");

    CPC_TAKE_TILL_ONE_OF(p_take_till_semicol, ";")

    CpcResult result3 = p_take_till_semicol(NULL, cpc_slice_from_cstr("token"));

    ASSERT(result3.ok);
    ASSERT(cpc_is_slice(&result3.out));
    ASSERT(STRNCMP(result3.out.as.slice.ptr, "token", result3.out.as.slice.len) == 0);
    ASSERT(result3.rest.len == 0);
  }

  {
    PUTS("The span_quoted parser works with doubled quotes...");

    CPC_TAKE_QUOTED_BY(p_span_dquoted, '"')
    CPC_TAKE_QUOTED_BY(p_span_squoted, '\'')

    CpcResult result = p_span_dquoted(NULL, cpc_slice_from_cstr("\"a\"\"b\",rest"));

    ASSERT(result.ok);
    ASSERT(cpc_is_slice(&result.out));
    ASSERT(STRNCMP(result.out.as.slice.ptr, "\"a\"\"b\"", result.out.as.slice.len) == 0);
    ASSERT(result.rest.len != 0);
    ASSERT(STRNCMP(result.rest.ptr, ",rest", result.rest.len) == 0);

    PUTS("The span_quoted parser works with plain quoted text...");

    CpcResult result_plain = p_span_dquoted(NULL, cpc_slice_from_cstr("\"abcdefgh\",rest"));

    ASSERT(result_plain.ok);
    ASSERT(cpc_is_slice(&result_plain.out));
    ASSERT(STRNCMP(result_plain.out.as.slice.ptr, "\"abcdefgh\"", result_plain.out.as.slice.len) ==
           0);
    ASSERT(result_plain.rest.len != 0);
    ASSERT(STRNCMP(result_plain.rest.ptr, ",rest", result_plain.rest.len) == 0);

    PUTS("The span_quoted parser works with an empty quoted span...");

    CpcResult result_empty_span = p_span_dquoted(NULL, cpc_slice_from_cstr("\"\",rest"));

    ASSERT(result_empty_span.ok);
    ASSERT(cpc_is_slice(&result_empty_span.out));
    ASSERT(STRNCMP(result_empty_span.out.as.slice.ptr, "\"\"",
                   result_empty_span.out.as.slice.len) == 0);
    ASSERT(result_empty_span.rest.len != 0);
    ASSERT(STRNCMP(result_empty_span.rest.ptr, ",rest", result_empty_span.rest.len) == 0);

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
    ASSERT(result4.rest.len != 0);
    ASSERT(STRNCMP(result4.rest.ptr, "\"abcde\"\"", result4.rest.len) == 0);

    PUTS("The span_quoted parser works with single quotes...");

    CpcResult result_single = p_span_squoted(NULL, cpc_slice_from_cstr("'abcdefgh',rest"));

    ASSERT(result_single.ok);
    ASSERT(cpc_is_slice(&result_single.out));
    ASSERT(STRNCMP(result_single.out.as.slice.ptr, "'abcdefgh'", result_single.out.as.slice.len) ==
           0);
    ASSERT(result_single.rest.len != 0);
    ASSERT(STRNCMP(result_single.rest.ptr, ",rest", result_single.rest.len) == 0);

    PUTS("The span_quoted parser works with doubled single quotes...");

    CpcResult result_single_escaped =
        p_span_squoted(NULL, cpc_slice_from_cstr("'abcd''efg''hi',rest"));

    ASSERT(result_single_escaped.ok);
    ASSERT(cpc_is_slice(&result_single_escaped.out));
    ASSERT(STRNCMP(result_single_escaped.out.as.slice.ptr, "'abcd''efg''hi'",
                   result_single_escaped.out.as.slice.len) == 0);
    ASSERT(result_single_escaped.rest.len != 0);
    ASSERT(STRNCMP(result_single_escaped.rest.ptr, ",rest", result_single_escaped.rest.len) == 0);
  }

  return EXIT_SUCCESS;
}
