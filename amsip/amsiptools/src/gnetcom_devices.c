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

#define GNNETCOM_VENDOR_ID 0x0B0E

#define GNNETCOM_PRODUCT_ID_A330 0xA330
#define GNNETCOM_PRODUCT_ID_BIZ620USB 0x0930
#define GNNETCOM_PRODUCT_ID_DIAL520 0x0520
#define GNNETCOM_PRODUCT_ID_LINK280 0x0910
#define GNNETCOM_PRODUCT_ID_LINK350OC 0xA340
#define GNNETCOM_PRODUCT_ID_BIZ2400USB 0x091C
#define GNNETCOM_PRODUCT_ID_GO6470 0x1003
#define GNNETCOM_PRODUCT_ID_GO9470 0x1041

#define GNNETCOM_PRODUCT_ID_M5390 0xA335
#define GNNETCOM_PRODUCT_ID_M5390USB 0xA338
#define GNNETCOM_PRODUCT_ID_BIZ2400 0x090A

#define GNNETCOM_PRODUCT_ID_GN9350 0x9350
#define GNNETCOM_PRODUCT_ID_GN9330 0x9330

#define GNNETCOM_PRODUCT_ID_GN8120 0x8120

#define GNNETCOM_PRODUCT_ID_UC250 0x0341
#define GNNETCOM_PRODUCT_ID_UC550DUO 0x0030
#define GNNETCOM_PRODUCT_ID_UC550MONO 0x0031
#define GNNETCOM_PRODUCT_ID_UC150MONO 0x0043
#define GNNETCOM_PRODUCT_ID_UC150DUO 0x0041
#define GNNETCOM_PRODUCT_ID_BIZ2400MONOUSB 0x2400
#define GNNETCOM_PRODUCT_ID_PRO930 0x1016
#define GNNETCOM_PRODUCT_ID_PRO9450 0x1022

int gnetcom_set_sendcalls(hid_hooks_t *ph, int enable)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	return HID_ERROR_NOTIMPLEMENTED;
}

int gnetcom_set_messagewaiting(hid_hooks_t *ph, int enable)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	return HID_ERROR_NOTIMPLEMENTED;
}

int gnetcom_set_ringer(hid_hooks_t *ph, int enable)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if ((ph->jdevice->hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
		(ph->jdevice->hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_GN8120) )
	{
		char bytes[] = { 0x00, 0x00 };
		if (enable==0)
		{
			// Write changed LED status as a report event to the connected GN USB HID device
			bytes[1] = ((int) 0x23); // -> TURN OFF LED
		}
		else
		{
			bytes[1] = ((int) 0x24); // -> TURN ON LED
		}
		unsigned long BytesWritten;
		if (WriteFile(ph->jdevice->hid_dev.HidDevice, bytes, 2, &BytesWritten, NULL)) {
			ph->jdevice->ringer=enable;
			return 0;
		}
		return -1;
	}
	else if (
		(ph->jdevice->hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID
		&&ph->jdevice->hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_GN9350)
		||
		(ph->jdevice->hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID
		&&ph->jdevice->hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_GN9330)
		||
		(ph->jdevice->hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID
		&&ph->jdevice->hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_A330)
		)
	{
		ULONG ReportID=0x00;

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
				ms_message("GN9350/A330 PC -> Device: led ringer");
			else
				ms_message("GN9350/A330 PC -> Device: led ringer OFF");

			if ( (ph->jdevice->hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_GN9350)
				||(ph->jdevice->hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_GN9330) )
				ph->jdevice->ringer=0; /* one headset ring only */
			else
				ph->jdevice->ringer=enable;
			return 0;
		}
		return -1;
	}
	return -1;
}

int gnetcom_set_audioenabled(hid_hooks_t *ph, int enable)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if ((ph->jdevice->hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
		(ph->jdevice->hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_GN8120) )
	{
		char bytes[] = { 0x00, 0x00 };
		if (enable==0)
		{
			// Write changed LED status as a report event to the connected GN USB HID device
			bytes[1] = ((int) 0x02); // -> TURN OFF GREEN LED
		}
		else
		{
			bytes[1] = ((int) 0x01); // -> TURN ON GREEN LED
		}
		unsigned long BytesWritten;
		if (WriteFile(ph->jdevice->hid_dev.HidDevice, bytes, 2, &BytesWritten, NULL)) {
			ph->jdevice->audioenabled=enable;
			return 0;
		}
		return -1;
	}
	else if (
		(ph->jdevice->hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID
		&&ph->jdevice->hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_GN9350)
		||
		(ph->jdevice->hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID
		&&ph->jdevice->hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_GN9330)
		)
	{
		PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
		ULONG ReportID=0x00;

		if (ph->jdevice->audioenabled==enable)
			return 0;

		ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x0B, 0x2A);
		ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x0B, 0x2B);
		if (enable>0)
			ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
				ph->jdevice->hid_dev.OutputDataLength,
				0x0B, 0x2A, 0x2A);
		else
			ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
				ph->jdevice->hid_dev.OutputDataLength,
				0x0B, 0x2B, 0x2B);

		if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
		{
			if (enable)
				ms_message("GN9350 PC -> Device: led audiolink");
			else
				ms_message("GN9350 PC -> Device: led audiolink OFF");
			ph->jdevice->audioenabled=enable;
			return 0;
		}
		return -1;
	}
	else if (
		(ph->jdevice->hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID
		&&ph->jdevice->hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_A330)
		)
	{
		PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
		ULONG ReportID=0x00;

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
				ms_message("GN A330 PC -> Device: led audiolink & off hook/active call led");
			else
				ms_message("GN A330 PC -> Device: led audiolink & off hook/active call led OFF");
			ph->jdevice->audioenabled=enable;
			//ph->jdevice->ringer = 0;
			//ph->jdevice->mute = 0;
			return 0;
		}
		return -1;
	}
	return -1;
}


