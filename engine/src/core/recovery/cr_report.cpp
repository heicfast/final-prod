/* cr_report.cpp - recovery report JSON serializer (G-09, R7.3).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/recovery/cr_report.h"
#include <cstdio>
#include <cstring>

#ifndef CR_ENGINE_VERSION
#define CR_ENGINE_VERSION "0.0.0-dev"
#endif

namespace cr {
namespace recovery {
namespace report {

const char* engine_version() {
    return CR_ENGINE_VERSION;
}

namespace {

/* Emit one byte of a UTF-16 code unit as \\uXXXX (controls only). */
void emit_control(std::string& out, unsigned v) {
    char buf[8];
    snprintf(buf, sizeof(buf), "\\u%04x", v & 0xFFFFu);
    out.append(buf);
}

} // namespace

std::string escape_json(const std::string& in) {
    std::string out;
    out.reserve(in.size() + 8);
    /* byte-wise scan: structural chars are ASCII; UTF-8 sequences pass
     * through untouched EXCEPT invalid bytes, which become U+FFFD so the
     * document stays valid JSON (a recovered name may be any byte soup) */
    for (size_t i = 0; i < in.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(in[i]);
        switch (c) {
        case '"':
            out.append("\\\"");
            break;
        case '\\':
            out.append("\\\\");
            break;
        case '\b':
            out.append("\\b");
            break;
        case '\f':
            out.append("\\f");
            break;
        case '\n':
            out.append("\\n");
            break;
        case '\r':
            out.append("\\r");
            break;
        case '\t':
            out.append("\\t");
            break;
        default:
            if (c < 0x20) {
                emit_control(out, c);
            } else if (c < 0x80) {
                out.push_back(char(c));
            } else if (c < 0xC0) {
                /* stray continuation byte: not valid UTF-8 by itself */
                out.append("\xEF\xBF\xBD"); /* U+FFFD */
            } else if (c < 0xE0) {
                if (i + 1 < in.size() && (static_cast<unsigned char>(in[i + 1]) & 0xC0) == 0x80) {
                    out.push_back(char(c));
                    out.push_back(in[++i]);
                } else {
                    out.append("\xEF\xBF\xBD");
                }
            } else if (c < 0xF0) {
                if (i + 2 < in.size() && (static_cast<unsigned char>(in[i + 1]) & 0xC0) == 0x80 &&
                    (static_cast<unsigned char>(in[i + 2]) & 0xC0) == 0x80) {
                    out.push_back(char(c));
                    out.push_back(in[++i]);
                    out.push_back(in[++i]);
                } else {
                    out.append("\xEF\xBF\xBD");
                }
            } else {
                if (i + 3 < in.size() && (static_cast<unsigned char>(in[i + 1]) & 0xC0) == 0x80 &&
                    (static_cast<unsigned char>(in[i + 2]) & 0xC0) == 0x80 &&
                    (static_cast<unsigned char>(in[i + 3]) & 0xC0) == 0x80) {
                    out.push_back(char(c));
                    out.push_back(in[++i]);
                    out.push_back(in[++i]);
                    out.push_back(in[++i]);
                } else {
                    out.append("\xEF\xBF\xBD");
                }
            }
            break;
        }
    }
    return out;
}

namespace {

void append_num(std::string& out, const char* key, u64 v, bool comma) {
    if (comma)
        out.push_back(',');
    out.append("\"");
    out.append(key);
    out.append("\":");
    char buf[32];
    snprintf(buf, sizeof(buf), "%llu", (unsigned long long)v);
    out.append(buf);
}

void append_str(std::string& out, const char* key, const std::string& v, bool comma) {
    if (comma)
        out.push_back(',');
    out.append("\"");
    out.append(key);
    out.append("\":\"");
    out.append(escape_json(v));
    out.push_back('"');
}

const char* outcome_name(Outcome o) {
    switch (o) {
    case Outcome::Done:
        return "done";
    case Outcome::Refused:
        return "refused";
    case Outcome::Error:
        return "error";
    case Outcome::HashMismatch:
        return "hash_mismatch";
    case Outcome::Resumed:
        return "resumed";
    case Outcome::Pending:
        return "pending";
    case Outcome::Salvaged:
        return "salvaged";
    case Outcome::NothingToRecover:
        return "nothing_to_recover";
    }
    return "error";
}

const char* hash_status_name(HashStatus s) {
    switch (s) {
    case HashStatus::NotComputed:
        return "not_computed";
    case HashStatus::Computed:
        return "computed";
    case HashStatus::Verified:
        return "verified";
    case HashStatus::Mismatch:
        return "mismatch";
    }
    return "not_computed";
}

} // namespace

