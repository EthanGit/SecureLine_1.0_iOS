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

#define PLANTRONICS_VENDOR_ID 0x047f
#define PLANTRONICS_PRODUCT_ID_CS60 0x0410
#define PLANTRONICS_PRODUCT_ID_SAVIOFFICE 0x0411
#define PLANTRONICS_PRODUCT_ID_SAVI7XX 0xAC01
#define PLANTRONICS_PRODUCT_ID_BTADAPTER 0x4254
#define PLANTRONICS_PRODUCT_ID_DA45 0xDA45
#define PLANTRONICS_PRODUCT_ID_MCD100 0xD101
#define PLANTRONICS_PRODUCT_ID_BUA200 0x0715
#define PLANTRONICS_PRODUCT_ID_C420 0xAA10

#define PLANTRONICS_PRODUCT_ID_C420_2 0xAA14
#define PLANTRONICS_PRODUCT_ID_A478USB 0xC011
#define PLANTRONICS_PRODUCT_ID_BT300 0x0415 

int cs60_set_ringer(hid_hooks_t *ph, int enable)
{
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->ringer==enable)
		return 0;

	ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x0B, 0x9E, 0x00);
	if (enable>0)
	{
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x0B, 0x9E, 0x9E);
	}

	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("CS60-USB PC -> Device: led ringer");
		else
			ms_message("CS60-USB PC -> Device: led ringer OFF");

		ph->jdevice->ringer=enable;
		return 0;
	}
	return -1;
}

int cs60_set_audioenabled(hid_hooks_t *ph, int enable)
{
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	memset(ph->jdevice->hid_dev.FeatureReportBuffer, 0,
		ph->jdevice->hid_dev.Caps.FeatureReportByteLength);

	if (ph->jdevice->audioenabled==enable)
		return 0;

	if (enable>0)
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.FeatureData,
			ph->jdevice->hid_dev.FeatureDataLength,
			0xFFA0, 0xA1, 0xA1);
	else
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.FeatureData,
			ph->jdevice->hid_dev.FeatureDataLength,
			0xFFA0, 0xA1, 0x00);
	if (SetFeature(&ph->jdevice->hid_dev, true)==TRUE)
	{
		ph->jdevice->audioenabled=enable;
		return 0;
	}
	return -1;
}


int cs60_set_mute(hid_hooks_t *ph, int enable)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	return -1;
}

int cs60_get_mute(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	return ph->jdevice->mute;
}

int cs60_get_audioenabled(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	return ph->jdevice->audioenabled;
}

int cs60_get_isattached(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	return ph->jdevice->isAttached;
}

