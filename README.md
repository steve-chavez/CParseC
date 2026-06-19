# CParseC

Parsing on C has problems:

- Flex and Bison generate hard to maintain code and complicates builds.
- Handwritten parsers (recursive descent, state machine, etc) are also hard to maintain.

**CParseC** (**C** **Parse**r **C**ombinators) offers a solution to parsing that is flexible and performant:

- Composable, expressive parsers written in plain C (inspired by Haskell's Parsec)
- Single header file (cparsec.h) with zero dependencies (including no libc)
- Zero-copy parsing
- No hidden allocations, user-supplied arena
- Inlining-friendly, macros instead of function pointers in hot paths
- SIMD specialized combinators

## Demo

A CSV parser looks like this:

```c
#define CPC_USE_MEMCHR
#include "cparsec.h"

CPC_TAKE_QUOTED(quotedField, '"')
CPC_TAKE_TILL_ONE_OF(unquotedField, ",\r\n")
CPC_ALT(field, quotedField, unquotedField)
CPC_STRING(p_comma, ",")
CPC_SEP_BY_1(record, field, p_comma)
CPC_ALT(lineEnd, CPC_END_OF_LINE_, CPC_EOF_)
CPC_LEFT(parse_csv_row, record, lineEnd)

int main(void){
  CpcArena arena;
  CpcValue arena_storage[8192];
  cpc_arena_init(&arena, arena_storage, sizeof(arena_storage) / sizeof(arena_storage[0]), NULL);
  //..
  CpcSlice input = (CpcSlice){.ptr = buffer, .len = len};
  CpcResult res = parse_csv_row(&arena, input);
}
```

When parsing 1 million CSV rows this parser is ~1.25 times faster than [BurntSushi/rust-csv](https://github.com/BurntSushi/rust-csv) and ~15 times faster than [attoparsec-csv](https://github.com/robinbb/attoparsec-csv/).
See the [continuous benchmarking on CI](https://github.com/steve-chavez/CParseC/actions/runs/27530243247) to confirm the results.

## API

Docs are in progress, for now you can see the usage on [test/basic.h](test/basic.h) and [test/simd.c](test/simd.c).

### Basic combinators

- `CPC_STRING`
- `CPC_STRING_`, requires `CPC_USE_UNNAMED`
- `CPC_ALT` (`<|>`)
- `CPC_RIGHT` (`*>`)
- `CPC_LEFT` (`<*`)
- `CPC_APPLY` (`<*>`)
- `CPC_TAKE_WHILE_1`
- `CPC_MANY_1`
- `CPC_SEP_BY_1`
- `CPC_PURE`
- `CPC_MAP` (`<$>`)
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

- `CPC_TAKE_TILL_ONE_OF`: a combination of `CPC_TAKE_TILL` + `CPC_ONE_OF` that uses `memchr` so it's SIMD-friendly.
- `CPC_TAKE_QUOTED`: parses a quoted string, handling double quotes as escaped content. Returns a slice.

### Labels

The leaf parsers that can fail have a `_LABEL` variant that can be used to change the builtin error message.

- `CPC_STRING_LABEL`
- `CPC_TAKE_WHILE_1_LABEL`
- `CPC_MANY_1_LABEL`
- `CPC_SEP_BY_1_LABEL`
- `CPC_EOF_LABEL`
- `CPC_ANY_LABEL`

The internal error messages that show the conditions of `arena surpassed` and `no progress` (in case of badly written parsers with infinite loops) cannot be overridden.

## Differences with Haskell

- Do or do not, there is no `try`. Unlike Haskell's Parsec we don't need a `try` since it's cheap to backtrack due to working with slices.
  Parsers like `CPC_STRING` do not consume input if they fail.
- CParseC parsers always terminate (many, manyTill, sepby, sepby1 can infinite loop in Haskell)
- There's no equivalent for `>>` as this can be already expressed with `*>`, which is `CPC_RIGHT`. To express something like `string "\"\"" >> return "\""`, you can do:
  ```c
  CPC_STRING(p_ddquote, "\"\"")
  CPC_PURE(p_dquote_, cpc_val_slice(cpc_slice_from_cstr("\"")))
  CPC_RIGHT(p_dquote, p_ddquote, p_dquote_)
  ```
- Only the parsers that can fail and have a builtin error message can have their error message overriden with a `_LABEL` variant of the parser.
