# SEVERE S-058 — the grading extension-table gate is INVERTED: the core twin-flags the wrong extension population

**Severity:** SEVERE
**Module:** grading / query (the "Recovery Chances" status engine — the user's focus battery)
**Parameters affected:** #15 Confidence grading model; #29 Merge and dedup strategy (overlap twins); #9 detection correctness (grade column)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:605736-605807` (the completion-pass generic path); `:324923-324939` (the comparator pair `FUN_1401bebe0`/`FUN_1401bec20`); `:413990-414019` (`FUN_140256d00` = `std::set::find`, the MSVC lower_bound+compare idiom); `:585885-585905` (the control case that calibrates the comparator direction); `:605502-605510` (the member-set twin lookup using the same wrappers); `re-workspace/artifacts/DRWUI-subsystems--assistant-grading-preview.md:33-52` (the CRecoverability decode)
**Core references:** `src/core/tree/cr_grading.cpp:272-281` (the inverted gate; comment cites the s5 polarity), `:284-296` (twin pass), `:150-169` (`ext_of_name`), `:29-65` (the 169-entry table); `include/cr/tree/cr_grading.h:15-40`; `tests/unit/test_grading.cpp:166-187` (the polarity pinned to the inverted reading); `diff-to-fix/evidence/decompiled-quotes/s5-grading-extension-table-dump.md` §5 item 4 (the polarity erratum this finding corrects) — all at audited core `b26017a7`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's CRecoverability completion pass runs its twin/overlap/deep-validation branch for files whose extension is **NOT** in the 169-key grading table; the core runs the equivalent twin detection **only** for files whose extension **IS** in the table. The populations are exactly inverted, so the core flags overlap/twins and re-grades the wrong file population and skips the one the og processes. The inversion originates in the session-5 evidence file's polarity line (`FUN_1401bebe0` misread as an equality test where its FALSE branch means "found") and was implemented and test-pinned from that misreading.

## What the reference does

The completion pass `FUN_14038ac90` (per DRWUI-subsystems :33-52, tree-verified this session at `:605686-605815`) walks each node: `size < 1 → grade 0` (:605726-605729); `category 0xC → grade 2` (:605731-605734); `FS ∈ {3 NTFS, 10 ReFS, 0xB Btrfs} → skip generic path` (:605736-605737). The generic path then extracts the uppercased extension (`FUN_1403895c0` :605738 + `FUN_140389400` :605739) and looks it up in the global 169-key set `DAT_1425e7980`:

```c
605740  local_68 = FUN_140256d00(&DAT_1425e7980,local_48,local_30);   /* std::set::find  */
605741  local_70 = FUN_1401be4c0(&DAT_1425e7980,local_40);           /* std::set::end   */
605742  cVar1   = FUN_1401bebe0(local_68,local_70);
605743  if (cVar1 == '\0') {                                          /* block runs here */
```

The comparator chain, read first-hand this session:

```c
324923  bool FUN_1401bebe0(a, b) { return FUN_1401bec20(a, b) == '\0'; }   /* INEQUALITY */
324925  bool FUN_1401bec20(longlong *a, longlong *b) { return *a == *b; } /* iterator node-pointer equality */
```

`FUN_140256d00` (:413990-414019) is the MSVC `std::set::find` idiom verbatim — `lower_bound` (:414024-414035, the `_Tree` descent at :414059-414083), `lower == end` check via `FUN_1401bec20`, then the `key < *lower` comparator `FUN_140060710` — returning the lower-bound iterator when equal, else `end()`. Therefore `FUN_1401bebe0(find, end)` returns **TRUE when the key IS FOUND** (find != end), and the gate `if (cVar1 == '\0')` enters the deep-validation block **when the extension is NOT in the table**.

The direction is calibrated by the control case in the same binary (:585885-585905), which uses the *equality* comparator with the same call shape:

```c
585888  local_78 = FUN_140256d00(local_90,local_68,param_3);   /* find   */
585889  local_80 = FUN_1401be4c0(local_88,local_60);           /* end    */
585890  cVar1   = FUN_1401bec20(local_78,local_80);            /* EQUALITY: TRUE iff find==end iff NOT FOUND */
585891  if (cVar1 == '\0') { local_c8 = 1; }                    /* FOUND → set c8 directly */
```

(the `else` arm additionally sets `c8` only when the name matches one of two 4-byte constants — the classic whitelist idiom `found-in-set || name-in-consts`; this only reads correctly with `FUN_1401bec20==0 ⇔ FOUND`, which fixes both comparators' directions simultaneously).

Inside the block (:605743-605803): the twin address is computed (`device vt+0x30`, `kind ∈ {1,4} → offset −2` :605752-605756, run+fixer :605757-605767), the node is inserted into the addr→node map at `this+0x20` (:605769-605770), a same-address **same-6-bit-type** twin found in the map gets `FUN_140089630(twin, 2)` (:605774-605784), and failing that the **deep content read** `FUN_14038c470(node, addr, 0, 0)` runs (:605798) — followed by the "graded" mark `FUN_140389540(node, 1)` (:605800). For extensions **in** the table the `else` branch (:605804-605807) does nothing but string cleanup: those files keep the engine-default grade 1 and never receive twins or deep reads on this path.

Corroboration — the `""` quirk: the og's runtime set contains `""` (the ctor's 2000-iteration init loop over a zero-padded array inserts the empty string after the 169 real keys; s5 dump §4/§6.2, `:605297-605305`). Under the verified polarity, dot-less files (extension `""`) are **in-table** → they skip the deep-validation block. The core's `EXPECT_FALSE(in_table(""))` produces the same *skip* for dot-less files — the only population where the two implementations coincidentally agree.

## What our core does

`cr_grading.cpp:272-276` (audited `b26017a7`):

```cpp
/* generic path (FS not in {NTFS, ReFS, Btrfs}): the extension
 * table gates the twin detection (found in the 169-entry map at
 * DRWUI :605740-605741 -> the deep path runs). */
if (!extension_in_grading_table(ext_of_name(pr)))
    continue;
TwinKey tk; ...
```

The core collects twin candidates **only for in-table extensions** — the exact inverse of the verified og gate. The comment cites the s5 evidence's polarity line, and `tests/unit/test_grading.cpp:166-187` pins the inverted behavior ("extension-table gate polarity pinned as in-table→twins"). The core's twin pass (:288-296) then applies overlap flags + forced High to that wrong population.

## Gap

Stated falsifiably: for a deleted `.jpg` file (in the 169-table), the og runs **no** twin/deep-validation in the completion pass while the core flags it as a twin candidate; for a deleted `.xyzzy` or `.ipynb` file (not in the table), the og **does** run the twin/deep-validation machinery while the core skips it. The two implementations disagree on every non-empty extension; they agree only on dot-less names (both skip, for different reasons).

## Impact

The "Recovery Chances" status — the grade column users sort and decide by — diverges on the twin/overlap surface for essentially the whole file population: the core's overlap flags and overlap-induced grade-2 promotions land on in-table extensions (the 169 common formats), while the og's land on the long-tail extensions the og considers unvalidated. Users see wrong overlap markers ("same data recovered twice" indicators) and wrong High-grade promotions on exactly the formats the og leaves at Average, and miss them on the ones the og promotes. Per the session directive (recovery-chances status is a focus area; a verified diff is filed severe), this is SEVERE. Downstream, the assistant's matched/total gauge inherits the shifted promotion set.

## Reproduction / verification

- **Decompiled evidence (first-hand, this session):** the comparator pair at `:324923-324939`; `FUN_140256d00` = `std::set::find` at `:413990-414019` (with `FUN_140256de0` = lower_bound at `:414024-414035` and the `_Tree` descent `FUN_140256ed0` at `:414059-414083`); the grading gate at `:605740-605743`; the control case at `:585885-585905`; the member-set twin lookup at `:605502-605510`; the block contents `:605743-605803` (twin map, grade-2-on-twin, `FUN_14038c470` deep read, `FUN_140389540` mark); the else-nothing at `:605804-605807`. Full worksheet: `diff-to-fix/evidence/decompiled-quotes/s42-grading-gate-polarity.md`.
- **Core evidence:** the inverted gate + the comment carrying the s5 reading at `cr_grading.cpp:272-276`; the pinned test at `test_grading.cpp:166-187`; the table at `cr_grading.cpp:29-65`.
- **Erratum identified:** `diff-to-fix/evidence/decompiled-quotes/s5-grading-extension-table-dump.md` §5 item 4 reads "In-table (find != end) → deep-validation branch" — the polarity label is inverted (the table dump itself, §2-§3, remains byte-exact and valid). The s5 §6.2 note ("a faithful port must reproduce this") survives, but under the corrected polarity the `""` quirk makes dot-less files **skip** the branch, not enter it.

## Fix plan (for the engineering team)

1. **Invert the gate in `grade_tree`** (`src/core/tree/cr_grading.cpp:275`): collect twin candidates when the extension is **NOT** in `kGradingExtensions` (the og's branch is `found → skip; not-found → twin/deep`); keep `""` excluded (the og's runtime set contains `""`, so dot-less files skip too — the current empty-string early-out in `extension_in_grading_table` stays).
2. **Update the header comment** (`include/cr/tree/cr_grading.h:15-40`) and the `.cpp` gate comment (`:272-274`) to the corrected polarity, citing `DRWUI:605740-605743` + `FUN_1401bebe0`'s inequality body at `:324923-324930`.
3. **Re-pin the tests** (`tests/unit/test_grading.cpp:166-187`): the polarity test flips — a not-in-table extension (e.g. `.ipynb`) enters the twin set; an in-table extension (e.g. `.jpg`) does not.
4. **File the s5 evidence erratum**: append a correction note to `s5-grading-extension-table-dump.md` §5 (per the CORPUS-ERRATA convention — the core's own errata register carries corpus-doc fixes; this one is in the core's evidence tree).
5. **Acceptance criteria:** a fixture with two same-extent-end same-category files, one `.jpg` and one `.xyzzy`, produces overlap flags on the `.xyzzy` pair and none on the `.jpg` pair (the inverse of today's test); the doc trail (finding → ledger row) records the s5 erratum.

## Tests to add

- `Grading.TableGatePolarityNotInTableEnters` — two `.xyzzy` files sharing extent-end + category → both overlap-flagged + High (fails today: not in table → skipped).
- `Grading.TableGatePolarityInTableSkips` — two `.jpg` files sharing extent-end + category → neither flagged (fails today: flagged).
- `Grading.DotlessSkipsTwins` — extensionless pair → no flags (passes today; pins the `""` coincidence so the polarity flip cannot regress it).

## Related findings

- **S-061** — ReFS/Btrfs insert-time grade (the other graded-population divergence in the same engine).
- **M-138** — the twin-key device-family set (the address key the twin map uses; renumbered from M-133 at the N-160 doc-truth repair).
- **M-134** — the deep-read validation (`FUN_14038c470`, the third leg of the same branch).
- **N-183** — the gauge population/score-formula batch (the same engine's aggregate surface).

## Open questions

None — the polarity is settled structurally (comparator bodies + control-case calibration + the `""` corroboration all agree). OQ-S42-3 tracks the *sub-gates* inside the deep read, not the polarity.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; S47 lane)
**Verified by:** `Grading.ExtensionTableGatesDeepValidation` (re-pinned to the verified polarity: out-of-table ZZQX twins flagged + High, in-table DOC never flagged + engine-default grade, dot-less never flagged - the pre-fix build fails 5 of its 9 assertions), `Grading.TwinAddressFlagsBoth` (re-based onto out-of-table candidates: zzqx/zzqx same-category twins; asd(Unsaved)/zzq2(Other) same-key different-category unflagged), full suite 1209/1167/0-fail.
**Resolution notes:** The gate in grade_tree's generic path flipped to the og's verified polarity: twin candidates are collected when the extension is NOT in the 169-key table, with the empty-ext early-out preserved (the og's runtime set contains "" from the ctor's 2000-iteration init over the zero-padded array, so dot-less files skip too - both sides skip, for their own reasons, exactly as the finding's corroboration section states). Corpus re-verified first-hand this session: FUN_1401bebe0 at :324923-324939 is `FUN_1401bec20(a,b) == 0` (the inequality twin), so `cVar1 == 0` at :605742-43 is find==end (NOT FOUND) and the deep block runs for not-in-table extensions; the control case at :585888-585891 (equality comparator, same call shape) calibrates the direction. The s5 evidence file's §5 item 4 carries the ERRATUM note now (its table dump §2-§3 remains byte-exact and valid). The finding's own fix plan items 2-3 (the FUN_14038c470 deep content read and the +0x73 "needs validate" flag) are the separate M-134 finding's scope - this fix lands the population/polarity correction (the twin flagging + grade-2 promotion the core's pass 2 already performs, now on the right population).
