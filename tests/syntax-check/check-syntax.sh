#!/usr/bin/env bash
# Compiles every source file with -fsyntax-only on a machine without macOS or the Adobe SDK (Linux CI).
#
#   tests/syntax-check/check-syntax.sh            compile everything, in both WVP_DEBUG variants
#   tests/syntax-check/check-syntax.sh --tidy     also run clang-tidy over the same translation units
#
# The Cocoa, WebKit and CoreFoundation headers are replaced by the declarations in stubs/. The Adobe SDK headers are
# replaced by ae-stubs/ unless AE_SDK_ROOT points at a real SDK (then the real headers are used and checked).
# Nothing here links or runs: a clean pass means the code parses and type-checks, not that it works on a Mac.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$root"

cxx="${CXX:-g++}"
clangxx="${CLANGXX:-clang++}"
tidy="${CLANG_TIDY:-clang-tidy}"
run_tidy=0
failures=0

warnings=(-Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wold-style-cast
  -Woverloaded-virtual -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wimplicit-fallthrough -Wcast-align)

# Plain C++: the platform-neutral core, the key routing and the unit tests.
neutral_sources() { ls src/core/*.cpp src/mac/wvp_keys.cpp tests/unit/*.cpp; }
# C++ that includes the AEGP SDK headers.
sdk_sources() { ls src/ae/*.cpp; }
# Objective-C++: the macOS layer, and the one AE file that must include Cocoa through the SDK.
objcxx_sources() { ls src/mac/*.mm src/ae/*.mm; }

sdk_includes() {
  if [[ -n "${AE_SDK_ROOT:-}" ]]; then
    local sdk="$AE_SDK_ROOT/Examples"
    [[ -d "$sdk/Headers" ]] || sdk="$AE_SDK_ROOT"
    printf '%s\n' -isystem "$sdk/Headers" -isystem "$sdk/Headers/SP" -isystem "$sdk/Resources"
  else
    printf '%s\n' -isystem "$root/tests/syntax-check/ae-stubs"
  fi
}

common_flags() {
  local debug="$1"
  printf '%s\n' -fsyntax-only -std=c++17 -I"$root/src" -I"$root/src/ae" -I"$root/src/core" -I"$root/src/mac" \
    -isystem "$root/tests/syntax-check/stubs" -include "$root/tests/syntax-check/stubs/macshim.h" \
    -D__MACH__ -DMAC_ENV=1 -D__LP64__ -DWVP_VERSION='"check"' -DWVP_DEBUG="$debug" "${warnings[@]}"
  sdk_includes
}

objcxx_flags() {
  printf '%s\n' -x objective-c++ -fobjc-arc -fobjc-weak -fblocks -fobjc-runtime=gnustep-2.0
}

report() {
  local label="$1" file="$2" status="$3"
  if [[ "$status" -eq 0 ]]; then echo "ok    $label $file"; else echo "FAIL  $label $file"; failures=$((failures + 1)); fi
}

check_one() {
  local compiler="$1" debug="$2" file="$3"
  shift 3
  local -a flags
  mapfile -t flags < <(common_flags "$debug")
  local status=0
  "$compiler" "${flags[@]}" "$@" "$file" || status=$?
  report "$(basename "$compiler") debug=$debug" "$file" "$status"
}

check_objcxx_one() {
  local debug="$1" file="$2"
  local -a extra
  mapfile -t extra < <(objcxx_flags)
  check_one "$clangxx" "$debug" "$file" "${extra[@]}"
}

# clang-tidy reads the same flags. The Objective-C++ files need the Objective-C flags too.
tidy_one() {
  local file="$1"
  local -a flags extra=()
  mapfile -t flags < <(common_flags 0)
  if [[ "$file" == *.mm ]]; then mapfile -t extra < <(objcxx_flags); fi
  local status=0
  "$tidy" --quiet --warnings-as-errors='*' "$file" -- "${flags[@]}" "${extra[@]}" || status=$?
  report "clang-tidy" "$file" "$status"
}

check_all() {
  local debug file
  for debug in 0 1; do
    while read -r file; do check_one "$cxx" "$debug" "$file"; done < <(neutral_sources)
    while read -r file; do check_one "$cxx" "$debug" "$file"; done < <(sdk_sources)
    while read -r file; do check_objcxx_one "$debug" "$file"; done < <(objcxx_sources)
  done
}

tidy_all() {
  local file
  while read -r file; do tidy_one "$file"; done < <({ neutral_sources; sdk_sources; objcxx_sources; })
}

main() {
  [[ "${1:-}" == "--tidy" ]] && run_tidy=1
  check_all
  if [[ "$run_tidy" -eq 1 ]]; then tidy_all; fi
  if [[ "$failures" -ne 0 ]]; then echo "$failures check(s) failed"; exit 1; fi
  echo "all checks passed"
}

main "$@"
