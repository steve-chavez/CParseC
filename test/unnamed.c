#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cparsec.h"

#include "assertions.h"
#include "hosted.h"

CPC_STRING(p_end, "END")
CPC_STRING(p_begin, "BEGIN")
CPC_ALT(p_combined_inline, p_begin, p_end)

int main(void) {
  {
    PUTS("The unnamed string parser can work standalone...");

    CpcResult result = CPC_PARSE(p_end, cpc_slice_from_cstr("END leftovers"), NULL);
    ASSERT_OUT_SLICE_EQ(result, "END");
    ASSERT_REST_EQ(result, " leftovers");
  }

  {
    PUTS("The unnamed string parser works inside combinators...");

    CpcResult result = CPC_PARSE(p_combined_inline, cpc_slice_from_cstr("END leftovers"), NULL);
    ASSERT_OUT_SLICE_EQ(result, "END");
    ASSERT_REST_EQ(result, " leftovers");
  }

  return EXIT_SUCCESS;
}
