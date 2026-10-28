/* test_config_display.cpp - N-132: the two display-config data layers
 * (the FilterFolder.ini SumaryFileType unification groups + the
 * PriorityDisplayType.xml Type-tree ordering), verbatim from the corpus
 * config files.
 */

#include <gtest/gtest.h>

#include <cr/query/cr_query.h>

using namespace cr;

TEST(ConfigDisplay, SummaryFamilyUnificationGroups) {
    /* N-132: the 11 groups, verbatim from FilterFolder.ini [Filter]
     * SumaryFileType - the members fold into their family. */
    struct Case {
        const char* ext;
        const char* family;
    };
    for (const Case& c : {Case{"aiff", "aiff_aif"},
                          Case{"aif", "aiff_aif"},
                          Case{"m4p", "m4p_m4a_m4v_mp4"},
                          Case{"m4a", "m4p_m4a_m4v_mp4"},
                          Case{"m4v", "m4p_m4a_m4v_mp4"},
                          Case{"mp4", "m4p_m4a_m4v_mp4"},
                          Case{"html", "html_htm"},
                          Case{"htm", "html_htm"},
                          Case{"rm", "rm_rmvb"},
                          Case{"rmvb", "rm_rmvb"},
                          Case{"mkv", "mkv_webm"},
                          Case{"webm", "mkv_webm"},
                          Case{"jpg", "jpg_jpeg"},
                          Case{"jpeg", "jpg_jpeg"},
                          Case{"tif", "tif_tiff"},
                          Case{"tiff", "tif_tiff"},
                          Case{"ai", "ai_pdf"},
                          Case{"pdf", "ai_pdf"},
                          Case{"zip", "zip_docx_pptx_xlsx"},
                          Case{"docx", "zip_docx_pptx_xlsx"},
                          Case{"pptx", "zip_docx_pptx_xlsx"},
                          Case{"xlsx", "zip_docx_pptx_xlsx"}}) {
        EXPECT_STREQ(query::summary_family_for_extension(c.ext), c.family) << "extension " << c.ext;
    }
}

TEST(ConfigDisplay, SummaryFamilyUngroupedStandsAlone) {
    /* N-132: an extension in NO group returns NULL (stands alone) - e.g.
     * "png" is Picture-category but no group folds it. */
    for (const char* e : {"png", "gif", "doc", "mp3", "exe", "txt"}) {
        EXPECT_EQ(query::summary_family_for_extension(e), nullptr) << "extension " << e;
    }
}

TEST(ConfigDisplay, SummaryFamilyNullAndEmptySafe) {
    EXPECT_EQ(query::summary_family_for_extension(nullptr), nullptr);
    EXPECT_EQ(query::summary_family_for_extension(""), nullptr);
}

TEST(ConfigDisplay, TypeDisplayOrderVerbatim) {
    /* N-132: PriorityDisplayType.xml - the alias ranks within the 5
     * categories, verbatim (jpg=00 < png=01 < jpeg=02... zip-family
     * first in Archive). */
    struct Case {
        const char* ext;
        query::TypeDisplayCategory cat;
        u32 rank;
    };
    for (const Case& c : {Case{"jpg", query::TypeDisplayCategory::Picture, 0},
                          Case{"png", query::TypeDisplayCategory::Picture, 1},
                          Case{"jpeg", query::TypeDisplayCategory::Picture, 2},
                          Case{"ico", query::TypeDisplayCategory::Picture, 8},
                          Case{"mp4", query::TypeDisplayCategory::Video, 0},
                          Case{"mov", query::TypeDisplayCategory::Video, 1},
                          Case{"wmv", query::TypeDisplayCategory::Video, 6},
                          Case{"doc", query::TypeDisplayCategory::Document, 0},
                          Case{"pdf", query::TypeDisplayCategory::Document, 2},
                          Case{"pptx", query::TypeDisplayCategory::Document, 6},
                          Case{"mp3", query::TypeDisplayCategory::Audio, 0},
                          Case{"flac", query::TypeDisplayCategory::Audio, 5},
                          Case{"aac", query::TypeDisplayCategory::Audio, 6},
                          Case{"zip", query::TypeDisplayCategory::Archive, 0},
                          Case{"rar", query::TypeDisplayCategory::Archive, 1},
                          Case{"7z", query::TypeDisplayCategory::Archive, 2},
                          Case{"gzip", query::TypeDisplayCategory::Archive, 4}}) {
        query::TypeDisplayCategory cat = query::TypeDisplayCategory::Picture;
        u32 rank = 0xFFFF;
        ASSERT_TRUE(query::type_display_order(c.ext, cat, rank)) << "extension " << c.ext;
        EXPECT_EQ(u32(cat), u32(c.cat)) << "extension " << c.ext;
        EXPECT_EQ(rank, c.rank) << "extension " << c.ext;
    }
}

TEST(ConfigDisplay, TypeDisplayOrderUnrankedReturnsFalse) {
    /* N-132: extensions outside the 35-entry tier have no rank - the og's
     * Type tab lists them after the ranked set, unordered. */
    query::TypeDisplayCategory cat = query::TypeDisplayCategory::Picture;
    u32 rank = 0;
    for (const char* e : {"nef", "cr2", "m4r", "cab", "iso", ""}) {
        EXPECT_FALSE(query::type_display_order(e, cat, rank)) << "extension " << e;
    }
    EXPECT_EQ(query::type_display_order(nullptr, cat, rank), false);
}

TEST(ConfigDisplay, TypeDisplayCategoryNamesVerbatim) {
    /* N-132: the XML's Description attributes verbatim. */
    EXPECT_STREQ(query::type_display_category_name(query::TypeDisplayCategory::Picture), "Picture");
    EXPECT_STREQ(query::type_display_category_name(query::TypeDisplayCategory::Archive), "Archive");
}
