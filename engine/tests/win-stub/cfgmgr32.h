/* cfgmgr32.h - declaration-only SDK stub (parse gate; see
 * tools/win_syntax_check.sh). The cfgmgr32 surface src/win consumes:
 * the N-071 disk-interface enumeration pair. Shapes are SDK-faithful
 * (CONFIGRET = DWORD; the W entry points since Windows 2000).
 * Not for linking - declarations only. */

#ifndef STUB_CFGMGR32_H_
#define STUB_CFGMGR32_H_

/* stub-windows provides DWORD/ULONG/wchar_t/GUID */

typedef DWORD CONFIGRET;
typedef const wchar_t* DEVINSTID_W;
typedef wchar_t* PZZWSTR;

#define CR_SUCCESS 0u
#define CR_FAILURE 1u
#define CR_BUFFER_SMALL 2u

#define CM_GET_DEVICE_INTERFACE_LIST_PRESENT 0u

#ifdef __cplusplus
extern "C" {
#endif

CONFIGRET CM_Get_Device_Interface_List_SizeW(PULONG pulLen, GUID* InterfaceClassGuid,
                                             DEVINSTID_W pDeviceID, ULONG ulFlags);

CONFIGRET CM_Get_Device_Interface_ListW(GUID* InterfaceClassGuid, DEVINSTID_W pDeviceID,
                                        PZZWSTR Buffer, ULONG BufferLen, ULONG ulFlags);

#ifdef __cplusplus
}
#endif

#endif /* STUB_CFGMGR32_H_ */