int gnetcom_set_mute(hid_hooks_t *ph, int enable)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if ((ph->jdevice->hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
		(ph->jdevice->hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_GN8120) )
	{
		char bytes[] = { 0x00, 0x00 };
		if (enable==0)
		{
			// Write changed LED status as a report event to the connected GN USB HID device
			bytes[1] = ((int) 0x09); // -> TURN OFF RED LED
		}
		else
		{
			bytes[1] = ((int) 0x0A); // -> TURN ON RED LED
		}
		unsigned long BytesWritten;
		if (WriteFile(ph->jdevice->hid_dev.HidDevice, bytes, 2, &BytesWritten, NULL)) {
			ph->jdevice->mute=enable;
			return 0;
		}
		return -1;
	}
	else if (
		(ph->jdevice->hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID
		&&ph->jdevice->hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_GN9350)
		||
		(ph->jdevice->hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID
		&&ph->jdevice->hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_GN9330)
		)
	{
		ULONG ReportID=0x00;

		if (ph->jdevice->mute==enable)
			return 0;

		//ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		//	ph->jdevice->hid_dev.OutputDataLength,
		//	0x0B, 0x21);
		if (enable>0)
			ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
				ph->jdevice->hid_dev.OutputDataLength,
				0x0B, 0x21, 0x21);
		else
			ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
				ph->jdevice->hid_dev.OutputDataLength,
				0x0B, 0x21, 0x00);

		if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
		{
			if (enable)
				ms_message("GN9350 PC -> Device: led mute");
			else
				ms_message("GN9350 PC -> Device: led unmute");
			ph->jdevice->mute=enable;
			return 0;
		}
		return -1;
	}
	else if (
		(ph->jdevice->hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID
		&&ph->jdevice->hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_A330)
		)
	{
		ULONG ReportID=0x00;

		if (ph->jdevice->mute==enable)
			return 0;

		if (enable>0)
		{
			ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
				ph->jdevice->hid_dev.OutputDataLength,
				0x08, 0x21, 0x21);
			ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
				ph->jdevice->hid_dev.OutputDataLength,
				0x08, 0x09, 0x09);
		}
		else
		{
			ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
				ph->jdevice->hid_dev.OutputDataLength,
				0x08, 0x21, 0x00);
			ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
				ph->jdevice->hid_dev.OutputDataLength,
				0x08, 0x09, 0x00);
		}

		if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
		{
			if (enable)
				ms_message("GN A330 PC -> Device: led mute");
			else
				ms_message("GN A330 PC -> Device: led unmute");
			ph->jdevice->mute=enable;
			return 0;
		}
		return -1;
	}
	return 0;
}

int gnetcom_get_mute(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	return ph->jdevice->mute;
}

int gnetcom_get_audioenabled(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	return ph->jdevice->audioenabled;
}

int gnetcom_get_isattached(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	return ph->jdevice->isAttached;
}

int gnetcom_get_events(hid_hooks_t *ph)
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

	if ((ph->jdevice->hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
		(ph->jdevice->hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_A330))
	{
		Data = ph->jdevice->hid_dev.InputData;
		USAGE UsageId=0;
		USAGE UsagePage=0;
		ULONG ReportID=999;
		for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
		{
			if (Data->ButtonData.Usages[0]==0x2A)
			{
				ms_message("A330 EVENT 0x2A");
				return 0;
			}
			if (Data->ButtonData.Usages[0]==0x2F)
			{
				//PHONE MUTE BUTTON
				if (ph->jdevice->mute>0)
				{
					hid_event = HID_EVENTS_UNMUTE; /* already mute */
					ms_message("A330 EVENT HID_EVENTS_UNMUTE");
					gnetcom_set_mute(ph, false);
					return hid_event;
				}
				hid_event = HID_EVENTS_MUTE;
				ms_message("A330 EVENT HID_EVENTS_MUTE");
				gnetcom_set_mute(ph, true);
				return hid_event;
			}
			if (Data->ButtonData.Usages[0]==0x20)
			{
				//HOOKSWITCH BUTTON
				hid_event = HID_EVENTS_HOOK;
				if (ph->jdevice->audioenabled>0)
					return 0; /* already hooked */
				ms_message("A330 EVENT HID_EVENTS_HOOK");
				gnetcom_set_audioenabled(ph, true);
				return hid_event;
			}			
			if (Data->ButtonData.Usages[0]==0x97)
			{
				//LINE BUSY
				ms_message("CURRENT DATA REPORT[1]=0x%0.2X RID=0x%0.2X PAGE=0x%0.2X ID=0x%0.2X",
					ph->jdevice->hid_dev.InputReportBuffer[1],
					Data->ReportID,
					Data->UsagePage,
					Data->ButtonData.Usages[0]);
			}
		}

		//CHECK ON HOOK
		Data = ph->jdevice->hid_dev.InputData;
		UsageId=0;
		UsagePage=0;
		ReportID=999;
		for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
		{
			if (Data->ButtonData.Usages[0]==0x97)
			{
				hid_event = HID_EVENTS_HOOK;
				if (ph->jdevice->audioenabled>0)
					return 0; /* already hooked */
				return 0;
				//ms_message("A330 EVENT HID_EVENTS_HOOK");
				//gnetcom_set_audioenabled(ph, true);
				//return hid_event;
			}
		}

		if (ph->jdevice->audioenabled>0)
		{
			ms_message("A330 EVENT HID_EVENTS_HANGUP");
			hid_event = HID_EVENTS_HANGUP;
			gnetcom_set_audioenabled(ph, false);
			return hid_event;
		}
		return 0;
	}

	Data = ph->jdevice->hid_dev.InputData;
	USAGE UsageId=0;
	USAGE UsagePage=0;
	ULONG ReportID=999;
    for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
	{
		if (Data->ButtonData.Usages[0]!=0)
		{
			UsageId = Data->ButtonData.Usages[0];
			UsagePage = Data->UsagePage;
			ReportID = Data->ReportID;
			break;
		}
	}

	ms_message("HID Event REPORT[1]=0x%0.2X RID=0x%0.2X PAGE=0x%0.2X ID=0x%0.2X",
		ph->jdevice->hid_dev.InputReportBuffer[1],
		ReportID,
		UsagePage,
		UsageId);

	if ((ph->jdevice->hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
		(ph->jdevice->hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_GN8120) )
	{
		if (ph->jdevice->hid_dev.InputReportBuffer[1]==0x08)
		{
			ms_message("GN8120 Event 0x08 ATTACH/DETACHED?");
			if (ph->jdevice->audioenabled>0)
			{
				hid_hooks_set_audioenabled(ph, 0);
			}
			else
			{
				hid_hooks_set_audioenabled(ph, 1);
			}

		}
		else if (ph->jdevice->hid_dev.InputReportBuffer[1]==0x10)
		{
			ms_message("GN8120 Event 0x10 ???");
		}
		else if (ph->jdevice->hid_dev.InputReportBuffer[1]==0x20)
		{
			ms_message("GN8120 Event 0x20 Mute?");
		}
		else if (ph->jdevice->hid_dev.InputReportBuffer[1]==0x40)
		{
			ms_message("GN8120 Event 0x20 VOLUME UP/DOWN?");
		}
		else if (ph->jdevice->hid_dev.InputReportBuffer[1]==0xffffff80)
		{
			ms_message("GN8120 Event 0xffffff80 VOLUME UP/DOWN?");
		}
	}
	else if ((ph->jdevice->hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
		((ph->jdevice->hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_GN9350)
		||(ph->jdevice->hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_GN9330)))
	{
		if (ph->jdevice->hid_dev.InputReportBuffer[1]==0x08
			&& UsageId==0x20)
		{
			ms_message("GN9350 EVENT ATTACH/DETACHED?");
			if (ph->jdevice->audioenabled>0)
			{
				hid_hooks_set_audioenabled(ph, 0);
				hid_event = HID_EVENTS_HANGUP;
			}
			else
			{
				hid_hooks_set_audioenabled(ph, 1);
				hid_event = HID_EVENTS_HOOK;
			}
		}
		else if (ph->jdevice->hid_dev.InputReportBuffer[1]==0x08
			&& UsageId==0x2F)
		{
			ms_message("GN9350 EVENT Mute");
			if (ph->jdevice->mute>0)
			{
				hid_hooks_set_mute(ph, 0);
				hid_event = HID_EVENTS_UNMUTE;
			}
			else
			{
				hid_hooks_set_mute(ph, 1);
				hid_event = HID_EVENTS_MUTE;
			}
		}
		else if (ph->jdevice->hid_dev.InputReportBuffer[1]==0x10)
		{
			ms_message("GN9350 EVENT ???");
		}
		else if (ph->jdevice->hid_dev.InputReportBuffer[1]==0x40
			&& UsageId==0xEA)
		{
			ms_message("GN9350 EVENT VOLUME DOWN");
			hid_event = HID_EVENTS_VOLDOWN;
		}
		else if (ph->jdevice->hid_dev.InputReportBuffer[1]==0xffffff80
			&& UsageId==0xE9)
		{
			ms_message("GN9350 EVENT VOLUME UP");
			hid_event = HID_EVENTS_VOLUP;
		}
	}

	if (ReportID==999)
		return 0; /* STATE modification: no event? */
	return hid_event;
}

static int hd_init(hid_hooks_t *ph)
{
	return 0;
}

static int hd_uninit(hid_hooks_t *ph)
{
	return 0;
}

struct hid_device_desc gnetcom_A330 = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_A330,

	hd_init,
	hd_uninit,

	NULL,
	gnetcom_set_ringer,
	gnetcom_set_audioenabled,
	gnetcom_set_mute,
	gnetcom_set_sendcalls,
	gnetcom_set_messagewaiting,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	gnetcom_get_events,
};

struct hid_device_desc gnetcom_M5390 = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_M5390,

	hd_init,
	hd_uninit,

	NULL,
	gnetcom_set_ringer,
	gnetcom_set_audioenabled,
	gnetcom_set_mute,
	gnetcom_set_sendcalls,
	gnetcom_set_messagewaiting,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	gnetcom_get_events,
};

