set -euo pipefail

echo -e "\n=== BENCH: Haskell, C and Rust demos give the same report \n"

# This will output a failure showing the differences if there were any
diff -u \
  <(./build/attoparsec_csv < bench/data/customers-100.csv) \
  <(./build/csv_demo.o < bench/data/customers-100.csv)

diff -u \
  <(./build/csv-rust-demo < bench/data/customers-100.csv) \
  <(./build/csv_demo.o < bench/data/customers-100.csv)

echo -e "\n=== BENCH: Haskell, C and Rust demos report over 1M CSV rows \n"

hyperfine \
  './build/csv_demo.o < bench/data/customers-1000000.csv' \
  './build/attoparsec_csv < bench/data/customers-1000000.csv' \
  --export-markdown build/report-c-hs.md

hyperfine \
  './build/csv_demo.o < bench/data/customers-1000000.csv' \
  './build/csv-rust-demo < bench/data/customers-1000000.csv' \
  --export-markdown build/report-c-rust.md
