set -euo pipefail

ref=$1
bin=$2
tmpdir=$(mktemp -d)
ref_worktree="$tmpdir/ref"

cleanup() {
  if [ -d "$ref_worktree" ]; then
    git worktree remove --force "$ref_worktree"
  fi
  rm -rf "$tmpdir"
}
trap cleanup EXIT

make "$bin" bench/data/customers-1000000.csv > /dev/null

git worktree add --detach "$ref_worktree" FETCH_HEAD > /dev/null
make -C "$ref_worktree" "$bin" > /dev/null

echo -e "\n## CParseC HEAD vs $ref for $bin\n"

hyperfine --warmup 3 \
  "$bin < bench/data/customers-1000000.csv" \
  "$ref_worktree/$bin < bench/data/customers-1000000.csv" \
  --export-markdown "build/report-head-$ref.md" 1>&2

cat "build/report-head-$ref.md"
