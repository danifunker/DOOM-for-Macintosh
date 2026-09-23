#!/usr/bin/env bash
# Package the Retro68 build into a small SCSI hard-disk image with rb-cli.
#
#   tools/make-disk.sh [-o DOOM.hda] [-s 48M] [-w path/to/WAD ...] [-m path/to/MIDI/FOLDER ...]
#                      [--no-shareware] [--no-music]
#   DOOM_ARGS="-bench -quit" tools/make-disk.sh      # also write a "DOOM Args" file
#   BUILD_DIR=build-release tools/make-disk.sh      # app from another build tree
#
# The disk (volume "DOOM") holds the application, the music and a WAD.
# Music lives in MIDI/<WAD name>/ (MIDI/DOOM1 for the shareware tracks);
# -m copies a host folder of .MID!/.MID files to MIDI/<folder name>.
#
# By default the freely redistributable shareware DOOM1.WAD is pulled from the
# "DOOM SW 1.0.2/DOOM.sea" archive in this repository; pass -w to add your own
# DOOM.WAD / DOOM2.WAD / TNT.WAD / PLUTONIA.WAD or add-on WADs as well.
#
# The result is an Apple Partition Map disk with an Apple SCSI driver, so a
# Quadra ROM (real or QEMU's q800) mounts it as a second SCSI disk.
set -euo pipefail

here=$(cd "$(dirname "$0")/.." && pwd)
repo=$(cd "$here/../../.." && pwd)
build="$here/${BUILD_DIR:-build}"
app="$build/UltimateDOOM.bin"
out="$build/DOOM.hda"
size=48M
wads=()
midis=()
shareware=1
music=1

while [ $# -gt 0 ]; do
    case "$1" in
        -o) out=$2; shift 2 ;;
        -s) size=$2; shift 2 ;;
        -w) wads+=("$2"); shift 2 ;;
        -m) midis+=("$2"); shift 2 ;;
        -a) app=$2; shift 2 ;;
        --no-shareware) shareware=0; shift ;;
        --no-music) music=0; shift ;;
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
# rb-cli releases since 2026-09 group "new" by media class
if rb new --help 2>&1 | grep -q -- '--fs'; then
    rb new --fs hfs --size "$size" --name DOOM "$flat"
else
    rb new volume hfs "$flat" --size "$size" --name DOOM
fi

python3 "$here/tools/mbrename.py" "$app" "$work/app.bin" "Ultimate DOOM"
rb put-macbinary "$flat" "$work/app.bin"
if [ "$music" = 1 ]; then
    rb mkdir "$flat" /MIDI
    rb mkdir "$flat" /MIDI/DOOM1
    for m in "$sw"/Music/*.bin; do
        rb put-macbinary --dst-dir /MIDI/DOOM1 "$flat" "$m"
    done
fi
[ ${#midis[@]} -gt 0 ] && rb mkdir "$flat" /MIDI 2>/dev/null || true
for d in "${midis[@]}"; do
    name=$(basename "$d" | tr '[:lower:]' '[:upper:]')
    rb mkdir "$flat" "/MIDI/$name" 2>/dev/null || true
    for m in "$d"/*; do
        case "$m" in
            *.bin) rb put-macbinary --dst-dir "/MIDI/$name" "$flat" "$m" ;;
            *)     rb put "$flat" "$m" "/MIDI/$name/$(basename "$m" | tr '[:lower:]' '[:upper:]')" ;;
        esac
    done
done
rb put-macbinary "$flat" "$sw/DOOM Read Me.bin"

if [ "$shareware" = 1 ]; then
    rb put-macbinary "$flat" "$sw/DOOM1.WAD.bin"
fi
if [ -n "${DOOM_ARGS:-}" ]; then
    printf "%s\n" "$DOOM_ARGS" > "$work/DOOM Args"
    rb put "$flat" "$work/DOOM Args" "/DOOM Args"
    rb chmeta "$flat" "/DOOM Args" --type TEXT --creator ttxt
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
