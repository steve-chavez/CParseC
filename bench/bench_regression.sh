set -euo pipefail

ref=$1
bin=${2:-build/csv_demo.o}
tmpdir=$(mktemp -d)
ref_worktree="$tmpdir/ref"

cleanup() {
  if [ -d "$ref_worktree" ]; then
    git worktree remove --force "$ref_worktree"
  fi
  rm -rf "$tmpdir"
}
trap cleanup EXIT

make "$bin" bench/data/customers-1000000.csv

git fetch origin "$ref"
git worktree add --detach "$ref_worktree" FETCH_HEAD
make -C "$ref_worktree" "$bin"

echo -e "\n## HEAD vs $ref for $bin\n"

hyperfine \
  "$bin < bench/data/customers-1000000.csv" \
  "$ref_worktree/$bin < bench/data/customers-1000000.csv"
