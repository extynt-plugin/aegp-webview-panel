#!/usr/bin/env bash
# Usage: rez-pipl.sh <AE SDK Examples dir> <PiPL .r file> <output .rsrc>. Compiles the PiPL with Xcode's Rez, trying
# a few equivalent command lines until one produces a non-empty file; WVP_REZ_VERBOSE=1 prints every attempt.
set -euo pipefail

sdk_examples="$1"
source_r="$2"
out_rsrc="$3"

fail() { echo "rez-pipl.sh: $*" >&2; exit 1; }

find_rez() {
  [ "$(uname -s)" = "Darwin" ] || fail "Rez only exists on macOS."
  local rez
  rez="$(xcrun --find Rez 2>/dev/null || true)"
  [ -n "$rez" ] && [ -x "$rez" ] || fail "Rez not found. Install full Xcode and select it: sudo xcode-select -s /Applications/Xcode.app/Contents/Developer"
  echo "$rez"
}

check_inputs() {
  [ -f "$sdk_examples/Headers/AEConfig.h" ] || fail "AEConfig.h not found under $sdk_examples/Headers (check AE_SDK_ROOT)."
  [ -f "$sdk_examples/Resources/AE_General.r" ] || fail "AE_General.r not found under $sdk_examples/Resources."
  [ -f "$source_r" ] || fail "PiPL source not found: $source_r"
}

attempt() {
  local label="$1" rez="$2" log status=0
  shift 2
  rm -f "$out_rsrc"
  log="$("$rez" "$@" -o "$out_rsrc" "$source_r" 2>&1)" || status=$?
  if [ "$status" -eq 0 ] && [ -s "$out_rsrc" ]; then
    echo "rez-pipl.sh: compiled the PiPL ($label) -> $out_rsrc"
    return 0
  fi
  if [ -n "${WVP_REZ_VERBOSE:-}" ] || [ "$label" = "plain" ]; then
    echo "rez-pipl.sh: variant '$label' failed (exit $status):" >&2
    printf '    %s\n' "${log//$'\n'/$'\n'    }" >&2
  fi
  return 1
}

main() {
  local rez sdkroot
  rez="$(find_rez)"
  check_inputs
  sdkroot="$(xcrun --sdk macosx --show-sdk-path 2>/dev/null || true)"
  mkdir -p "$(dirname "$out_rsrc")"
  local common=(-useDF -d __MACH__ -i "$sdk_examples/Headers" -i "$sdk_examples/Resources" -i "$(dirname "$source_r")")
  [ -z "$sdkroot" ] || common+=(-isysroot "$sdkroot")
  attempt "two archs" "$rez" "${common[@]}" -arch arm64 -arch x86_64 && return 0
  attempt "arm64" "$rez" "${common[@]}" -arch arm64 && return 0
  attempt "define" "$rez" "${common[@]}" -d __arm64__ && return 0
  attempt "plain" "$rez" "${common[@]}" && return 0
  fail "every Rez variant failed. Run with WVP_REZ_VERBOSE=1. Common causes: Xcode license not accepted (sudo xcodebuild -license), wrong AE_SDK_ROOT."
}

main
