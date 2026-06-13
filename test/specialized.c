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

  return EXIT_SUCCESS;
}
