/* cr_health_wmi.cpp - the og's LAST SMART route: the WMI
 * MSStorageDriver_FailurePredict{Data,Thresholds} fallback.
 *
 * Corpus (REFERENCE-SMART.md §2): the og DiskHealth companion queries
 * `SELECT * FROM MSStorageDriver_FailurePredictData` /
 * `...FailurePredictThresholds` from \\.\root\WMI (InstanceName +
 * VendorSpecific) — the storage-driver SMART class pair (the ATA port
 * driver publishes them; VERIFICATION V10-resolution). We implement the
 * same route as the fallback AFTER the transport chain (ATA passthrough,
 * legacy, SAT, CSMI) fails — the same last-position it occupies in the
 * reference.
 *
 * Payload semantics: VendorSpecific of FailurePredictData = the 512-byte
 * SMART READ DATA table; of FailurePredictThresholds = the 512-byte
 * threshold table (the classes the disk class driver fills from the same
 * commands our transports issue directly). Both feed the SAME pure
 * parser as the chain path (read_smart_ata_buffers) — one parser, two
 * sources, zero divergence in grading.
 *
 * Instance-to-disk matching (documented; the corpus does not decode the
 * og's own matching): the WMI InstanceName embeds the device's PnP
 * identity (e.g. "IDE\DiskWDC_WD40EFRX-...___WD-XXXXXXXX"). We match the
 * TARGET disk by serial (STORAGE_DEVICE_DESCRIPTOR SerialNumber,
 * space-stripped, uppercased, substring) with the model string
 * (ProductId) as fallback. A disk whose WMI instance cannot be tied to
 * it by either token gets NO WMI health — truthful unavailable, never a
 * another disk's table.
 *
 * No IDENTIFY exists on this route: the model string comes from the
 * storage descriptor (the same source the og's WMI cross-check reads via
 * Win32_DiskDrive), ata_version stays 0. No processes are spawned (the
 * reference force-starts Winmgmt via `sc config`; a down WMI service
 * fails our ConnectServer fast and the disk reports unavailable — no
 * visible console, no service fiddling from a health read).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/win/cr_health.h"
#include "cr/health/cr_smart_ata.h"
#include "cr/cr_error.h"
#define _WIN32_DCOM 1
#include <windows.h>
#include <winioctl.h>
#include <wbemidl.h>
#include <cstring>
#include <cctype>
#include <string>

namespace cr {
namespace win {
namespace health {

namespace {

/* The disk's own identity tokens from the storage stack: model
 * (ProductId) and serial (SerialNumber), space-stripped uppercased
 * ( ATA serials are space-padded; instance names are not ). */
struct DiskTokens {
    std::string model;
    std::string serial;
};

std::string strip_upper_ascii(const char* s, size_t cap) {
    std::string out;
    for (size_t i = 0; s != nullptr && i < cap && s[i] != '\0'; ++i) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            continue;
        out.push_back(char(toupper(c)));
    }
    return out;
}

bool query_disk_tokens(HANDLE h, DiskTokens& out) {
    STORAGE_PROPERTY_QUERY q = {};
    q.PropertyId = StorageDeviceProperty;
    q.QueryType = PropertyStandardQuery;
    u8 buf[2048] = {};
    DWORD br = 0;
    if (!DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, &q, sizeof(q), buf, sizeof(buf), &br,
                         nullptr))
        return false;
    if (br < sizeof(STORAGE_DEVICE_DESCRIPTOR))
        return false;
    const auto* d = reinterpret_cast<const STORAGE_DEVICE_DESCRIPTOR*>(buf);
    const auto str_at = [&](ULONG off) -> const char* {
        return (off != 0 && off < sizeof(buf)) ? reinterpret_cast<const char*>(buf + off) : nullptr;
    };
    out.model = strip_upper_ascii(str_at(d->ProductIdOffset), sizeof(buf));
    out.serial = strip_upper_ascii(str_at(d->SerialNumberOffset), sizeof(buf));
    return true;
}

