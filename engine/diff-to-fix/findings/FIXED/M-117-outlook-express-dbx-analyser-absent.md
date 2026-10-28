# MAJOR M-117 → M-121 — the ID-collision stub (Outlook Express Dbx carve analyser)

**This file is a renumber stub, not the finding.** The full finding body and the
convergent Resolution live in
[`findings/FIXED/M-121-outlook-express-dbx-analyser-absent.md`](M-121-outlook-express-dbx-analyser-absent.md) — the chain runs M-117 → M-118 (a renumber that never landed: the parallel lanes' lostpart finding consumed M-118 first) → M-121.

## Why the renumber (SA-9)

The M-117 ID was consumed **three times** across the hot parallel windows:

1. **The session-40 suppress-overlap landing** (FIXED-LEDGER row 442 — the earliest
   push; retired to history; **keeps the M-117 ID**).
2. **The S41 audit lane's Dbx filing** (this file's original identity —
   `findings/MAJOR/M-117-outlook-express-dbx-analyser-absent.md`).
3. **The parallel fix lane's `1d4c96b6`** commit message ("fix(carve): M-117 — the
   Outlook Express Dbx analyser lands") + its ledger row and matrix cell.

Per the SA-9 push-time renumbering convention, the Dbx finding renumbers to
**M-118** (the S41 block's own declared next-free ID), reconciling all three
usages. The renumber note lives in the M-118 file's heading; this stub preserves
the M-117 path for navigation per the A-005/A-006 renumber-pair precedent (no
resolved finding is ever deleted — the no-delete mandate).

## Status

**FIXED (CONVERGENT)** — see the M-121 Resolution: the parallel lane's `1d4c96b6`
landing (JMF6 version word `03 00 10 00`, disasm-settled) + this lane's convergent
correction push (the marked pattern's byte 6 `0xEE` — the parallel lane's `0xDE`
never matched a real file; the even-rounded non-zero prefix in
FUN_180042b30's zero-walk; the renumber). 9/9 tests GREEN.