int cs60_get_events(hid_hooks_t *ph)
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
		ms_message("CURRENT DATA REPORT[1]=0x%0.2X RID=0x%0.2X PAGE=0x%0.2X ID=0x%0.2X",
			ph->jdevice->hid_dev.InputReportBuffer[1],
			Data->ReportID,
			Data->UsagePage,
			Data->ButtonData.Usages[0]);
	}

	//CHECK OFF HOOK
	Data = ph->jdevice->hid_dev.InputData;
	USAGE UsageId=0;
	USAGE UsagePage=0;
	ULONG ReportID=999;
    for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
	{
		if (Data->ButtonData.Usages[0]==0xB7)
		{
			/* what is the real meaning of this button? */
			/* long press of this button is detaching audio automatically */
			/* while short press will only trigger this button but will keep */
			/* audio active. */
			hid_event = HID_EVENTS_TALK;
			if (ph->jdevice->audioenabled<=0)
				return 0; /* already hanged */
			ms_message("CS60-USB EVENT HID_EVENTS_TALK");
			//cs60_set_audioenabled(ph, 0);
			return hid_event;
		}
		if (Data->ButtonData.Usages[0]==0xB6)
		{
			ms_message("CS60-USB EVENT SMART (LONG MUTE)");
			hid_event = HID_EVENTS_SMART;
			return hid_event;
		}
		if (Data->ButtonData.Usages[0]==0xB5)
		{
			ms_message("CS60-USB EVENT MUTE/UNMUTE");
			hid_event = HID_EVENTS_UNMUTE;

			memset(ph->jdevice->hid_dev.FeatureReportBuffer, 0x00, ph->jdevice->hid_dev.Caps.FeatureReportByteLength);
			ph->jdevice->hid_dev.FeatureReportBuffer[0]=ph->jdevice->hid_dev.InputReportBuffer[0];
			if (fHidD_GetFeature(ph->jdevice->hid_dev.HidDevice, ph->jdevice->hid_dev.FeatureReportBuffer, ph->jdevice->hid_dev.Caps.FeatureReportByteLength))
			{
				if (ph->jdevice->hid_dev.FeatureReportBuffer[1]==0x03)
					hid_event = HID_EVENTS_MUTE;
			}
			return hid_event;
		}
		if (Data->ButtonData.Usages[0]==0xB4)
		{
			ms_message("CS60-USB EVENT FLASH (LONG VOL DOWN)");
			hid_event = HID_EVENTS_FLASHDOWN;

			return hid_event;
		}
		if (Data->ButtonData.Usages[0]==0xB3)
		{
			ms_message("CS60-USB EVENT FLASH (LONG VOL UP)");
			hid_event = HID_EVENTS_FLASHUP;

			return hid_event;
		}
		if (Data->ButtonData.Usages[0]==0xB2)
		{
			ms_message("CS60-USB EVENT VOLUME UP/DOWN");
			hid_event = HID_EVENTS_VOLUP;

			return hid_event;
		}
		if (Data->ButtonData.Usages[0]==0xB1)
		{
			ms_message("CS60-USB EVENT VOLUME UP/DOWN");
			hid_event = HID_EVENTS_VOLDOWN;

			return hid_event;
		}
	}

	//CHECK ON HOOK
	Data = ph->jdevice->hid_dev.InputData;
	UsageId=0;
	UsagePage=0;
	ReportID=999;
    for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
	{
		if (Data->ButtonData.Usages[0]==0xA1)
		{
			hid_event = HID_EVENTS_HOOK;
			if (ph->jdevice->audioenabled>0)
				return 0; /* already hooked */
			ms_message("CS60-USB EVENT HID_EVENTS_HOOK");
			ph->jdevice->audioenabled=1;
			return hid_event;
		}
	}

	if (ph->jdevice->audioenabled>0)
	{
		ms_message("CS60-USB EVENT HID_EVENTS_HANGUP");
		hid_event = HID_EVENTS_HANGUP;
		ph->jdevice->audioenabled = 0;
		return hid_event;
	}

	return 0; /* STATE modification: no event? */
}

static int hd_init(hid_hooks_t *ph)
{
	return 0;
}

static int hd_uninit(hid_hooks_t *ph)
{
	return 0;
}

//TODO: Only the HOOK & TALK button are currently
//working/supported. Even the HANGUP doesn't work.
//All buttons currently trigger the TALK button
//when you are in a call.
struct hid_device_desc plantronics_btadapter = {
	PLANTRONICS_VENDOR_ID,
	PLANTRONICS_PRODUCT_ID_BTADAPTER,

	hd_init,
	hd_uninit,

	NULL,
	cs60_set_ringer,
	cs60_set_audioenabled,
	cs60_set_mute,
	NULL,
	NULL,
	cs60_get_mute,
	cs60_get_audioenabled,
	cs60_get_isattached,
	cs60_get_events,
};

struct hid_device_desc plantronics_cs60 = {
	PLANTRONICS_VENDOR_ID,
	PLANTRONICS_PRODUCT_ID_CS60,

	hd_init,
	hd_uninit,

	NULL,
	cs60_set_ringer,
	cs60_set_audioenabled,
	cs60_set_mute,
	NULL,
	NULL,
	cs60_get_mute,
	cs60_get_audioenabled,
	cs60_get_isattached,
	cs60_get_events,
};

int A478USB_set_ringer(hid_hooks_t *ph, int enable)
{
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->ringer==enable)
		return 0;

	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0xFFA0, 0x9E);
	if (enable>0)
	{
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0xFFA0, 0x9E, 0x9E);
	}

	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("A478USB PC -> Device: led ringer");
		else
			ms_message("A478USB PC -> Device: led ringer OFF");

		ph->jdevice->ringer=enable;
		return 0;
	}
	return -1;
}

int A478USB_set_audioenabled(hid_hooks_t *ph, int enable)
{
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->audioenabled==enable)
		return 0;

	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0xFFA0, 0xDC);
	if (enable>0)
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0xFFA0, 0xDC, 0xDC);
	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("SAVI OFFICE -> Device: led audiolink & off hook/active call led");
		else
			ms_message("SAVI OFFICE -> Device: led audiolink & off hook/active call led OFF");
		ph->jdevice->audioenabled=enable;
		return 0;
	}
	return -1;
}

