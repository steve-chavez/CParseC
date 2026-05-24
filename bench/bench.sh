set -euo pipefail

echo -e "\n=== TEST: both haskell and C parsing give the same result \n"

# This will output a failure showing the differences if there were any
diff -u \
  <(./build/attoparsec_csv < bench/data/customers-100.csv) \
  <(./build/csv_demo.o < bench/data/customers-100.csv)
