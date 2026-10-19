#!/usr/bin/env bash
# Checks the coding limits (file <= 500 lines, function <= 80, complexity <= 10, nesting <= 4, parameters <= 5).
# C++ goes through lizard; shell scripts get the length checks here, since lizard does not read shell.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

max_file_lines=500
max_function_lines=80
failures=0

fail() {
  echo "FAIL  $*"
  failures=$((failures + 1))
}

tracked_files() { git ls-files -- "$@"; }

check_file_lengths() {
  local file lines
  while read -r file; do
    lines=$(wc -l <"$file")
    if [[ "$lines" -gt "$max_file_lines" ]]; then fail "$file has $lines lines (limit $max_file_lines)"; fi
  done < <(tracked_files '*.cpp' '*.h' '*.mm' '*.sh' '*.cmake' 'CMakeLists.txt' '*.html' '*.r' '*.yml')
}

check_lizard() {
  if ! command -v lizard >/dev/null; then fail "lizard is not installed (pip install lizard)"; return; fi
  lizard -l cpp -l objectivec -C 10 -L "$max_function_lines" -a 5 -Ens -T max_nested_structures=4 -w src tests/unit ||
    fail "lizard reported functions over the limits"
}

long_shell_functions() {
  awk -v limit="$max_function_lines" '
    /^[A-Za-z_][A-Za-z0-9_]*\(\) *\{/ { name = $1; start = FNR }
    /^\}$/ && name != "" {
      if (FNR - start + 1 > limit) printf "%s:%d:%s:%d\n", FILENAME, start, name, FNR - start + 1
      name = ""
    }
  ' "$@"
}

check_shell_functions() {
  local -a scripts
  mapfile -t scripts < <(tracked_files '*.sh')
  local line
  while read -r line; do
    [[ -n "$line" ]] && fail "shell function over $max_function_lines lines: $line"
  done < <(long_shell_functions "${scripts[@]}")
}

main() {
  check_file_lengths
  check_lizard
  check_shell_functions
  if [[ "$failures" -ne 0 ]]; then echo "$failures limit check(s) failed"; exit 1; fi
  echo "limits ok"
}

main "$@"