struct hid_device_desc gnetcom_M5390USB = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_M5390USB,

	hd_init,
	hd_uninit,

	NULL,
	gnetcom_set_ringer,
	gnetcom_set_audioenabled,
	gnetcom_set_mute,
	gnetcom_set_sendcalls,
	gnetcom_set_messagewaiting,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	gnetcom_get_events,
};

struct hid_device_desc gnetcom_BIZ2400 = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_BIZ2400,

	hd_init,
	hd_uninit,

	NULL,
	gnetcom_set_ringer,
	gnetcom_set_audioenabled,
	gnetcom_set_mute,
	gnetcom_set_sendcalls,
	gnetcom_set_messagewaiting,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	gnetcom_get_events,
};

struct hid_device_desc gnetcom_GN9350 = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_GN9350,

	hd_init,
	hd_uninit,

	NULL,
	gnetcom_set_ringer,
	gnetcom_set_audioenabled,
	gnetcom_set_mute,
	gnetcom_set_sendcalls,
	gnetcom_set_messagewaiting,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	gnetcom_get_events,
};

struct hid_device_desc gnetcom_GN9330 = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_GN9330,

	hd_init,
	hd_uninit,

	NULL,
	gnetcom_set_ringer,
	gnetcom_set_audioenabled,
	gnetcom_set_mute,
	gnetcom_set_sendcalls,
	gnetcom_set_messagewaiting,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	gnetcom_get_events,
};

struct hid_device_desc gnetcom_GN8120 = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_GN8120,

	hd_init,
	hd_uninit,

	NULL,
	gnetcom_set_ringer,
	gnetcom_set_audioenabled,
	gnetcom_set_mute,
	gnetcom_set_sendcalls,
	gnetcom_set_messagewaiting,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	gnetcom_get_events,
};


int biz620_set_audioenabled(hid_hooks_t *ph, int enable)
{
	PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->audioenabled==enable)
		return 0;

	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x17);
	if (enable>0)
	{
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x17, 0x17);
	}

	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("BIZ620 USB -> Device: led audiolink & off hook/active call led");
		else
			ms_message("BIZ620 USB -> Device: led audiolink & off hook/active call led OFF");
		ph->jdevice->audioenabled=enable;
		return 0;
	}
	return -1;
}


int biz620_set_mute(hid_hooks_t *ph, int enable)
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
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x09, 0x09);
	}

	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("BIZ620/DIAL520 USB -> Device: mute led");
		else
			ms_message("BIZ620/DIAL520 USB -> Device: mute led OFF");
		ph->jdevice->mute=enable;
		return 0;
	}
	return -1;
}