/* Uppercase a wide instance name for matching (ASCII fold). */
std::string upper_wide(const wchar_t* ws) {
    std::string out;
    for (; ws != nullptr && *ws != L'\0'; ++ws) {
        wchar_t c = *ws;
        if (c == L' ' || c == L'\t')
            continue;
        if (c >= L'a' && c <= L'z')
            c = wchar_t(c - L'a' + L'A');
        if (c < 0x80)
            out.push_back(char(c));
    }
    return out;
}

/* Does this WMI InstanceName belong to the target disk? Serial wins
 * (unique per device); the model string is the fallback (matches when
 * the driver reports no serial or the instance embeds only the model). */
bool instance_matches(const std::string& instance_up, const DiskTokens& tok) {
    if (!tok.serial.empty() && instance_up.find(tok.serial) != std::string::npos)
        return true;
    if (!tok.model.empty() && instance_up.find(tok.model) != std::string::npos)
        return true;
    return false;
}

/* Pull one instance's InstanceName + 512-byte VendorSpecific table. */
struct WmiTable {
    std::wstring instance;
    u8 data[512];
};

bool get_instance_table(IWbemClassObject* obj, WmiTable& out) {
    VARIANT vt;
    VariantInit(&vt);
    if (FAILED(obj->Get(L"InstanceName", 0, &vt, nullptr, nullptr)) || vt.vt != VT_BSTR) {
        VariantClear(&vt);
        return false;
    }
    out.instance.assign(vt.bstrVal ? vt.bstrVal : L"");
    VariantClear(&vt);

    VariantInit(&vt);
    if (FAILED(obj->Get(L"VendorSpecific", 0, &vt, nullptr, nullptr)) || (vt.vt & VT_ARRAY) == 0 ||
        vt.vt != (VT_ARRAY | VT_UI1)) {
        VariantClear(&vt);
        return false;
    }
    LONG lb = 0, ub = -1;
    bool ok = false;
    if (SUCCEEDED(SafeArrayGetLBound(vt.parray, 1, &lb)) &&
        SUCCEEDED(SafeArrayGetUBound(vt.parray, 1, &ub)) && ub - lb + 1 >= 512) {
        void* raw = nullptr;
        if (SUCCEEDED(SafeArrayAccessData(vt.parray, &raw))) {
            /* the SMART table is the FIRST 512 bytes of VendorSpecific */
            std::memcpy(out.data, raw, 512);
            SafeArrayUnaccessData(vt.parray);
            ok = true;
        }
    }
    VariantClear(&vt);
    return ok;
}

/* RAII-lite: every exit path releases what it took. */
void release_all(IWbemServices* svc, IWbemLocator* loc, IEnumWbemClassObject* en, bool com_init,
                 bool security_set) {
    if (en != nullptr)
        en->Release();
    if (svc != nullptr)
        svc->Release();
    if (loc != nullptr)
        loc->Release();
    (void)security_set;
    if (com_init)
        CoUninitialize();
}

} // namespace

