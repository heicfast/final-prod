# Evidence — session 6: writer collision naming + mkdir acceptance gates (DRWUI decompile, first-hand)

**Session:** 6 deep-continuation (2026-10-26) · **Audited core commit:** `022c81d`-lineage · **Corpus:** `03ad46c`
**Cited by:** N-065 (and the D-19 refutation in N-058). Quotes verbatim from `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c`.

## 1. Collision-naming loop — `%s_%d` forms, 10,000-attempt cap

`DRWUI_decompiled.c:357112-357132` (the conflict loop; first-cited this session):

```c
    while( true ) {
      FUN_14051f200(local_1168);
      iVar2 = FUN_14051f4e0(local_1168,local_1058,0);
      if (iVar2 == 0) break;
      lVar6 = FUN_1400a6200(local_1270);
      if (lVar6 == 0) {
        local_12f4 = local_1308;
        local_1308 = local_1308 + 1;
        uVar3 = FUN_14006d520(local_1250);
        local_1318 = (wchar_t *)CONCAT44(local_1318._4_4_,local_12f4);
        FID_conflict_sprintf_s(local_1058,0x410,L"%s_%d",uVar3);
      }
      else {
        local_12a8 = FUN_14006d520(local_1270);
        local_12f8 = local_1308;
        local_1308 = local_1308 + 1;
        local_12a0 = FUN_14006d520(local_1250);
        local_1310 = local_12a8;
        local_1318 = (wchar_t *)CONCAT44(local_1318._4_4_,local_12f8);
        FID_conflict_sprintf_s(local_1058,0x410,L"%s_%d%s",local_12a0);
      }
      if (10000 < local_1308) {
        FUN_14051f410(local_1168);
        goto LAB_1401f02f4;
      }
      FUN_14051f410(local_1168);
    }
```

Reading: on conflict, candidates are formatted `name_N` (no-extension branch) or `name_N.ext`-shaped (`%s_%d%s`: name, counter, extension) into a 0x410-byte buffer; the counter increments per attempt; after the counter passes **10,000** the loop exits to the failure path (the file is failed, no fallback suffix).

## 2. mkdir acceptance — SHCreateDirectoryExW, 0 or 0xB7 only

`DRWUI_decompiled.c:870435-870443`:

```c
    ppppWVar4 = &local_60;
    if (7 < uStack_48) {
      ppppWVar4 = (LPCWSTR ***)local_60;
    }
    iVar3 = SHCreateDirectoryExW((HWND)0x0,(LPCWSTR)pppppWVar4,(SECURITY_ATTRIBUTES *)0x0);
    uVar1 = local_50;
    if ((iVar3 == 0) || (iVar3 == 0xb7)) {
```

Reading: the directory creation result is accepted only on `0` (success) or `0xB7` (ERROR_ALREADY_EXISTS); every other code takes the failure path — regardless of whether the path exists as a directory.

## 3. Core side (verification snapshot, audited commit)

- `src/core/recovery/cr_safewriter.cpp:65-70` — `snprintf(buf, sizeof(buf), " (%u)", attempt + 1);` → `name (2).ext` form, inserted before the last dot (:72-77); loop bounded by `cfg_.rename_attempts` (`cr_safewriter.cpp:53`); default 64 (`include/cr/recovery/cr_safewriter.h:92`); exhaustion fallback `base_name + u"_dup"` (`cr_safewriter.cpp:78`).
- `src/win/cr_destio.cpp:111-116` — mkdir: exist-as-dir beats any error.

## 4. D-19 refutation

`docs/PARITY-DELTAS.md:424-428` currently states collision handling is "not evidenced as rename-policy (corpus architecture/04-recovery-workflow.md §7.5; no collision policy in any artifact — AUDIT-UI §5.7 calls it 'unevidenced in og')". The quotes above evidence the og's exact collision policy — D-19's premise is refuted; the fix (N-065 sub-item 3) updates the delta text. Our atomic temp+rename + SHA-256 verify (D-19's other halves) remain deliberate supersets, unaffected.
