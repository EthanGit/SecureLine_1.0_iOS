#ifndef __HID_DEVICES_H__
#define __HID_DEVICES_H__

#include <amsiptools/hid_hooks.h>

#if defined(ENABLE_HID)

#if defined(WIN32)
#include <stdlib.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <setupapi.h>
#endif

#if defined(__APPLE__)
#include "HID_Utilities.h"
#endif

#include <mediastreamer2/mscommon.h>

#ifdef __cplusplus
extern "C"{
#endif

#if defined(WIN32)
	
#include <hidsdi.h>
#include "hid.h"

/* WINDOWS METHODS */

typedef VOID (__stdcall *PHidD_GetProductString)(HANDLE, PVOID, ULONG);
typedef VOID (__stdcall *PHidD_GetHidGuid)(LPGUID);
typedef BOOLEAN (__stdcall *PHidD_GetAttributes)(HANDLE, PHIDD_ATTRIBUTES);
typedef BOOLEAN (__stdcall *PHidD_SetFeature)(HANDLE, PVOID, ULONG);
typedef BOOLEAN (__stdcall *PHidD_GetFeature)(HANDLE, PVOID, ULONG);
typedef BOOLEAN (__stdcall *PHidD_GetInputReport)(HANDLE, PVOID, ULONG);

typedef BOOLEAN (__stdcall *PHidD_GetPreparsedData)(HANDLE, PHIDP_PREPARSED_DATA*);
typedef BOOLEAN (__stdcall *PHidD_FreePreparsedData)(PHIDP_PREPARSED_DATA);

typedef NTSTATUS (__stdcall *PHidP_GetCaps)(PHIDP_PREPARSED_DATA, PHIDP_CAPS);
typedef NTSTATUS (__stdcall *PHidP_GetButtonCaps) (HIDP_REPORT_TYPE, PHIDP_BUTTON_CAPS,
				 PUSHORT, PHIDP_PREPARSED_DATA);
typedef NTSTATUS (__stdcall *PHidP_GetValueCaps) (HIDP_REPORT_TYPE, PHIDP_VALUE_CAPS,
				 PUSHORT, PHIDP_PREPARSED_DATA);

typedef NTSTATUS (__stdcall *PHidP_GetUsageValue) (
    HIDP_REPORT_TYPE ,
    USAGE ,
    USHORT ,
    USAGE ,
    PULONG ,
    PHIDP_PREPARSED_DATA ,
    PCHAR ,
    ULONG);

typedef NTSTATUS (__stdcall *PHidP_GetScaledUsageValue) (
    HIDP_REPORT_TYPE ,
    USAGE ,
    USHORT ,
    USAGE ,
    PLONG ,
    PHIDP_PREPARSED_DATA ,
    PCHAR ,
    ULONG);
typedef NTSTATUS (__stdcall *PHidP_SetScaledUsageValue) (
    HIDP_REPORT_TYPE ,
    USAGE ,
    USHORT ,
    USAGE ,
    PLONG ,
    PHIDP_PREPARSED_DATA ,
    PCHAR ,
    ULONG);

typedef NTSTATUS (__stdcall *PHidP_GetUsages) (
    HIDP_REPORT_TYPE ,
    USAGE ,
    USHORT ,
    PUSAGE ,
    PULONG ,
    PHIDP_PREPARSED_DATA ,
    PCHAR ,
    ULONG);

typedef NTSTATUS (__stdcall *PHidP_SetUsages) (
    HIDP_REPORT_TYPE ,
    USAGE ,
    USHORT ,
    PUSAGE ,
    PULONG ,
    PHIDP_PREPARSED_DATA ,
    PCHAR ,
    ULONG);

typedef NTSTATUS (__stdcall *PHidP_SetUsageValue) (
    HIDP_REPORT_TYPE ,
    USAGE ,
    USHORT ,
    USAGE ,
    ULONG ,
    PHIDP_PREPARSED_DATA ,
    PCHAR ,
    ULONG);

typedef ULONG (__stdcall*PHidP_MaxUsageListLength) (
   HIDP_REPORT_TYPE,
   USAGE,
   PHIDP_PREPARSED_DATA);


extern PHidD_GetProductString fHidD_GetProductString;
extern PHidD_GetHidGuid fHidD_GetHidGuid;
extern PHidD_GetAttributes fHidD_GetAttributes;
extern PHidD_SetFeature fHidD_SetFeature;
extern PHidD_GetFeature fHidD_GetFeature;
extern PHidD_GetInputReport fHidD_GetInputReport;
extern PHidD_GetPreparsedData fHidD_GetPreparsedData;
extern PHidD_FreePreparsedData fHidD_FreePreparsedData;
extern PHidP_GetCaps fHidP_GetCaps;
extern PHidP_GetButtonCaps fHidP_GetButtonCaps;
extern PHidP_GetValueCaps fHidP_GetValueCaps;

extern PHidP_GetUsageValue fHidP_GetUsageValue;
extern PHidP_GetScaledUsageValue fHidP_GetScaledUsageValue;
extern PHidP_GetUsages fHidP_GetUsages;
extern PHidP_SetUsages fHidP_SetUsages;
extern PHidP_SetUsageValue fHidP_SetUsageValue;
extern PHidP_MaxUsageListLength fHidP_MaxUsageListLength;

/* amsiptools methods */
ULONG ResetAllButtonUsage(PHID_DATA pData, ULONG DataLength);
ULONG ResetButtonUsage(PHID_DATA pData, ULONG DataLength, ULONG usage_page, ULONG usage_id);
ULONG SetButtonUsage(PHID_DATA pData, ULONG DataLength, ULONG usage_page, ULONG usage_id, USHORT val);
ULONG SetOutputValue(PHID_DATA pData, ULONG DataLength, ULONG usage_page, ULONG usage_id, USHORT val);

struct hid_device {
	HID_DEVICE hid_dev;
	HANDLE hEvent;

	char your_devicePath[1024];

	int ringer;
	int audioenabled;
	int mute;
	int isAttached;
};
#endif
	
#if defined(__APPLE__)
	struct hid_device {
		IOHIDDeviceRef gCurrentIOHIDDeviceRef;
		CFArrayRef gElementsCFArrayRef;
		IOHIDQueueRef queueref;
		
		char your_devicePath[1024];
		
		int ringer;
		int audioenabled;
		int mute;
		int isAttached;
	};
#endif
	
typedef int (*hd_set_presenceindicator_func)(struct hid_hooks *ph, int value);
typedef int (*hd_set_ringer_func)(struct hid_hooks *ph, int enable);
typedef int (*hd_set_audioenabled_func)(struct hid_hooks *ph, int enable);
typedef int (*hd_set_mute_func)(struct hid_hooks *ph, int enable);
typedef int (*hd_set_sendcalls_func)(struct hid_hooks *ph, int enable);
typedef int (*hd_set_messagewaiting_func)(struct hid_hooks *ph, int enable);
typedef int (*hd_get_mute_func)(struct hid_hooks *ph);
typedef int (*hd_get_audioenabled_func)(struct hid_hooks *ph);
typedef int (*hd_get_isattached_func)(struct hid_hooks *ph);
typedef int (*hd_get_events_func)(hid_hooks_t *ph);

typedef int (*hd_init_func)(hid_hooks_t *ph);
typedef int (*hd_uninit_func)(hid_hooks_t *ph);

typedef struct hid_device_desc {
	int vendor;
	int product_id;
	hd_init_func hd_init;
	hd_uninit_func hd_uninit;

	hd_set_presenceindicator_func hd_set_presenceindicator;
	hd_set_ringer_func hd_set_ringer;
	hd_set_audioenabled_func hd_set_audioenabled;
	hd_set_mute_func hd_set_mute;
	hd_set_sendcalls_func hd_set_sendcalls;
	hd_set_messagewaiting_func hd_set_messagewaiting;
	hd_get_mute_func hd_get_mute;
	hd_get_audioenabled_func hd_get_audioenabled;
	hd_get_isattached_func hd_get_isattached;
	hd_get_events_func hd_get_events;
} hid_device_desc_t;

#ifdef __cplusplus
}
#endif

#endif

#endif
