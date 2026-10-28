#!/usr/bin/env bash
# solve every tiny map exactly, one process per map, logs in out/solve/<map>.log
# usage: scripts/solve_suite.sh [table_log2] [jobs]
set -uo pipefail
cd "$(dirname "$0")/.."
table="${1:-24}"
jobs="${2:-4}"
mkdir -p out/solve
ls maps/tiny/*.sexp | xargs -P "$jobs" -I{} sh -c \
    'name=$(basename {} .sexp); ./build/shoal search {} --solve --quiet --table '"$table"' > out/solve/$name.log 2>&1'
for log in out/solve/*.log; do
    printf '%-12s %s | %s\n' "$(basename "$log" .log)" "$(grep '^solved' "$log")" \
        "$(grep '^best' "$log" | sed 's/  depth.*nodes/ nodes/')"
done
