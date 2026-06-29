#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CPC_USE_UNNAMED
#include "cparsec.h"

#include "assertions.h"
#include "hosted.h"

int main(void) {
  {
    PUTS("The unnamed string parser can work standalone...");

    CpcResult result = CPC_STRING_("END")(NULL, cpc_slice_from_cstr("END leftovers"));
    ASSERT_OUT_SLICE_EQ(result, "END");
    ASSERT_REST_EQ(result, " leftovers");
  }

  {
    PUTS("The unnamed string parser works inside combinators...");

    CPC_ALT(p_combined_inline, CPC_STRING_("BEGIN"), CPC_STRING_("END"))

    CpcResult result = p_combined_inline(NULL, cpc_slice_from_cstr("END leftovers"));
    ASSERT_OUT_SLICE_EQ(result, "END");
    ASSERT_REST_EQ(result, " leftovers");
  }

  return EXIT_SUCCESS;
}
