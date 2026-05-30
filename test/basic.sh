set -euo pipefail

echo -e "\n=== TEST: attoparsec csv\n"
./build/attoparsec_csv < test/data/customers-100.csv

echo -e "\n=== TEST: basic functionality\n"
./build/basic.o
