// mask_probe.cpp - decode REC_SESSION_STATE + REC_VOLUME_INFO masks from .crsf
// files to test the load_session_summary mask-integrity check offline.
#include <cr/session/cr_crsf.h>
#include <cr/session/cr_records.h>
#include <cstdio>

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
        if (rd.open(&n)) { printf("  open failed\n"); fclose(g_f); continue; }
        session::SessionState st; bool haveState = false;
        session::VolumeInfo vi; bool haveVi = false;
        for (u32 i = 0; i < n; ++i) {
            session::Record rec;
            if (rd.read_record(i, rec)) continue;
            if (rec.type == session::REC_SESSION_STATE) {
                haveState = session::deserialize_session_state(rec.data.data(), rec.data.size(), st) == 0;
                printf("  STATE len=%zu done_mask=0x%x files=%llu deleted=%llu carved=%llu\n",
                       rec.data.size(), st.stage_done_mask,
                       (unsigned long long)st.files_found, (unsigned long long)st.deleted_found,
                       (unsigned long long)st.carved_found);
            }
            if (rec.type == session::REC_VOLUME_INFO) {
                haveVi = session::deserialize_volume_info(rec.data.data(), rec.data.size(), vi) == 0;
                printf("  VOLINFO len=%zu fs=%u flags(configured)=0x%x slice=%llu+%llu label=%s boot_hash=0x%x\n",
                       rec.data.size(), vi.fs_kind, vi.flags,
                       (unsigned long long)vi.slice_offset, (unsigned long long)vi.slice_bytes,
                       vi.label, vi.boot_hash);
            }
        }
        u32 done = haveState ? st.stage_done_mask : 0;
        u32 cfg = haveVi ? vi.flags : 0x7;
        printf("  INTEGRITY: done=0x%x configured=0x%x done&~cfg=0x%x => %s\n",
               done, cfg, (done & ~cfg),
               (done & ~cfg) ? "*** ERR_SESSION_FORMAT (the load failure!) ***" : "ok");
        fclose(g_f);
    }
    return 0;
}