int A478USB_set_mute(hid_hooks_t *ph, int enable)
{
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->ringer==enable)
		return 0;

	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0xFFA0, 0xA5);
	if (enable>0)
	{
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0xFFA0, 0xA5, 0xA5);
	}

	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("A478USB PC -> Device: led ringer");
		else
			ms_message("A478USB PC -> Device: led ringer OFF");

		ph->jdevice->ringer=enable;
		return 0;
	}
	return -1;
}

int A478USB_get_events(hid_hooks_t *ph)
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
		ms_message("CURRENT DATA REPORT[1]=0x%0.2X RID=0x%0.2X PAGE=0x%0.2X ID=0x%0.2X",
			ph->jdevice->hid_dev.InputReportBuffer[1],
			Data->ReportID,
			Data->UsagePage,
			Data->ButtonData.Usages[0]);
	}

	//CHECK OFF HOOK
	Data = ph->jdevice->hid_dev.InputData;
	USAGE UsageId=0;
	USAGE UsagePage=0;
	ULONG ReportID=999;
	int skip=0;
    for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
	{
		if (Data->ButtonData.Usages[0]==0xB5)
		{
			ms_message("A478USB EVENT MUTE/UNMUTE");
			if (ph->jdevice->mute==0) {
				hid_event = HID_EVENTS_MUTE;
				ph->jdevice->mute=1;
				return hid_event;
			}
			skip=1;
		}
		if (Data->ButtonData.Usages[0]==0xB2)
		{
			ms_message("A478USB EVENT VOLUME UP/DOWN");
			hid_event = HID_EVENTS_VOLUP;

			return hid_event;
		}
		if (Data->ButtonData.Usages[0]==0xB1)
		{
			ms_message("A478USB EVENT VOLUME UP/DOWN");
			hid_event = HID_EVENTS_VOLDOWN;

			return hid_event;
		}
	}

	//CHECK ON HOOK
	Data = ph->jdevice->hid_dev.InputData;
	UsageId=0;
	UsagePage=0;
	ReportID=999;
    for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
	{
		if (Data->ButtonData.Usages[0]==0xB7)
		{
			if (ph->jdevice->audioenabled>0) {
				ms_message("A478USB EVENT HID_EVENTS_HANGUP");
				hid_event = HID_EVENTS_HANGUP;
				hid_hooks_set_audioenabled(ph, 0);
				return hid_event;
			} else {
				ms_message("A478USB EVENT HID_EVENTS_HOOK");
				hid_event = HID_EVENTS_HOOK;
				hid_hooks_set_audioenabled(ph, 1);
				return hid_event;
			}
		}
	}

	if (skip==0 && ph->jdevice->mute>0) {
		ms_message("A478USB EVENT HID_EVENTS_UNMUTE");
		hid_event = HID_EVENTS_UNMUTE;
		ph->jdevice->mute = 0;
		return hid_event;
	}

	return 0; /* STATE modification: no event? */
}

struct hid_device_desc plantronics_A478USB = {
	PLANTRONICS_VENDOR_ID,
	PLANTRONICS_PRODUCT_ID_A478USB,

	hd_init,
	hd_uninit,

	NULL,
	A478USB_set_ringer,
	A478USB_set_audioenabled,
	A478USB_set_mute,
	NULL,
	NULL,
	cs60_get_mute,
	cs60_get_audioenabled,
	cs60_get_isattached,
	A478USB_get_events,
};

int savi_set_ringer(hid_hooks_t *ph, int enable)
{
	PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->ringer==enable)
		return 0;

	//ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
	//	ph->jdevice->hid_dev.OutputDataLength,
	//	0x08, 0x2A);
	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x18);
	if (enable>0)
	{
		//ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
		//	ph->jdevice->hid_dev.OutputDataLength,
		//	0x08, 0x2A, 0x2A);
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x18, 0x18);
	}

	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("SAVI OFFICE -> Device: led ringer");
		else
			ms_message("SAVI OFFICE -> Device: led ringer OFF");
		ph->jdevice->ringer = enable;
		return 0;
	}
	return -1;
}

