#!/usr/bin/env bash
# Builds (and optionally installs) the plugin on macOS.
#
#   AE_SDK_ROOT=~/sdk/AfterEffectsSDK ./build-mac.sh [options]
#
#   --arm64          Apple Silicon only (default: a universal arm64 + x86_64 binary)
#   --debug-page     WVP_DEBUG build: Web Inspector, and dev-url.txt can point the panel at a dev server. Never ship it.
#   --no-sign        skip code signing (After Effects may still load an unsigned plugin built on this Mac)
#   --clean          delete the build directory first
#   --install        copy the bundle into the After Effects Plug-ins folder (sudo is used when needed)
#   --uninstall      remove the installed bundle and exit
#   --ae-year YEAR   which After Effects to install into (default 2026); the folder is
#                    /Applications/Adobe After Effects YEAR/Plug-ins
#
# Signing: with WVP_CODESIGN_ID set to a "Developer ID Application: ..." identity (see `security find-identity -v -p
# codesigning`) the bundle is signed with it and the hardened runtime. Without it the bundle is ad-hoc signed ("-"),
# which is enough to run on the Mac that built it. A bundle copied to another Mac gets quarantined by macOS: run
# `xattr -cr wvpanel.plugin` there, or sign and notarize it with a Developer ID.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="$root/build"
bundle="$build_dir/wvpanel.plugin"

archs="arm64;x86_64"
debug_page=OFF
sign=1
clean=0
install=0
uninstall=0
ae_year=2026

die() {
  echo "build-mac.sh: $*" >&2
  exit 1
}

parse_args() {
  while [[ $# -gt 0 ]]; do
    case "$1" in
      --arm64) archs="arm64" ;;
      --debug-page) debug_page=ON ;;
      --no-sign) sign=0 ;;
      --clean) clean=1 ;;
      --install) install=1 ;;
      --uninstall) uninstall=1 ;;
      --ae-year) shift; ae_year="${1:?--ae-year needs a year}" ;;
      -h | --help) sed -n '2,20p' "${BASH_SOURCE[0]}"; exit 0 ;;
      *) die "unknown option: $1" ;;
    esac
    shift
  done
}

plugins_dir() { echo "/Applications/Adobe After Effects $ae_year/Plug-ins"; }

privileged() {
  if [[ -w "$(plugins_dir)" ]]; then "$@"; else sudo "$@"; fi
}

check_environment() {
  [[ "$(uname -s)" == "Darwin" ]] || die "this script builds the macOS plugin; run it on a Mac."
  command -v cmake >/dev/null || die "cmake not found (brew install cmake)."
  [[ -n "${AE_SDK_ROOT:-}" ]] || die "set AE_SDK_ROOT to the unpacked After Effects SDK (see README.md)."
}

configure_and_build() {
  [[ "$clean" -eq 1 ]] && rm -rf "$build_dir"
  cmake -S "$root" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release -DAE_SDK_ROOT="$AE_SDK_ROOT" \
    -DCMAKE_OSX_ARCHITECTURES="$archs" -DWVP_DEBUG="$debug_page" -DWVP_BUILD_PLUGIN=ON -DWVP_BUILD_TESTS=OFF
  cmake --build "$build_dir" --config Release
  [[ -d "$bundle" ]] || die "the build did not produce $bundle"
}

sign_bundle() {
  [[ "$sign" -eq 1 ]] || return 0
  local identity="${WVP_CODESIGN_ID:--}"
  if [[ "$identity" == "-" ]]; then
    codesign --force --deep --sign - "$bundle"
  else
    codesign --force --deep --options runtime --timestamp --sign "$identity" "$bundle"
  fi
  codesign --verify --deep --strict "$bundle" && echo "signed ($([[ "$identity" == "-" ]] && echo ad-hoc || echo "$identity"))"
}

install_bundle() {
  local dest
  dest="$(plugins_dir)"
  [[ -d "$dest" ]] || die "$dest does not exist; is After Effects $ae_year installed? (see --ae-year)"
  privileged rm -rf "$dest/wvpanel.plugin"
  privileged cp -R "$bundle" "$dest/"
  privileged xattr -cr "$dest/wvpanel.plugin" || true
  echo "installed to $dest/wvpanel.plugin. Restart After Effects, then Window > WebView Panel."
}

uninstall_bundle() {
  privileged rm -rf "$(plugins_dir)/wvpanel.plugin"
  echo "removed $(plugins_dir)/wvpanel.plugin"
}

main() {
  parse_args "$@"
  if [[ "$uninstall" -eq 1 ]]; then uninstall_bundle; return 0; fi
  check_environment
  configure_and_build
  sign_bundle
  echo "built $bundle (architectures: $archs)"
  if [[ "$install" -eq 1 ]]; then install_bundle; fi
}

main "$@"
