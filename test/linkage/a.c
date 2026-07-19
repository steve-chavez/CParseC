#include "shared.h"

CpcResult parse_ok(CpcArena *A, CpcSlice input) {
  return CPC_PARSE(p_ok, input, A);
}
