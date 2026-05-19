# cparsec (WIP)

Features:

- Zero-copy parsing
- No hidden allocations, the user must supply an arena
- Macro-based, no function pointers in hot paths

Pending for CSV example:

- [ ] many
- [ ] manyTill
- [ ] takeWhile1
- [ ] takeWhile
- [ ] sepBy1
- [ ] <?>

## Features

Haskell combinators:

- `CPC_ALT` (`<|>`)
- `CPC_RIGHT` (`*>`)
- `CPC_LEFT` (`<*`)
- `CPC_APPLY` (`<*>`)
- `CPC_FMAP` (`<$>`).
