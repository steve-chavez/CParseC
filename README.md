# CParseC

Parsing on C has problems:

- Handwritten parsers (recursive descent, state machine, etc) are hard to maintain.
- Flex or Bison generated code is also hard to maintain plus it complicates builds.

**CParseC** (**C** **Parse**r **C**ombinators) offers a solution to parsing that is flexible and performant:

- Composable, expressive parsers written in plain C99 (inspired by Haskell's Parsec)
- Single header file (cparsec.h) with no dependencies (no libc assumed by default)
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

  const char csv[] = "alpha,\"beta\",\"ga,mm,a\",d\"\"elta\n";
  CpcSlice   input = cpc_slice_from_cstr(csv);
  CpcResult  result = parse_csv_row(&arena, input);

  for (size_t i = 0; i < result.out.as.list.len; ++i) {
    const CpcValue *cell = cpc_val_list_at(&arena, &result.out, i);
    CpcSlice        slice = cell->as.slice;
    printf("%.*s  ", (int)slice.len, slice.ptr);
    //alpha  "beta"  "ga,mm,a"  delta
  }

  return EXIT_SUCCESS;
}
```

When parsing 1 million CSV rows the above parser is ~1.25 times faster than [BurntSushi/rust-csv](https://github.com/BurntSushi/rust-csv) and ~20 times faster than [attoparsec-csv](https://github.com/robinbb/attoparsec-csv/).
See the [continuous benchmarking on CI](https://github.com/steve-chavez/CParseC/actions/runs/27530243247) to confirm the results.

## API

### Basic combinators

All the macros basically generate inlinable functions that take other inlinable functions as parameters. They return `CpcValue`, which can be a slice (`CpcSlice`) or a list (`CpcList`, which requires `CpcArena` for storage).

| Macro | Description | Label | Unnamed |
| --- | --- | --- | --- |
| `CPC_STRING(name, lit)` | Parses the exact string literal `lit` and returns it as a slice. | `CPC_STRING_LABEL` | `CPC_STRING_` * |
| `CPC_ALT(name, x, y)` | Tries parser `x`, and if it fails, tries parser `y` on the same input. | N/A | N/A |
| `CPC_RIGHT(name, x, y)` | Runs `x` then `y`, returning only the output of `y`. | N/A | N/A |
| `CPC_LEFT(name, x, y)` | Runs `x` then `y`, returning only the output of `x`. | N/A | N/A |
| `CPC_APPLY(name, x, y)` | Runs `x` then `y`, returning both outputs as a list. | N/A | N/A |
| `CPC_TAKE_WHILE_1(name, pred)` | Consumes one or more characters while `pred` is true and returns the consumed slice. | `CPC_TAKE_WHILE_1_LABEL` | N/A |
| `CPC_MANY_1(name, parser)` | Runs `parser` one or more times and returns the outputs as a list. | `CPC_MANY_1_LABEL` | N/A |
| `CPC_SEP_BY_1(name, item, sep)` | Parses one or more `item` values separated by `sep`, returning a list. | `CPC_SEP_BY_1_LABEL` | N/A |
| `CPC_PURE(name, value)` | Succeeds without consuming input and returns `value`. | N/A | N/A |
| `CPC_MAP(name, parser, fn)` | Runs `parser` and transforms its output with `fn`. | N/A | N/A |
| `CPC_MANY(name, parser)` | Runs `parser` zero or more times and returns the outputs as a list. | N/A | N/A |
| `CPC_SEP_BY(name, item, sep)` | Parses zero or more `item` values separated by `sep`, returning a list. | N/A | N/A |
| `CPC_TAKE_WHILE(name, pred)` | Consumes zero or more characters while `pred` is true and returns the consumed slice. | N/A | N/A |
| `CPC_MANY_TILL(name, parser, end)` | Repeats `parser` until `end` succeeds, returning the collected outputs as a list. | N/A | N/A |
| `CPC_TAKE_TILL(name, pred)` | Consumes input until `pred` becomes true and returns the consumed slice. | N/A | N/A |
| `CPC_MATCH(name, parser)` | Runs `parser` and returns the exact consumed input as a slice instead of its parsed value. | N/A | N/A |
| `CPC_ONE_OF(name, chars)` | Succeeds if the next character is one of the characters in `chars`, returning it as a slice. | `CPC_ONE_OF_LABEL` | N/A |
| `CPC_END_OF_LINE(name)` | Parses `\\n` or `\\r\\n` and returns the matched slice. | `CPC_END_OF_LINE_LABEL` | `CPC_END_OF_LINE_` |
| `CPC_ANY(name)` | Consumes and returns any single character as a slice. | `CPC_ANY_LABEL` | `CPC_ANY_` |
| `CPC_EOF(name)` | Succeeds only at end of input. | `CPC_EOF_LABEL` | `CPC_EOF_` |
| `CPC_BETWEEN(name, open, parser, close)` | Parses `open`, then `parser`, then `close`, returning only the output of `parser`. | N/A | N/A |

For convenience some parsers can be unnamed to reduce the overhead of naming every function. The ones that are marked with `*` like `CPC_STRING_`,
need `#define CPC_USE_UNNAMED` since they require non-standard C99 behavior ([Nested Functions](https://gcc.gnu.org/onlinedocs/gcc/Nested-Functions.html), [Statement Exprs](https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html)
and `__COUNTER__`).

The `_LABEL` variants take an extra `label` parameter that can be used to change the builtin error message.

> [!NOTE]
> The internal error messages that show the conditions of `arena size surpassed` and `no progress` (in case of badly written parsers with infinite loops) cannot be overridden.

### SIMD combinators

These parsers are specialized versions that make use of `memchr` to be SIMD enabled. Since `memchr` is not available everywhere, they require `#define CPC_USE_MEMCHR`.

| Macro | Description | Label | Anonymous |
| --- | --- | --- | --- |
| `CPC_TAKE_TILL_ONE_OF(name, stops)` | A combination of `CPC_TAKE_TILL` + `CPC_ONE_OF`. Returns a slice. | N/A | N/A |
| `CPC_TAKE_QUOTED(name, quote, escape)` | Parses a quoted string, handling escaped content. Returns a slice. | `CPC_TAKE_QUOTED_LABEL` | N/A |

## Haskell Comparison

### Differences with Haskell

- Do or do not, there is no `try`. Unlike Haskell's Parsec we don't need a `try` since it's cheap to backtrack due to working with slices. Parsers like `CPC_STRING` do not consume input if they fail.
- CParseC parsers always terminate. Unlike `many`, `manyTill`, `sepby`, `sepby1` which can infinite loop in Haskell.
- There's no equivalent for `>>` as this can be already expressed with `*>`, which is `CPC_RIGHT`.
- Only the parsers that can fail and have a builtin error message can have their error message overriden with a `_LABEL` variant of the parser.
  + Why? It was found on https://github.com/steve-chavez/CParseC/pull/2 that wrapping non-leaf parsers in functions hurt performance severily since they prevented inlining.

### Similarities with Haskell

All the functions are inspired by Haskell Parsec or AttoParsec. Here's a table with some equivalences:

| CParseC | Haskell |
| --- | --- |
| `CPC_ALT` | `<\|>` |
| `CPC_RIGHT` | `*>` |
| `CPC_LEFT` | `<*` |
| `CPC_APPLY` | `<*>` |
| `CPC_MAP` | `<$>` |
| `CPC_PURE` | `pure` |