int biz620_get_events(hid_hooks_t *ph)
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
		if (Data->ButtonData.UsageMin==0x20 && Data->ButtonData.Usages[0]==0x20) {
			if (ph->jdevice->audioenabled==0)
			{
				hid_hooks_set_audioenabled(ph, 1);
				hid_event = HID_EVENTS_HOOK;
				ms_message("BIZ620 HOOK");
				return hid_event;
			}
		}
		if (Data->ButtonData.UsageMin==0x20 && Data->ButtonData.Usages[0]==0x00) {
			if (ph->jdevice->audioenabled>0)
			{
				hid_hooks_set_audioenabled(ph, 0);
				hid_event = HID_EVENTS_HANGUP;
				ms_message("BIZ620 HANGUP");
				return hid_event;
			}
		}

		if (Data->ButtonData.UsageMin==0x2F && Data->ButtonData.Usages[0]==0x2F) {
			if (ph->jdevice->mute==0)
			{
				hid_event = HID_EVENTS_MUTE;
				ms_message("BIZ620 MUTE");
				ph->jdevice->mute=1;
				return hid_event;
			}
		} if (Data->ButtonData.UsageMin==0x2F && Data->ButtonData.Usages[0]==0x00) {
			if (ph->jdevice->mute>0)
			{
				hid_event = HID_EVENTS_UNMUTE;
				ms_message("BIZ620 UNMUTE");
				ph->jdevice->mute=0;
				return hid_event;
			}
		}
	}

	return 0;
}

struct hid_device_desc gnetcom_BIZ620USB = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_BIZ620USB,

	hd_init,
	hd_uninit,

	NULL,
	NULL,
	biz620_set_audioenabled,
	biz620_set_mute,
	NULL,
	NULL,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	biz620_get_events,
};

int dial520_set_ringer(hid_hooks_t *ph, int enable)
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
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x18, 0x18);
	}

	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("DIAL520 -> Device: led ringer");
		else
			ms_message("DIAL520 -> Device: led ringer OFF");
		ph->jdevice->ringer=enable;
		return 0;
	}
	return -1;
}

int dial520_set_audioenabled(hid_hooks_t *ph, int enable)
{
	PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->audioenabled==enable)
		return 0;

	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x18);
	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x17);
	if (enable>0)
	{
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x17, 0x17);
	}

	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("DIAL520 -> Device: led audiolink & off hook/active call led");
		else
			ms_message("DIAL520 -> Device: led audiolink & off hook/active call led OFF");
		ph->jdevice->audioenabled=enable;
		ph->jdevice->ringer=0;
		return 0;
	}
	return -1;
}

int dial520_get_events(hid_hooks_t *ph)
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
			ms_message("CURRENT DATA REPORT[1]=0x%0.2X RID=0x%0.2X PAGE=0x%0.2X ID=0x%0.2X",
				ph->jdevice->hid_dev.InputReportBuffer[1],
				Data->ReportID,
				Data->UsagePage,
				Data->ButtonData.Usages[0]);
		else
			ms_message("CURRENT VALUE DATA REPORT[1]=0x%0.2X RID=0x%0.2X PAGE=0x%0.2X ID=0x%0.2X",
				ph->jdevice->hid_dev.InputReportBuffer[1],
				Data->ReportID,
				Data->UsagePage,
				Data->ValueData.Value);
	}

	//CHECK OFF HOOK
	Data = ph->jdevice->hid_dev.InputData;
	USAGE UsageId=0;
	USAGE UsagePage=0;
	ULONG ReportID=999;
    for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
	{
		if (Data->IsButtonData && Data->ButtonData.UsageMin==0xB0 && Data->ButtonData.Usages[0]==0xB0) {
			ms_message("DIAL520 KEY 0");
			return HID_EVENTS_KEY0;
		}
		if (Data->IsButtonData && Data->ButtonData.UsageMin==0xB0 && Data->ButtonData.Usages[0]==0xB1) {
			ms_message("DIAL520 KEY 1");
			return HID_EVENTS_KEY1;
		}
		if (Data->IsButtonData && Data->ButtonData.UsageMin==0xB0 && Data->ButtonData.Usages[0]==0xB2) {
			ms_message("DIAL520 KEY 2");
			return HID_EVENTS_KEY2;
		}
		if (Data->IsButtonData && Data->ButtonData.UsageMin==0xB0 && Data->ButtonData.Usages[0]==0xB3) {
			ms_message("DIAL520 KEY 3");
			return HID_EVENTS_KEY3;
		}
		if (Data->IsButtonData && Data->ButtonData.UsageMin==0xB0 && Data->ButtonData.Usages[0]==0xB4) {
			ms_message("DIAL520 KEY 4");
			return HID_EVENTS_KEY4;
		}
		if (Data->IsButtonData && Data->ButtonData.UsageMin==0xB0 && Data->ButtonData.Usages[0]==0xB5) {
			ms_message("DIAL520 KEY 5");
			return HID_EVENTS_KEY5;
		}
		if (Data->IsButtonData && Data->ButtonData.UsageMin==0xB0 && Data->ButtonData.Usages[0]==0xB6) {
			ms_message("DIAL520 KEY 6");
			return HID_EVENTS_KEY6;
		}
		if (Data->IsButtonData && Data->ButtonData.UsageMin==0xB0 && Data->ButtonData.Usages[0]==0xB7) {
			ms_message("DIAL520 KEY 7");
			return HID_EVENTS_KEY7;
		}
		if (Data->IsButtonData && Data->ButtonData.UsageMin==0xB0 && Data->ButtonData.Usages[0]==0xB8) {
			ms_message("DIAL520 KEY 8");
			return HID_EVENTS_KEY8;
		}
		if (Data->IsButtonData && Data->ButtonData.UsageMin==0xB0 && Data->ButtonData.Usages[0]==0xB9) {
			ms_message("DIAL520 KEY 9");
			return HID_EVENTS_KEY9;
		}
		if (Data->IsButtonData && Data->ButtonData.UsageMin==0xB0 && Data->ButtonData.Usages[0]==0xBA) {
			ms_message("DIAL520 KEY *");
			return HID_EVENTS_KEYSTAR;
		}
		if (Data->IsButtonData && Data->ButtonData.UsageMin==0xB0 && Data->ButtonData.Usages[0]==0xBB) {
			ms_message("DIAL520 KEY B");
			return HID_EVENTS_KEYPOUND;
		}
		if (Data->IsButtonData && Data->ButtonData.UsageMin==0x07 && Data->ButtonData.Usages[0]==0x07) {
			ms_message("DIAL520 KEY ERASE");
			return HID_EVENTS_KEYERASE;
		}

		if (Data->IsButtonData && Data->ButtonData.UsageMin==0x20 && Data->ButtonData.Usages[0]==0x20) {
			if (ph->jdevice->audioenabled==0)
			{
				ph->jdevice->audioenabled=1;
				hid_event = HID_EVENTS_HOOK;
				ms_message("DIAL520 HOOK");
				ph->jdevice->ringer=0;
				return hid_event;
			}
		}

		if (Data->IsButtonData && Data->ButtonData.UsageMin==0x20 && Data->ButtonData.Usages[0]==0x00) {
			if (ph->jdevice->audioenabled>0)
			{
				ph->jdevice->audioenabled=0;
				hid_event = HID_EVENTS_HANGUP;
				ms_message("DIAL520 HANGUP");
				//dial520_set_ringer(ph, 0);
				return hid_event;
			}
		}

		if (Data->IsButtonData && Data->ButtonData.UsageMin==0x2F && Data->ButtonData.Usages[0]==0x2F) {
			if (ph->jdevice->mute==0)
			{
				hid_event = HID_EVENTS_MUTE;
				ms_message("DIAL520 MUTE");
				ph->jdevice->mute=1;
				return hid_event;
			}
		}

		if (Data->IsButtonData && Data->ButtonData.UsageMin==0x2F && Data->ButtonData.Usages[0]==0x00) {
			if (ph->jdevice->mute>0)
			{
				hid_event = HID_EVENTS_UNMUTE;
				ms_message("DIAL520 UNMUTE");
				ph->jdevice->mute=0;
				return hid_event;
			}
		}

		if (Data->IsButtonData && Data->ButtonData.UsageMin==0x21 && Data->ButtonData.Usages[0]==0x21) {
				hid_event = HID_EVENTS_FLASHUP;
				ms_message("DIAL520 HID_EVENTS_FLASHUP");
				return hid_event;
		}
	}

	return 0;
}

