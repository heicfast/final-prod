# INFORMATIONAL I-026 — the driver ships unsigned-by-design with CI/dev test-signing (vs the reference's vendor Authenticode chain): deliberate, documented, and now execution-verified

**Severity:** INFORMATIONAL
**Module / Surface:** driver signing posture (packaging surface)
**Parameters affected:** release artifact signature state; production signing pipeline (OSSign, prepared + disabled); dev/CI signing (job-local cert)
**Corpus references:** `easeus-drw-research/architecture/02-kernel-driver-architecture.md:19` — all six reference drivers are "Authenticode-signed (EaseUS = 'Chengdu Yiwo Tech Development Co., Ltd')"; `:26-27` WinPE deployment "where no driver-signing enforcement issues exist" (the reference's answer to enforcement variance)
**Core references:** `docs/DRIVER-RELEASE.md` §7 (the posture's full statement: `SignMode=Off`, release asserts unsigned, OSSign pipeline prepared-but-disabled pending onboarding, OSSign ≠ Microsoft kernel signature, WinPE relaxed); `docs/PREBUILT-ARTIFACTS.md` §2 ("Signing posture is documented truthfully per release (DRIVER-RELEASE §7: unsigned-by-design artifacts, CI test-signs for load)"); `.github/actions/install-driver/action.yml:44-64` (the verified test-sign mechanism); executed at run 40549660074 (job-log.txt:229-230); audited commit `dd82db9`
**Status:** READ-ONLY FINDING — no fix applied (deliberate divergence, recorded)
**Prior audit check:** not in prior scope (signing posture was not a prior axis)

## Summary

The reference ships its drivers vendor-signed; the merged product ships its driver **unsigned by design** and signs at consumption time (job-local cert in CI, documented test-sign recipe for dev, a prepared OSSign pipeline for production that stays disabled until certificate onboarding). This is a deliberate, documented divergence — reproducible byte-comparable artifacts, signing authority kept in the release pipeline — and this session's validation run (40549660074) now also execution-proves the consumption-time half: the unsigned release binary, test-signed on the runner, loads and answers its control interface.

## What the reference does

Corpus: the og driver family is Authenticode-signed by "Chengdu Yiwo Tech Development Co., Ltd" (02-kernel-driver-architecture.md:19) — a vendor chain that satisfies live-Windows enforcement of its era, with per-context binaries (Win8+ / Win7- / WinPE-preloaded) where enforcement allowed relaxation (:26-27).

## What our implementation does

`docs/DRIVER-RELEASE.md` §7 states the full posture and its reasons: the build never signs (`SignMode=Off`) so release artifacts stay byte-comparable; CI and release both *assert* the artifact is unsigned; GitHub Releases carry the unsigned `rda.sys` with release-body text saying so; consumers test-sign (the verified job-local mechanism) or enable OS test-signing; the OSSign production pipeline exists but is disabled pending repo variables + a protected environment; and the docs are explicit that an OSSign signature is standard Authenticode, not a Microsoft kernel signature — Win10/11 retail still needs Hardware Dev Center attestation/HLK, Win7/8/8.1 accept the chain, WinPE is relaxed. Run 40549660074 verifies the load path end to end (sign → install → RUNNING → control-device round-trip → 5-facet ready).

## Gap

None — the difference from the reference is deliberate and documented. Recorded so future sessions know the signing posture was audited, not missed: the divergence is the absence of a vendor-grade signature on shipped artifacts, consciously deferred to the OSSign onboarding milestone.

## Impact

- None at dev/CI posture (proven by execution).
- Production retail-Windows loading remains blocked on attestation/HLK regardless of OSSign (the docs already say so; M-063 tracks the environment matrix).

## Reproduction / verification

Executed (strength-1): run 40549660074 — unsigned release asset (the release's own `assert unsigned` gate upstream), test-signed on the runner (`Successfully signed: driver\rda.sys`), loaded (`STATE: 4 RUNNING`), control interface green. Corpus citation for the reference posture: :19, :26-27.

## Fix plan (for the engineering team)

None required. (The OSSign onboarding milestone, when it happens, supersedes this record; the attestation/HLK track for retail Win10/11 kernel loading is the real production gate and is already truthfully documented.)

## Tests to add

None (the existing release-assert-unsigned gate + the CI load lane are the tests).

## Related findings

- **M-063** — the environments matrix this posture lands in.
- **N-084** — the timestamp note for the copyable recipe.

## Open questions

None.
