// This program parses a CSV from stdin, and prints a report (this matches the report on bench/haskell/ParseCSV.hs)
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "utils.h"
#include "csv.h"
#include "cparsec.h"

static void write_slice(FILE *out, CpcSlice s) {
  for (size_t i = 0; i < s.len; ++i)
    fputc(s.ptr[i], out);
}

static CpcSlice header_at(const CpcArena *A, const CpcValue *header_row, size_t i) {
  const CpcValue *field = cpc_val_list_at(A, header_row, i);
  if (cpc_is_slice(field)) return field->as.slice;
  return (CpcSlice){.ptr = "", .len = 0}; // TODO this shouldn't be necessary
}

static void print_headers(const CpcArena *A, const CpcValue *header_row, size_t count) {
  puts("Headers:");
  for (size_t i = 0; i < count; ++i) {
    fputs("  - ", stdout);
    write_slice(stdout, header_at(A, header_row, i));
    fputc('\n', stdout);
  }
}

static void print_row(const CpcArena *row_arena, const CpcValue *row,
                      const CpcArena *header_arena, const CpcValue *header_row,
                      size_t header_count, size_t row_index) {

  printf("Row %zu\n", row_index + 1);
  for (size_t i = 0; i < header_count; ++i) {
    fputs("  ", stdout);
    if (i < header_count) {
      write_slice(stdout, header_at(header_arena, header_row, i));
    }
    fputs(": ", stdout);
    const CpcValue *field = cpc_val_list_at(row_arena, row, i);
    if (cpc_is_slice(field)) {
      write_slice(stdout, field->as.slice);
    }
    fputc('\n', stdout);
  }
  fputc('\n', stdout);
}

int main(void) {
  char  *buffer = NULL;
  size_t len    = 0;

  if (!read_alloc_entire_stdin(&buffer, &len))
    return EXIT_FAILURE;

  enum { MAX_SIZE = 8192 };
  // use two arenas for header and rows to prevent forgetting to reset the arena
  CpcValue header_arena_storage[MAX_SIZE];
  CpcValue row_arena_storage[MAX_SIZE];
  CpcArena header_arena;
  CpcArena row_arena;

  cpc_arena_init(&header_arena, header_arena_storage, sizeof(header_arena_storage) / sizeof(header_arena_storage[0]), NULL);
  cpc_arena_init(&row_arena, row_arena_storage, sizeof(row_arena_storage) / sizeof(row_arena_storage[0]), NULL);
  CpcSlice  input = (CpcSlice){.ptr = buffer, .len = len};

  CpcResult header_res = parse_csv_row(&header_arena, input);
  if (!header_res.ok || !cpc_is_list(&header_res.out)) {
    fprintf(stderr, "failed to parse header");
    goto finally;
  }

  const CpcValue *header_row   = &header_res.out;
  size_t          header_count = header_row->as.list.len;
  print_headers(&header_arena, header_row, header_count);
  fputc('\n', stdout);

  CpcSlice rest = header_res.rest;

  size_t row_index = 0;
  while (rest.len > 0) {
    CpcResult row_res = parse_csv_row(&row_arena, rest);
    if (!row_res.ok) {
      fprintf(stderr, "parse error");
      goto finally;
    }

    if (cpc_is_list(&row_res.out) && (row_index < 2))
      print_row(&row_arena, &row_res.out, &header_arena, header_row, header_count, row_index);

    rest = row_res.rest;
    cpc_arena_reset(&row_arena);
    row_index++;
  }

  printf("Parsed %zu data rows.\n", row_index);

finally:
  free(buffer);

  return EXIT_SUCCESS;
}
