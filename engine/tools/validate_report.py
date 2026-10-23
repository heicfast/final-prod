#!/usr/bin/env python3
"""validate_report.py - recovery report JSON schema checker (G-09).

Validates a report emitted by recovery::report::build_report against
docs/recovery-report-schema.md v1: required keys, enum spellings, types,
sha256 shape, and items/queued consistency. Exits 1 with a message on the
first violation.

Usage: python3 tools/validate_report.py <report.json> [more.json ...]
"""

import json
import re
import sys

SHA256 = re.compile(r"^[0-9a-f]{64}$")
INT_KEYS = {"elapsed_ms"}
TOTALS_KEYS = {
    "queued", "done", "dirs", "refused",
    "errors", "hash_mismatch", "resumed", "bytes",
}
OUTCOMES = {"done", "refused", "error", "hash_mismatch", "resumed", "pending"}
HASH_STATUS = {"verified", "computed", "mismatch", "not_computed"}
ITEM_KEYS = {
    "item_id", "path", "name", "kind", "outcome", "reason",
    "bytes", "duration_ms", "sha256", "hash_status",
}


def fail(path, msg):
    print(f"{path}: schema violation: {msg}")
    sys.exit(1)


def check(path, doc):
    if not isinstance(doc, dict):
        fail(path, "root is not an object")
    for key in ("schema_version", "engine_version", "source", "destination",
                "elapsed_ms", "totals", "items"):
        if key not in doc:
            fail(path, f"missing key {key!r}")
    if doc["schema_version"] != 1:
        fail(path, f"schema_version {doc['schema_version']!r} != 1")
    for key in ("engine_version", "source", "destination"):
        if not isinstance(doc[key], str):
            fail(path, f"{key!r} is not a string")
    for key in INT_KEYS:
        if not isinstance(doc[key], int) or doc[key] < 0:
            fail(path, f"{key!r} is not a non-negative integer")

    totals = doc["totals"]
    if not isinstance(totals, dict):
        fail(path, "totals is not an object")
    for key in TOTALS_KEYS:
        if key not in totals:
            fail(path, f"totals missing {key!r}")
        if not isinstance(totals[key], int) or totals[key] < 0:
            fail(path, f"totals.{key} is not a non-negative integer")

    items = doc["items"]
    if not isinstance(items, list):
        fail(path, "items is not an array")
    if len(items) != totals["queued"]:
        fail(path, f"items count {len(items)} != totals.queued {totals['queued']}")
    for i, it in enumerate(items):
        if not isinstance(it, dict):
            fail(path, f"items[{i}] is not an object")
        for key in ITEM_KEYS:
            if key not in it:
                fail(path, f"items[{i}] missing {key!r}")
        for key in ("item_id", "reason", "bytes", "duration_ms"):
            if not isinstance(it[key], int) or it[key] < 0:
                fail(path, f"items[{i}].{key} is not a non-negative integer")
        for key in ("path", "name", "kind", "outcome", "sha256", "hash_status"):
            if not isinstance(it[key], str):
                fail(path, f"items[{i}].{key} is not a string")
        if it["outcome"] not in OUTCOMES:
            fail(path, f"items[{i}].outcome {it['outcome']!r} not in {sorted(OUTCOMES)}")
        if it["hash_status"] not in HASH_STATUS:
            fail(path, f"items[{i}].hash_status {it['hash_status']!r} not in "
                      f"{sorted(HASH_STATUS)}")
        if it["kind"] not in ("file", "dir"):
            fail(path, f"items[{i}].kind {it['kind']!r} not file/dir")
        sha = it["sha256"]
        if sha and not SHA256.match(sha):
            fail(path, f"items[{i}].sha256 {sha!r} is not 64 lowercase hex digits")
        if it["hash_status"] == "not_computed" and sha:
            fail(path, f"items[{i}] hash_status not_computed but sha256 present")
        if it["hash_status"] != "not_computed" and not sha:
            fail(path, f"items[{i}] hash_status {it['hash_status']!r} but sha256 empty")
        if it["kind"] == "dir" and it["hash_status"] != "not_computed":
            fail(path, f"items[{i}] dir row must be hash_status not_computed")
    print(f"{path}: OK ({len(items)} item(s))")


def main():
    if len(sys.argv) < 2:
        print("usage: validate_report.py <report.json> [...]", file=sys.stderr)
        return 2
    for path in sys.argv[1:]:
        with open(path, "r", encoding="utf-8") as f:
            check(path, json.load(f))
    return 0


if __name__ == "__main__":
    sys.exit(main())
