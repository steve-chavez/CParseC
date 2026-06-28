# CParseC

Parsing on C has problems:

- Handwritten parsers (recursive descent, state machine, etc) are hard to maintain.
- Flex or Bison generated code is also hard to maintain plus it complicates builds.

**CParseC** (**C** **Parse**r **C**ombinators) offers a solution to parsing that is flexible and performant:

- Composable, expressive parsers written in plain C (inspired by Haskell's Parsec)
- C99, single header file (cparsec.h) with no dependencies (no libc assumed by default)
- Zero-copy parsing
- No hidden allocations, user-supplied arena
- Inlining-friendly, macros instead of function pointers in hot paths
- SIMD specialized combinators

## Demo

A CSV parser looks like this:

```c
#include <stdio.h>
#include <stdlib.h>

#define CPC_USE_MEMCHR
#define CPC_USE_UNNAMED
#include "cparsec.h"

CPC_TAKE_QUOTED(quotedField, '"', '"')
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
```

When parsing 1 million CSV rows the above parser is ~1.25 times faster than [BurntSushi/rust-csv](https://github.com/BurntSushi/rust-csv) and ~20 times faster than [attoparsec-csv](https://github.com/robinbb/attoparsec-csv/).
See the [continuous benchmarking on CI](https://github.com/steve-chavez/CParseC/actions/runs/27530243247) to confirm the results.

## API

Docs are in progress, for now you can see the usage on [test/basic.h](test/basic.h) and [test/simd.c](test/simd.c).

### Basic combinators

- `CPC_STRING`
- `CPC_STRING_`, requires `CPC_USE_UNNAMED`
- `CPC_ALT`
- `CPC_RIGHT`
- `CPC_LEFT`
- `CPC_APPLY`
- `CPC_TAKE_WHILE_1`
- `CPC_MANY_1`
- `CPC_SEP_BY_1`
- `CPC_PURE`
- `CPC_MAP`
- `CPC_MANY`
- `CPC_SEP_BY`
- `CPC_TAKE_WHILE`
- `CPC_MANY_TILL`
- `CPC_TAKE_TILL`
- `CPC_MATCH`
- `CPC_ONE_OF`
- `CPC_END_OF_LINE_`
- `CPC_ANY_`
- `CPC_EOF_`
- `CPC_BETWEEN`

### SIMD combinators

- `CPC_TAKE_TILL_ONE_OF`: a combination of `CPC_TAKE_TILL` + `CPC_ONE_OF` that uses the SIMD-optimized `memchr`. Returns a slice.
- `CPC_TAKE_QUOTED`: parses a quoted string, handling escaped content. Uses `memchr` internally. Returns a slice.

### Error Reporting (WIP)

Parsers that can fail have a `_LABEL` variant that can be used to change the builtin error message.

- `CPC_STRING_LABEL`
- `CPC_TAKE_WHILE_1_LABEL`
- `CPC_MANY_1_LABEL`
- `CPC_SEP_BY_1_LABEL`
- `CPC_EOF_LABEL`
- `CPC_ANY_LABEL`

The internal error messages that show the conditions of `arena surpassed` and `no progress` (in case of badly written parsers with infinite loops) cannot be overridden.

For now only the parsers that can fail and have a builtin error message can have their error message overriden with a `_LABEL` variant of the parser.

> [!NOTE]
> Why not a `CPC_LABEL` wrapper parser instead? We found out that the extra wrapping prevented inlining and affected performance on `cc45b07`

## Haskell Comparison

### Similarities with Haskell

All the functions are inspired by Haskell Parsec or AttoParsec. Here's a table with the equivalences:

| CParseC | Haskell |
| --- | --- |
| `CPC_ALT` | `<\|>` |
| `CPC_RIGHT` | `*>` |
| `CPC_LEFT` | `<*` |
| `CPC_APPLY` | `<*>` |
| `CPC_MAP` | `<$>` |
| `CPC_PURE` | `pure` |

### Differences with Haskell

- Do or do not, there is no `try`. Unlike Haskell's Parsec we don't need a `try` since it's cheap to backtrack due to working with slices. Parsers like `CPC_STRING` do not consume input if they fail.
- CParseC parsers always terminate. Unlike `many`, `manyTill`, `sepby`, `sepby1` which can infinite loop in Haskell.
- There's no equivalent for `>>` as this can be already expressed with `*>`, which is `CPC_RIGHT`.
