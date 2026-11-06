#!/usr/bin/env python3
"""elf2mclf.py — convert a linked AArch64 ELF into a Kinibi/MobiCore MCLF (.tlbin).

Produces (see notes/trustlet-abi.md):
  file = [ text ][ data ][ trailer ]
  text[0x00:0x80]  mclfHeaderV2 + permittedSuid
  text[0x80:0xa8]  mclfTextHeader (mcLibEntry@0x8c and mcLibBase@0x98 left 0 = loader fills)
  text[0x100:..]   code+rodata (linked at VA 0x1100)
  data             initialized data, mapped at a page-aligned dataVa
  trailer          copied verbatim from a real TA: the Trustonic key-blob makes G2
                   (signer hash-pin) pass for free; the signature is ignored because
                   the Kinibi G3/PSS verdict is patched (see build_metapatch.py).
"""
import argparse, struct, hashlib
from elftools.elf.elffile import ELFFile

TEXT_VA = 0x1000
CODE_VA = 0x1100          # [0x1000,0x1100) reserved for the two headers

def sym(elf, name):
    st = elf.get_section_by_name('.symtab')
    for s in st.iter_symbols():
        if s.name == name:
            return s['st_value']
    raise SystemExit(f"symbol {name} not found")

def alloc_progbits(elf):
    out = []
    for s in elf.iter_sections():
        if (s['sh_flags'] & 0x2) and s['sh_type'] == 'SHT_PROGBITS' and s['sh_size']:
            out.append((s['sh_addr'], s.data()))
    return out

def place(blob, base, secs, lo, hi):
    for addr, d in secs:
        if lo <= addr < hi:
            off = addr - base
            if off + len(d) > len(blob):
                blob.extend(b'\0' * (off + len(d) - len(blob)))
            blob[off:off+len(d)] = d

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('elf')
    ap.add_argument('-o', '--out', required=True)
    ap.add_argument('--donor', required=True, help='real .tlbin whose trailer to copy')
    ap.add_argument('--uuid', default='0a0a0a0a000000000000000000000001')
    ap.add_argument('--service-type', type=lambda x: int(x, 0), default=3)
    ap.add_argument('--flags',        type=lambda x: int(x, 0), default=0x28)  # bit3 EXTENDED_LAYOUT + bit5 (validator requires bit5 set, else a global check)
    ap.add_argument('--num-instances',type=lambda x: int(x, 0), default=1)
    ap.add_argument('--num-threads',  type=lambda x: int(x, 0), default=1)
    ap.add_argument('--heap-init',    type=lambda x: int(x, 0), default=0x8000)
    ap.add_argument('--heap-max',     type=lambda x: int(x, 0), default=0x20000)
    a = ap.parse_args()

    elf = ELFFile(open(a.elf, 'rb'))
    entry      = sym(elf, '_start')
    data_start = sym(elf, '__data_start')
    data_end   = sym(elf, '__data_end')
    bss_end    = sym(elf, '__bss_end')
    secs = alloc_progbits(elf)

    # text image: 0x100 header region + code/rodata at (VA - 0x1000)
    text = bytearray()
    place(text, TEXT_VA, secs, CODE_VA, data_start)
    if len(text) < 0x100:
        text.extend(b'\0' * (0x100 - len(text)))
    while len(text) % 8:
        text.append(0)
    text_len = len(text)

    # data image
    data = bytearray()
    place(data, data_start, secs, data_start, data_end)
    while len(data) % 8:
        data.append(0)
    data_len = len(data)

    bss_len = bss_end - data_end
    if bss_len < 8:
        raise SystemExit("bss too small for the 8-byte TCI descriptor")

    uuid = bytes.fromhex(a.uuid)
    if len(uuid) != 16:
        raise SystemExit("uuid must be 16 bytes (32 hex chars)")

    # --- mclfHeaderV2 @ 0x00 ---
    struct.pack_into('<4sII', text, 0x00, b'MCLF', 0x00020005, a.flags)
    struct.pack_into('<II',   text, 0x0c, 2, a.service_type)        # memType=EXTERNAL, serviceType
    struct.pack_into('<I',    text, 0x14, a.num_instances)
    text[0x18:0x28] = uuid
    struct.pack_into('<I',    text, 0x28, 0)                        # driverId
    struct.pack_into('<I',    text, 0x2c, a.num_threads)
    struct.pack_into('<II',   text, 0x30, TEXT_VA, text_len)        # text segment
    struct.pack_into('<II',   text, 0x38, data_start, data_len)     # data segment
    struct.pack_into('<I',    text, 0x40, bss_len)
    struct.pack_into('<I',    text, 0x44, entry)
    struct.pack_into('<I',    text, 0x48, 0)                        # serviceVersion
    text[0x4c:0x54] = b'\xff' * 8                                   # permittedSuid = any

    # --- mclfTextHeader @ 0x80 (VA 0x1080) ---
    struct.pack_into('<III', text, 0x80, 0x00020005, 0x3c, 0)      # version, textHeaderLen, requiredFeat
    struct.pack_into('<I',   text, 0x8c, 0)                        # mcLibEntry  <- loader fills
    struct.pack_into('<II',  text, 0x90, a.heap_init, a.heap_max)  # heapSize {init,max}
    struct.pack_into('<I',   text, 0x98, 0)                        # mcLibBase   <- loader fills
    struct.pack_into('<II',  text, 0x9c, 0x00010015, 0x00010005)   # tlApiVers, drApiVers
    struct.pack_into('<I',   text, 0xa4, 0)                        # ta_properties

    # --- trailer (fakesign) copied from the donor ---
    dn = open(a.donor, 'rb').read()
    if dn[:4] != b'MCLF':
        raise SystemExit("donor is not an MCLF")
    d_tlen = struct.unpack_from('<I', dn, 0x34)[0]
    d_dlen = struct.unpack_from('<I', dn, 0x3c)[0]
    trailer = dn[d_tlen + d_dlen:]
    if not trailer:
        raise SystemExit("donor has no trailer")

    out = bytes(text) + bytes(data) + trailer
    open(a.out, 'wb').write(out)

    print(f"wrote {a.out}  size=0x{len(out):x}  sha256={hashlib.sha256(out).hexdigest()[:16]}")
    print(f"  entry={entry:#06x}  text=0x{text_len:x}  data@{data_start:#x}=0x{data_len:x}  "
          f"bss=0x{bss_len:x} (end {bss_end:#x})")
    print(f"  tci_desc @ {bss_end-8:#x}   uuid={a.uuid}  svcType={a.service_type}  flags={a.flags:#x}")
    print(f"  trailer=0x{len(trailer):x}B from {a.donor}")

if __name__ == '__main__':
    main()
