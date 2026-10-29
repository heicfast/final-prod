# MINOR N-195 — bench/ours/verify-driver.ps1 carries the phantom-zero extended-IOCTL codes (0x60002440/0x60002448 vs the contract's 0x6002440/0x6002448): the driver-verification bench can never exercise the real GET_INFO/GET_VERSION path

**Severity:** MINOR
**Module:** bench (driver verification tooling) — the in-repo residue of M-089's phantom-zero class
**Parameters affected:** Q65 (test coverage — the bench exercises nothing), C22 (IOCTL codes), Q68 (tooling truthfulness)
**Corpus references:** n/a (core-side defect; surfaced by the S54 liveness battery's M-089 verification — the driver-repo side of M-089 stays tracked-OQ per OQ-S10-3, this is the core-repo half)
**Core references:** audited `34d6cf10` — `bench/ours/verify-driver.ps1:47` (`DeviceIoControl(h, 0x60002440, …)`) and `:55` (`0x60002448`) vs the driver contract's real codes `include/cr/win/cr_rda.h:55-57` (`kIoctlGetInfo = 0x6002440u` fn 0x910; `kIoctlGetVersion = 0x6002448u` fn 0x912)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The PowerShell driver-verification bench calls the extended IOCTLs with an extra zero digit in the code constants (0x60002440/0x60002448); the contract codes are 0x6002440/0x6002448. Against the real driver, both calls fail with an unknown-IOCTL error and the bench reports "GET_INFO rejected / GET_VERSION rejected" — the tool never exercises the paths it exists to verify, and a green-looking run is impossible (or worse, a future "fix" could mask it).

## What the reference does

n/a — core-side tooling defect. The driver contract (cr_rda.h, DRIVER-CONTRACT.md) defines the codes; the bench disagrees with the core's own header.

## What our core does

The two wrong constants quoted above; the header constants are the ground truth (the CTL_CODE derivation is documented beside them).

## Gap

Falsifiable: run the bench against any driver implementing the contract — both calls return the unknown-IOCTL error; the reported strings carry the wrong codes. Grep `0x60002440` vs `0x6002440` across the repo confirms the bench is the only wrong-site family.

## Impact

The driver-verification bench is dead weight (cannot pass against a correct driver); the M-089 fuzz-gap discussion inherits a misleading artifact. Minor (tooling), but it is the exact phantom-zero class M-089 documents — the class has now surfaced inside this repo.

## Reproduction / verification

Executed-verification-class evidence: the constants compared side-by-side (bench vs header) at `34d6cf10`; the CTL_CODE(fn 0x910/0x912) derivation in the header's comment yields the header values. Static; no driver needed to prove the mismatch.

## Fix plan (for the engineering team)

1. Correct the two constants in `bench/ours/verify-driver.ps1` (0x60002440 → 0x6002440; 0x60002448 → 0x6002448).
2. Sweep the bench family for the same class (`grep -rn "0x60000\|0x600024" bench/`) and pin the codes against `cr_rda.h` in a comment (or generate them from the same source of truth if PowerShell can import it).
3. Add the sweep to the repo's script-lint if one exists (the ci-local gates).
4. Acceptance criteria: the bench's codes byte-match the header's; a grep for the phantom forms returns zero hits.

## Tests to add

- A CI grep gate (or a unit test on the bench script's constants if the harness allows): `Bench.IoctlCodesMatchContract`.

## Related findings

- M-089 (the driver-repo fuzz-gap finding whose phantom-zero class this is; the OQ-S10-3 boundary keeps the driver-side fix tracked — this finding covers the core-side residue).
- N-107/N-127 (the other driver-repo-resident UNCERTAINs — same boundary family).

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; S60-B lane)
**Verified by:** `doc_truth_gate` check 7 (the new `check_bench_ioctl_codes`) — RED pre-fix first-hand (exit 1, both violations named: `bench/ours/verify-driver.ps1:47 issues IOCTL 0x60002440` / `:55 issues IOCTL 0x60002448`, neither defined by any cr_rda.h constant) → GREEN post-fix (exit 0, "bench IOCTL literals match the contract (2 literal(s), all in cr_rda.h)"); the static side-by-side re-verified at the current tree (bench vs `include/cr/win/cr_rda.h:55-57`).
**Resolution notes:** Both phantom-zero constants corrected to the contract forms (0x60002440 → 0x6002440 kIoctlGetInfo fn 0x910; 0x60002448 → 0x6002448 kIoctlGetVersion fn 0x912), with an in-place comment pinning the codes to the header. The class guard the finding's own fix plan prescribed landed as doc_truth_gate check 7 (`check_bench_ioctl_codes`, CI-wired at checks.yml:173): every 0x6xxxxxxx-form CTL literal in bench/**/*.{ps1,cs,cpp} must byte-match a constant in include/cr/win/cr_rda.h — the repo-wide sweep found no third site (the two corrected literals are the census). The driver-repo side of M-089's phantom-zero class stays tracked under OQ-S10-3 per the grant boundary; this closes the core-repo half the finding owns.


---

## Addendum II — the S60-B convergence record (2026-10-29)

This lane independently verified and fixed the same two constants in the same
window (identical correction; the ledger's second row). Per SA-9 the S60
landing above is the fix of record. This lane's unique supplement rides its
own push: **doc_truth_gate check 7** (`check_bench_ioctl_codes`, CI-wired at
checks.yml:173) — every 0x6xxxxxxx-form CTL literal in bench/**/*.{ps1,cs,cpp}
must byte-match a constant defined in include/cr/win/cr_rda.h. RED exit-1
pre-fix first-hand (both violations named with file:line) → GREEN exit-0
post-fix. The class guard makes the phantom-zero defect a gated one for every
future bench script, beside the landed one-shot pin.
