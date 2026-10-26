# MINOR N-080 — STORAGE_DEVICE_DESCRIPTOR offset transposition: model reads the serial offset; serial reads into RawDeviceProperties

**Severity:** MINOR
**Module:** storage (win enumeration layer)
**Parameters affected:** D11 (node/identity fields — the device identity data), B1-adjacent (enumeration correctness)
**Corpus references:** `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md:228-242` (record 3: model +0x004 and serial +0x108 as DISTINCT axes — the og's card keeps them separate), the Windows SDK `winioctl.h` STORAGE_DEVICE_DESCRIPTOR layout (ground truth for the field offsets this finding is about)
**Core references:** `src/win/cr_enumerate.cpp` query_disk/query_disk_handle (the descriptor decode reads `prod_off = base + 24` and `serial_off = base + 36`) — found during the N-066 fix session (DFX31) while adding the firmware/WWN/interface axes
**Status:** READ-ONLY FINDING — filed during the N-066 fix; fixed in the same landing

## Summary

The win layer's STORAGE_DEVICE_DESCRIPTOR decode reads the product offset at +24 and
the serial offset at +36. The SDK layout (winioctl.h) is: VendorIdOffset@+12,
ProductIdOffset@+16, ProductRevisionOffset@+20, SerialNumberOffset@+24, BusType@+28,
RawPropertiesLength@+32, RawDeviceProperties@+36. So the decode fills `model` from the
SERIAL string's offset, and fills `serial` from the first four bytes of the raw
properties block reinterpreted as a string offset (garbage or a wild-but-bounded
read inside the 4096-byte query buffer).

## What the reference does

The og's record-3 identity card keeps model (+0x004) and serial (+0x108) as distinct,
correctly-populated axes (the corpus artifact quotes both at their offsets).

## What our core does

`PhysicalDisk.model` receives the serial string; `PhysicalDisk.serial` receives
whatever the raw-properties-prefix-as-offset lands on (typically nothing usable).
The transposed data flows into ScanConfig → VolumeInfo → the CLI's `[N] model
(serial)` line and the cr_ui card.

## Gap

Falsifiable: on any real disk with distinct product and serial strings, the core's
`model` field equals the device's SERIAL text (and `serial` is empty/garbage), while
the og's card carries the true product string in model and the true serial in serial.

## Impact

Identity-card data quality only (the boot_hash CRC32 still discriminates volumes on
resume — the verify path is unaffected). MINOR, but it degrades every surface that
prints or persists the identity axes, and it undermines N-066's new axes (a decode
built on transposed offsets would mis-fill firmware too).

## Reproduction / verification

SDK layout read first-hand (winioctl.h); the code's two offset constants compared
against it. Linux-lane proof added by the fix: a synthetic descriptor buffer with
distinct strings at the SDK offsets decodes correctly only with the fixed offsets
(`Enumerate.DeviceDescriptorFieldsDecode` — fail-before executed on the transposed
tree).

## Fix plan

1. Fix the offsets: ProductIdOffset@+16, ProductRevisionOffset@+20 (the new
   firmware axis), SerialNumberOffset@+24.
2. Factor the descriptor decode into a pure function (Linux-testable) with the
   bounded-string scan the raw code already had.
3. Pin with a synthetic-buffer test proving vendor/product/revision/serial each land
   in their own field.

## Tests to add

- `Enumerate.DeviceDescriptorFieldsDecode` (the transposition pin)

## Related findings

- N-066 (the identity-card field coverage this bug was found during — same landing)
- N-071 (the enumeration layer the decode lives in)

## Open questions

None — the SDK layout is authoritative and the synthetic-buffer test pins it.


## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-066 landing — DFX31; found + filed + fixed during N-066, same commit)
**Verified by:** Enumerate.DeviceDescriptorFieldsDecode + Enumerate.DeviceDescriptorBoundedAndAbsent (tests/unit/test_enumerate_merge.cpp) — fail-before EXECUTED on the re-transposed tree (model received the SERIAL text — RED; the canonical offsets restored — GREEN). Suite 855/855.
**Resolution notes:** The decode moved into the pure header function decode_device_descriptor (cr_enumerate.h) with the SDK-exact offsets: ProductIdOffset@+16, ProductRevisionOffset@+20 (now the firmware axis), SerialNumberOffset@+24, BusType@+28 — the pre-fix constants read the product at +24 (the serial's offset, so PhysicalDisk.model carried the SERIAL text) and the serial at +36 (inside RawDeviceProperties — garbage offset). Absent-offset-0 semantics + a 128-char string bound + the 36-byte header gate are pinned by the bounded/absent test; both enumeration stacks (the numbered loop and the N-071 interface pass) share the fixed decode through query_disk_handle.