int savi_set_audioenabled(hid_hooks_t *ph, int enable)
{
	PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->audioenabled==enable)
		return 0;

	//ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
	//	ph->jdevice->hid_dev.OutputDataLength,
	//	0x08, 0x2A);
	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x17);
	if (enable>0)
	{
		//ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
		//	ph->jdevice->hid_dev.OutputDataLength,
		//	0x08, 0x2A, 0x2A);
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x17, 0x17);
	}

	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("SAVI OFFICE -> Device: led audiolink & off hook/active call led");
		else
			ms_message("SAVI OFFICE -> Device: led audiolink & off hook/active call led OFF");
		ph->jdevice->audioenabled=enable;
		//ph->jdevice->ringer = 0;
		//ph->jdevice->mute = 0;
		return 0;
	}
	return -1;
}

int savi_set_mute(hid_hooks_t *ph, int enable)
{
	PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->mute==enable)
		return 0;

	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x09);
	if (enable>0)
	{
		//ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
		//	ph->jdevice->hid_dev.OutputDataLength,
		//	0x08, 0x2A, 0x2A);
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x09, 0x09);
	}
	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("SAVI OFFICE -> Device: led mute");
		else
			ms_message("SAVI OFFICE -> Device: led unmute");
		ph->jdevice->mute=enable;
		return 0;
	}
	return -1;
}

int savi_get_events(hid_hooks_t *ph)
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
		ms_message("SAVI OFFICE CURRENT DATA REPORT[1]=0x%0.2X RID=0x%0.2X PAGE=0x%0.2X ID=0x%0.2X",
			ph->jdevice->hid_dev.InputReportBuffer[1],
			Data->ReportID,
			Data->UsagePage,
			Data->ButtonData.Usages[0]);
	}

	{
		Data = ph->jdevice->hid_dev.InputData;
		USAGE UsageId=0;
		USAGE UsagePage=0;
		ULONG ReportID=999;
		int skip = 0;
		for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
		{
			if (Data->ButtonData.Usages[0]==0x20)
			{
				//HOOKSWITCH BUTTON
				hid_event = HID_EVENTS_HOOK;
				if (ph->jdevice->audioenabled>0)
				{
					skip=1;
					break; /* still hooked: continue */
				}
				ms_message("SAVI OFFICE EVENT HID_EVENTS_HOOK");
				//ph->jdevice->audioenabled=1;
				hid_hooks_set_audioenabled(ph, 1);
				return hid_event;
			}			
		}
		if (skip==0 && ph->jdevice->audioenabled>0)
		{
			//audio is not active any more
			ms_message("SAVI OFFICE EVENT HID_EVENTS_HANGUP");
			hid_event = HID_EVENTS_HANGUP;
			//ph->jdevice->mute=0;
			//ph->jdevice->audioenabled=0;
			hid_hooks_set_mute(ph, 0);
			hid_hooks_set_audioenabled(ph, 0);
			return hid_event;
		}

		Data = ph->jdevice->hid_dev.InputData;
		UsageId=0;
		UsagePage=0;
		ReportID=999;
		skip = 0;
		for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
		{
			if (Data->ButtonData.Usages[0]==0x2F)
			{
				if (ph->jdevice->mute>=1)
				{
					skip=1;
					continue; /* still muted: continue */
				}
				hid_event = HID_EVENTS_MUTE;
				ms_message("SAVI OFFICE EVENT HID_EVENTS_MUTE");
				//ph->jdevice->mute=1;
				hid_hooks_set_mute(ph, 1);
				return hid_event;
			}
			if (Data->ButtonData.Usages[0]==0x21)
			{
				ms_message("SAVI OFFICE EVENT HID_EVENTS_FLASHUP");
				return HID_EVENTS_FLASHUP;
			}
			if (Data->ButtonData.Usages[0]==0x23) /* Hold */
			{
				ms_message("SAVI OFFICE EVENT 0x23");
				return 0;
			}
			if (Data->ButtonData.Usages[0]==0x2B) /* 0x2B	Speaker Phone */
			{
				ms_message("SAVI OFFICE EVENT 0x2B");
				return 0;
			}
		}

		//PHONE MUTE BUTTON REMOVED
		if (skip==0 && ph->jdevice->mute>0)
		{
			hid_event = HID_EVENTS_UNMUTE; /* already mute */
			ms_message("C420 EVENT HID_EVENTS_UNMUTE");
			//ph->jdevice->mute=0;
			hid_hooks_set_mute(ph, 0);
			return hid_event;
		}

		return 0;
	}
	return -1;
}

struct hid_device_desc plantronics_savioffice  = {
	PLANTRONICS_VENDOR_ID,
	PLANTRONICS_PRODUCT_ID_SAVIOFFICE,

