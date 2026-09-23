#!/usr/bin/env bash
# Build the release artefacts into build/dist:
#
#   DOOM-68040-shareware.hda   ready to play: app, music, shareware DOOM1.WAD
#   DOOM-68040-noWAD.hda       same without a WAD; add your own DOOM.WAD/DOOM2.WAD
#   UltimateDOOM-68040.sit.hqx the application alone, to drop into an existing
#                              Mac DOOM folder (which already has its Music)
set -euo pipefail
here=$(cd "$(dirname "$0")/.." && pwd)
dist="$here/build/dist"
mkdir -p "$dist"

"$here/tools/make-disk.sh" -o "$dist/DOOM-68040-shareware.hda" >/dev/null
"$here/tools/make-disk.sh" --no-shareware -o "$dist/DOOM-68040-noWAD.hda" >/dev/null

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
# Round-trip through the disk image so the archive gets both forks and the
# Finder info (rb-cli's archiver wants BinHex or MacBinary II input).
rb-cli --progress never -q get-binhex "$dist/DOOM-68040-noWAD.hda@1" "/Ultimate DOOM" "$work/Ultimate DOOM.hqx"
rm -f "$dist/UltimateDOOM-68040.sit.hqx"
rb-cli --progress never -q archive create "$dist/UltimateDOOM-68040.sit.hqx" "$work/Ultimate DOOM.hqx"

ls -la "$dist"
