#!/usr/bin/env bash
# Build a release from a clean tree:
#
#   tools/release.sh 1.3          # -> build-release/release/
#   tools/release.sh              # version from "git describe", or the date
#
# Needs the Retro68 toolchain (RETRO68_TOOLCHAIN, default
# ~/repos/Retro68-build/toolchain), cmake, python3 and rb-cli
# (https://github.com/danifunker/rusty-backup).  The GitHub Actions workflow
# (.github/workflows/release.yml) runs this same script.
#
# Output, besides the dist/ files from make-dist.sh:
#   UltimateDOOM-<v>-68040.sit.hqx           the 68K application (StuffIt, BinHex)
#   UltimateDOOM-<v>-PPC.sit.hqx             the native PowerPC application
#   DOOM-<v>-68040-shareware.hda.zip         playable SCSI disk image
#   DOOM-<v>-68040-noWAD.hda.zip             the same without a WAD
#   SHA256SUMS
set -euo pipefail
here=$(cd "$(dirname "$0")/.." && pwd)
cd "$here"

version=${1:-}
if [ -z "$version" ]; then
    version=$(git describe --tags --always 2>/dev/null || date +%Y%m%d)
fi
version=${version#v}

tc=${RETRO68_TOOLCHAIN:-$HOME/repos/Retro68-build/toolchain}
build=build-release
rm -rf "$build"
cmake -S . -B "$build" \
    -DCMAKE_TOOLCHAIN_FILE="$tc/m68k-apple-macos/cmake/retro68.toolchain.cmake" \
    -DDOOM_VERSION="$version" >/dev/null
cmake --build "$build" -j"$(nproc 2>/dev/null || echo 2)"

BUILD_DIR=$build VERSION=$version tools/make-dist.sh >/dev/null

out="$build/release"
rm -rf "$out"
mkdir -p "$out"
cp "$build/dist/UltimateDOOM-$version-68040.sit.hqx" "$out/"
for d in shareware noWAD; do
    ( cd "$build/dist" && zip -q -9 "../release/DOOM-$version-68040-$d.hda.zip" "DOOM-$version-68040-$d.hda" )
done
# Native PowerPC build (same sources, retroppc toolchain), packed the same
# way: MacBinary -> HFS image -> BinHex -> StuffIt, keeping both forks.
ppc=build-release-ppc
rm -rf "$ppc"
cmake -S . -B "$ppc" \
    -DCMAKE_TOOLCHAIN_FILE="$tc/powerpc-apple-macos/cmake/retroppc.toolchain.cmake" \
    -DDOOM_VERSION="$version" >/dev/null
cmake --build "$ppc" -j"$(nproc 2>/dev/null || echo 2)"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
python3 tools/mbrename.py "$ppc/UltimateDOOM.bin" "$work/app.bin" "Ultimate DOOM PPC"
rb-cli --progress never -q new --fs hfs --size 4M --name PPC "$work/ppc.hfs" >/dev/null 2>&1 ||
    rb-cli --progress never -q new volume hfs --size 4M --name PPC "$work/ppc.hfs" >/dev/null
rb-cli --progress never -q put-macbinary "$work/ppc.hfs" "$work/app.bin" >/dev/null
rb-cli --progress never -q get-binhex "$work/ppc.hfs" "/Ultimate DOOM PPC" "$work/Ultimate DOOM PPC.hqx"
rb-cli --progress never -q archive create "$out/UltimateDOOM-$version-PPC.sit.hqx" "$work/Ultimate DOOM PPC.hqx"

( cd "$out" && sha256sum * > SHA256SUMS )
ls -la "$out"
