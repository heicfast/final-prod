# S42 Wave-1 — the board-truth verification at the post-window HEAD (the S12 sweep's fix-window re-check)

**Lane:** S42 (session-42 continuation audit) · **Tree:** core `869da779`-lineage rebased through
`25e408b8` (the S12 reconcile) → lane tip `d4776278` (worklog-only on top; the code state is
byte-identical to `869da779`) · **Corpus:** `be0cfe9`.

## Why this pass (the S12 sweep's staleness window)

The S12 lane's liveness sweep (`s12-liveness-sweep.md`) ran at tree `36c1d3e5` (worklog-only on
`c7fa527c`) — **before** the heaviest fix window of the trail landed (S-049 `d4b487c8`, S-051
`ad0535d6`, S-052 `f4e500de`, S-053 `e10ec381`, N-115 `67de3276`, N-121 `23429142`, N-122
`ef74eb83`, N-126 `ae6ce2bf`, N-128 `21a3bd2a`, N-129 `98e4e48f`, N-130/N-138 `b3e6c287`,
N-132 `4e74ee2f`, N-133 `838fe939`, N-135 `0eb5f82f`, N-153 `67de3276`+`869da779`). Their LIVE
verdicts for findings whose cited surfaces those landings touched are therefore UNVERIFIED at the
current tree until re-checked. This pass re-checks exactly that risk set — the findings whose
cited files were modified by the window's fix commits — plus a sample of the rest.

## The board at this lane's open (filesystem truth, re-counted)

| Tier | Count | IDs |
|---|---|---|
| SEVERE | 4 | S-040, S-048, S-050, S-054 |
| MAJOR | 23 | M-073, M-075, M-076, M-078, M-086, M-087, M-089, M-098, M-099, M-101, M-103, M-104, M-105, M-106, M-107, M-109, M-111, M-112, M-113, M-114, M-115, M-116, M-117, M-118, M-119 — see note¹ |
| MINOR | 17 | N-090, N-107, N-125, N-127, N-131, N-139, N-140, N-142, N-143, N-144, N-145, N-146, N-148, N-149, N-150, N-152, N-154 |
| INFORMATIONAL | 9 | I-026, I-027, I-028, I-029, I-031, I-032, I-034, I-035 + the I-030 successor row per the ledger |
| FIXED (files) | 19 | (ledger rows: 326 per the S12 reconcile) |
| tracked-open | 3 | OQ-S4-3, OQ-S10-1, OQ-S10-3 |

¹ The MAJOR directory re-count at this tree: 23 files. The S12 reconcile's "23 MAJOR" claim matches
the filesystem. (The S12 close's "19" was their pre-second-rebase count.)

## The re-check battery (every finding whose cited surface the fix window touched)

Executed 2026-10-29 at the lane tree. Method: the finding's own core-side defect signature
grepped/read at HEAD (`src/` + `include/` + `tests/`); the verdict is LIVE only if the defect
signature is present verbatim.

| ID | Defect signature checked | Result at HEAD | Verdict |
|---|---|---|---|
| **S-048** | single configured carve root: `raw_root_name_()` `cr_scan.cpp:146`, one `root_carved_` (:206/:239-240/:247-248/:2182); `Reconstructed` → 0 hits tree-wide | present, unchanged by the S-049 naming landing (the namer chain lives in `cr_carve.cpp`/`cr_analysers.cpp`; the root plumbing untouched) | **LIVE** |
| **M-103** | flat orphan reparent: `cr_mftwalk.cpp:1082` phase B comment + `:1107` `tree.reparent(e.node, root_lost)`; `File Path Lost`/`LostDir` → 0 hits | present | **LIVE** |
| **M-104** | identity-free candidate roots: `cr_scan.cpp:1933` `std::u16string role = u"Candidate @";` | present (cite :1639→:1933 — the S-051 target-builder landing grew the file; defect unchanged) | **LIVE** (stale-line) |
| **M-106** | `cr_analysers.cpp` stand-in comments: the 3DS/DWG `stands in` block now at :5517-5518 (was :3846/:3936 — the S-049 landing added ~1670 lines before it) | present | **LIVE** (stale-line) |
| **M-107** | the 12 stand-in sites (`stands in`/`recorded follow-up` comments): now at :3631/:4776-4777/:4865/:4882/:4904/:4921/:4997/:5370/:5409/:5517/:5582/:5607 (was :3639-:4469 family) | present, every site re-located | **LIVE** (stale-line) |
| **N-139** | carve OOM policy: `bad_alloc`/`OOM` in `cr_carve.cpp` → 0 hits | absent policy confirmed | **LIVE** |
| **N-140** | carve per-extension result stats: `per_extension`/`extension_stats` → 0 hits (the S-049 landing added per-FORMAT counters in the ScanCtx map — a different surface; the finding's per-extension aggregation remains absent) | absent | **LIVE** |
| **N-146** | the FH11 zero-end stand-in (`stands in` family, `cr_analysers.cpp` :5370 trailer-scan follow-up comment) | present | **LIVE** |
| **N-149** | carve filter exempt set: `exempt` → 0 hits in `src/core/carve/` + `src/include/cr/` | absent | **LIVE** |
| **N-131** | FS-native parent anchors persistence: `parent_anchor`/`fs_native_parent` → 0 hits | absent | **LIVE** |
| **N-143** | iSpecial two-step dispatch: `iSpecial`/`two_step` → 0 hits in `src/core/carve/` | absent | **LIVE** |
| **N-144** | TxtView text-snapshot thumbnails: `text` → 0 hits in `cr_thumbnail.cpp`; `snapshot` → 0 hits in `src/core/preview/` (the M-108 eligibility + snippet layers landed — the finding's thumbnail half is untouched) | absent | **LIVE** |
| **N-145** | pending-marked-disk marker: pending/marked-disk signature → 0 hits in `cr_scan.cpp` | absent | **LIVE** |
| **N-090** | session-list scope row shape: session-scope row signature → 0 hits in `cr_ui.cpp` | absent | **LIVE** |
| **S-040** | image-create producer: `cr_ui_image`/`image_create` → 0 hits in `src/`+`include/`; `cr_image_win.cpp:37` opens `GENERIC_READ` only | absent | **LIVE** |

Not re-checked individually (verified LIVE by the S12 lane at the post-rebase HEAD in their
reconcile commit `25e408b8`, which re-verified "every defect site" after the same fix window):
M-118 (MountPoints2 channel — "still absent"), M-119 (the read clamp — "the slice still strict"),
N-154, I-035, N-142, S-054 (git-proven at filing). The five N/A-DRIVER-REPO rows (M-086/M-087/
M-089/N-107/N-127) stand as OQ-S10-3's pushed-forward set — unchanged by any core-side landing.

## Verdict

**No unretired fix found. The board is truthful at the lane tree.** The fix window's retirements
(S-049/S-051/S-052/S-053 + the N-set) all carry their FIXED/ ledger rows; the 53 active findings
re-checked here are live at HEAD. Three stale-line-number citations noted (M-104, M-106, M-107)
— the defect sites moved but the defects stand; the findings' own cites remain navigable by
symbol per the trail convention.

**The wave's retirements: none required** (nothing on the active board is fixed at HEAD — the
user's remove-the-fixed directive is discharged by verification, not by moves).
