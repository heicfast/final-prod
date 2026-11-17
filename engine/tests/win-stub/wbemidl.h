/* wbemidl.h - DECLARATION-ONLY stub for the Linux-side win-layer parse
 * gate (M-055). See tests/win-stub/windows.h for the rationale.
 *
 * The WMI SMART fallback (cr_health_wmi.cpp) is the only win-layer user
 * of the COM/WMI surface; the shapes below match the SDK headers in the
 * methods and fields the code touches: CoInitializeEx/CoCreateInstance/
 * CoSetProxyBlanket, IWbemLocator::ConnectServer, IWbemServices::
 * ExecQuery, IEnumWbemClassObject::Next, IWbemClassObject::Get (BSTR
 * InstanceName + VT_UI1|VT_ARRAY VendorSpecific), VARIANT/VARIANT
 * helpers, SAFEARRAY access, SysAllocString/SysFreeString. Interface
 * method signatures are declaration-only; nothing links on Linux.
 *
 * Licensed under the MIT license. See LICENSE.
 */
#pragma once

#include "windows.h"

typedef long HRESULT;
typedef void* PVOID;
typedef wchar_t OLECHAR;
typedef OLECHAR* BSTR;

/* GUID comes from the windows.h stub (the config-API surface already
 * defines it); only the COM aliases are added here. */
typedef GUID CLSID;
typedef GUID IID;
typedef const CLSID& REFCLSID;
typedef const IID& REFIID;

#define S_OK ((HRESULT)0L)
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#define RPC_E_CHANGED_MODE ((HRESULT)0x80010106L)

/* apartment / class-context / proxy-blanket constants (SDK values) */
#define COINIT_MULTITHREADED 0x0
#define CLSCTX_INPROC_SERVER 0x1
#define RPC_C_AUTHN_WINNT 10
#define RPC_C_AUTHZ_NONE 0
#define RPC_C_AUTHN_LEVEL_CALL 3
#define RPC_C_IMP_LEVEL_IMPERSONATE 3
#define EOAC_NONE 0x0

/* the WBEM flags the fallback passes */
#define WBEM_INFINITE 0xFFFFFFFF
#define WBEM_FLAG_FORWARD_ONLY 0x20
#define WBEM_FLAG_RETURN_IMMEDIATELY 0x10

typedef unsigned short VARTYPE;
#define VT_EMPTY 0
#define VT_NULL 1
#define VT_BSTR 8
#define VT_UI1 17
#define VT_ARRAY 0x2000

typedef struct tagSAFEARRAY {
    unsigned short cDims;
    unsigned short fFeatures;
    unsigned long cbElements;
    unsigned long cLocks;
    PVOID pvData;
} SAFEARRAY;

typedef struct tagVARIANT {
    VARTYPE vt;
    unsigned short wReserved1;
    unsigned short wReserved2;
    unsigned short wReserved3;
    union {
        BSTR bstrVal;
        SAFEARRAY* parray;
        long lVal;
        PVOID pv;
    };
} VARIANT;

/* the interfaces, with exactly the methods the fallback calls */
struct IWbemClassObject;
struct IEnumWbemClassObject;
struct IWbemServices;
struct IWbemLocator;

struct IWbemClassObject {
    HRESULT Get(const wchar_t* /*wszName*/, long /*lFlags*/, VARIANT* pVal, void* /*pType*/,
                void* /*plFlavor*/);
    ULONG Release();
};

struct IEnumWbemClassObject {
    HRESULT Next(long lTimeout, ULONG uCount, IWbemClassObject** apObjects, ULONG* puReturned);
    ULONG Release();
};

struct IWbemServices {
    HRESULT ExecQuery(const wchar_t* wszQueryLanguage, const wchar_t* wszQuery, long lFlags,
                      void* /*pCtx*/, IEnumWbemClassObject** ppEnum);
    ULONG Release();
};

struct IWbemLocator {
    HRESULT ConnectServer(const BSTR strNetworkResource, const BSTR strUser, const BSTR strPassword,
                          const wchar_t* strLocale, long lSecurityFlags, const BSTR strAuthority,
                          void* /*pCtx*/, IWbemServices** ppNamespace);
    ULONG Release();
};

/* the SDK's CLSID/IID pair (extern: parse-only, never linked here) */
extern const CLSID CLSID_WbemLocator;
extern const IID IID_IWbemLocator;

HRESULT CoInitializeEx(PVOID pvReserved, unsigned long dwCoInit);
void CoUninitialize(void);
HRESULT CoCreateInstance(REFCLSID rclsid, PVOID pUnkOuter, unsigned long dwClsContext, REFIID riid,
                         PVOID* ppv);
HRESULT CoSetProxyBlanket(void* /*pProxy*/, unsigned long /*dwAuthnSvc*/,
                          unsigned long /*dwAuthzSvc*/, const wchar_t* /*pServerPrincName*/,
                          unsigned long /*dwAuthnLevel*/, unsigned long /*dwImpLevel*/,
                          PVOID /*pAuthInfo*/, unsigned long /*dwCapabilities*/);

void VariantInit(VARIANT* pvarg);
HRESULT VariantClear(VARIANT* pvarg);

BSTR SysAllocString(const wchar_t* psz);
void SysFreeString(BSTR bstrString);

HRESULT SafeArrayGetLBound(SAFEARRAY* psa, unsigned int uDim, LONG* plLbound);
HRESULT SafeArrayGetUBound(SAFEARRAY* psa, unsigned int uDim, LONG* plUbound);
HRESULT SafeArrayAccessData(SAFEARRAY* psa, void** ppvData);
HRESULT SafeArrayUnaccessData(SAFEARRAY* psa);
