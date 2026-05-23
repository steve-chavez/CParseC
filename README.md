# cparsec (WIP)

Features:

- Zero-copy parsing
- No hidden allocations, the user must supply an arena
- Macro-based, no function pointers in hot paths

Pending for CSV example:

- [ ] sepBy1
- [ ] <?>

## Differences with Haskell

- Parsers always terminate (many, manyTill)

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
