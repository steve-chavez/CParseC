// don't forget to update the README.md when this is changed
#include <stdio.h>
#include <stdlib.h>

#define CPC_USE_MEMCHR
#define CPC_USE_UNNAMED
#include "cparsec.h"

CPC_TAKE_QUOTED(quotedField, '"')
CPC_TAKE_TILL_ONE_OF(unquotedField, ",\r\n")
CPC_ALT(field, quotedField, unquotedField)
CPC_SEP_BY_1(record, field, CPC_STRING_(","))
CPC_ALT(lineEnd, CPC_END_OF_LINE_, CPC_EOF_)
CPC_LEFT(parse_csv_row, record, lineEnd)

int main(void) {
  CpcArena arena;
  CpcValue arena_storage[8192];
  cpc_arena_init(&arena, arena_storage, sizeof(arena_storage) / sizeof(arena_storage[0]), NULL);

  const char csv[] = "alpha,\"beta\",\"ga,mm,a\",delta\n";
  CpcSlice   input = cpc_slice_from_cstr(csv);
  CpcResult  result = parse_csv_row(&arena, input);

  for (size_t i = 0; i < result.out.as.list.len; ++i) {
    const CpcValue *cell = cpc_val_list_at(&arena, &result.out, i);
    CpcSlice        slice = cell->as.slice;
    printf("%.*s\n", (int)slice.len, slice.ptr);
  }

  return EXIT_SUCCESS;
}
