# cparsec (WIP)

Features:

- Zero-copy parsing
- No hidden allocations, the user must supply an arena
- Macro-based, no function pointers in hot paths

Pending for CSV example:

- [ ] `<?>` (`label`)

## Differences with Haskell

- Parsers always terminate (many, manyTill, sepby, sepby1 can infinite loop in Haskell)

## Features

Basic combinators:

- `CPC_ALT` (`<|>`)
- `CPC_RIGHT` (`*>`)
- `CPC_LEFT` (`<*`)
- `CPC_APPLY` (`<*>`)
- `CPC_FMAP` (`<$>`).

Others:

- `CPC_TAKE_WHILE`
- `CPC_TAKE_WHILE_1`
- `CPC_MANY`
- `CPC_MANY_1`
- `CPC_MANY_TILL`
- `CPC_SEP_BY`
- `CPC_SEP_BY_1`
- `cpc_parser_eof`: end of input
