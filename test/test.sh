set -euo pipefail

echo -e "\n=== TEST: linking works correctly\n"
./build/linkage

echo -e "\n=== TEST: freestanding symbol hygiene\n"
# Fil-C adds calls to its runtime for safety checks even for freestanding, so we exclude the `fil_c/verse_` prefixes.
if nm -u build/freestanding | awk '$NF !~ /^(filc_|verse_)/ { print; found = 1 } END { exit !found }'; then
  exit 1
fi

echo -e "\n=== TEST: basic parsers\n"
./build/basic

echo -e "\n=== TEST: unnamed parsers\n"
./build/unnamed

echo -e "\n=== TEST: SIMD parsers\n"
./build/simd
