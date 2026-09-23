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
#   UltimateDOOM-<v>-68040.sit.hqx           the application (StuffIt, BinHex)
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
( cd "$out" && sha256sum * > SHA256SUMS )
ls -la "$out"
