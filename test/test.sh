set -euo pipefail

echo -e "\n=== TEST: linking works correctly\n"
./build/linkage.o

echo -e "\n=== TEST: freestanding symbol hygiene\n"
if nm -u build/freestanding.o | grep .; then
  exit 1
fi

echo -e "\n=== TEST: basic functionality\n"
./build/basic.o