struct hid_device_desc gnetcom_DIAL520 = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_DIAL520,

	hd_init,
	hd_uninit,

	NULL,
	dial520_set_ringer,
	dial520_set_audioenabled,
	biz620_set_mute,
	NULL,
	NULL,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	dial520_get_events,
};


int LINK280_set_ringer(hid_hooks_t *ph, int enable)
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
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x18, 0x18);
	}

	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("LINK280 -> Device: led ringer");
		else
			ms_message("LINK280 -> Device: led ringer OFF");
		ph->jdevice->ringer=enable;
		return 0;
	}
	return -1;
}

int LINK280_set_audioenabled(hid_hooks_t *ph, int enable)
{
	PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->audioenabled==enable)
		return 0;

	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x18);
	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x17);
	if (enable>0)
	{
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x17, 0x17);
	}

	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("LINK280 -> Device: led audiolink & off hook/active call led");
		else
			ms_message("LINK280 -> Device: led audiolink & off hook/active call led OFF");
		ph->jdevice->audioenabled=enable;
		ph->jdevice->ringer=0;
		return 0;
	}
	return -1;
}

int LINK280_set_mute(hid_hooks_t *ph, int enable)
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
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x09, 0x09);
	}

	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("LINK280 -> Device: led mute");
		else
			ms_message("LINK280 -> Device: led mute OFF");
		ph->jdevice->mute=enable;
		return 0;
	}
	return -1;
}

int LINK280_get_events(hid_hooks_t *ph)
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
		if (Data->ButtonData.UsageMin==0x20 && Data->ButtonData.Usages[0]==0x20) {
			if (ph->jdevice->audioenabled==0)
			{
				hid_hooks_set_audioenabled(ph, 1);
				hid_event = HID_EVENTS_HOOK;
				ms_message("LINK280 HOOK");
				return hid_event;
			}
		}
		if (Data->ButtonData.UsageMin==0x20 && Data->ButtonData.Usages[0]==0x00) {
			if (ph->jdevice->audioenabled>0)
			{
				hid_hooks_set_audioenabled(ph, 0);
				hid_event = HID_EVENTS_HANGUP;
				ms_message("LINK280 HANGUP");
				return hid_event;
			}
		}

		if (Data->ButtonData.UsageMin==0x2F && Data->ButtonData.Usages[0]==0x2F) {
			if (ph->jdevice->mute==0)
			{
				hid_event = HID_EVENTS_MUTE;
				ms_message("LINK280 MUTE");
				hid_hooks_set_mute(ph, 1);
				return hid_event;
			}
			if (ph->jdevice->mute>0)
			{
				hid_event = HID_EVENTS_UNMUTE;
				ms_message("LINK280 UNMUTE");
				hid_hooks_set_mute(ph, 0);
				return hid_event;
			}
		}
	}

	return 0;
}

struct hid_device_desc gnetcom_LINK280 = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_LINK280,

	hd_init,
	hd_uninit,

	NULL,
	LINK280_set_ringer,
	LINK280_set_audioenabled,
	LINK280_set_mute,
	NULL,
	NULL,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	LINK280_get_events,
};

struct hid_device_desc gnetcom_LINK350OC = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_LINK350OC,

	hd_init,
	hd_uninit,

	NULL,
	LINK280_set_ringer,
	LINK280_set_audioenabled,
	LINK280_set_mute,
	NULL,
	NULL,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	LINK280_get_events,
};

struct hid_device_desc gnetcom_BIZ2400USB = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_BIZ2400USB,

	hd_init,
	hd_uninit,

	NULL,
	LINK280_set_ringer,
	LINK280_set_audioenabled,
	LINK280_set_mute,
	NULL,
	NULL,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	LINK280_get_events,
};