	hd_init,
	hd_uninit,

	NULL,
	savi_set_ringer,
	savi_set_audioenabled,
	savi_set_mute,
	NULL,
	NULL,
	cs60_get_mute,
	cs60_get_audioenabled,
	cs60_get_isattached,
	savi_get_events,
};

struct hid_device_desc plantronics_savi7xx  = {
	PLANTRONICS_VENDOR_ID,
	PLANTRONICS_PRODUCT_ID_SAVI7XX,

	hd_init,
	hd_uninit,

	NULL,
	savi_set_ringer,
	savi_set_audioenabled,
	savi_set_mute,
	NULL,
	NULL,
	cs60_get_mute,
	cs60_get_audioenabled,
	cs60_get_isattached,
	savi_get_events,
};

int da45_set_audioenabled(hid_hooks_t *ph, int enable)
{
	PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	memset(ph->jdevice->hid_dev.FeatureReportBuffer, 0,
		ph->jdevice->hid_dev.Caps.FeatureReportByteLength);

	if (ph->jdevice->audioenabled==enable)
		return 0;

	//off this one
	//ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
	//	ph->jdevice->hid_dev.OutputDataLength,
	//	0x08, 0x1E);

	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x17);
	if (enable>0)
	{
		//ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
		//	ph->jdevice->hid_dev.OutputDataLength,
		//	0x08, 0x2A, 0x2A);
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x17, 0x17);
	}
	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		ph->jdevice->ringer = 0;
		ph->jdevice->audioenabled=enable;
		if (enable)
			ms_message("DA45 -> Device: led audioenabled");
		else
			ms_message("DA45 -> Device: led audioenabled OFF");
		//ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
		//	ph->jdevice->hid_dev.OutputDataLength,
		//	0x08, 0x1E, 0x1E);
		//Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID);
		return 0;
	}
	return -1;
}

int da45_set_ringer(hid_hooks_t *ph, int enable)
{
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->ringer==enable)
		return 0;

	if (enable>0 && ph->jdevice->audioenabled>0)
	{
		ms_warning("disable audio first");
		da45_set_audioenabled(ph, 0);
	}

	//off this one
	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x1E);

	//ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
	//	ph->jdevice->hid_dev.OutputDataLength,
	//	0x08, 0x18);
	if (enable>0)
	{
		//ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
		//	ph->jdevice->hid_dev.OutputDataLength,
		//	0x08, 0x18, 0x18);
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x1E, 0x1E);
	}

	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("DA45 -> Device: led ringer");
		else
			ms_message("DA45 -> Device: led ringer OFF");

		ph->jdevice->ringer=enable;

		//ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
		//	ph->jdevice->hid_dev.OutputDataLength,
		//	0x08, 0x1E, 0x1E);
		//Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID);
		return 0;
	}
	return -1;
}

int da45_set_mute(hid_hooks_t *ph, int enable)
{
	PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	memset(ph->jdevice->hid_dev.FeatureReportBuffer, 0,
		ph->jdevice->hid_dev.Caps.FeatureReportByteLength);

	if (ph->jdevice->mute==enable)
		return 0;

	//off this one
	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x1E);

	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x09);
	if (enable>0)
	{
		//ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
		//	ph->jdevice->hid_dev.OutputDataLength,
		//	0x08, 0x2A, 0x2A);
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x09, 0x09);
	}
	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		ph->jdevice->mute=enable;
		if (enable)
			ms_message("DA45 -> Device: led mute");
		else
			ms_message("DA45 -> Device: led mute OFF");

		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x1E, 0x1E);
		Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID);
		return 0;
	}
	return -1;
}

