#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT(expr) assert(expr)
#define PUTS(msg) puts(msg)
#define STRCMP(lhs, rhs) strcmp((lhs), (rhs))
#define STRNCMP(lhs, rhs, n) strncmp((lhs), (rhs), (n))

#include "cparsec.h"

#include "basic.h"

int main(void) {
  return cpc_basic_test_run();
}
