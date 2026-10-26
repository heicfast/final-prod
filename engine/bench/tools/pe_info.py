#!/usr/bin/env python3
"""pe_info.py - PE provenance dump (stdlib only): machine, timestamps, OS/subsystem
version, section list, imported DLLs + import counts. Used to record driver/binary
provenance in the benchmark evidence.

Usage: python pe_info.py <file.exe|file.sys|file.dll> [more files...]

Licensed under the MIT license. See LICENSE.
"""

import json
import struct
import sys


def rv_to_off(section, rva):
    if section['va'] <= rva < section['va'] + section['vsize']:
        return section['raw'] + (rva - section['va'])
    return None


def parse(path):
    out = {'path': path}
    with open(path, 'rb') as f:
        data = f.read()
    if data[:2] != b'MZ':
        out['error'] = 'not an MZ PE'
        return out
    e_lfanew = struct.unpack_from('<I', data, 0x3C)[0]
    if data[e_lfanew:e_lfanew + 4] != b'PE\x00\x00':
        out['error'] = 'bad PE signature'
        return out
    coff = e_lfanew + 4
    machine, nsec, timedate, ptr_sym, nsym, opt_size, chars = struct.unpack_from('<HHIIIHH', data, coff)
    out['machine'] = hex(machine)
    out['timestamp_utc'] = timedate
    out['characteristics'] = hex(chars)
    opt = coff + 20
    magic = struct.unpack_from('<H', data, opt)[0]
    if magic == 0x20b:
        fmt = '<HHIILLQQHHHHHHIIQQQQIIQQ'
    else:
        fmt = '<HHIILLQQHHHHHHIIIIIIII'
    fields = struct.unpack_from(fmt, data, opt)
    # offsets within the optional header differ; pull the fixed ones by known layout
    major_link, minor_link = fields[1], fields[2]
    size_code, size_init, size_uninit = fields[4], fields[5], fields[6]
    entry = fields[8]
    if magic == 0x20b:
        base = opt + 20 + 68   # PE32+ data dirs start
        n_dd = (struct.unpack_from('<I', data, opt + 108)[0])
        os_maj, os_min = struct.unpack_from('<HH', data, opt + 40)[0:2]
        sub_maj, sub_min = struct.unpack_from('<HH', data, opt + 48)[0:2]
        subsystem = struct.unpack_from('<H', data, opt + 68)[0]
        dd_off = opt + 112
    else:
        os_maj, os_min = struct.unpack_from('<HH', data, opt + 40)[0:2]
        sub_maj, sub_min = struct.unpack_from('<HH', data, opt + 48)[0:2]
        subsystem = struct.unpack_from('<H', data, opt + 68)[0]
        dd_off = opt + 96
    out['linker_version'] = f'{major_link}.{minor_link}'
    out['os_version'] = f'{os_maj}.{os_min}'
    out['subsystem_version'] = f'{sub_maj}.{sub_min}'
    out['subsystem'] = subsystem
    out['entry_rva'] = hex(entry)
    sections = []
    sec_off = opt + opt_size
    for i in range(nsec):
        name = data[sec_off:sec_off + 8].rstrip(b'\x00').decode('ascii', 'replace')
        vsize, va, rsize, raw = struct.unpack_from('<IIII', data, sec_off + 8)
        sections.append({'name': name, 'va': va, 'vsize': vsize, 'raw': raw, 'rsize': rsize})
        sec_off += 40
    out['sections'] = [s['name'] for s in sections]
    # import table = data directory index 1
    if magic == 0x20b:
        imp_rva, imp_sz = struct.unpack_from('<II', data, opt + 112 + 8)
    else:
        imp_rva, imp_sz = struct.unpack_from('<II', data, opt + 96 + 8)
    imports = {}
    if imp_rva:
        # find the section containing the import dir
        sec = next((s for s in sections if s['va'] <= imp_rva < s['va'] + max(s['vsize'], s['rsize'])), None)
        if sec:
            off = rv_to_off(sec, imp_rva)
            while off and off + 20 <= len(data):
                oft, ts, fc, name_rva, iat = struct.unpack_from('<IIIII', data, off)
                if name_rva == 0:
                    break
                noff = rv_to_off(sec, name_rva)
                if noff is None:
                    break
                end = data.index(b'\x00', noff)
                dll = data[noff:end].decode('ascii', 'replace')
                # count thunk entries via the OriginalFirstThunk
                funcs = 0
                if oft:
                    toff = rv_to_off(sec, oft)
                    if toff is not None:
                        step = 8 if magic == 0x20b else 4
                        p = toff
                        while p + step <= len(data):
                            v = struct.unpack_from('<Q' if magic == 0x20b else '<I', data, p)[0]
                            if v == 0:
                                break
                            funcs += 1
                            p += step
                imports[dll] = funcs
                off += 20
    out['imports'] = imports
    out['import_dll_count'] = len(imports)
    out['total_import_functions'] = sum(imports.values())
    return out


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    results = [parse(p) for p in sys.argv[1:]]
    print(json.dumps(results, indent=1))
    return 0


if __name__ == '__main__':
    sys.exit(main())
