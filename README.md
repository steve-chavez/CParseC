# CParseC

Features:

- Zero-copy parsing
- No hidden allocations, the user must supply an arena
- Macro-based, no function pointers in hot paths

## Differences with Haskell

- Parsers always terminate (many, manyTill, sepby, sepby1 can infinite loop in Haskell)
- There's no equivalent for `>>` as this can be already expressed with `*>`, which is `CPC_RIGHT`. To express something like `string "\"\"" >> return "\""`, you can do:
  ```c
  CPC_STRING(p_ddquote, "\"\"")
  CPC_PURE(p_dquote_, cpc_val_slice(cpc_slice_from_cstr("\"")))
  CPC_RIGHT(p_dquote, p_ddquote, p_dquote_)
  ```
- Only the parsers that can fail and have a builtin error message can have their error message overriden with a `_LABEL` variant of the parser.

## Features

Basic combinators:

- `CPC_STRING`
- `CPC_ALT` (`<|>`)
- `CPC_RIGHT` (`*>`)
- `CPC_LEFT` (`<*`)
- `CPC_APPLY` (`<*>`)
- `CPC_TAKE_WHILE_1`
- `CPC_MANY_1`
- `CPC_SEP_BY_1`
- `CPC_PURE`: always succeeds
- `CPC_MAP` (`<$>`): always succeeds
- `CPC_MANY`: always succeeds
- `CPC_SEP_BY`: always succeeds
- `CPC_TAKE_WHILE`: always succeeds
- `CPC_MANY_TILL`: always succeeds
- `cpc_parser_eof`: end of input
- `CPC_MATCH`

### Labels

The leaf parsers that can fail have a `_LABEL` variant that can be used to change the builtin error message.

- `CPC_STRING_LABEL`
- `CPC_TAKE_WHILE_1_LABEL`
- `CPC_MANY_1_LABEL`
- `CPC_SEP_BY_1_LABEL`
- `CPC_EOF_LABEL`

The internal error messages that show the conditions of `arena surpassed` and `no progress` (in case of badly written parsers with infinite loops) cannot be overridden.

### Compressed combinators

- `CPC_BETWEEN`
