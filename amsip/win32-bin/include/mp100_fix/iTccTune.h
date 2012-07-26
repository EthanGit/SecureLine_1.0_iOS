
#ifndef __ITCCTUNE_H__
#define __ITCCTUNE_H__

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_INTERFACE_(ITccTune, IUnknown)
{
	STDMETHOD(GetInfo) (THIS_ DWORD selector, PVOID pParam) PURE;
	STDMETHOD(SetInfo) (THIS_ DWORD selector, PVOID pParam, BOOL bThreadMode) PURE;
};

enum {
	TSIF_TUNER_COMMAND_SET_BAND,
	TSIF_TUNER_COMMAND_SET_FREQUENCY,
	TSIF_TUNER_COMMAND_GET_FREQUENCY,
	TSIF_TUNER_COMMAND_SET_CHANNEL,
	TSIF_TUNER_COMMAND_GET_CHANNEL,
	TSIF_TUNER_COMMAND_GET_FREQUENCYNUM,
	TSIF_TUNER_COMMAND_GET_AUTOSCANINFO,
	TSIF_TUNER_COMMAND_GET_MANUALSCANINFO,
	TSIF_TUNER_COMMAND_GET_STRENGTH,

	// extra commands for abnormal case
	TSIF_TUNER_COMMAND_SET_RESYNC,
	TSIF_TUNER_COMMAND_GET_RECONFIGINFO,

	// get version info
	TSIF_TUNER_COMMAND_VERSION = 100
};

// {9BE304DD-E6E0-4577-A203-A07F298D11B2}
static const GUID CLSID_TCC_TDMBTune = 
{ 0x9be304dd, 0xe6e0, 0x4577, 0xa2, 0x3, 0xa0, 0x7f, 0x29, 0x8d, 0x11, 0xb2 };

// {BEA39A5D-A95E-4fc0-8789-1AC03CA8EC54}
static const GUID IID_ITccTune = 
{ 0xbea39a5d, 0xa95e, 0x4fc0, { 0x87, 0x89, 0x1a, 0xc0, 0x3c, 0xa8, 0xec, 0x54 } };

#define TDMB_TUNE_FILTER_NAMEW               L"TCC TDMBTune"

#ifdef __cplusplus
};
#endif
#endif