int da45_get_events(hid_hooks_t *ph)
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
		ms_message("CURRENT DATA REPORT[1]=0x%0.2X RID=0x%0.2X PAGE=0x%0.2X ID=0x%0.2X",
			ph->jdevice->hid_dev.InputReportBuffer[1],
			Data->ReportID,
			Data->UsagePage,
			Data->ButtonData.Usages[0]);
	}

	Data = ph->jdevice->hid_dev.InputData;
	USAGE UsageId=0;
	USAGE UsagePage=0;
	ULONG ReportID=999;
	for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
	{
		if (Data->ButtonData.UsageMin==0x2F && Data->ButtonData.Usages[0]==0x2F)	{
			if (ph->jdevice->mute==0)
			{
				hid_event = HID_EVENTS_MUTE;
				ms_message("DA45 EVENT HID_EVENTS_MUTE");
				ph->jdevice->mute=1;
				return hid_event;
			}
			continue;
		} else if (Data->ButtonData.UsageMin==0x2F && Data->ButtonData.Usages[0]==0x00) {
			//PHONE MUTE BUTTON
			if (ph->jdevice->mute>0)
			{
				hid_event = HID_EVENTS_UNMUTE; /* already mute */
				ms_message("DA45 EVENT HID_EVENTS_UNMUTE");
				ph->jdevice->mute=0;
				return hid_event;
			}
			continue;
		}
		if (Data->ButtonData.UsageMin==0x21 && Data->ButtonData.Usages[0]==0x21) {
			if (Data->ButtonData.Usages[0]==0x21)
			{
				hid_event = HID_EVENTS_FLASHUP;
				ms_message("DA45 EVENT HID_EVENTS_FLASHUP");
				return hid_event;
			}
			continue;
		}
		if (Data->ButtonData.UsageMin==0x20 && Data->ButtonData.Usages[0]==0x20) {
			if (ph->jdevice->audioenabled==0)
			{
				//HOOKSWITCH BUTTON state
				hid_event = HID_EVENTS_HOOK;
				ms_message("DA45 EVENT HID_EVENTS_HOOK");
				ph->jdevice->audioenabled = 1;
				ph->jdevice->ringer = 0;
				return hid_event;
			}
			continue;
		}
		if (Data->ButtonData.UsageMin==0x20 && Data->ButtonData.Usages[0]==0x00) {
			//Don't know why but this can't be used

			if (ph->jdevice->audioenabled>0)
			{
				//HANGUP BUTTON state
				ms_message("DA45 EVENT HID_EVENTS_HANGUP");
				hid_event = HID_EVENTS_HANGUP;
				ph->jdevice->audioenabled = 0;
				ph->jdevice->ringer = 0;
				return hid_event;
			}
			continue;
		}
	}

	return 0;
}

struct hid_device_desc plantronics_da45 = {
	PLANTRONICS_VENDOR_ID,
	PLANTRONICS_PRODUCT_ID_DA45,

	hd_init,
	hd_uninit,

	NULL,
	da45_set_ringer,
	da45_set_audioenabled,
	da45_set_mute,
	NULL,
	NULL,
	cs60_get_mute,
	cs60_get_audioenabled,
	cs60_get_isattached,
	da45_get_events,
};

int mcd100_set_audioenabled(hid_hooks_t *ph, int enable)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->audioenabled==enable)
		return 0;

	ph->jdevice->audioenabled=enable;
	if (enable)
		ms_message("mcd100 -> Device: led audioenabled");
	else
		ms_message("mcd100 -> Device: led audioenabled OFF");
	return 0;
}

int mcd100_get_audioenabled(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	return ph->jdevice->audioenabled;
}

int mcd100_get_events(hid_hooks_t *ph)
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
		ms_message("CURRENT DATA REPORT[1]=0x%0.2X RID=0x%0.2X PAGE=0x%0.2X ID=0x%0.2X",
			ph->jdevice->hid_dev.InputReportBuffer[1],
			Data->ReportID,
			Data->UsagePage,
			Data->ButtonData.Usages[0]);
	}

	return 0;
}

struct hid_device_desc plantronics_mcd100 = {
	PLANTRONICS_VENDOR_ID,
	PLANTRONICS_PRODUCT_ID_MCD100,

	hd_init,
	hd_uninit,

	NULL,
	NULL,
	mcd100_set_audioenabled,
	NULL,
	NULL,
	NULL,
	NULL,
	cs60_get_audioenabled,
	cs60_get_isattached,
	mcd100_get_events,
};


int plantronics_set_ringer_08_18(hid_hooks_t *ph, int enable)
{
	PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->ringer==enable)
		return 0;

	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x18);
	if (enable>0)
	{
		//ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
		//	ph->jdevice->hid_dev.OutputDataLength,
		//	0x08, 0x2A, 0x2A);
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x18, 0x18);
	}

	if (WriteOneOutputButton(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID, 0x08, 0x18)==TRUE)
	{
		if (enable)
			ms_message("BUA200 -> Device: led ringer");
		else
			ms_message("BUA200 -> Device: led ringer OFF");
		ph->jdevice->ringer = enable;
		return 0;
	}
	return -1;
}

