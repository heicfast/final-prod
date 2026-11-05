# Evidence — S-054: the N-013 destination-arming regression (git-proven)

Session-12 continuation lane, 2026-10-29. Audited tree `c7fa527c`. Every quote below
was produced first-hand this session (git show / sed / grep at that tree).

## 1. The landing: `3b5397a1` — the complete N-013 fix

```
$ git show --stat 3b5397a1 --oneline | head -12
3b5397a1 fix(N-013): destination overlap armed for any resolvable path, ALL extents,
junctions resolved to target volume
 ...overlap-check-optional-and-first-extent-only.md |  7 +++
 include/cr/recovery/cr_safewriter.h                | 10 ++-
 include/cr/win/cr_transport.h                      | 30 ++++++++-
 src/app/rrecov.cpp                                 | 52 ++++++++++------
 src/core/recovery/cr_safewriter.cpp                | 34 ++++++----
 src/ui/cr_ui.cpp                                   | 37 ++++++++---
 src/win/CMakeLists.txt                              |  2 +-
 src/win/cr_transport.cpp                           | 72 +++++++++++++++++++
 tests/unit/test_cr_ui.cpp                          | 21 +++++++++++
 tests/unit/test_destio_win.cpp                     | 67 ++++++++++++++++++++
 tests/unit/test_report.cpp                         | 35 +++++++++++++
```

The rrecov.cpp diff's `+` side (the landed shape — multi-extent + junction resolution):

```c
+     * have more than one - first-extent-only missed overlap there). */
+    {
+        wchar_t dletter = 0;
+        if (win::destination_volume_letter(to_dir, &dletter)) {
+            std::vector<win::VolExtent> ve;
+            if (win::volume_extents(dletter, &ve)) {
+                bool on_source = false;
+                for (const auto& v : ve)
+                    if (v.disk == disk) { on_source = true; break; }
...
+                std::vector<std::pair<u64, u64>> spans;
+                spans.reserve(ve.size());
+                for (const auto& v : ve)
+                    spans.push_back({v.start, v.len});
+                writer.set_destination_spans(spans);
+            }
+        }
```

## 2. The clobber: `1ebd1bcc` — the UNRELATED M-036 NVMe fix

```
$ git show --stat 1ebd1bcc --oneline | head -12
1ebd1bcc fix(M-036): NVMe health full field surface + the og's data-unit conversion
 docs/ABI-CONTRACT.md           |  2 +-
 docs/API-SURFACE-AUDIT.md      |  2 +-
 docs/ground-truth-contracts.md |  2 +-
 include/cr/cr_ui.h             |  9 +++++-
 include/cr/health/cr_nvme.h    | 14 +++++++-
 src/app/rrecov.cpp             | 69 +++++++++++++++-----------------------
 src/core/health/cr_nvme.cpp    | 11 ++++++-
 src/ui/cr_ui.cpp               | 43 +++++++++----------------
 tests/unit/test_nvme_health.cpp | 42 +++++++++++++++++++++---
```

`git show 1ebd1bcc -- src/app/rrecov.cpp` — the `-` side removes the N-013 spans
block (quoted in full in the finding) and the `+` side restores the pre-N-013
single-extent shape:

```c
+    if (to_dir.size() >= 2 && to_dir[1] == L':') {
+        u64 dstart = 0, dlen = 0;
+        u32 ddisk = 0;
+        if (win::volume_extent(to_dir[0], &dstart, &dlen))
+            writer.set_destination(dstart, dlen);
+    }
```

(An M-036 health fix has no business touching the recover-path destination block;
the diff shape is consistent with the commit having been authored on a pre-N-013
tree and landed over it.)

## 3. The state at the audited HEAD `c7fa527c`

```
$ grep -rn "volume_extents\|set_destination_spans" src/ --include="*.cpp" \
    | grep -v "cr_transport.cpp\|cr_safewriter.cpp"
(no output — rc=1: ZERO production consumers)
```

`src/app/rrecov.cpp:2029-2034` (the first-extent-only arming) and
`src/ui/cr_ui.cpp:3863-3870` (the ABI twin, drive-letter-prefix paths only) —
both quoted verbatim in the finding.

`include/cr/win/cr_transport.h:124-126` — the published contract the code now
contradicts:

```c
/* N-013: reports the FIRST extent only - kept for the source-disk lookup
 * call sites; destination arming must use volume_extents() (all spans). */
bool volume_extent(wchar_t letter, u32* disk, u64* start, u64* len);
```

`src/win/cr_transport.cpp:1037` — `destination_volume_letter` (the junction/
relative-path resolver from the landing) survives with its helper tests
(`tests/unit/test_destio_win.cpp:145-204`) but no production caller.

## 4. The suite-green false confidence

From-scratch configure + build + ctest at the same tree (full log:
`evidence/executions/s12-build-execution.md`):

```
Total Tests: 1117
1117/1117 Testing Passed — 100% tests passed, 0 tests failed
```

The surviving N-013-era tests pin the HELPERS (`volume_extents`,
`destination_volume_letter`, `set_destination_spans` mechanics), not the
production wiring — which is why the regression is invisible to the suite. The
wiring-pin tests proposed in S-054's fix plan are the fence that would have
caught `1ebd1bcc`.
