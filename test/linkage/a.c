#include "shared.h"

CpcResult parse_ok(CpcArena *A, CpcSlice input) {
  return p_ok(input, A, NULL);
}