int plantronics_set_audioenabled_08_17(hid_hooks_t *ph, int enable)
{
	PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->audioenabled==enable)
		return 0;

	//ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
	//	ph->jdevice->hid_dev.OutputDataLength,
	//	0x08, 0x2A);
	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x17);
	if (enable>0)
	{
		//ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
		//	ph->jdevice->hid_dev.OutputDataLength,
		//	0x08, 0x2A, 0x2A);
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x17, 0x17);
	}

	if (WriteOneOutputButton(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID, 0x08, 0x17)==TRUE)
	{
		if (enable)
			ms_message("BUA200 -> Device: led audiolink & off hook/active call led");
		else
			ms_message("BUA200 -> Device: led audiolink & off hook/active call led OFF");
		ph->jdevice->audioenabled=enable;
		//ph->jdevice->ringer = 0;
		//ph->jdevice->mute = 0;
		return 0;
	}
	return -1;
}

int plantronics_set_mute_08_09(hid_hooks_t *ph, int enable)
{
	PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->mute==enable)
		return 0;

	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x09);
	if (enable>0)
	{
		//ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
		//	ph->jdevice->hid_dev.OutputDataLength,
		//	0x08, 0x2A, 0x2A);
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x09, 0x09);
	}
	if (WriteOneOutputButton(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID, 0x08, 0x09)==TRUE)
	{
		if (enable)
			ms_message("BUA200 -> Device: led mute");
		else
			ms_message("BUA200 -> Device: led unmute");
		ph->jdevice->mute=enable;
		return 0;
	}
	return -1;
}

int bua200_get_events(hid_hooks_t *ph)
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
		ms_message("BUA200 CURRENT DATA REPORT[1]=0x%0.2X RID=0x%0.2X PAGE=0x%0.2X ID=0x%0.2X",
			ph->jdevice->hid_dev.InputReportBuffer[1],
			Data->ReportID,
			Data->UsagePage,
			Data->ButtonData.Usages[0]);
	}

	{
		Data = ph->jdevice->hid_dev.InputData;
		USAGE UsageId=0;
		USAGE UsagePage=0;
		ULONG ReportID=999;
		int skip = 0;
		for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
		{
			if (Data->ButtonData.Usages[0]==0x20)
			{
				//HOOKSWITCH BUTTON
				hid_event = HID_EVENTS_HOOK;
				if (ph->jdevice->audioenabled>0)
				{
					skip=1;
					break; /* still hooked: continue */
				}
				ms_message("BUA200 EVENT HID_EVENTS_HOOK");
				ph->jdevice->audioenabled=1;
				//savi_set_audioenabled(ph, 1);
				return hid_event;
			}			
		}
		if (skip==0 && ph->jdevice->audioenabled>0)
		{
			//audio is not active any more
			ms_message("BUA200 EVENT HID_EVENTS_HANGUP");
			hid_event = HID_EVENTS_HANGUP;
			ph->jdevice->mute=0;
			ph->jdevice->audioenabled=0;
			//savi_set_audioenabled(ph, 0);
			return hid_event;
		}

		Data = ph->jdevice->hid_dev.InputData;
		UsageId=0;
		UsagePage=0;
		ReportID=999;
		for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
		{
			if (Data->ButtonData.Usages[0]==0x2F)
			{
				hid_event = HID_EVENTS_MUTE;
				ms_message("BUA200 EVENT HID_EVENTS_MUTE");
				ph->jdevice->mute=1;
				return hid_event;
			}
			if (Data->ButtonData.Usages[0]==0x21)
			{
				ms_message("BUA200 EVENT HID_EVENTS_FLASHUP");
				return HID_EVENTS_FLASHUP;
			}
		}

		//PHONE MUTE BUTTON REMOVED
		if (ph->jdevice->mute>0)
		{
			hid_event = HID_EVENTS_UNMUTE; /* already mute */
			ms_message("BUA200 EVENT HID_EVENTS_UNMUTE");
			ph->jdevice->mute=0;
			return hid_event;
		}

		return 0;
	}
	return -1;
}

struct hid_device_desc plantronics_bua200  = {
	PLANTRONICS_VENDOR_ID,
	PLANTRONICS_PRODUCT_ID_BUA200,

	hd_init,
	hd_uninit,

	NULL,
	plantronics_set_ringer_08_18,
	plantronics_set_audioenabled_08_17,
	plantronics_set_mute_08_09,
	NULL,
	NULL,
	cs60_get_mute,
	cs60_get_audioenabled,
	cs60_get_isattached,
	bua200_get_events,
};

