# Evidence — N-142: the .rsf variant-save device-descriptor record (the 5th GUID)

Session-12 continuation lane, 2026-10-29. Corpus `be0cfe9`, core `c7fa527c`.
Quotes read first-hand this session.

## 1. The corpus artifact — `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md`

The record-type table (§2, :70-75):

```
| 3 — partition context | DAT_1414d47c8 | 16f4f594 17436745 ae4415b3 21ed998 | ✓ |
| 4 — result tree       | DAT_140f64c80 | 6cf208d2 8c07344e 9d71d91f 535f7b57 | ✓ |

(A 5th constant `DAT_1414d47a0` = `76a672b048058b46ba9e91093a63b517` drives a record type
written by FUN_1403f1180 = `[552-byte struct] + [5400-byte blob]` — used by a variant save
path, not present in the shipped auto-save samples.)
```

The 5400-byte volumeDevice blob's observed layout (§"Observed layout of the
5400-byte volumeDevice blob", :225-233):

```
+0x000  u32   7                         (device/bus type code — USB)
+0x004  char[?]  "ST500DM0"             (disk model, padded field)
+0x108  char[?]  "02-1BD142"            (disk serial)
+0x206  char[?]  "1.02"                 (firmware revision)
+0x30A  char[16] "0000000000000000"     (WWN/serial-2 field)
+0x414  char[?]  "USB Device"           (interface description)
+0x518  byte[512] MBR/BOOT SECTOR SNAPSHOT (33 C0 8E D0 … 55 AA — full first-sector
        image)
```

(The same artifact's :205-212 shows the record-3 partition context carrying a
`byte[0x1518]` (5400) volumeDevice sub-blob in the standard path's partition
record — the variant record promotes the device descriptor to its own
GUID-keyed record.)

## 2. The core side at `c7fa527c`

`include/cr/session/cr_records.h:190-210` — the identity card:

```cpp
struct VolumeSessionInfo {
    ...
    char label[32] = {0}; /* utf8, NUL padded */
    ...
    /* identity card (SESS-2 v2 tail; zeros on v1 sessions = unverifiable) */
    char model[48] = {0};
    char serial[32] = {0};
    u32 boot_hash = 0;
    /* S-030 (v3 tail): the image file path ... */
    char image[256] = {0};
    /* W1a: the embedded save time (v5+ sessions; 0 = pre-v5). */
    u64 save_unix = 0;
};
```

Field-by-field against the og's descriptor: model ✓ (48 vs the og's padded
field), serial ✓, boot sector → `boot_hash` (u32 hash — presence/integrity
signal, not content), **firmware revision ✗, WWN/serial-2 ✗, interface string
✗, the 512-byte boot-sector snapshot ✗** (grep over `src/core/session/` for
firmware/WWN/interface/snapshot fields: zero hits).

The session parity table (`modules/session.md`) carries record types 1-4 and
the core's own 5-9 extensions; the variant record's descriptor field set has
no row (grep-verified).
