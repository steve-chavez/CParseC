set -euo pipefail

echo -e "\n=== TEST: linking works correctly\n"
./build/linkage

# Clang emits memcpy/memset calls for some reason at -O0, so we skip this test under clang
if ! readelf --string-dump=.comment build/freestanding 2>/dev/null | grep -qi 'clang'; then
  echo -e "\n=== TEST: freestanding symbol hygiene\n"

  # Fil-C adds calls to its runtime for safety checks even for freestanding, so we exclude the `fil_c/verse_` prefixes.
  if nm -u build/freestanding | awk '$NF !~ /^(filc_|verse_)/ { print; found = 1 } END { exit !found }'; then
    exit 1
  fi
fi

echo -e "\n=== TEST: basic parsers\n"
./build/basic

echo -e "\n=== TEST: unnamed parsers\n"
./build/unnamed

echo -e "\n=== TEST: SIMD parsers\n"
./build/simd
