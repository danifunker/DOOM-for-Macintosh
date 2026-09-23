#!/usr/bin/env python3
"""Rewrite the file name stored in a MacBinary II header (and its CRC).

usage: mbrename.py IN.bin OUT.bin "New Name"
"""
import struct, sys

from rsrcfilter import crc16_xmodem

src, dst, name = sys.argv[1], sys.argv[2], sys.argv[3].encode('mac_roman')[:63]
d = bytearray(open(src, 'rb').read())
d[1] = len(name)
d[2:65] = name + bytes(63 - len(name))
d[124:126] = struct.pack('>H', crc16_xmodem(bytes(d[:124])))
open(dst, 'wb').write(d)
