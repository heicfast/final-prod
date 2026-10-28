# Session-52 diff-to-fix prompt (token redacted)

## IM Chat Context (gateway metadata)
- session_id: web-7a7f5b7d-192f-48c1-9e3b-3fd2717d7c3f
- chat_id: e98e4dff-86e4-4ed1-944a-aa23442658ba
- channel: zai-web
- trace_id: 1a1849dc65e0ae39

## Mandate (abridged, token redacted)

Senior systems engineer mandate: execute the roadmap in `core_recovery/diff-to-fix/`.
For every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, and TRACKED-OPEN —
independently verify the finding, fix it in the codebase, prove the fix with tests
and benchmarks, and mark it fixed in the audit trail with evidence. The audit is a
hypothesis; every finding re-verified against the research corpus and the running
code before any change. A false finding is corrected in the audit trail rather than
"fixed" by breaking good code.
