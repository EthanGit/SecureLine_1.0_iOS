#include <amsiptools/hid_hooks.h>

#if defined(WIN32) && defined(ENABLE_HID)

#include "hid_devices.h"

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

#define ZMM_VENDOR_ID 0x1994
#define ZMM_PRODUCT_ID_USBPHONE 0x2004


int zmm_set_audioenabled(hid_hooks_t *ph, int enable)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->hid_dev.Attributes.VendorID != ZMM_VENDOR_ID)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->jdevice->hid_dev.Attributes.ProductID != ZMM_PRODUCT_ID_USBPHONE)
		return HID_ERROR_NODEVICEFOUND;

	if (enable)
		ms_message("ZMM USBPHONE-> Device: led audiolink & off hook/active call led");
	else
		ms_message("ZMM USBPHONE -> Device: led audiolink & off hook/active call led OFF");
	ph->jdevice->audioenabled=enable;
	return 0;
}

int zmm_get_audioenabled(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	return ph->jdevice->audioenabled;
}

int zmm_get_events(hid_hooks_t *ph)
{
	int hid_event=-1;
	DWORD BytesRead = 0;
	DWORD ret;

	static OVERLAPPED overlapped;
    memset(&overlapped, 0, sizeof(OVERLAPPED));

	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	overlapped.hEvent=ph->jdevice->hEvent;
	BOOL ReadStatus;
	ResetEvent(ph->jdevice->hEvent);
	ReadStatus = ReadFile (ph->jdevice->hid_dev.HidDevice,
		ph->jdevice->hid_dev.InputReportBuffer,
		ph->jdevice->hid_dev.Caps.InputReportByteLength,
		&BytesRead,&overlapped);
	if(!ReadStatus)
	{
		if(GetLastError() == ERROR_IO_PENDING)
		{
			ret = WaitForSingleObject(ph->jdevice->hEvent, 0);
			if(ret == WAIT_TIMEOUT)
			{
				CancelIo(ph->jdevice->hid_dev.HidDevice);
				//ms_message("WAIT_TIMEOUT: get hid failed");
				return 0;
			}
			else if(ret == WAIT_FAILED)
			{
				ms_message("Read Wait Error");
				return HID_ERROR_DEVICEFAILURE;
			}
			GetOverlappedResult(ph->jdevice->hid_dev.HidDevice, &overlapped, 
					&BytesRead, FALSE);
		}
		else
		{
			ms_message("get hid failed");
			return HID_ERROR_DEVICEFAILURE;
		}
	}

	UnpackReport(ph->jdevice->hid_dev.InputReportBuffer,
		ph->jdevice->hid_dev.Caps.InputReportByteLength,
		HidP_Input,
		ph->jdevice->hid_dev.InputData,
		ph->jdevice->hid_dev.InputDataLength,
		ph->jdevice->hid_dev.Ppd);

	PHID_DATA Data = ph->jdevice->hid_dev.InputData;
    for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
	{
		if (Data->IsButtonData)
		{
			ms_message("CURRENT BUTTON DATA REPORT[1]=0x%0.2X RID=0x%0.2X PAGE=0x%0.2X ID=0x%0.2X",
				ph->jdevice->hid_dev.InputReportBuffer[1],
				Data->ReportID,
				Data->UsagePage,
				Data->ButtonData.Usages[0]);
		}
	}

	Data = ph->jdevice->hid_dev.InputData;
    for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
	{
		if (Data->IsButtonData && Data->ButtonData.Usages[0]==0x00)
		{
			if (ph->jdevice->audioenabled==0
				&& ph->jdevice->hid_dev.InputReportBuffer[1]==0x02
				&& ph->jdevice->hid_dev.InputReportBuffer[2]==0x00)
			{
				ms_message("ZMM USBPHONE HID_EVENTS_HOOK");
				zmm_set_audioenabled(ph, 1);
				return HID_EVENTS_HOOK;
			}
			else if (ph->jdevice->audioenabled==1
				&& ph->jdevice->hid_dev.InputReportBuffer[1]==0x02
				&& ph->jdevice->hid_dev.InputReportBuffer[2]==0x01)
			{
				ms_message("ZMM USBPHONE HID_EVENTS_HANGUP");
				zmm_set_audioenabled(ph, 0);
				return HID_EVENTS_HANGUP;
			}
		}
	}
	return 0;
}

static int hd_init(hid_hooks_t *ph)
{
	return 0;
}

static int hd_uninit(hid_hooks_t *ph)
{
	return 0;
}

struct hid_device_desc zmm_usbphone = {
	ZMM_VENDOR_ID,
	ZMM_PRODUCT_ID_USBPHONE,

	hd_init,
	hd_uninit,

	NULL,
	NULL,
	zmm_set_audioenabled,
	NULL,
	NULL,
	NULL,
	NULL,
	zmm_get_audioenabled,
	NULL,
	zmm_get_events,
};

#endif
