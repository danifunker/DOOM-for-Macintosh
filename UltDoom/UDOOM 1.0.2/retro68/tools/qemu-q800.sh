#!/usr/bin/env bash
# Boot QEMU's Quadra 800 with a Mac OS system disk plus the DOOM disk.
#
#   ROM=path/to/F1ACAD13.rom SYSDISK=path/to/system.hda tools/qemu-q800.sh [DOOM.hda] [qemu args...]
#
# ROM      Quadra 800 ROM (first bytes F1 AC AD 13), e.g. from MAME's macqd800.zip
# SYSDISK  bootable Mac OS 7.1-8.1 SCSI disk image (APM); used read-write
# PRAM     PRAM file (default build/pram.img).  Turn on 32-bit addressing in the
#          Memory control panel once and it is remembered here; without it the
#          game only gets the 8 MB below the 24-bit limit.
# MEM      RAM in MB (default 128)
#
# For deterministic benchmark timing add:  -icount shift=5,align=off
# (the guest clock then advances ~31M instructions per second).
set -euo pipefail
here=$(cd "$(dirname "$0")/.." && pwd)
doom=${1:-$here/build/DOOM.hda}
shift || true
: "${ROM:?set ROM to the Quadra 800 ROM image}"
: "${SYSDISK:?set SYSDISK to a bootable Mac OS disk image}"
pram=${PRAM:-$here/build/pram.img}
[ -f "$pram" ] || dd if=/dev/zero of="$pram" bs=256 count=1 2>/dev/null

exec qemu-system-m68k -M q800 -m "${MEM:-128}" -bios "$ROM" -g 640x480x8 \
    -drive file="$pram",format=raw,if=mtd \
    -device scsi-hd,scsi-id=0,drive=hd0 \
    -drive file="$SYSDISK",media=disk,format=raw,if=none,id=hd0 \
    -device scsi-hd,scsi-id=1,drive=hd1 \
    -drive file="$doom",media=disk,format=raw,if=none,id=hd1 \
    "$@"
