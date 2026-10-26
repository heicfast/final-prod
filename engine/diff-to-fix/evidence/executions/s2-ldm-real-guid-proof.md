# s2-ldm-real-guid-proof — executed proof of the A-002 regression

**Session 2 (T7 deep pass) · current core (post-`1cb6a43`) vs pre-fix constants**

Method: `Ldm.GptDynamicDiskRealCanonicalGuidsParse` — a byte-identical copy of the
suite's `Ldm.GptDynamicDiskCanonicalGuidsParse` fixture
(`tests/unit/test_ldm.cpp:519-593`: StripeDisk LDM database + full GPT wrapper with
CRCs, PRIVHEAD at the metadata partition's last block), with ONLY the two GPT entry
type GUIDs replaced by the real Microsoft canonical bytes (mixed-endian disk form):

- metadata: `aa c8 08 58 8f 7e e0 42 85 d2 e1 e9 04 34 cf b3` = `5808c8aa-7e8f-42e0-85d2-e1e90434cfb3`
- data:      `a0 60 9b af 31 14 62 4f bc 68 33 11 71 4a 69 ad` = `af9b60a0-1431-4f62-bc68-3311714a69ad`

Source preserved: `real_guid_gtest.cpp` (this directory).

## Run 1 — CURRENT core (constants as of `1cb6a43`, audited tree)

Build: `g++ -std=c++20 -Iinclude -Itests/unit <gtest includes> real_guid_gtest.cpp cr_ldm.cpp cr_crc32.cpp cr_table_parse.cpp libgtest.a`

```
[ RUN      ] Ldm.GptDynamicDiskRealCanonicalGuidsParse
real_guid_gtest.cpp:105: Failure
Value of: rc
  Actual: 1342373889 (ERR_BAD_MAGIC)
Expected: OK
real canonical GUIDs: GPT dynamic disk MUST parse (og + Microsoft + kernel all use these bytes)
[  FAILED  ] Ldm.GptDynamicDiskRealCanonicalGuidsParse (5 ms)
[  PASSED  ] 0 tests.
```

## Run 2 — same test, pre-`1cb6a43` constants (scratch include-dir copy, repo untouched)

Build: same, with `-I<scratch>/prefix/include` first — a copy of `cr_table_wire.h`
with the two LDM constants restored to the pre-`1cb6a43` (= real canonical) bytes.

```
[ RUN      ] Ldm.GptDynamicDiskRealCanonicalGuidsParse
[       OK ] Ldm.GptDynamicDiskRealCanonicalGuidsParse (5 ms)
[  PASSED  ] 1 test.
```

(The pre-fix run additionally asserts `info.is_gpt == true` and `layouts.size() == 2` —
both hold; the full volume graph resolves.)

## Conclusion

The constants restored by the A-001 fix reject real-canonical GPT dynamic disks;
the constants the A-001 fix removed accept them and resolve the full volume graph.
This is the A-002 regression: **every real Windows GPT dynamic disk gets zero
dynamic-volume recovery on the current tree.**
