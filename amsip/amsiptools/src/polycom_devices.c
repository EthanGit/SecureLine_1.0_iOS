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

#define POLYCOM_VENDOR_ID 0x095D
#define POLYCOM_PRODUCT_ID_COMMUNICATOR 0x0005

/*
Note: currently have some issue with RED color...

colors:
0x04: RED - not always working?
0x05: REMOVE RED

0x06: GREEN
0x07: REMOVE GREEN

0x11: GREEN
0x12: RED - not always working?
0x13: BLINKING RED
0x14: BLINKING GREEN
0x15: BLINKING ORANGE
*/

int current_presencecolor = 0x07; //0x00; // no color!

int cx100_set_color(hid_hooks_t *ph, int value)
{
	PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->hid_dev.Attributes.VendorID != POLYCOM_VENDOR_ID)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->jdevice->hid_dev.Attributes.ProductID != POLYCOM_PRODUCT_ID_COMMUNICATOR)
		return HID_ERROR_NODEVICEFOUND;

	if (value<0)
		value=0x00;
	if (value>0x7F)
		value=0x7F;

	ReportID = SetOutputValue(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0xffA1, 0x0005, 0x05);
	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)!=TRUE)
	{
		return HID_ERROR_DEVICEFAILURE;
	}
	ReportID = SetOutputValue(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0xffA1, 0x0005, 0x07);
	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)!=TRUE)
	{
		return HID_ERROR_DEVICEFAILURE;
	}
	if (value>=0)
	{
		ReportID = SetOutputValue(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0xffA1, 0x0005, value);

		if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
		{
			ms_message("cx100 PC -> Device: led color (0x%0.2X)", value);
			return 0;
		}
	}

	return HID_ERROR_DEVICEFAILURE;
}


int cx100_set_presenceindicator(hid_hooks_t *ph, int value)
{
	PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->hid_dev.Attributes.VendorID != POLYCOM_VENDOR_ID)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->jdevice->hid_dev.Attributes.ProductID != POLYCOM_PRODUCT_ID_COMMUNICATOR)
		return HID_ERROR_NODEVICEFOUND;

	if (value<0)
		value=0x00;
	if (value>0x7F)
		value=0x7F;

	//this color will be used when audioenabled==false
	current_presencecolor = value;
	if (ph->jdevice->audioenabled==0)
		cx100_set_color(ph, current_presencecolor);
	return -1;
}

int cx100_set_sendcalls(hid_hooks_t *ph, int enable)
{
	return cx100_set_color(ph, 0x15);
}

int cx100_set_messagewaiting(hid_hooks_t *ph, int enable)
{
	return cx100_set_color(ph, 0x15);
}

int cx100_set_ringer(hid_hooks_t *ph, int enable)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->hid_dev.Attributes.VendorID != POLYCOM_VENDOR_ID)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->jdevice->hid_dev.Attributes.ProductID != POLYCOM_PRODUCT_ID_COMMUNICATOR)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->ringer==enable)
		return 0;

	cx100_set_color(ph, 0x14);
	if (enable)
		ms_message("cx100 PC -> Device: led ringer");
	else
		ms_message("cx100 PC -> Device: led ringer OFF");
	ph->jdevice->ringer = enable;
	return 0;
}

int cx100_set_audioenabled(hid_hooks_t *ph, int enable)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->hid_dev.Attributes.VendorID != POLYCOM_VENDOR_ID)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->jdevice->hid_dev.Attributes.ProductID != POLYCOM_PRODUCT_ID_COMMUNICATOR)
		return HID_ERROR_NODEVICEFOUND;

	if (enable)
		cx100_set_color(ph, 0x06);
	else
		cx100_set_color(ph, 0x07);

	if (enable)
		ms_message("cx100 PC -> Device: led audiolink & off hook/active call led");
	else
		ms_message("cx100 PC -> Device: led audiolink & off hook/active call led OFF");
	ph->jdevice->audioenabled=enable;
	ph->jdevice->ringer = 0;
	ph->jdevice->mute = 0;
	cx100_set_presenceindicator(ph, current_presencecolor);
	return 0;
}

