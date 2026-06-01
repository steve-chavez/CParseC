set -euo pipefail

echo -e "\n=== TEST: attoparsec csv works\n"
./build/attoparsec_csv < bench/data/customers-100.csv
