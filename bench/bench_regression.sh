set -euo pipefail

bin=${1:-build/csv_demo.o}
tmpdir=$(mktemp -d)
master_worktree="$tmpdir/master"

cleanup() {
  git worktree remove --force "$master_worktree"
  rm -rf "$tmpdir"
}
trap cleanup EXIT

make "$bin" bench/data/customers-1000000.csv

git worktree add --detach "$master_worktree" master
make -C "$master_worktree" "$bin"

echo -e "\n## HEAD vs master for $bin\n"

hyperfine \
  "$bin < bench/data/customers-1000000.csv" \
  "$master_worktree/$bin < bench/data/customers-1000000.csv"
