
## Session-81 addendum — N-216: the MaxFileSize cap decoded and landed (2026-10-29)

**The consumer decode (the finding's open question, closed this
session):** the +0xa0 cap field on the scan-task config (ctor
FUN_140478d10, DRWUI:749901-749903) is read at **DRWUI:750172-175**
inside the tree-builder that also consumes FilterFolder.ini's name pool
(the StrStrIA gate at :750147-150 in the same function): the node's size
accessor `FUN_140089170` compared against the cap; `cap < size →
FUN_140088f40(node, 0)` — the node is HIDDEN from the results tree
through the same visibility clear the junk-name filter uses. A
tree-visibility size cap, not a scan gate, not display truncation. The
shipped ini carries no key (read first-hand) → the 1 TiB default
governs — the og's corrupt-size guard (torn petabyte sizes do not flood
the tree). **Landing:** the predicate in `filterfolder` (M-018's seam —
the og consumes both gates in the same tree-builder function):
`max_file_size()` default `0x400 << 30`, `set_max_file_size()` (the
per-host tunable — the og's version.ini `[ver]` "MaxFileSize " key, GiB
units), `size_exceeds_cap()`, and `apply()` flags oversize nodes
NODE_FLAG_JUNK_FILTERED (the presentation surfaces skip flagged nodes —
the visibility semantics). One registered divergence: `set(0)` = OFF
(the og has no 0 semantics — minimum 1 GiB via the << 0x1e shift);
PARITY-DELTAS row D-150. Tests:
FilterFolder.MaxFileSizeDefaultIsOneTebibyte / SizeCapHidesOversizeNode
(the 2 TiB node flags; the exact-cap boundary stays — the og's strict
compare) / SizeCapHostOverrideRoundTrip. Suite 1621/1621. The #16/#46
rows: MATCH.