int cx100_set_mute(hid_hooks_t *ph, int enable)
{
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->hid_dev.Attributes.VendorID != POLYCOM_VENDOR_ID)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->jdevice->hid_dev.Attributes.ProductID != POLYCOM_PRODUCT_ID_COMMUNICATOR)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->mute==enable)
		return 0;

	ms_message("cx100 PC -> Device: led mute/unmute: not implemented");
	return -1;
}

int cx100_get_mute(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	return ph->jdevice->mute;
}

int cx100_get_audioenabled(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	return ph->jdevice->audioenabled;
}

int cx100_get_isattached(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	return ph->jdevice->isAttached;
}

int cx100_get_events(hid_hooks_t *ph)
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
		else
		{
			ms_message("CURRENT VALUE DATA REPORT[1]=0x%0.2X RID=0x%0.2X PAGE=0x%0.2X ID=0x%0.2X",
				ph->jdevice->hid_dev.InputReportBuffer[1],
				Data->ReportID,
				Data->UsagePage,
				Data->ValueData.Usage);
		}
	}

	if (ph->jdevice->hid_dev.InputReportBuffer[1]==0x01)
	{
		ms_message("Polycom Communicator HID_EVENTS_VOLUP");
		return HID_EVENTS_VOLUP;
	}
	else if (ph->jdevice->hid_dev.InputReportBuffer[1]==0x02)
	{
		ms_message("Polycom Communicator HID_EVENTS_VOLDOWN");
		return HID_EVENTS_VOLDOWN;
	}
	else if (ph->jdevice->hid_dev.InputReportBuffer[1]==0x03)
	{
		ms_message("Polycom Communicator HID_EVENTS_SMART");
		return HID_EVENTS_SMART;
	}
	else if (ph->jdevice->hid_dev.InputReportBuffer[1]==0x04)
	{
		if (ph->jdevice->audioenabled==0)
		{
			ms_message("Polycom Communicator HID_EVENTS_HOOK");
			cx100_set_audioenabled(ph, 1);
			return HID_EVENTS_HOOK;
		}
		else
		{
			ms_message("Polycom Communicator HID_EVENTS_HANGUP");
			cx100_set_audioenabled(ph, 0);
			ph->jdevice->mute=0;
			return HID_EVENTS_HANGUP;
		}
	}
	else if (ph->jdevice->hid_dev.InputReportBuffer[1]==0x05)
	{
		if (ph->jdevice->mute==0)
		{
			ms_message("Polycom Communicator HID_EVENTS_MUTE");
			ph->jdevice->mute=1;
			return HID_EVENTS_MUTE;
		}
		else
		{
			ms_message("Polycom Communicator HID_EVENTS_UNMUTE");
			ph->jdevice->mute=0;
			if (ph->jdevice->audioenabled>0)
				cx100_set_color(ph, 0x06);
			return HID_EVENTS_UNMUTE;
		}
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
		cx100_set_color(ph, current_presencecolor);
		return 0;
	}

	cx100_set_color(ph, 0x04);
	return -1;
}

static int hd_uninit(hid_hooks_t *ph)
{
	cx100_set_color(ph, 0x07);
	return 0;
}

struct hid_device_desc polycom_communicator = {
	POLYCOM_VENDOR_ID,
	POLYCOM_PRODUCT_ID_COMMUNICATOR,

	hd_init,
	hd_uninit,

	NULL,
	cx100_set_ringer,
	cx100_set_audioenabled,
	cx100_set_mute,
	cx100_set_sendcalls,
	cx100_set_messagewaiting,
	cx100_get_mute,
	cx100_get_audioenabled,
	cx100_get_isattached,
	cx100_get_events,
};

#endif
