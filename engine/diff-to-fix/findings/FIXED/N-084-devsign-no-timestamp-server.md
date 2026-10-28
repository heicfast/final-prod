# MINOR N-084 — the dev-sign sign step uses no timestamp server; the omission (and its rationale, or lack of one) is undocumented

**Severity:** MINOR
**Module / Surface:** driver dev-sign path (signing step hygiene/documentation)
**Parameters affected:** signature validity lifetime; signing-posture documentation
**Corpus references:** `easeus-drw-research/architecture/02-kernel-driver-architecture.md:19` — the reference's signatures are vendor Authenticode (timestamped by construction via the vendor's signing pipeline; the corpus documents the signed state, not the signing ceremony, so there is no reference parameter to diverge from here — this finding is a documentation-hygiene gap, not a parity divergence)
**Core references:** `.github/actions/install-driver/action.yml:64` — `& $signtool.FullName sign /fd SHA256 /f rda-ci.pfx /p rrecov-ci 'driver\rda.sys'` (no `/tr`, no `/t`); `docs/PREBUILT-ARTIFACTS.md` §2 (documents the sign command's shape without mentioning timestamping); `docs/DRIVER-RELEASE.md` §7 (test-signing posture summary — no timestamp statement); executed at run 40549660074 (job-log.txt:229-230 `Successfully signed`); audited commit `dd82db9`
**Status:** FIXED — see Resolution below
**Prior audit check:** not in prior scope

## Summary

The verified sign command (`signtool sign /fd SHA256 /f <pfx>`) omits a timestamp server (`/tr` or `/t`). For the job-local CI posture this is defensible — the certificate lives 2 days and the signature is only needed for the load-and-test window, and timestamping an ephemeral self-signed cert adds a network dependency for zero benefit — but the repo nowhere records that reasoning, and the recipe is explicitly offered as the pattern to copy ("In-tree example of the CI test-signing recipe to copy", DRIVER-RELEASE.md §7). A consumer copying it for a longer-lived dev cert (e.g. a team CA valid a year) will produce signatures that silently become invalid when the cert expires, on machines that were fine at install time.

## What the reference does

The corpus documents the reference drivers as Authenticode-signed by the vendor (:19). The vendor pipeline timestamps by construction (standard Authenticode ceremonies do); the corpus does not document the ceremony, so there is no corpus parameter asserting or forbidding timestamping — the gap is internal to our own posture documentation, which is why this is MINOR and not a parity finding.

## What our implementation does

The composite signs with `/fd SHA256` only (action.yml:64). The PREBUILT-ARTIFACTS §2 prose describing the posture ("test-signed with a job-local self-signed cert ... `signtool sign /fd SHA256`") and DRIVER-RELEASE §7's copy-me note both reproduce the timestampless form without a word about validity lifetime. No doc states: (a) why the CI form omits `/tr` (ephemeral cert, no value), (b) when a consumer MUST add one (any cert whose validity outlives the machine's need to keep trusting the signature — i.e. any shared/team dev cert or a driver binary kept installed past cert expiry).

## Gap

Falsifiable claim: `rg -i "timestamp|/tr |/t " .github/actions docs/PREBUILT-ARTIFACTS.md docs/DRIVER-RELEASE.md` yields no timestamp-server reference or rationale anywhere in the dev-sign chain, while the sign command is documented as a copyable recipe.

## Impact

- Low, and bounded: CI loads are unaffected (signature verified at load time while the cert is valid and trusted); a dev who copies the recipe with a long-lived cert gets time-bombed signatures — a machine that loads the driver today may refuse after cert expiry with a confusing signature error.
- Documentation hygiene: the recipe's one non-obvious omission is silent.

## Reproduction / verification

Source evidence (strength-3): the quoted command line (action.yml:64), the two docs describing it, and the executed transcript (job-log.txt:230 — `Successfully signed: driver\rda.sys`, signtool's standard output when no timestamp is requested; with `/tr` the tool prints the timestamp line). Grep for timestamp mentions across the chain → none.

## Fix plan (for the engineering team)

1. One sentence in `docs/PREBUILT-ARTIFACTS.md` §2 + the DRIVER-RELEASE §7 copy-me note: "The CI sign omits a timestamp server because the job-local cert outlives the run; when signing with any cert whose validity exceeds the test window, add `/tr <tsa> /td SHA256` (e.g. DigiCert's or Sectigo's public RFC 3161 TSA) so the signature survives cert expiry."
2. Optionally parameterize the composite with a `timestamp_server` input, empty by default (CI posture unchanged), consumed when present.
3. Acceptance criteria: the docs state the rule; the composite input exists or the docs-only fix is accepted.

## Tests to add

- If parameterized: a drill sign with `/tr` against a public TSA asserting `signtool verify /pa /all` passes post-expiry (synthetic: verify the countersignature structure, since real expiry can't be waited out in CI).

## Related findings

- **M-063** — the environment matrix the timestamp note belongs next to.
- **I-026** — the signing posture frame.

## Open questions

None.

## Resolution (FIXED — documented + a composite input)

`docs/PREBUILT-ARTIFACTS.md` §2 and `docs/DRIVER-RELEASE.md` §7 now carry the
one-sentence rationale the finding asked for: the sign step omits `/tr`
because the job-local cert outlives the job and nothing needs the signature
afterwards; ANY cert whose signature must outlive its validity window (a
real release cert, the OSSign chain) requires `/tr <TSA> /td SHA256`.
The composite gained the `timestamp_server:` input (default empty = the
current behavior; non-empty routes the signtool call through
`/tr <TSA> /td SHA256`) — the optional input half of the fix plan.

**Landed:** core_recovery "fix(ci): S-036 close-out — ... (timestamp_server
input, N-084)".
