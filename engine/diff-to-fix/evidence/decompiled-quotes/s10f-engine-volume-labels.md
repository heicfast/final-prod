# Evidence — S10-F: the og's engine-side volume-label decode vs the core's absent surface

> Session-10 S10-F lane. Corpus `be0cfe9`; core product-code tree `44f043a5`.
> Lead first-hand reads; the NTFS decode chain and the ext4 decode were both
> re-read at the decompile/artifact sites before filing.

## 1. The og reads the volume label engine-side (both engines verified)

### 1.1 NTFS — the $Volume $VOLUME_NAME extraction

`NTFSSearchDLL_decompiled.c:647-676` (inside the volume-init walk of the
quick-search controller; the volume node emission at :675-743 builds the
0x290 "NTFS" node with name ≤ 64):

```c
if (*(longlong *)(param_1 + 0x450) == 0) {
    ...
    pcVar2 = *(code **)(*param_2 + 8);
    iVar7 = (**(code **)(*param_2 + 0x28))(param_2);
    iVar7 = (*pcVar2)(param_2,local_1048,local_1210 + 6, ...);   /* read $Volume */
    if ((iVar7 != 0) &&
        (FUN_18003f3a0(local_1048,local_1148,iVar6), pcVar15 = (char *)0x0,
         local_1148[0] != '\0')) {
        pcVar15 = local_1148;                                     /* the label */
    }
}
else {
    pcVar15 = (char *)(param_1 + 0x440);                          /* stored label */
    ...
}
```

`FUN_18003f3a0` (:50963-50980) is the $VOLUME_NAME extractor — attribute
lookup by type, then the value's wide→narrow conversion:

```c
void FUN_18003f3a0(int *param_1,LPSTR param_2,int param_3)
{
  ...
  lVar4 = FUN_180041850((longlong)param_1,0x60,0);       /* $VOLUME_NAME = 0x60 */
  ...
  local_18[0] = 0x50;
  FUN_18002cc30((LPCWSTR)(lVar4 + 0x18),(ulonglong)uVar2 / 2,param_2,local_18);
}
```

So the NTFS engine extracts `$Volume`'s `$VOLUME_NAME` (type 0x60)
attribute, converts it, and feeds it to the volume node's name field (the
`:650-664` read + the `:675-743` emission).

### 1.2 ext4 — the s_volume_name strncpy (corpus-artifact documented)

`re-workspace/artifacts/SearchDLL/ext4-hfs-engine-algorithms-v2.md` §2
step 5 (line 45):

> 5. `strncpy(label, s_volume_name, 0xF)` — volume label for the UI.

(The Ext4Volume ctor at `EXT4SearchDLL_decompiled.c` FUN_1800035b0
:2385-2495 carries the label field; the artifact is the documented decode.)

### 1.3 The FAT family and HFS+

The boot-sector 11-byte label (FAT12/16/32 +0x2B / exFAT +0x80) and the
HFS+ volume name (MDB/HFS+VH key -2 catalog record) are the analogous
engine-side label sources; the engines' deep-dive artifacts document the
volume-node emission with label for the family (e.g.
`fat-engine--deep-dive.md` volume-node walk). This finding's og-side claim
rests on the two instruction-level decodes above (NTFS) + the artifact
decode (ext4); the FAT/HFS+ halves are the same surface class and are
named in the fix plan as the completion set.

## 2. The core NEVER decodes an engine-side volume label (first-hand census at `44f043a5`)

```
$ grep -rn "volume_label" src/ include/ | grep -v "fat\|exfat\|ntfs\|hfs\|btrfs\|refs\|zfs"
  src/core/orchestrator/cr_scan.cpp:1933   (copies cfg_.volume_label — caller-supplied)
  src/ui/cr_ui.cpp:1181                    (ABI card copy from in->volume_label_utf8)
  src/ui/cr_ui.cpp:2026 + src/app/rrecov.cpp:1714  (the two auto-save naming reads)
  include/cr/orchestrator/cr_scan.h:239    (char volume_label[32] — the config decl)
$ grep -rn "volume_name" src/core/ include/cr/ | grep -v s_volume
  src/core/ext4/cr_ext4_engine.cpp:200-201  (the decode — memcpy into out.volume_name)
  include/cr/ext4/cr_ext4_engine.h:79       (u8 volume_name[17] — the struct field)
```

- **ext4**: the ONLY engine that decodes its on-disk label
  (`std::memcpy(out.volume_name, sb + SB_VOLUME_NAME, 16)` at
  `cr_ext4_engine.cpp:200-201`) — and the field has **zero consumers**
  (the two grep hits are the decode and the declaration; nothing reads
  `volume_name` off the Volume struct).
- **NTFS**: no `$Volume` label read exists (the `$Volume` hits in
  `cr_mftwalk.cpp:218/:247` are the metafile filter name lists only).
- **FAT/exFAT**: no boot-sector label decode at all.
- `fill_identity_card` (`src/ui/cr_ui.cpp:1850-1866`) fills model / serial /
  firmware / wwn / interface — **not the label** (M-079's gap, current-line
  verified).

## 3. The M-079 premise correction

M-079's fix plan step 1 says: "Image targets: derive from the image
filename (**the og has no image-scan label semantics** — document the
choice)." That parenthetical is factually wrong: the og's engines decode
the label from the volume itself (§1 above), which works identically for
image scans — the engine-side decode IS the og's image-scan label
semantics. The addendum in M-079 corrects this and points the fill-site
fix at the engine decode as the image-scan source.
