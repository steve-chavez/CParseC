set -euo pipefail

make bench > /dev/null

# Haskell, C and Rust should give the same report,
# this will output a failure showing the differences if there were any.
diff -u \
  <(./build/attoparsec_csv < bench/data/customers-100.csv) \
  <(./build/csv_demo.o < bench/data/customers-100.csv)

diff -u \
  <(./build/csv-rust-demo < bench/data/customers-100.csv) \
  <(./build/csv_demo.o < bench/data/customers-100.csv)

# Print the report in markdown
echo -e "# Parsing 1M CSV rows"

echo -e "\n## Haskell vs CParseC\n"

hyperfine \
  './build/csv_demo.o < bench/data/customers-1000000.csv' \
  './build/attoparsec_csv < bench/data/customers-1000000.csv' \
  --export-markdown build/report-c-hs.md 1>&2

cat build/report-c-hs.md

echo -e "\n## Rust vs CParseC\n"

hyperfine \
  './build/csv_simd_demo.o < bench/data/customers-1000000.csv' \
  './build/csv-rust-demo < bench/data/customers-1000000.csv' \
  --export-markdown build/report-c-rust.md 1>&2

cat build/report-c-rust.md