static void append_analysis(std::string& out, const ScanAnalysis& a) {
    out.append("  \"scan_analysis\": {\n");
    append_num(out, "total_file_count", a.total_file_count, false);
    out.append(",\n");
    append_num(out, "raw_count", a.raw_count, false);
    out.append(",\n");
    append_num(out, "lost_partition_exist_file_count", a.lost_partition_exist_file_count, false);
    out.append(",\n");
    append_num(out, "lost_partition_delete_file_count", a.lost_partition_delete_file_count, false);
    out.append(",\n");
    append_num(out, "quick_scan_exist_file_count", a.quick_scan_exist_file_count, false);
    out.append(",\n");
    append_num(out, "quick_scan_delete_file_count", a.quick_scan_delete_file_count, false);
    out.append(",\n");
    append_num(out, "deep_scan_file_count", a.deep_scan_file_count, false);
    out.append(",\n");
    append_num(out, "quick_scan_delete_extend", a.quick_scan_delete_extend, false);
    out.append(",\n");
    static const char* kCatNames[8] = {"image",   "video", "audio", "document",
                                       "archive", "other", "email", "bookmarks"};
    out.append("    \"categories\": {");
    for (size_t i = 0; i < 8; ++i) {
        out.append(i ? ", " : "");
        out.append("\"");
        out.append(kCatNames[i]);
        out.append("\": ");
        out.append(std::to_string(a.category_counts[i]));
    }
    out.append("},\n");
    append_num(out, "matched", a.matched, false);
    out.append(",\n");
    append_num(out, "total", a.total, false);
    out.append(",\n");
    append_num(out, "small_result", a.small_result ? 1 : 0, false);
    out.append(",\n");
    append_num(out, "gauge_matched", a.gauge_matched, false);
    out.append(",\n");
    append_num(out, "gauge_total", a.gauge_total, false);
    out.append(",\n");
    append_num(out, "score", a.score, false);
    out.append(",\n");
    append_num(out, "ntfs_device_index_high", a.ntfs_device_index_high, false);
    out.append("\n  },\n");
}

std::string build_report(const std::vector<Item>& items, const Totals& t,
                         const ScanAnalysis* analysis) {
    std::string out;
    out.reserve(256 + items.size() * 160);
    out.append("{\n");

    /* schema + identities */
    out.append("  \"schema_version\": 1,\n");
    append_str(out, "engine_version", engine_version(), false);
    out.append(",\n");
    append_str(out, "source", t.source, false);
    out.append(",\n");
    append_str(out, "destination", t.destination, false);
    out.append(",\n");
    append_num(out, "elapsed_ms", t.elapsed_ms, false);
    out.append(",\n");

    /* totals */
    out.append("  \"totals\": {");
    append_num(out, "queued", t.queued, false);
    append_num(out, "done", t.done, true);
    append_num(out, "dirs", t.dirs, true);
    append_num(out, "refused", t.refused, true);
    append_num(out, "errors", t.errors, true);
    append_num(out, "hash_mismatch", t.hash_mismatch, true);
    append_num(out, "resumed", t.resumed, true);
    append_num(out, "salvaged", t.salvaged, true);
    append_num(out, "nothing_to_recover", t.nothing_to_recover, true);
    append_num(out, "pending", t.pending, true);
    append_num(out, "bytes", t.bytes, true);
    /* N-159: the og's failed-bytes split (CSaveFailedFilesWithHtml's
     * +0x268 accumulator) - additive-within-v1 per the schema doc. */
    append_num(out, "failed_bytes", t.failed_bytes, true);
    out.append("},\n");

    /* S-023: the optional scan-analysis block (the og's RecoveryAssistant
     * statistics; the og emits them through its UI/log, we surface them in
     * the report schema per the finding's fix plan). */
    if (analysis)
        append_analysis(out, *analysis);

    /* per-item outcomes (queue order; item_id monotonic) */
    out.append("  \"items\": [");
    for (size_t i = 0; i < items.size(); ++i) {
        const Item& it = items[i];
        out.append(i ? ",\n   {" : "\n   {");
        append_num(out, "item_id", it.item_id, false);
        append_str(out, "path", it.path, true);
        append_str(out, "name", it.name, true);
        append_str(out, "kind", it.is_dir ? "dir" : "file", true);
        append_str(out, "outcome", outcome_name(it.outcome), true);
        append_num(out, "reason", it.reason, true);
        append_num(out, "bytes", it.bytes, true);
        append_num(out, "duration_ms", it.duration_ms, true);
        append_str(out, "sha256", it.sha256_hex, true);
        append_str(out, "hash_status", hash_status_name(it.hash_status), true);
        /* N-159: the og's per-row tree classification (the failed-files
         * HTML report's classification column, decoded at DRWUI
         * :642755-642822). Emitted for EVERY row - the og surfaces it
         * only on its failed-files funnel (D13), the core's report is
         * the structured superset. Omitted when empty (additive-
         * within-v1; unclassified value-enqueued rows). */
        if (!it.tree_class.empty())
            append_str(out, "tree_class", it.tree_class, true);
        /* S-060: the metadata restoration outcome per row - "restored",
         * "failed:<code>", "unsupported" (a metadata-less backend), or
         * omitted when not attempted (dirs, refused items). The og
         * traces both restoration failures and continues; the core's
         * report tells the same truth per row. */
        if (!it.metadata.empty())
            append_str(out, "metadata", it.metadata, true);
        out.push_back('}');
    }
    out.append(items.empty() ? "]\n}\n" : "\n  ]\n}\n");
    return out;
}

} // namespace report
} // namespace recovery
} // namespace cr
