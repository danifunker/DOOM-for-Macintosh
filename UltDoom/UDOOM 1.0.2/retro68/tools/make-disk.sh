#!/usr/bin/env bash
# Package the Retro68 build into a small SCSI hard-disk image with rb-cli.
#
#   tools/make-disk.sh [-o DOOM.hda] [-s 48M] [-w path/to/WAD ...] [--no-shareware]
#
# The disk (volume "DOOM") holds the application, the QuickTime MIDI music
# and a WAD.  By default the freely redistributable shareware DOOM1.WAD is
# pulled from the "DOOM SW 1.0.2/DOOM.sea" archive in this repository; pass
# -w to add your own DOOM.WAD / DOOM2.WAD instead or as well.
#
# The result is an Apple Partition Map disk with an Apple SCSI driver, so a
# Quadra ROM (real or QEMU's q800) mounts it as a second SCSI disk.
set -euo pipefail

here=$(cd "$(dirname "$0")/.." && pwd)
repo=$(cd "$here/../../.." && pwd)
app="$here/build/UltimateDOOM.bin"
out="$here/build/DOOM.hda"
size=48M
wads=()
shareware=1

while [ $# -gt 0 ]; do
    case "$1" in
        -o) out=$2; shift 2 ;;
        -s) size=$2; shift 2 ;;
        -w) wads+=("$2"); shift 2 ;;
        -a) app=$2; shift 2 ;;
        --no-shareware) shareware=0; shift ;;
        *) echo "unknown option: $1" >&2; exit 1 ;;
    esac
done

[ -f "$app" ] || { echo "build the app first ($app missing)" >&2; exit 1; }

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
rb() { rb-cli --progress never -q "$@"; }

# Shipped shareware release: music, read-me and (optionally) DOOM1.WAD.
rb archive extract --format macbinary "$repo/DOOM SW 1.0.2/DOOM.sea" "$work/sw"
sw="$work/sw/DOOM"

flat="$work/doom.hfs"
rb new --fs hfs --size "$size" --name DOOM "$flat"

rb put-macbinary "$flat" "$app"
rb mkdir "$flat" /Music
for m in "$sw"/Music/*.bin; do
    rb put-macbinary --dst-dir /Music "$flat" "$m"
done
rb put-macbinary "$flat" "$sw/DOOM Read Me.bin"

if [ "$shareware" = 1 ]; then
    rb put-macbinary "$flat" "$sw/DOOM1.WAD.bin"
fi
for w in "${wads[@]}"; do
    name=$(basename "$w" | tr '[:lower:]' '[:upper:]')
    rb put "$flat" "$w" "/$name"
    rb chmeta "$flat" "/$name" --type '.WAD' --creator idSW
done

# Wrap in an Apple Partition Map and add the SCSI driver.
rb expand --size "$size" --output "$out" "$flat"
rb mac-scsi-bless "$out"

echo "wrote $out"
rb ls "$out@1" /
