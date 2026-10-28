# Session 9 — full parity audit prompt (token redacted)

Saved at session open per the standing SOP. Read twice before beginning.

**Mandate:** full parity audit of the merged repository (core engine + driver
+ bundled components: engines, algorithms, modules, helpers, threading,
hardcoded values/parameters, api formats, functions, features) against the
research corpus and og-codebase. Document every divergence in `diff-to-fix/`
in an organized, structured, in-depth format with citations, so future fixing
sessions can work efficiently.

**Repositories (confirmed by clone, this session):**
- Merged product (core engine, audited repo): https://github.com/heicfast/core_recovery
  — driver ships as prebuilt `rda.sys` release asset per `docs/PREBUILT-ARTIFACTS.md`
- Driver: https://github.com/heicfast/rda-driver (integrated with core; core uses it)
- Research corpus (ground truth): https://github.com/heicfast/easeus-drw-research
- GitHub token: REDACTED (env/git-config only, never committed; user rotates after session)

**Read-only.** Only writes permitted: new files under `diff-to-fix/` in the
audited repository, and `worklog.md` entries. No source code, headers, tests,
workflows, build files, or existing documentation modified.

**Scope is defined by the corpus, not by the prompt.** Surfaces and parameters
are discovered by reading the corpus end-to-end. Do not invent modules or
parameters; do not import a taxonomy from any prior session or prompt. If the
corpus documents a surface, it is in scope; if not, it is not.

**Reading protocol:** parallel agents A–E read both repositories end to end
(exhaustive, not sampled), each producing a structured context sheet in a
scratch area (not committed), consolidated before findings are written:
- A — corpus full inventory extraction (every doc, artifact, decompile, config,
  format artifact, installed og-codebase tree)
- B — corpus per-surface parameter extraction (behavior, states, constants,
  offsets, enums, error codes, threading, cancellation, persistence, defaults,
  thresholds, formats — with file+address citations)
- C — product repository public surface and orchestration (every header,
  source, test, workflow, build file, doc; every public/internal surface)
- D — product repository per-surface deep read (implementation, tests, docs,
  divergence record vs the corpus parameter matrix)
- E — independent verification (RE tooling / controlled fixtures; execution
  where feasible; UNVERIFIED-BY-EXECUTION marking otherwise; evidence in
  diff-to-fix/evidence/)

**Comparison methodology** — per surface × parameter verdicts:
MATCH / DIVERGENCE-ACUTE (wrong results, silent data loss, crashes, security
regressions, broken ABI, transport corruption, install-blocking) /
DIVERGENCE-SEVERE (significant behavioral divergence, missing feature, wrong
error semantics, state-machine mismatch, unbounded cancellation, uncontrolled
memory growth, user-observable difference) / DIVERGENCE-MAJOR (performance
gap, missing edge-case coverage, incomplete state handling, architectural
mismatch, format value differs) / DIVERGENCE-MINOR (cosmetic, naming drift,
doc drift, low-impact inefficiency) / DIVERGENCE-INFORMATIONAL (deliberate
difference, recorded not a defect) / UNVERIFIED (must be resolved in-session
by RE probe or execution, or moved to tracked-open/ with a precise question).
Only MATCH and DIVERGENCE-INFORMATIONAL are acceptable end states.

**Output structure** (extend, do not replace, prior audit work):
diff-to-fix/{README.md, SUMMARY.md, SEVERITY-SCALE.md, PARAMETERS.md,
SURFACE-INVENTORY.md, CROSS-REF-MATRIX.md, findings/{ACUTE,SEVERE,MAJOR,
MINOR,INFORMATIONAL}/, modules/, evidence/{decompiled-quotes/,executions/},
tracked-open/} — with the standing finding template (Severity, Module/Surface,
Parameters affected, Corpus references, Core references, Status: READ-ONLY
FINDING, Prior audit check, Summary, What the reference does, What our
implementation does, Gap, Impact, Reproduction/verification, Fix plan, Tests
to add, Related findings, Open questions).

**Finding IDs:** inspect existing findings/ + FIXED/ first; pick the next
available ID per severity; never reuse; never delete prior findings.

**Verification protocol (strongest first):** executed comparison; decompiled
evidence (exact snippet, file+address); corpus documentation evidence
(file+line range+quote); independent RE probe. Never assert a reference
behavior not verified. Never assert a surface/parameter the corpus does not
document. A false finding costs more than a missing one.

**Workflow:** long-horizon waves; commit+push after every coherent wave with
detailed messages; maintain worklog.md; pull frequently; record the audited
commit hash in README.md and every finding; do not duplicate prior findings
(expand instead); the matrix must not have two rows for the same divergence.

**Rules:** read-only; respect prior work; the corpus defines scope; evidence
or nothing; no hallucination; no shortcuts/no samples/no assumptions — read
line by line; consistent severity; actionable fix plans; SURFACE-INVENTORY.md
is what makes the roadmap auditable; work long-horizon; commit and push often.

**Deliverables:** SURFACE-INVENTORY.md, PARAMETERS.md, README/SUMMARY/
SEVERITY-SCALE/CROSS-REF-MATRIX, findings per template with cited evidence,
modules/*.md per-surface overviews, evidence/ artifacts, tracked-open/ for
unresolved UNVERIFIEDs, audited-commit citations everywhere, worklog.md
handoff with coverage stats + recommended first 20 fixes, all pushed, zero
source modifications.

**First TODOs:** T1 clone+record+save prompt → T2 parallel agents A–E →
T3 gate docs → T4 scaffolding → T5–T18 surface walk → T19 UNVERIFIED closure
→ T20 final verification sweep.
