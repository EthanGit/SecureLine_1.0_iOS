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

#define MICROSOFT_VENDOR_ID 0x045e
#define MICROSOFT_PRODUCT_ID_CATALINA 0xffca

int catalina_set_presenceindicator(hid_hooks_t *ph, int value)
{
	PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (value<0)
		value=0x00;
	if (value>0xF)
		value=0x0F;

	ReportID = SetOutputValue(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0xff99, 0xff18, 0x00);
	if (value>0)
	{
		ReportID = SetOutputValue(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0xff99, 0xff18, value);
	}

	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		ms_message("CATALINA -> Device: led presence indicator (0x%0.2X)", value);
		return 0;
	}
	return -1;
}

int catalina_set_sendcalls(hid_hooks_t *ph, int enable)
{
	PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	//if (ph->jdevice->audioenabled>0)
	//	return 0; /* don't support sendcalls when audio is active !!! */

	//ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
	//	ph->jdevice->hid_dev.OutputDataLength,
	//	0x08, 0x2A);
	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x24); //0x19);
	if (enable>0)
	{
		//ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
		//	ph->jdevice->hid_dev.OutputDataLength,
		//	0x08, 0x2A, 0x2A);
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x24, 0x24); //0x19, 0x19);
	}

	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("CATALINA -> Device: led send calls");
		else
			ms_message("CATALINA -> Device: led send calls OFF");
		return 0;
	}
	return -1;
}

int catalina_set_messagewaiting(hid_hooks_t *ph, int enable)
{
	PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	//if (ph->jdevice->audioenabled>0)
	//	return 0; /* don't support messagewaiting when audio is active !!! */

	//ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
	//	ph->jdevice->hid_dev.OutputDataLength,
	//	0x08, 0x2A);
	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x19);
	if (enable>0)
	{
		//ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
		//	ph->jdevice->hid_dev.OutputDataLength,
		//	0x08, 0x2A, 0x2A);
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x19, 0x19);
	}

	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("CATALINA -> Device: led message waiting");
		else
			ms_message("CATALINA -> Device: led message waiting OFF");
		return 0;
	}
	return -1;
}

int catalina_set_ringer(hid_hooks_t *ph, int enable)
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
			ms_message("CATALINA -> Device: led ringer");
		else
			ms_message("CATALINA -> Device: led ringer OFF");
		ph->jdevice->ringer = enable;
		return 0;
	}
	return -1;
}

int catalina_set_audioenabled(hid_hooks_t *ph, int enable)
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
			ms_message("CATALINA -> Device: led audiolink & off hook/active call led");
		else
			ms_message("CATALINA -> Device: led audiolink & off hook/active call led OFF");
		ph->jdevice->audioenabled=enable;
		//ph->jdevice->ringer = 0;
		//ph->jdevice->mute = 0;
		return 0;
	}
	return -1;
}


int catalina_set_mute(hid_hooks_t *ph, int enable)
{
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->mute==enable)
		return 0;

	if (enable>0)
	{
		//ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
		//	ph->jdevice->hid_dev.OutputDataLength,
		//	0x08, 0x21, 0x21);
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x09, 0x09);
	}
	else
	{
		//ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
		//	ph->jdevice->hid_dev.OutputDataLength,
		//	0x08, 0x21, 0x00);
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x09, 0x00);
	}

	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("CATALINA -> Device: led mute");
		else
			ms_message("CATALINA -> Device: led unmute");
		ph->jdevice->mute=enable;
		return 0;
	}
	return -1;
}

int catalina_get_mute(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	return ph->jdevice->mute;
}

int catalina_get_audioenabled(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	return ph->jdevice->audioenabled;
}

int catalina_get_isattached(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	return ph->jdevice->isAttached;
}

int catalina_get_events(hid_hooks_t *ph)
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
		if (Data->ButtonData.Usages[0]==0x20)
		{
			if (ph->jdevice->audioenabled==0 && ph->jdevice->hid_dev.InputReportBuffer[i]==0x01)
			{
				hid_hooks_set_audioenabled(ph, 1);
				hid_event = HID_EVENTS_HOOK;
				ms_message("Catalina HOOK");
				return hid_event;
			}
		}

		if (Data->ButtonData.Usages[0]==0x2F)
		{
			if (ph->jdevice->mute>0)
			{
				hid_hooks_set_mute(ph, 0);
				hid_event = HID_EVENTS_UNMUTE;
				ms_message("Catalina UNMUTE");
				return hid_event;
			}
			else
			{
				hid_hooks_set_mute(ph, 1);
				hid_event = HID_EVENTS_MUTE;
				ms_message("Catalina MUTE");
				return hid_event;
			}
		}
	}

	/* 0x20 is disabled & audio is not active: disable */
	if (ph->jdevice->audioenabled>0 && ph->jdevice->hid_dev.InputReportBuffer[1]==0x00)
	{
		hid_hooks_set_audioenabled(ph, 0);
		hid_event = HID_EVENTS_HANGUP;
		ms_message("Catalina HANGUP");
		return hid_event;
	}

	return 0;
}

static int hd_init(hid_hooks_t *ph)
{
	ULONG ReportID=0x00;
	ReportID = ResetAllButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength);
	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		catalina_set_presenceindicator(ph, 0x0F);
		return 0;
	}

	catalina_set_presenceindicator(ph, 0x03);
	return -1;
}

static int hd_uninit(hid_hooks_t *ph)
{
	catalina_set_presenceindicator(ph, 0x03);
	return 0;
}

struct hid_device_desc microsoft_catalina = {
	MICROSOFT_VENDOR_ID,
	MICROSOFT_PRODUCT_ID_CATALINA,

	hd_init,
	hd_uninit,

	catalina_set_presenceindicator,
	catalina_set_ringer,
	catalina_set_audioenabled,
	catalina_set_mute,
	catalina_set_sendcalls,
	catalina_set_messagewaiting,
	catalina_get_mute,
	catalina_get_audioenabled,
	catalina_get_isattached,
	catalina_get_events,
};

#endif
