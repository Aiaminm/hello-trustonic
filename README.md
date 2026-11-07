# hello-trustonic

A hello world aarch64 MCLF trustlet "from scratch" (no trusted-world SDK, based on reverse engineering existing binaries).

Self-signs with throwaway keys, which means you can only load it on a TEE with signature checking patched out - setting this up is left as an exercise for the reader ;)

This is a vibecoded PoC, tested on a pwned Nothing Phone 2a.

```sh
$ make test
[...]
session opened: id=4353
ECHO: status=15  result="HELLO TRUSTZONE"
ADD:  status=0  40+2=42
INFO: status=0  magic=0x6d79746c  ta_tci_len=4096
DEVID: status=0x00000000  gpd.tee.deviceID=00000009-1229-0000-xxxx-xxxxxxxxxxxx
```

### Assorted links:

- https://blog.quarkslab.com/a-deep-dive-into-samsungs-trustzone-part-1.html
- https://github.com/radareorg/radare2/blob/master/libr/bin/p/bin_mclf.c
- https://github.com/kgp700/exyroid-sgs2-ics/tree/master/drivers/gud/include/TlApi
- https://github.com/ghassani/mclf-ghidra-loader
- https://github.com/NeatMonster/mclf-ghidra-loader
