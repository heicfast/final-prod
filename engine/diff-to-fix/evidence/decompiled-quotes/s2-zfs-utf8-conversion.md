# s2-zfs-utf8-conversion — ZfsSearchDLL's CP_UTF8 name conversion (verbatim quote)

**Session 2 · corpus `03ad46c` · `re-workspace/reversed/ghidra-searchdll/ZfsSearchDLL_decompiled.c`**

Import census (this session, exact):

```
EXT4SearchDLL_imports.txt:  MultiByteToWideChar / WideCharToMultiByte -> 0 hits
BtrfsSearchDLL_imports.txt: MultiByteToWideChar / WideCharToMultiByte -> 0 hits
ZfsSearchDLL_imports.txt:   MultiByteToWideChar / WideCharToMultiByte -> 2 hits
  KERNEL32.DLL::WideCharToMultiByte
  KERNEL32.DLL::MultiByteToWideChar
NTFSSearchDLL_imports.txt (control): 2 hits (both conversion APIs, as expected for an engine with UTF-16 names)
```

## FUN_18001c630 (:24895-25030) — the in-engine CP_UTF8 conversion (:24985-25014)

Context: a virtual call `(**(code **)(*param_1 + 0x70))(param_1, &local_a8)` obtains a
string object (source at `local_a8/local_a0`, with length and SSO-style storage
`if (0xf < *(longlong *)(puVar7 + 6)) puVar12 = *(uint **)puVar7;`), then:

```c
      iVar6 = MultiByteToWideChar(0xfde9,0,(LPCSTR)puVar12,(int)*(longlong *)(puVar7 + 4),
                                  (LPWSTR)0x0,0);            /* 0xfde9 = 65001 = CP_UTF8; length probe */
      uVar13 = (ulonglong)iVar6;
      local_88 = 0;
      uStack_80 = 7;
      local_98 = L'\0';
      if (uVar13 < 8) {
        ... small-string path ...
      }
      else {
        FUN_180021760((longlong *)&local_98,uVar13);         /* wstring reserve */
      }
      ...
      MultiByteToWideChar(0xfde9,0,(LPCSTR)puVar7,(int)*(undefined8 *)puVar12,pWVar8,iVar6);
```

`dwFlags = 0` — invalid input is not rejected; Windows substitutes U+FFFD per maximal
ill-formed subpart. The reverse-direction `WideCharToMultiByte(0, 0, ...)` at `:1518/:1525`
uses code page 0 (CP_ACP) and belongs to a different (logging) context.

## Interpretation (N-010 evidence)

- The og's ZFS engine actively decodes UTF-8 names on at least one path — unlike its
  ext4/Btrfs engines, which import no conversion API at all (raw byte pass-through to the
  UTF-8 .rsf).
- FUN_18001c630 takes `this` + a virtual-call-derived string and produces a wstring via
  CP_UTF8 — a display/name getter. It has no direct callers in the decompile
  (vtable-dispatched); **which** name it serves (pool display name / dataset name / dnode
  file name) is the open question tracked in `tracked-open/` (Z-1).
