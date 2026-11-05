# MINOR N-128 — CAPABILITY-MATRIX's prebuilt-CI row is doubly untrue: it cites four workflows and an actions/ directory that do not exist at HEAD (only checks/installer/release/test-benchmark exist), and it buries an unowned follow-up ("ext4/HFS+/Btrfs/ZFS/ReFS staging") inside a DONE verdict

**Severity:** MINOR
**Module / Surface:** documentation truth (docs/CAPABILITY-MATRIX.md) · CI coverage registration
**Parameters affected:** A68 (documentation accuracy), A65-adjacent (test coverage registration — the FS-staging follow-up has no owner)
**Corpus references:** none needed (core-side doc-truth defect; the og's CI is corpus-silent by the matrix's own row framing "n/a (og CI not in corpus)").
**Core references:** audited commit `ea1ac86`-lineage — `docs/CAPABILITY-MATRIX.md:21` (the "Prebuilt-artifact CI" row): evidence column cites `.github/workflows/{driver-install,core-install,staged-scenarios,release}.yml; .github/actions/install-driver/action.yml`; the actual tree has only `checks.yml`, `installer.yml`, `release.yml`, `test-benchmark.yml` under `.github/workflows/` (verified by directory listing, SA-10) and NO `.github/actions/` directory (verified). The same row's verdict column reads "**DONE** (v1 matrix: NTFS/FAT32/exFAT deletion + NTFS quick-format + the refusal phases; ext4/HFS+/Btrfs/ZFS/ReFS staging is recorded follow-up)". The staging follow-up's only other registration is `docs/CI-SCENARIOS.md:66-73` ("recorded here as the matrix's documented boundary, not a silent gap") — no finding on the board owns it (grep verified across diff-to-fix/). M-088 owns the stale-CI-doc-cluster class but its file list (README:125-132, PREBUILT-ARTIFACTS §2/§5, DRIVER-RELEASE/DRIVER-CONTRACT/CI-LOCAL/CI-SCENARIOS/RELEASE-NOTES same-class sites) does NOT include CAPABILITY-MATRIX.md (verified against M-088's enumerated sites; M-088 itself is now FIXED at `41d8829` — one floor + the 4-lane CI-shape doc truth — but its fix did NOT touch this row, so the class is now owned by this finding alone).
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The capability matrix's prebuilt-CI row claims DONE while citing a workflow set that the CI-rebuild consolidation deleted (driver-install/core-install/staged-scenarios merged away; the composite action removed) and while carrying a registered-but-unowned test-coverage follow-up (the ext4/HFS+/Btrfs/ZFS/ReFS staged truth matrix) inside the DONE verdict. Both halves mislead a reader: the evidence column points at files that are not there, and the DONE verdict hides an open work item that no finding tracks.

## What the reference does

Not applicable (core-side doc truth; the row itself declares the og CI corpus-silent).

## What our core does

The row exists at `docs/CAPABILITY-MATRIX.md:21` with the stale citation set and the DONE+follow-up verdict (quoted in Core references). The actual CI at HEAD: `checks.yml` (the consolidated correctness lane), `installer.yml` (S-038's installer lane), `release.yml` (the sole build source), `test-benchmark.yml` (the benchmark lane) — the four-workflow + composite-action citation describes the pre-rebuild tree. The staged FS matrix covers NTFS/FAT32/exFAT deletion + NTFS quick-format + refusal phases; the ext4/HFS+/Btrfs/ZFS/ReFS staging is future work recorded only in CI-SCENARIOS.md's boundary note.

## Gap

Falsifiable: (1) `ls .github/workflows/` at HEAD returns exactly {checks, installer, release, test-benchmark}.yml and `.github/actions/` does not exist — the row's evidence column cites 5 nonexistent paths; (2) no finding in diff-to-fix/findings/ owns the ext4/HFS+/Btrfs/ZFS/ReFS staging follow-up (grep verified); (3) M-088's enumerated file list does not include CAPABILITY-MATRIX.md, so the drift class is unowned for this file.

## Impact

Doc-truth: a reader verifying the DONE verdict follows dead paths and concludes the matrix is unmaintained; the unowned staging follow-up can silently age (the exact failure mode the session mandates close — "recorded outside an active finding"). No runtime behavior affected.

## Reproduction / verification

Executed (strength 1): directory listings at HEAD (the four workflows, no actions/ dir); grep for the follow-up's ownership across diff-to-fix/ (zero findings); M-088's file list read and compared.

## Fix plan (for the engineering team)

1. Update `docs/CAPABILITY-MATRIX.md:21`'s evidence column to the actual lane set: `.github/workflows/{checks,installer,release,test-benchmark}.yml` (and wherever the install-driver logic now lives post-consolidation — checks.yml's job or the scripts/ helper — cite that path).
2. Split the verdict: "**DONE** (v1 matrix: NTFS/FAT32/exFAT deletion + NTFS quick-format + the refusal phases)" + a separate FOLLOW-UP cell for ext4/HFS+/Btrfs/ZFS/ReFS staging (the CI-SCENARIOS.md:66-73 boundary note stays the detail record).
3. Either fold this file into M-088's stale-CI-cluster fix batch (adding CAPABILITY-MATRIX.md to its file list when the engineering team executes it) or fix it standalone — one of the two, so the class has an owner for this file.

## Tests to add

None (documentation). A CI-side guard could assert the matrix's cited workflow paths exist (a docs-lint step), preventing recurrence — optional, note only.

## Related findings

- **M-088 (FIXED `41d8829`)** — the FloorVersion split + stale-CI-doc cluster (the same drift class; its fix covered its own file list — this row is the residue this finding owns).
- **N-104 / N-095** — the doc-truth siblings.
- **CI-REBUILD-WIN (worklog)** — the consolidation that deleted the cited workflows (the event that made the row stale).

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-128 landing — recorded post-push in the ledger row)
**Verified by:** the falsifiable checks re-run this session: `ls .github/workflows/` returns exactly {checks, installer, release, test-benchmark}.yml; `.github/actions/` does not exist; the row's evidence column now cites exactly those four paths; the DONE verdict no longer carries the follow-up inside it (a separate FOLLOW-UP cell names the ext4/HFS+/Btrfs/ZFS/ReFS staging, its owner, and its next concrete step).
**Resolution notes:** Both halves repaired per the fix plan: (1) the evidence column rewritten to the post-consolidation lane set — checks.yml (the consolidated correctness lane; the M-088 fix's own landing), installer.yml (the S-038 installer gates, where the install-driver logic now lives — it builds the installer and fetches/signs the prebuilt driver), release.yml (the sole build source), test-benchmark.yml; (2) the verdict split — DONE covers the v1 matrix, the FS-staging follow-up is now its own owned cell (the unowned-follow-up failure mode closed), with the CI-SCENARIOS.md:81-83 boundary note as the detail record and the next concrete step named (a per-filesystem staging script feeding the same expected-vs-observed harness). Doc-only; the directory listing is the verification.