int GO6470_set_ringer(hid_hooks_t *ph, int enable)
{
	PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->ringer==enable)
		return 0;

	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x0B, 0x9E);
		//0x08, 0x18);
	if (enable>0)
	{
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x0B, 0x9E, 0x9E);
			//0x08, 0x18, 0x18);
	}

	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("GO6470 -> Device: led ringer");
		else
			ms_message("GO6470 -> Device: led ringer OFF");
		ph->jdevice->ringer=enable;
		return 0;
	}
	if (enable)
		ms_message("GO6470 (ERROR)-> Device: led ringer");
	else
		ms_message("GO6470 (ERROR)-> Device: led ringer OFF");
	ph->jdevice->ringer=enable;
	return 0;
	return -1;
}

int GO6470_set_audioenabled(hid_hooks_t *ph, int enable)
{
	PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
	ULONG ReportID=0x00;
	BOOLEAN ret = false;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->audioenabled==97)
	{
		ms_message("GO6470 on going HOOKSWITCH/pls, retry later!");
		return 0; /* skip! */
	}

	if (ph->jdevice->audioenabled==enable)
		return 0;

	if (enable>0)
	{
		//ringer
		ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x0B, 0x9E);
		//audio
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x2A, 0x2A);
		//hook
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x17, 0x17);
		if (ph->jdevice->ringer>0)
		{
			//ringer
			if (ph->desc->product_id==GNNETCOM_PRODUCT_ID_GO6470)
				ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
					ph->jdevice->hid_dev.OutputDataLength,
					0x0B, 0x9E, 0x9E);
			else
				ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
					ph->jdevice->hid_dev.OutputDataLength,
					0x0B, 0x9E);
		}

		ret = Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID);
		//ret = WriteOneOutputButton(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID, 0x08, 0x17);
	}
	else
	{
		//ringer
		ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x0B, 0x9E);
			//0x08, 0x18);
		ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x17);
		ret = Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID);
		//ret = WriteOneOutputButton(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID, 0x08, 0x17);
	}

	if (ret==TRUE)
	{
		if (enable)
			ms_message("GO6470 -> Device: led audiolink & off hook/active call led");
		else
			ms_message("GO6470 -> Device: led audiolink & off hook/active call led OFF");
		ph->jdevice->audioenabled=enable;
		ph->jdevice->ringer=0;
		return 0;
	}
	if (enable)
		ms_message("GO6470 (ERROR)-> Device: led audiolink & off hook/active call led");
	else
		ms_message("GO6470 (ERROR)-> Device: led audiolink & off hook/active call led OFF");
	ph->jdevice->audioenabled=enable;
	//ph->jdevice->ringer=0;
	return 0;
	return -1;
}

int GO6470_set_mute(hid_hooks_t *ph, int enable)
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
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x09, 0x09);
	}

	//if (WriteOneOutputButton(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID, 0x08, 0x09)==TRUE)
	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("GO6470 -> Device: led mute");
		else
			ms_message("GO6470 -> Device: led mute OFF");
		ph->jdevice->mute=enable;
		return 0;
	}
	if (enable)
		ms_message("GO6470 (ERROR)-> Device: led mute");
	else
		ms_message("GO6470 (ERROR)-> Device: led mute OFF");
	ph->jdevice->mute=enable;
	return 0;
}

#if 0
int GO6470_get_events(hid_hooks_t *ph)
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
	int hook_switch=1;
    for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
	{
		if (Data->ButtonData.UsageMin==0x20 && Data->ButtonData.Usages[0]==0x20) {
			if (ph->jdevice->audioenabled==0)
			{
				//NEXT AUTHORIZED ACTION AFTER -000B 0097 is 0x97- & ph->jdevice->audioenabled=97
				hid_hooks_set_audioenabled(ph, 97);
				hid_event = HID_EVENTS_HOOK;
				ms_message("GO6470 HOOK");
				return hid_event;
			}
		}
		if (Data->ButtonData.UsageMin==0x20 && Data->ButtonData.Usages[0]==0x00) {
			if (ph->jdevice->audioenabled>0)
			{
				hook_switch=0;
			}
		}

		if (Data->ButtonData.UsageMin==0x2F && Data->ButtonData.Usages[0]==0x2F) {
			if (ph->jdevice->mute==0)
			{
				hid_event = HID_EVENTS_MUTE;
				ms_message("GO6470 MUTE");
				hid_hooks_set_mute(ph, 1);
				return hid_event;
			}
			if (ph->jdevice->mute>0)
			{
				hid_event = HID_EVENTS_UNMUTE;
				ms_message("GO6470 UNMUTE");
				hid_hooks_set_mute(ph, 0);
				return hid_event;
			}
		}
	}

	Data = ph->jdevice->hid_dev.InputData;
	UsageId=0;
	UsagePage=0;
	ReportID=999;
    for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
	{
		if (Data->ButtonData.UsageMin==0x97 && Data->ButtonData.Usages[0]==0x97) {
			//NEXT AUTHORIZED ACTION AFTER -000B 0097 is 0x97- & ph->jdevice->audioenabled=97
			if (ph->jdevice->audioenabled==97)
			{
				ms_message("GO6470 HOOK CONFIRMED");
				ph->jdevice->audioenabled=1;
			}
			else if (hook_switch==0)
			{
				hid_hooks_set_audioenabled(ph, 0);
				hid_event = HID_EVENTS_HANGUP;
				ms_message("GO6470 HANGUP");
				return hid_event;
			}
		}
	}
	return 0;
}
#endif

