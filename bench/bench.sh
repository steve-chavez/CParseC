set -euo pipefail

echo -e "\n=== BENCH: both haskell and C parsing give the same result \n"

# This will output a failure showing the differences if there were any
diff -u \
  <(./build/attoparsec_csv < bench/data/customers-100.csv) \
  <(./build/csv_demo.o < bench/data/customers-100.csv)

echo -e "\n=== BENCH: haskell and C parsing 1M CSV rows \n"

hyperfine \
  './build/csv_demo.o < bench/data/customers-1000000.csv' \
  './build/attoparsec_csv < bench/data/customers-1000000.csv' \
  --export-markdown build/report.md