int c420_get_events(hid_hooks_t *ph)
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
		ms_message("C420 CURRENT DATA REPORT[1]=0x%0.2X RID=0x%0.2X PAGE=0x%0.2X ID=0x%0.2X",
			ph->jdevice->hid_dev.InputReportBuffer[1],
			Data->ReportID,
			Data->UsagePage,
			Data->ButtonData.Usages[0]);
	}

	{
		Data = ph->jdevice->hid_dev.InputData;
		USAGE UsageId=0;
		USAGE UsagePage=0;
		ULONG ReportID=999;
		int skip = 0;
		for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
		{
			if (Data->ButtonData.Usages[0]==0x20)
			{
				//HOOKSWITCH BUTTON
				hid_event = HID_EVENTS_HOOK;
				if (ph->jdevice->audioenabled>0)
				{
					skip=1;
					break; /* still hooked: continue */
				}
				ms_message("C420 EVENT HID_EVENTS_HOOK");
				//ph->jdevice->audioenabled=1;
				hid_hooks_set_audioenabled(ph, 1);
				return hid_event;
			}			
		}
		if (skip==0 && ph->jdevice->audioenabled>0)
		{
			//audio is not active any more
			ms_message("C420 EVENT HID_EVENTS_HANGUP");
			hid_event = HID_EVENTS_HANGUP;
			//ph->jdevice->mute=0;
			//ph->jdevice->audioenabled=0;
			hid_hooks_set_mute(ph, 0);
			hid_hooks_set_audioenabled(ph, 0);
			return hid_event;
		}

		Data = ph->jdevice->hid_dev.InputData;
		UsageId=0;
		UsagePage=0;
		ReportID=999;
		skip = 0;
		for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
		{
			if (Data->ButtonData.Usages[0]==0x2F)
			{
				if (ph->jdevice->mute>=1)
				{
					skip=1;
					continue; /* still muted: continue */
				}
				hid_event = HID_EVENTS_MUTE;
				ms_message("C420 EVENT HID_EVENTS_MUTE");
				//ph->jdevice->mute=1;
				hid_hooks_set_mute(ph, 1);
				return hid_event;
			}
			if (Data->ButtonData.Usages[0]==0x21)
			{
				ms_message("C420 EVENT HID_EVENTS_FLASHUP");
				return HID_EVENTS_FLASHUP;
			}
		}

		//PHONE MUTE BUTTON REMOVED
		if (skip==0 && ph->jdevice->mute>0)
		{
			hid_event = HID_EVENTS_UNMUTE; /* already mute */
			ms_message("C420 EVENT HID_EVENTS_UNMUTE");
			//ph->jdevice->mute=0;
			hid_hooks_set_mute(ph, 0);
			return hid_event;
		}

		return 0;
	}
	return -1;
}

struct hid_device_desc plantronics_c420 = {
	PLANTRONICS_VENDOR_ID,
	PLANTRONICS_PRODUCT_ID_C420,

	hd_init,
	hd_uninit,

	NULL,
	plantronics_set_ringer_08_18,
	plantronics_set_audioenabled_08_17,
	plantronics_set_mute_08_09,
	NULL,
	NULL,
	cs60_get_mute,
	cs60_get_audioenabled,
	cs60_get_isattached,
	c420_get_events,
};

struct hid_device_desc plantronics_c420_2 = {
	PLANTRONICS_VENDOR_ID,
	PLANTRONICS_PRODUCT_ID_C420_2,

	hd_init,
	hd_uninit,

	NULL,
	plantronics_set_ringer_08_18,
	plantronics_set_audioenabled_08_17,
	plantronics_set_mute_08_09,
	NULL,
	NULL,
	cs60_get_mute,
	cs60_get_audioenabled,
	cs60_get_isattached,
	c420_get_events,
};

struct hid_device_desc plantronics_BT300 = {
	PLANTRONICS_VENDOR_ID,
	PLANTRONICS_PRODUCT_ID_BT300,

	hd_init,
	hd_uninit,

	NULL,
	plantronics_set_ringer_08_18,
	plantronics_set_audioenabled_08_17,
	plantronics_set_mute_08_09,
	NULL,
	NULL,
	cs60_get_mute,
	cs60_get_audioenabled,
	cs60_get_isattached,
	c420_get_events,
};


#endif