int GO6470_get_events(hid_hooks_t *ph)
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
		ms_message("GO6470 CURRENT DATA REPORT[1]=0x%0.2X RID=0x%0.2X PAGE=0x%0.2X ID=0x%0.2X",
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
		int hook_switch=1;
		for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
		{
			if (Data->ButtonData.Usages[0]==0x20)
			{
				//HOOKSWITCH BUTTON
				hid_event = HID_EVENTS_HOOK;
				if (ph->jdevice->audioenabled>0)
				{
					break; /* still hooked: continue */
				}
				ms_message("GO6470 EVENT HID_EVENTS_HOOK");
				//ringer
				ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
					ph->jdevice->hid_dev.OutputDataLength,
					0x0B, 0x9E);
					//0x08, 0x18);
				hid_hooks_set_audioenabled(ph, 97); //to be confirmed
				ph->jdevice->ringer=0;
				return hid_event;
			}
			if (Data->ButtonData.UsageMin==0x20 && Data->ButtonData.Usages[0]==0x00) {
				if (ph->jdevice->audioenabled>0)
				{
					hook_switch=0;
				}
			}
		}

		Data = ph->jdevice->hid_dev.InputData;
		UsageId=0;
		UsagePage=0;
		ReportID=999;
		int skip = 0;
		for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
		{
			if (Data->ButtonData.UsageMin==0x2F && Data->ButtonData.Usages[0]==0x2F) {
				if (ph->jdevice->mute>=1)
				{
					hid_event = HID_EVENTS_UNMUTE;
					ms_message("GO6470 EVENT HID_EVENTS_UNMUTE");
					hid_hooks_set_mute(ph, 0);
					return hid_event;
				}
				else if (ph->jdevice->mute==0)
				{
					hid_event = HID_EVENTS_MUTE;
					ms_message("GO6470 EVENT HID_EVENTS_MUTE");
					hid_hooks_set_mute(ph, 1);
					return hid_event;
				}
			}
			if (Data->ButtonData.Usages[0]==0x21)
			{
				//Does this event exist on GO?
				ms_message("GO6470 EVENT HID_EVENTS_FLASHUP");
				return HID_EVENTS_FLASHUP;
			}
		}

		Data = ph->jdevice->hid_dev.InputData;
		UsageId=0;
		UsagePage=0;
		ReportID=999;
		for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
		{
			if (Data->ButtonData.UsageMin==0x97 && Data->ButtonData.Usages[0]==0x97) {
				//NEXT AUTHORIZED ACTION AFTER -000B 0097 is 0x97- & ph->jdevice->audioenabled=97
				if (ph->jdevice->audioenabled==97)
				{
					ms_message("GO6470 HOOK CONFIRMED");
					ph->jdevice->audioenabled=1;
				}
				else if (hook_switch==0)
				{
					ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
						ph->jdevice->hid_dev.OutputDataLength,
						0x08, 0x17);
					hid_hooks_set_audioenabled(ph, 0);
					hid_event = HID_EVENTS_HANGUP;
					ms_message("GO6470 HANGUP");
					return hid_event;
				}
			}
		}
		return 0;
	}
	return -1;
}

static int GO6470_hd_init(hid_hooks_t *ph)
{
	PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	ResetAllButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength);

	ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x2A, 0x2A);
	Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID);
	return 0;
}

static int GO6470_hd_uninit(hid_hooks_t *ph)
{
	return 0;
}

//CANNOT REJECT INCOMING CALLS
struct hid_device_desc gnetcom_GO6470 = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_GO6470,

	GO6470_hd_init,
	GO6470_hd_uninit,

	NULL,
	GO6470_set_ringer,
	GO6470_set_audioenabled,
	GO6470_set_mute,
	NULL,
	NULL,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	GO6470_get_events,
};

int GO9470_set_audioenabled(hid_hooks_t *ph, int enable)
{
	PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->audioenabled==0x2A)
	{
		ms_message("GO9470 on going HOOKSWITCH/pls, retry later!");
		return 0; /* skip! */
	}

	if (ph->jdevice->audioenabled==enable)
		return 0;

	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x18);
	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x17);
	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x2A);
	if (enable>0)
	{
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x2A, 0x2A);
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x17, 0x17);
	}

	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("GO9470 -> Device: led audiolink & off hook/active call led");
		else
			ms_message("GO9470 -> Device: led audiolink & off hook/active call led OFF");
		ph->jdevice->audioenabled=enable;
		ph->jdevice->ringer=0;
		return 0;
	}
	return -1;
}

int GO9470_get_events(hid_hooks_t *ph)
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
	int hook_switch=1;
    for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
	{
		if (Data->ButtonData.UsageMin==0x20 && Data->ButtonData.Usages[0]==0x20) {
			if (ph->jdevice->audioenabled==0)
			{
				//NEXT AUTHORIZED ACTION AFTER -000B 0097 is 0x97- & ph->jdevice->audioenabled=97
				hid_hooks_set_audioenabled(ph, 0x2A);
				hid_event = HID_EVENTS_HOOK;
				ms_message("GO9470 HOOK");
				return hid_event;
			}
		}
		if (Data->ButtonData.UsageMin==0x20 && Data->ButtonData.Usages[0]==0x00) {
			if (ph->jdevice->audioenabled>0)
			{
				hook_switch=0;
			}
		}

		if (Data->ButtonData.UsageMin==0x2F && Data->ButtonData.Usages[0]==0x2F) {
			if (ph->jdevice->mute==0)
			{
				hid_event = HID_EVENTS_MUTE;
				ms_message("GO9470 MUTE");
				hid_hooks_set_mute(ph, 1);
				return hid_event;
			}
			if (ph->jdevice->mute>0)
			{
				hid_event = HID_EVENTS_UNMUTE;
				ms_message("GO9470 UNMUTE");
				hid_hooks_set_mute(ph, 0);
				return hid_event;
			}
		}
	}

	Data = ph->jdevice->hid_dev.InputData;
	UsageId=0;
	UsagePage=0;
	ReportID=999;
    for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
	{
		if (Data->ButtonData.UsageMin==0x2A && Data->ButtonData.Usages[0]==0x2A) {
			//NEXT AUTHORIZED ACTION AFTER -000B 002A is 0x2A- & ph->jdevice->audioenabled=0x2A
			if (ph->jdevice->audioenabled==0x2A)
			{
				ms_message("GO9470 HOOK CONFIRMED");
				ph->jdevice->audioenabled=1;
			}
			else if (hook_switch==0)
			{
				//ph->jdevice->audioenabled=0;
				hid_hooks_set_audioenabled(ph, 0);
				hid_event = HID_EVENTS_HANGUP;
				ms_message("GO9470 HANGUP");
				return hid_event;
			}
		}
	}
	return 0;
}

//CANNOT REJECT INCOMING CALLS
struct hid_device_desc gnetcom_GO9470 = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_GO9470,

	GO6470_hd_init, /* hd_init, */
	GO6470_hd_uninit, /* hd_uninit, */

	NULL,
	GO6470_set_ringer,
	GO6470_set_audioenabled, /* GO9470_set_audioenabled, */
	GO6470_set_mute,
	NULL,
	NULL,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	GO6470_get_events, /* GO9470_get_events, */
};


