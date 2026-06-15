set -euo pipefail

ref=$1
bin=${2:-build/csv_demo.o}
tmpdir=$(mktemp -d)
master_worktree="$tmpdir/master"

cleanup() {
  git worktree remove --force "$master_worktree"
  rm -rf "$tmpdir"
}
trap cleanup EXIT

make "$bin" bench/data/customers-1000000.csv

git worktree add --detach "$master_worktree" "$ref"
make -C "$master_worktree" "$bin"

hyperfine \
  "$bin < bench/data/customers-1000000.csv" \
  "$master_worktree/$bin < bench/data/customers-1000000.csv"
