#include <stdio.h>
#include <stdlib.h>

#define ZPARSEC_IMPLEMENTATION
#include "zparsec.h"

int main() {
  ZParsecSlice slice = slice_from_cstr("Hello world");
  printf("%.*s\n", (int)slice.len, slice.ptr);

  ZParsecSlice slice2 = slice_sub(slice, 0, 5);
  printf("%.*s\n", (int)slice2.len, slice2.ptr);

  return EXIT_SUCCESS;
}
