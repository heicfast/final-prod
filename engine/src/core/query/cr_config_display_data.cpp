/* cr_config_display_data.cpp - the og's two display-config data layers
 * (N-132): the FilterFolder.ini SumaryFileType unification groups and the
 * PriorityDisplayType.xml Type-tree ordering.
 *
 * Sources (verbatim, byte-exact):
 *   - og-codebase/config/FilterFolder.ini [Filter] SumaryFileType
 *     ("aiff_aif;m4p_m4a_m4v_mp4;html_htm;rm_rmvb;mpg_mpeg_mov;mkv_webm;
 *      asf_wma_wmv;jpg_jpeg;tif_tiff;ai_pdf;zip_docx_pptx_xlsx;")
 *     - 11 groups; the underscore-joined members are counted/presented as
 *       one family ("MP4 family: N files" - the container-identity
 *       grouping, filtering-priority--config-layer.md SS3 item 5).
 *   - og-codebase/config/PriorityDisplayType.xml - 5 categories, 35
 *     extensions, each with a 2-digit alias sort key (00jpg, 01png, ...):
 *     the Type tree's extension ordering WITHIN each category
 *     (filtering-priority--config-layer.md SS2 - "jpg before png before
 *     jpeg" in the results Type tab).
 *
 * The layering decision (registered D-113): the og consumes both tables in
 * DRWUI's presentation (the Type tab + the summary lines); the core lands
 * them as query-layer data + lookups - the ABI host's presentation
 * consults the same tables the og's UI reads.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/query/cr_query.h"

#include <cstring>

namespace cr {
namespace query {

/* ---- SumaryFileType: the unification groups (the og's order preserved) ---- */

namespace {

struct SummaryGroup {
    const char* family;  /* the og's group token (the family identity) */
    const char* members; /* underscore-joined members, NUL-terminated */
};

/* the 11 groups, the ini's order verbatim */
const SummaryGroup kSummaryGroups[] = {
    {"aiff_aif", "aiff_aif"},
    {"m4p_m4a_m4v_mp4", "m4p_m4a_m4v_mp4"},
    {"html_htm", "html_htm"},
    {"rm_rmvb", "rm_rmvb"},
    {"mpg_mpeg_mov", "mpg_mpeg_mov"},
    {"mkv_webm", "mkv_webm"},
    {"asf_wma_wmv", "asf_wma_wmv"},
    {"jpg_jpeg", "jpg_jpeg"},
    {"tif_tiff", "tif_tiff"},
    {"ai_pdf", "ai_pdf"},
    {"zip_docx_pptx_xlsx", "zip_docx_pptx_xlsx"},
};

/* the display-order table: (category, rank-within-category) per
 * extension, the XML's alias order verbatim */
struct DisplayOrderEntry {
    const char* ext;
    TypeDisplayCategory category;
    u8 rank; /* the alias key's numeric value (00, 01, ...) */
};

} // namespace

const char* summary_family_for_extension(const char* ext_utf8_lower) {
    /* the unification-group membership: an extension in a group folds its
     * counts/presentation into the family; NULL = no group (the extension
     * stands alone). The lookup is exact lowercase (the og's members are
     * lowercase in the ini). */
    if (ext_utf8_lower == nullptr || ext_utf8_lower[0] == 0)
        return nullptr;
    const size_t len = std::strlen(ext_utf8_lower);
    for (const SummaryGroup& g : kSummaryGroups) {
        const char* m = g.members;
        while (*m != 0) {
            const char* seg_end = m;
            while (*seg_end != 0 && *seg_end != '_')
                ++seg_end;
            const size_t seg_len = size_t(seg_end - m);
            if (seg_len == len && std::memcmp(m, ext_utf8_lower, len) == 0)
                return g.family;
            m = *seg_end == '_' ? seg_end + 1 : seg_end;
        }
    }
    return nullptr;
}

bool type_display_order(const char* ext_utf8_lower, TypeDisplayCategory& out_category,
                        u32& out_rank) {
    /* the Type-tree ordering datum: the extension's priority-tier category
     * + its alias rank within it. Extensions absent from the 35-entry
     * table have no display rank (the og's Type tab lists them after the
     * ranked set, unordered - the presentation falls back to arrival
     * order; false here leaves that to the host). */
    if (ext_utf8_lower == nullptr || ext_utf8_lower[0] == 0)
        return false;
    const size_t len = std::strlen(ext_utf8_lower);
    static const DisplayOrderEntry kOrder[] = {
        {"jpg", TypeDisplayCategory::Picture, 0},   {"png", TypeDisplayCategory::Picture, 1},
        {"jpeg", TypeDisplayCategory::Picture, 2},  {"gif", TypeDisplayCategory::Picture, 3},
        {"tif", TypeDisplayCategory::Picture, 4},   {"eps", TypeDisplayCategory::Picture, 5},
        {"psd", TypeDisplayCategory::Picture, 6},   {"bmp", TypeDisplayCategory::Picture, 7},
        {"ico", TypeDisplayCategory::Picture, 8},   {"mp4", TypeDisplayCategory::Video, 0},
        {"mov", TypeDisplayCategory::Video, 1},     {"avi", TypeDisplayCategory::Video, 2},
        {"swf", TypeDisplayCategory::Video, 3},     {"m4v", TypeDisplayCategory::Video, 4},
        {"fla", TypeDisplayCategory::Video, 5},     {"wmv", TypeDisplayCategory::Video, 6},
        {"doc", TypeDisplayCategory::Document, 0},  {"docx", TypeDisplayCategory::Document, 1},
        {"pdf", TypeDisplayCategory::Document, 2},  {"xls", TypeDisplayCategory::Document, 3},
        {"xlsx", TypeDisplayCategory::Document, 4}, {"ppt", TypeDisplayCategory::Document, 5},
        {"pptx", TypeDisplayCategory::Document, 6}, {"mp3", TypeDisplayCategory::Audio, 0},
        {"wav", TypeDisplayCategory::Audio, 1},     {"wma", TypeDisplayCategory::Audio, 2},
        {"m4a", TypeDisplayCategory::Audio, 3},     {"ape", TypeDisplayCategory::Audio, 4},
        {"flac", TypeDisplayCategory::Audio, 5},    {"aac", TypeDisplayCategory::Audio, 6},
        {"zip", TypeDisplayCategory::Archive, 0},   {"rar", TypeDisplayCategory::Archive, 1},
        {"7z", TypeDisplayCategory::Archive, 2},    {"tar", TypeDisplayCategory::Archive, 3},
        {"gzip", TypeDisplayCategory::Archive, 4},
    };
    for (const DisplayOrderEntry& e : kOrder) {
        if (std::strlen(e.ext) == len && std::memcmp(e.ext, ext_utf8_lower, len) == 0) {
            out_category = e.category;
            out_rank = e.rank;
            return true;
        }
    }
    return false;
}

const char* type_display_category_name(TypeDisplayCategory c) {
    /* the XML's Description attributes verbatim */
    switch (c) {
    case TypeDisplayCategory::Picture:
        return "Picture";
    case TypeDisplayCategory::Video:
        return "Video";
    case TypeDisplayCategory::Document:
        return "Document";
    case TypeDisplayCategory::Audio:
        return "Audio";
    case TypeDisplayCategory::Archive:
        return "Archive";
    }
    return "";
}

} // namespace query
} // namespace cr