u32 read_smart_wmi(void* disk_handle, SmartReport& out) {
    HANDLE h = static_cast<HANDLE>(disk_handle);
    if (h == nullptr || h == INVALID_HANDLE_VALUE)
        return ERR_BAD_PARAMETER;

    DiskTokens tok;
    if (!query_disk_tokens(h, tok))
        return ERR_HEALTH_UNAVAILABLE;

    /* COM on this thread; RPC_E_CHANGED_MODE (already initialized in a
     * different apartment) is fine — the existing apartment serves WMI. */
    const HRESULT hr_init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool com_init = SUCCEEDED(hr_init);
    if (FAILED(hr_init) && hr_init != RPC_E_CHANGED_MODE)
        return ERR_HEALTH_UNAVAILABLE;

    IWbemLocator* loc = nullptr;
    IWbemServices* svc = nullptr;
    IEnumWbemClassObject* en = nullptr;
    bool security_set = false;
    if (SUCCEEDED(CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_IWbemLocator, reinterpret_cast<void**>(&loc))) &&
        loc != nullptr &&
        SUCCEEDED(loc->ConnectServer(const_cast<BSTR>(L"ROOT\\WMI"), nullptr, nullptr, 0, 0,
                                     nullptr, nullptr, &svc)) &&
        svc != nullptr) {
        security_set = SUCCEEDED(CoSetProxyBlanket(
            svc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL,
            RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE));
    }

    WmiTable table;
    bool have_table = false;
    if (svc != nullptr) {
        BSTR q_data = SysAllocString(L"SELECT * FROM MSStorageDriver_FailurePredictData");
        BSTR wql = SysAllocString(L"WQL");
        if (q_data != nullptr && wql != nullptr &&
            SUCCEEDED(svc->ExecQuery(wql, q_data,
                                     WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr,
                                     &en))) {
            IWbemClassObject* obj = nullptr;
            ULONG got = 0;
            while (en->Next(WBEM_INFINITE, 1, &obj, &got) == S_OK && got == 1 && obj != nullptr) {
                WmiTable cand;
                if (get_instance_table(obj, cand) &&
                    instance_matches(upper_wide(cand.instance.c_str()), tok)) {
                    table = cand;
                    have_table = true;
                    obj->Release();
                    obj = nullptr;
                    break;
                }
                obj->Release();
                got = 0;
            }
        }
        SysFreeString(q_data);
        SysFreeString(wql);
        if (en != nullptr) {
            en->Release();
            en = nullptr;
        }
    }

    WmiTable thr;
    bool have_thr = false;
    if (have_table && svc != nullptr) {
        BSTR q_thr = SysAllocString(L"SELECT * FROM MSStorageDriver_FailurePredictThresholds");
        BSTR wql = SysAllocString(L"WQL");
        if (q_thr != nullptr && wql != nullptr &&
            SUCCEEDED(svc->ExecQuery(
                wql, q_thr, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &en))) {
            IWbemClassObject* obj = nullptr;
            ULONG got = 0;
            while (en->Next(WBEM_INFINITE, 1, &obj, &got) == S_OK && got == 1 && obj != nullptr) {
                WmiTable cand;
                if (get_instance_table(obj, cand) && cand.instance == table.instance) {
                    thr = cand;
                    have_thr = true;
                    obj->Release();
                    obj = nullptr;
                    break;
                }
                obj->Release();
                got = 0;
            }
        }
        SysFreeString(q_thr);
        SysFreeString(wql);
    }

    release_all(svc, loc, en, com_init, security_set);

    if (!have_table)
        return ERR_HEALTH_UNAVAILABLE; /* no tied instance — truthful miss */

    /* The same pure parser as the chain path; model from the storage
     * stack, ata_version honestly 0 (IDENTIFY-only datum). */
    cr::health::SmartInfo info;
    if (cr::health::read_smart_ata_buffers(table.data, have_thr ? thr.data : nullptr, tok.model,
                                           info) != OK)
        return ERR_HEALTH_UNAVAILABLE;

    out.available = true;
    out.transport = "wmi-failurepredict";
    out.ata_version = 0;
    out.smart_version = info.smart_version;
    out.failing_now = info.failing_now;
    out.grade = info.grade;
    out.reallocated_sectors = info.reallocated_sectors;
    out.pending_sectors = info.pending_sectors;
    out.uncorrectable = info.uncorrectable;
    out.power_on_hours = info.power_on_hours;
    out.spin_retry = info.spin_retry;
    out.ssd_wear_level = info.ssd_wear_level;
    out.temperature_c = info.temperature_c;
    out.attributes.clear();
    out.attributes.reserve(info.attributes.size());
    for (const cr::health::SmartAttrInfo& a : info.attributes)
        out.attributes.push_back(
            SmartAttribute{a.id,
                           a.value,
                           a.worst,
                           {a.raw[0], a.raw[1], a.raw[2], a.raw[3], a.raw[4], a.raw[5]},
                           a.vendor_threshold,
                           a.failing,
                           a.name,
                           a.grade});
    out.is_ssd = info.model_says_ssd || looks_like_ssd(h);
    return OK;
}

} // namespace health
} // namespace win
} // namespace cr
