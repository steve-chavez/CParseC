set -euo pipefail

echo -e "\n=== TEST: linking works correctly\n"
./build/linkage

echo -e "\n=== TEST: freestanding symbol hygiene\n"
if nm -u build/freestanding | grep .; then
  exit 1
fi

echo -e "\n=== TEST: basic parsers\n"
./build/basic

echo -e "\n=== TEST: unnamed parsers\n"
./build/unnamed

echo -e "\n=== TEST: SIMD parsers\n"
./build/simd
