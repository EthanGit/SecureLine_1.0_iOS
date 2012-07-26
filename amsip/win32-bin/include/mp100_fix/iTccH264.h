/////////////////////////////////////////////////////
#ifndef __ITCCH264_H__
#define __ITCCH264_H__

#ifdef __cplusplus
extern "C" {
#endif

enum {
	ITCCH264_VERSION,
	ITCCH264_MODE
};

//{FF0A4E95-A8AF-4f4e-9453-BC8BEA5826DE}
static const GUID IID_ITccH264 = 
{ 0xFF0A4E95, 0xA8AF, 0x4F4E, {0x94, 0x53, 0xBC, 0x8B, 0xEA, 0x58, 0x26, 0xDE} };

DECLARE_INTERFACE_(ITccH264, IUnknown)
{
	STDMETHOD(GetInfo) (THIS_ DWORD selector, PVOID pParam) PURE;
	STDMETHOD(SetInfo) (THIS_ DWORD selector, PVOID pParam) PURE;
};

#ifdef __cplusplus
};
#endif

#endif
