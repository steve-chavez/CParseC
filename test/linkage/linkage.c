// This is a linkage test, just to check if we're correctly scoping cparsec.h function definitions.
// If not, this will cause a failure at link time. This can happen when a cparsec.h function is not defined as `static inline` for example.
#include "cparsec.h"
#include "shared.h"

int main(void) {
  return 0;
}
