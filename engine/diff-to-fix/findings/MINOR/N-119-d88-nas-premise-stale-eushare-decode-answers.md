# MINOR N-119 — PARITY-DELTAS D-88's corpus-evidence premise is stale: the EuShare artifact DOES document the NAS scan's recovery semantics (the SSH-root agent's on-box bitmap scan serving results) — the row's recorded revisit trigger has fired unnoticed

**Severity:** MINOR
**Module / Surface:** docs (PARITY-DELTAS) / the NAS scope registration
**Parameters affected:** 46 (contract documentation), A1-adjacent (the registration's evidence quality)
**Corpus references:** `easeus-drw-research/re-workspace/artifacts/EuShare/eumrx-nas-architecture.md:10-21` (the 3-layer NAS model: SSH-root agent push to /tmp/EaseUS doing the ON-BOX BITMAP-BASED SCAN and serving results — the decode D-88 said the corpus lacked); eumrx-rdbss-internals--minirdr-dispatch.md (the redirector's dispatch).
**Core references:** audited commit `ff879f9` — `docs/PARITY-DELTAS.md:2917-2934` (D-88: "whether NAS scan actually recovers deleted files from shares … the corpus does not answer" + the revisit trigger "a corpus decode of the EuMrx scan's actual recovery semantics" — **both now answerable from the EuShare artifacts**, which the row never cites; M-034's cites cover only architecture/05 + DRWUI-exe).
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** D-88 registered the NAS exclusion with an honest unknown; the EuShare artifacts landed in the corpus before this session and were never consumed by the row.

## Summary

('D-88 excludes NAS/remote scanning (a defensible product-scope stance) but justifies part of it with "the corpus does not answer whether the NAS scan recovers deleted files" — the EuShare artifact answers exactly that (the agent does an on-box bitmap scan and serves the results), and the row\'s own revisit trigger ("a corpus decode of the EuMrx scan\'s recovery semantics") has been satisfied without anyone re-adjudicating. The exclusion itself may stand (host-layer NAS is still the posture); the evidence field and the trigger must be refreshed so the register stays truthful.',)

## What the reference does

The EuShare architecture artifact (cited above) documents the agent-based on-box scan.

## What our core does

D-88's WHAT-OG-DOES/WHY field as cited.

## Gap

Falsifiable: read D-88's evidence field vs the EuShare artifact's :10-21 content.

## Impact

Register truthfulness (the single-source-of-truth discipline); a future audit would otherwise re-file this.

## Reproduction / verification

Read both this session.

## Fix plan (for the engineering team)

1. Update D-88's evidence field to cite the EuShare artifacts and re-adjudicate the trigger: the exclusion now rests on the host-layer/product-scope rationale alone (the 'corpus is silent' leg is gone). 2. Record the re-adjudication date + this finding's ID in the row.

## Tests to add

None (doc fix).

## Related findings

N-120 (the other registration/pointer drifts); GAP-SWEEP context sheet (this session's evidence).

## Open questions

None.

---

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-119 doc-truth batch — the session-38 minor-first lane)
**Verified by:** the EuShare artifact re-read first-hand before the edit: eumrx-nas-architecture.md:10-21 documents the 3-layer model (SSH-root agent push to /tmp/EaseUS, the ON-BOX BITMAP-BASED SCAN, results served back) — exactly the decode the finding says D-88 lacked.
**Resolution notes:** Re-verified at current main: D-88 (PARITY-DELTAS:2894+) still justified leg (2) of its WHY with "the corpus does not answer" while its own revisit trigger ("a corpus decode of the EuMrx scan's actual recovery semantics") had fired unnoticed via the EuShare artifacts. The row is now re-adjudicated: leg (2) struck with the adjudication note (the corpus answers the semantics — the og's deleted-file yield on shares rides the on-box agent's block access, not a client-side raw-SMB path), the RISK field's "No such device class is documented" clause replaced with the truthful on-box-agent statement, and the revisit trigger marked FIRED-and-adjudicated with the standing rule (the exclusion rests on the host-layer/product-scope rationale; re-file only if the product ever wants an on-box agent architecture). The exclusion itself is unchanged — the evidence field and trigger are now truthful, which is what the finding demanded.
