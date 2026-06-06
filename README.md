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

## Features

Basic combinators:

- `CPC_ALT` (`<|>`)
- `CPC_RIGHT` (`*>`)
- `CPC_LEFT` (`<*`)
- `CPC_APPLY` (`<*>`)
- `CPC_FMAP` (`<$>`).
- `CPC_PURE`.

The non-failing combinators are suffixed with `_0`:

- `CPC_MANY_0`
- `CPC_SEP_BY_0`
- `CPC_TAKE_WHILE_0`
- `CPC_MANY_TILL_0`

Others:

- `CPC_TAKE_WHILE_1`
- `CPC_MANY_1`
- `CPC_SEP_BY_1`
- `CPC_LABEL`
- `cpc_parser_eof`: end of input
