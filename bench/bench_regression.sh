set -euo pipefail

ref=$1
tmpdir=$(mktemp -d)
master_worktree="$tmpdir/master"

cleanup() {
  git worktree remove --force "$master_worktree"
  rm -rf "$tmpdir"
}
trap cleanup EXIT

make build/csv_demo.o bench/data/customers-1000000.csv

git worktree add --detach "$master_worktree" "$ref"
make -C "$master_worktree" build/csv_demo.o

hyperfine \
  "build/csv_demo.o < bench/data/customers-1000000.csv" \
  "$master_worktree/build/csv_demo.o < bench/data/customers-1000000.csv" \
