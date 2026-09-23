#!/usr/bin/env python3
"""Copy a raw Macintosh resource fork, dropping the listed resource types.

Used to lift the icons, dialogs, menus, splash PICTs etc. out of the shipped
DOOM application while discarding its compiled code (CODE/DATA/cfrg), which
the Retro68 build replaces.

usage: rsrcfilter.py IN.rsrc OUT [--drop TYPE ...] [--list]

IN is a raw resource fork.  OUT is written as MacBinary II when it ends in
.bin (the form Retro68's Rez accepts as input), otherwise as a raw fork.
"""
import struct, sys

def read_fork(data):
    doff, moff, dlen, mlen = struct.unpack('>IIII', data[:16])
    m = data[moff:moff + mlen]
    tlo, nlo = struct.unpack('>HH', m[24:28])
    ntypes = struct.unpack('>h', m[tlo:tlo + 2])[0] + 1
    res = []
    for i in range(ntypes):
        rtype, n, refoff = struct.unpack('>4sHH', m[tlo + 2 + i * 8:tlo + 10 + i * 8])
        for j in range(n + 1):
            e = tlo + refoff + j * 12
            rid, noff, attr_off = struct.unpack('>hhI', m[e:e + 8])
            attrs, off = attr_off >> 24, attr_off & 0xFFFFFF
            ln = struct.unpack('>I', data[doff + off:doff + off + 4])[0]
            body = data[doff + off + 4:doff + off + 4 + ln]
            name = None
            if noff != -1:
                l = m[nlo + noff]
                name = m[nlo + noff + 1:nlo + noff + 1 + l]
            res.append((rtype, rid, attrs, name, body))
    return res

def write_fork(res):
    types = []
    for r in res:
        if r[0] not in types:
            types.append(r[0])
    data = b''
    names = b''
    reflists = []
    for t in types:
        refs = b''
        for rtype, rid, attrs, name, body in res:
            if rtype != t:
                continue
            noff = -1
            if name is not None:
                noff = len(names)
                names += bytes([len(name)]) + name
            refs += struct.pack('>hhI', rid, noff, (attrs << 24) | len(data)) + b'\0\0\0\0'
            data += struct.pack('>I', len(body)) + body
        reflists.append(refs)
    typelist_len = 2 + 8 * len(types)
    typelist = struct.pack('>h', len(types) - 1)
    off = typelist_len
    for t, refs in zip(types, reflists):
        typelist += struct.pack('>4sHH', t, len(refs) // 12 - 1, off)
        off += len(refs)
    reflist_blob = b''.join(reflists)
    map_hdr_len = 28
    tlo = map_hdr_len
    nlo = tlo + len(typelist) + len(reflist_blob)
    rmap = bytes(16) + bytes(4) + bytes(2) + struct.pack('>HH', 0, 0)[:2] + struct.pack('>HH', tlo, nlo)
    rmap = rmap[:24] + struct.pack('>HH', tlo, nlo)
    rmap += typelist + reflist_blob + names
    doff = 256
    moff = doff + len(data)
    hdr = struct.pack('>IIII', doff, moff, len(data), len(rmap))
    rmap = hdr + rmap[16:]
    return hdr + bytes(doff - 16) + data + rmap

def crc16_xmodem(data):
    crc = 0
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) if crc & 0x8000 else crc << 1
            crc &= 0xFFFF
    return crc

def macbinary(name, rsrc, ftype=b'rsrc', creator=b'RSED'):
    hdr = bytearray(128)
    name = name.encode('mac_roman')[:63]
    hdr[1] = len(name)
    hdr[2:2 + len(name)] = name
    hdr[65:69] = ftype
    hdr[69:73] = creator
    hdr[87:91] = struct.pack('>I', len(rsrc))
    hdr[122] = hdr[123] = 129
    hdr[124:126] = struct.pack('>H', crc16_xmodem(bytes(hdr[:124])))
    return bytes(hdr) + rsrc + bytes(-len(rsrc) % 128)

def main():
    args = sys.argv[1:]
    src, dst = args[0], args[1]
    drop = set()
    if '--drop' in args:
        drop = {t.ljust(4).encode('mac_roman') for t in args[args.index('--drop') + 1:] if not t.startswith('--')}
    res = read_fork(open(src, 'rb').read())
    kept = [r for r in res if r[0] not in drop]
    if '--list' in args:
        for r in kept:
            print(r[0].decode('mac_roman'), r[1], len(r[4]), (r[3] or b'').decode('mac_roman'))
    fork = write_fork(kept)
    if dst.endswith('.bin'):
        import os
        fork = macbinary(os.path.basename(dst)[:-4], fork)
    open(dst, 'wb').write(fork)

if __name__ == '__main__':
    main()
