/*
 * cr_extractplan.h - the structure-preserving extract plan (F-00d/F-01,
 * the named feature: "reconstructed files with original directory
 * structure preserved, deleted only, as a post-scan option").
 *
 * Pure-core planner over a stable ResultTree (post-scan) or a Snapshot
 * (live scan). SectorForge-parity semantics, re-implemented from the
 * documented behavior (SECTORFORGE-FEATURES.md sheet section 3, citing
 * crates/sf-core/src/ntfs/extract.rs + sanitize.rs - no code copied):
 *
 *   - deleted-only selection with the listing fast-path (SF recover.rs
 *     :241-243 "quicker on very large volumes"): an in-use subtree that
 *     contains no deleted descendant is pruned whole, in one pass;
 *   - original directory structure via the ancestry chain (a deleted
 *     file under a still-live folder keeps its full original path - the
 *     live ancestors become path components only, never items);
 *   - orphans (nodes under the "File Path Lost" root / LostDir groups)
 *     land under "_orphaned/" and the TOP of every orphaned chain gets
 *     its id appended - "orphan roots also get their id, so two
 *     orphaned 'Documents' folders don't merge" (extract.rs:199-207) -
 *     files as "name (id N).ext", dirs as "name (id N)/" with the real
 *     relative structure preserved beneath;
 *   - every emitted component is sanitized with safe_component_sf()
 *     (reserved device names, trailing dots/spaces, control chars,
 *     UTF-16 cap 240 preserving a short extension);
 *   - items are emitted parents-before-children (mkdir order), the walk
 *     is depth-capped, and carved/raw-origin nodes are excluded (they
 *     carry no original structure - the raw-category flow owns them).
 *
 * The plan never reads content and never pulls metadata implicitly; the
 * SafeWriter consumes the items (dir items materialize structure, file
 * items stream content) with the SF collision scheme armed
 * (WriterConfig::CollisionScheme::SfId).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_EXTRACTPLAN_H_
#define CR_EXTRACTPLAN_H_

#include "../tree/cr_node.h"
#include <string>
#include <vector>

namespace cr {
namespace recovery {

/* SectorForge-parity component sanitizer (F-01; SF sanitize.rs:11-36
 * behavior, re-derived - sheet section 3). Rules, applied to ONE path
 * component:
 *   - backslash, slash, colon, star, question, quote, angle brackets,
 *     pipe and control chars (< 0x20) become '_';
 *   - "." becomes "_", ".." becomes "__" (neutralized, never a ref);
 *   - trailing dots and spaces become '_' (Windows silently strips
 *     them, which would make different names collide);
 *   - empty becomes "_";
 *   - reserved device names - CON, PRN, AUX, NUL, CONIN$, CONOUT$,
 *     COM1-9 (incl. the superscript-1/2/3 spellings), LPT1-9 - are
 *     prefixed with '_' (checked against the part before the first
 *     dot, so "CON.txt" is neutralized too);
 *   - the result is truncated to `max_units` UTF-16 units (SF: 240)
 *     preserving a short (<= 16 unit) extension when one is present;
 *   - a truncation that leaves a trailing dot/space replaces it with
 *     '_'.
 * Identical rules on every OS, "so a recovery made on Linux can still
 * be copied onto a Windows machine" (SF sanitize.rs:1-5). */
std::u16string sanitize_component_sf(const std::u16string& in, u16 max_units = 240);

/* Insert " (id N)" before the extension (SF with_id_suffix,
 * sanitize.rs:66-71): "report.docx" -> "report (id 17).docx";
 * extensionless names append " (id N)". The extension splits at the
 * LAST dot (dot-at-position-0 is not an extension). */
std::u16string with_id_suffix_sf(const std::u16string& name, u64 id);

namespace extractplan {

struct PlanOptions {
    /* F-31/SF recover.rs:241-243: skip in-use entries. When true (the
     * named feature's posture) an Active subtree with no deleted
     * descendant is pruned whole and Active nodes never become items
     * (they can still appear as path components - the ORIGINAL
     * directory structure is the deleted file's full ancestry). */
    bool deleted_only = true;
    /* Carved/raw-origin nodes carry no original structure (the
     * raw-category flow owns them); the named feature excludes them.
     * Set false to include everything regardless of origin. */
    bool exclude_carved = true;
    /* SF extract.rs:14 ORPHANS = "_orphaned" - the dest-side base for
     * nodes whose ancestry is broken ("File Path Lost" root and its
     * LostDir groups). Sanitized once at plan start. */
    std::u16string orphan_base = u"_orphaned";
    /* SF UTF-16 component cap (sanitize.rs). */
    u16 max_component_units = 240;
    /* SK-style defensive depth cap for the walk (sheet SK section 10
     * family; the tree is acyclic by construction - this is the
     * belt-and-suspenders bound). */
    u32 max_depth = 128;
};

struct PlanItem {
    /* exactly one of the two is set (live-tree plan vs snapshot plan) */
    const tree::FileNode* node = nullptr; /* live tree (post-scan) */
    u32 snap_index = 0xFFFFFFFFu;         /* snapshot row (live scan) */
    /* destination-relative directory, '' = output root, else with a
     * trailing separator; components are sanitize_component_sf-clean */
    std::u16string rel_dir;
    /* final component (sanitized; WITHOUT collision resolution - the
     * writer's SfId scheme owns that) */
    std::u16string name;
    bool is_dir = false;
    /* the top of an orphaned chain: name already carries the
     * " (id N)" suffix (dir roots and file tops alike) */
    bool orphan_root = false;
    /* the node's stable id (node_id / SnapNode.node_id) - the writer's
     * SfId collision suffix uses the same value */
    u64 id = 0;
    /* content truth for the preview (files only) */
    u64 logical_size = 0;
};

struct PlanStats {
    u64 files = 0;           /* file items emitted */
    u64 dirs = 0;            /* deleted dir items emitted */
    u64 orphan_files = 0;    /* file items under _orphaned/ */
    u64 orphan_dirs = 0;     /* dir items under _orphaned/ */
    u64 bytes_total = 0;     /* logical bytes over file items */
    u64 pruned_active = 0;   /* in-use nodes visited then skipped
                              * (F-31 accounting; components of
                              * deleted paths are NOT counted here -
                              * they were used, not pruned) */
    u64 pruned_subtrees = 0; /* in-use roots/branches dropped whole */
    u64 carved_excluded = 0; /* carved/raw-origin nodes excluded */
};

/* Plan over a stable ResultTree (post-scan: the tree is complete and
 * immutable for the plan's duration - the caller holds the session).
 * Items are appended to `out` parents-before-children. Never fails on
 * content (content is not read); structural hostility (oversized
 * depth) truncates the walk and is reflected in the stats the caller
 * can check (pruned_subtrees grows by the dropped branch count). */
PlanStats plan_deleted_tree(const tree::ResultTree& tree, const PlanOptions& opt,
                            std::vector<PlanItem>& out);

/* Snapshot variant (live scans; G-02b value-path parity). Same rules;
 * items carry snap_index and the snapshot's node_id. */
PlanStats plan_deleted_snapshot(const tree::Snapshot& snap, const PlanOptions& opt,
                                std::vector<PlanItem>& out);

/* true when the FullNode role string marks a lost/orphan root (the
 * orchestrator creates "File Path Lost"; "Lost Files" is the
 * resume-recognized alternate - cr_scan.cpp's role matcher). */
bool is_lost_root_role(const std::u16string& role);

} // namespace extractplan
} // namespace recovery
} // namespace cr

#endif // CR_EXTRACTPLAN_H_
