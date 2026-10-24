// session_probe.cpp - dissect .crsf session files: per-record read results,
// container flags, and a tree-deserialize attempt. Pure core, Linux-buildable.
#include <cr/session/cr_crsf.h>
#include <cr/session/cr_records.h>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace cr;

static FILE* g_f = nullptr;
static u32 file_read(void*, void* buf, u32 len) { return (u32)fread(buf, 1, len, g_f) == len ? 0 : 1u; }
static u32 file_seek(void*, u64 abs) { return fseeko(g_f, (off_t)abs, SEEK_SET) == 0 ? 0 : 1u; }
static u64 file_size(void*) { fseeko(g_f, 0, SEEK_END); u64 s = (u64)ftello(g_f); fseeko(g_f, 0, SEEK_SET); return s; }

int main(int argc, char** argv) {
    for (int a = 1; a < argc; ++a) {
        printf("=== %s ===\n", argv[a]);
        g_f = fopen(argv[a], "rb");
        if (!g_f) { printf("  cannot open\n"); continue; }
        session::Reader rd(file_read, file_seek, file_size, nullptr);
        u32 n = 0;
        u32 rc = rd.open(&n);
        printf("  open rc=0x%x header_records=%u flags=0x%x (ABS_EXTENTS=%d)\n",
               rc, n, rd.flags(), (int)(rd.flags() & 1));
        if (rc) { fclose(g_f); continue; }
        u32 readable = 0;
        for (u32 i = 0; i < n; ++i) {
            session::Record rec;
            u32 r = rd.read_record(i, rec);
            if (r == 0) {
                readable++;
                if (rec.type == session::REC_VOLUME_INFO && rec.data.size() >= 80) {
                    /* VolumeInfo: slice offset/len live in the v1 tail per crsf-format.md */
                    printf("    rec[%u] type=%u len=%zu ok\n", i, (unsigned)rec.type, rec.data.size());
                } else {
                    printf("    rec[%u] type=%u len=%zu ok\n", i, (unsigned)rec.type, rec.data.size());
                }
            } else {
                printf("    rec[%u] READ FAILED rc=0x%x (%s) - header claims it, reader refuses\n",
                       i, r, r == 0x50040001 ? "ERR_SESSION_FORMAT" : "?");
            }
        }
        printf("  readable=%u/%u\n", readable, n);
        /* tree deserialize attempt on the last readable TREE record */
        for (u32 i = n; i-- > 0;) {
            session::Record rec;
            if (rd.read_record(i, rec) == 0 && rec.type == session::REC_RESULT_TREE) {
                tree::ResultTree t;
                u32 r = session::deserialize_tree(rec.data.data(), rec.data.size(), t);
                printf("  tree deser from rec[%u]: rc=0x%x nodes=%zu\n", i, r, t.node_count());
                break;
            }
        }
        fclose(g_f);
    }
    return 0;
}
