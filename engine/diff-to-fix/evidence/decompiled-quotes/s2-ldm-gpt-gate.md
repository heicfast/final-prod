# s2-ldm-gpt-gate — the og's GPT-LDM discovery path (decompiled quotes)

**Session 2 (T7 deep pass) · corpus `03ad46c`** — all quotes verbatim from
`re-workspace/reversed/ghidra-mo/`. These settle session-1 OQ-T7's environment
question and the "how does the og find LDM on GPT" question raised by session-2's
A-001 re-verification.

## 1. GPT-dynamic gate + PRIVHEAD-candidate selection — LdmManager FUN_1800724f0 (:59393-59472)

The og decides a GPT disk is dynamic by scanning GPT entries for ONE GUID
(DAT_1800c4960 = real `5808c8aa-7e8f-42e0-85d2-e1e90434cfb3`, extracted from the
binary — see `../executions/s2-ldm-guid-constants.md`). On match: GPT mode + the
PRIVHEAD candidate = the partition's LAST LBA (entry+0x28 — the documented
backup-PRIVHEAD position at the metadata-partition tail; same anchor the core's
`find_privhead` uses, `cr_ldm.cpp:1020-1024`). No header/entry CRC checks on this
path (matches the core, which also skips CRC checks here).

```c
        /* GPT header validation: signature, revision 0x10000, size 0x5C, reserved 0 */
        if ((((*plVar1 == 0x5452415020494645) && ((int)plVar1[1] == 0x10000)) &&
            (*(int *)((longlong)plVar1 + 0xc) == 0x5c)) && (*(int *)((longlong)plVar1 + 0x14) == 0)) {
          uVar3 = *(int *)((longlong)plVar1 + 0x54) * (int)plVar1[10];   /* entries*entry_size */
          _Memory = operator_new((ulonglong)uVar3);
          ...
          if (uVar3 == uVar4) {                                          /* read entry array */
            local_30 = _Memory;
            for (local_44 = 0; local_44 < *(uint *)(plVar1 + 10); local_44 = local_44 + 1) {
              iVar2 = memcmp(local_30,&DAT_1800c4960,0x10);              /* real LDM metadata GUID */
              if (iVar2 == 0) {
                *(undefined4 *)(param_1 + 0x100) = 1;                    /* GPT mode */
                *(undefined8 *)(param_1 + 0x68) = *(undefined8 *)((longlong)local_30 + 0x28);  /* last LBA */
                break;
              }
              local_30 = (void *)((longlong)local_30 + 0x80);            /* next 128-byte entry */
            }
            free(_Memory);
            uVar5 = (ulonglong)(*(int *)(param_1 + 0x100) == 1);
```

Differences vs the core's `find_privhead` (`cr_ldm.cpp:1005-1027`): the og also
checks revision==0x10000 and reserved==0; the core checks entry_size==128 and
caps entry_count at 1024 (the og trusts the header's count, bounded only by its
allocation). All informational-level (both accept all real disks).

## 2. Disk-type classification — LdmManager FUN_180077ef0 (:61431-61534)

MBR walk: type 0x42 → MBR-dynamic (candidate sector defaults to 6); 0xEE → the
GPT path above; 0x05/0x0F/0x85 → extended-chain walk. Matches the core's
`find_privhead` MBR logic (`cr_ldm.cpp:988-997`).

```c
          for (local_40 = 0; local_40 < 4; local_40 = local_40 + 1) {
            cVar1 = *(char *)(local_30 + 4);
            if ((cVar1 == '\x05') || (cVar1 == '\x0f')) {
LAB_18007812c:
              local_38 = local_28 + (ulonglong)*(uint *)(local_30 + 8);
              ...
            }
            else if (cVar1 == 'B') {          /* 0x42 = MBR dynamic disk */
              local_48 = 1;
            }
            else {
              if (cVar1 == -0x7b) goto LAB_18007812c;    /* 0x85 extended */
              if (cVar1 == -0x12) {                       /* 0xEE GPT protective */
                uVar6 = FUN_1800724f0(param_1);
                local_48 = (byte)uVar6;
              }
            }
```

## 3. Partition.mo's GPT-entry LDM scan — FUN_180002bd0 (:1290-1340)

Partition.mo (the table layer) scans the GPT entry array against BOTH LDM GUIDs
(its own data copies, DAT_180036630/0x36640 — the same real values) and requires
BOTH a metadata and a data entry to call the disk GPT-dynamic:

```c
        for (local_1074 = 0; local_1074 < *(uint *)(local_1080 + 10); local_1074 = local_1074 + 1) {
          iVar1 = memcmp(local_1058,&DAT_180036640,0x10);   /* real LDM metadata GUID */
          if (iVar1 == 0) {
            local_1088 = '\x01';
          }
          iVar1 = memcmp(local_1058,&DAT_180036630,0x10);   /* real LDM data GUID */
          if (iVar1 == 0) {
            local_1087 = '\x01';
          }
          local_1058 = (void *)((longlong)local_1058 + 0x80);
        }
        ...
        if ((local_1088 == '\0') || (local_1087 == '\0')) {
          local_1060 = 0;                                     /* not GPT-dynamic */
        }
        else {
          local_1060 = 1;                                     /* GPT-dynamic */
        }
```

Display labels for the two entries live in Partition.mo's strings ("LDM Metadata
Partition", "LDM Data Partition") — the core classifies metadata as `Reserved`
and data as its LDM kind; presentation nuance recorded in `modules/ldm.md`.

## 4. PRIVHEAD validation — LdmManager FUN_180072f40 ("LdmDiskAnalyser::ValidatePrivheads", :59671-59783)

Validates magic (`PRIVHEAD`, both the 8-byte and 7-byte-prefix forms accepted),
version 2.11/2.12 (fields at +0xC==2, +0xE in {0x0B, 0x0C}), config size
normally 0x800 (warns otherwise), GUID, disk-size consistency (skipped in GPT
mode via the +0x100 flag), and the database extent fields:

```c
      plVar1 = *(longlong **)(param_1 + 0x60);
      if ((*plVar1 == 0x5052495648454144) || (*plVar1 == 0x52495648454144)) {
        FUN_18008b350((longlong)plVar1 + 0xc,2);
        FUN_18008b350((longlong)plVar1 + 0xe,2);
        if ((*(short *)((longlong)plVar1 + 0xc) == 2) &&
           ((*(short *)((longlong)plVar1 + 0xe) == 0xb ||
            (*(short *)((longlong)plVar1 + 0xe) == 0xc)))) {
          ...
          if (*(longlong *)((longlong)plVar1 + 0x133) != 0x800) {
            ... "disk%d Database is normally %u bytes, it claims to be %I64d bytes."
          }
```

The core's `parse_privhead` accepts version 2.x per its header
(`cr_table_wire.h` PRIVHEAD block) — the og accepts 2.11 and 2.12 only at this
site; nuance recorded in `modules/ldm.md` (both values cover all real disks;
informational).

## 5. RAID-5 — see OQ-T7 resolution in `tracked-open/` (left-symmetric, R=0, four dormant rotations)