int UC250_set_ringer(hid_hooks_t *ph, int enable)
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
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x18, 0x18);
	}

	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("UC250 -> Device: led ringer");
		else
			ms_message("UC250 -> Device: led ringer OFF");
		ph->jdevice->ringer=enable;
		return 0;
	}
	return -1;
}

int UC250_set_audioenabled(hid_hooks_t *ph, int enable)
{
	PHID_DATA pData = ph->jdevice->hid_dev.OutputData;
	ULONG ReportID=0x00;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	if (ph->jdevice->audioenabled==enable)
		return 0;

	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x18);
	ReportID = ResetButtonUsage(ph->jdevice->hid_dev.OutputData,
		ph->jdevice->hid_dev.OutputDataLength,
		0x08, 0x17);
	if (enable>0)
	{
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x17, 0x17);
	}

	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("UC250 -> Device: led audiolink & off hook/active call led");
		else
			ms_message("UC250 -> Device: led audiolink & off hook/active call led OFF");
		ph->jdevice->audioenabled=enable;
		ph->jdevice->ringer=0;
		return 0;
	}
	return -1;
}

int UC250_set_mute(hid_hooks_t *ph, int enable)
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
		ReportID = SetButtonUsage(ph->jdevice->hid_dev.OutputData,
			ph->jdevice->hid_dev.OutputDataLength,
			0x08, 0x09, 0x09);
	}

	if (Write(&ph->jdevice->hid_dev, ph->jdevice->hEvent,ReportID)==TRUE)
	{
		if (enable)
			ms_message("UC250 -> Device: led mute");
		else
			ms_message("UC250 -> Device: led mute OFF");
		ph->jdevice->mute=enable;
		return 0;
	}
	return -1;
}

int UC250_get_events(hid_hooks_t *ph)
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
		if (Data->ButtonData.Usages!=NULL) {
			ms_message("CURRENT DATA REPORT[1]=0x%0.2X RID=0x%0.2X PAGE=0x%0.2X ID=0x%0.2X",
				ph->jdevice->hid_dev.InputReportBuffer[1],
				Data->ReportID,
				Data->UsagePage,
				Data->ButtonData.Usages[0]);
		}
	}

	//CHECK OFF HOOK
	Data = ph->jdevice->hid_dev.InputData;
	USAGE UsageId=0;
	USAGE UsagePage=0;
	ULONG ReportID=999;
    for (unsigned int i = 0; i < ph->jdevice->hid_dev.InputDataLength; i++, Data++) 
	{
		if (Data->IsButtonData==0)
			continue;
		if (Data->ButtonData.UsageMin==0x20 && Data->ButtonData.Usages[0]==0x20) {
			if (ph->jdevice->audioenabled==0)
			{
				hid_hooks_set_audioenabled(ph, 1);
				hid_event = HID_EVENTS_HOOK;
				ms_message("UC250 HOOK");
				return hid_event;
			}
		}
		if (Data->ButtonData.UsageMin==0x20 && Data->ButtonData.Usages[0]==0x00) {
			if (ph->jdevice->audioenabled>0)
			{
				hid_hooks_set_audioenabled(ph, 0);
				hid_event = HID_EVENTS_HANGUP;
				ms_message("UC250 HANGUP");
				return hid_event;
			}
		}

		if (Data->ButtonData.UsageMin==0x2F && Data->ButtonData.Usages[0]==0x2F) {
			if (ph->jdevice->mute==0)
			{
				hid_event = HID_EVENTS_MUTE;
				ms_message("UC250 MUTE");
				hid_hooks_set_mute(ph, 1);
				return hid_event;
			}
			if (ph->jdevice->mute>0)
			{
				hid_event = HID_EVENTS_UNMUTE;
				ms_message("UC250 UNMUTE");
				hid_hooks_set_mute(ph, 0);
				return hid_event;
			}
		}
	}

	return 0;
}

struct hid_device_desc gnetcom_UC250 = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_UC250,

	hd_init,
	hd_uninit,

	NULL,
	UC250_set_ringer,
	UC250_set_audioenabled,
	UC250_set_mute,
	NULL,
	NULL,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	UC250_get_events,
};

struct hid_device_desc gnetcom_UC550DUO = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_UC550DUO,

	hd_init,
	hd_uninit,

	NULL,
	UC250_set_ringer,
	UC250_set_audioenabled,
	UC250_set_mute,
	NULL,
	NULL,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	UC250_get_events,
};

struct hid_device_desc gnetcom_UC550MONO = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_UC550MONO,

	hd_init,
	hd_uninit,

	NULL,
	UC250_set_ringer,
	UC250_set_audioenabled,
	UC250_set_mute,
	NULL,
	NULL,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	UC250_get_events,
};

 /* windows: 
 MUTE BUTTON DOES NOT WORK
 button is becoming RED, but audio is still sent...
 */
struct hid_device_desc gnetcom_UC150MONO = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_UC150MONO,

	hd_init,
	hd_uninit,

	NULL,
	UC250_set_ringer,
	UC250_set_audioenabled,
	UC250_set_mute,
	NULL,
	NULL,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	UC250_get_events,
};

struct hid_device_desc gnetcom_UC150DUO = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_UC150DUO,

	hd_init,
	hd_uninit,

	NULL,
	UC250_set_ringer,
	UC250_set_audioenabled,
	UC250_set_mute,
	NULL,
	NULL,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	UC250_get_events,
};

struct hid_device_desc gnetcom_BIZ2400MONOUSB = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_BIZ2400MONOUSB,

	hd_init,
	hd_uninit,

	NULL,
	UC250_set_ringer,
	UC250_set_audioenabled,
	UC250_set_mute,
	NULL,
	NULL,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	UC250_get_events,
};

struct hid_device_desc gnetcom_PRO930 = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_PRO930,

	hd_init,
	hd_uninit,

	NULL,
	UC250_set_ringer,
	UC250_set_audioenabled,
	UC250_set_mute,
	NULL,
	NULL,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	UC250_get_events,
};


struct hid_device_desc gnetcom_PRO9450 = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_PRO9450,

	hd_init,
	hd_uninit,

	NULL,
	UC250_set_ringer,
	UC250_set_audioenabled,
	UC250_set_mute,
	NULL,
	NULL,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	UC250_get_events,
};

#endif
