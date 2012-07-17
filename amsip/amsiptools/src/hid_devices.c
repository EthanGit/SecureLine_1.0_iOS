#include <amsiptools/hid_hooks.h>

#if defined(WIN32) && defined(ENABLE_HID)

#include "hid_devices.h"

PHidD_GetProductString fHidD_GetProductString = NULL;
PHidD_GetHidGuid fHidD_GetHidGuid = NULL;
PHidD_GetAttributes fHidD_GetAttributes = NULL;
PHidD_SetFeature fHidD_SetFeature = NULL;
PHidD_GetFeature fHidD_GetFeature = NULL;
PHidD_GetInputReport fHidD_GetInputReport = NULL;
PHidD_GetPreparsedData fHidD_GetPreparsedData = NULL;
PHidD_FreePreparsedData fHidD_FreePreparsedData = NULL;
PHidP_GetCaps fHidP_GetCaps = NULL;
PHidP_GetButtonCaps fHidP_GetButtonCaps = NULL;
PHidP_GetValueCaps fHidP_GetValueCaps = NULL;

PHidP_GetUsageValue fHidP_GetUsageValue = NULL;
PHidP_GetScaledUsageValue fHidP_GetScaledUsageValue = NULL;
PHidP_GetUsages fHidP_GetUsages = NULL;
PHidP_SetUsages fHidP_SetUsages = NULL;
PHidP_SetUsageValue fHidP_SetUsageValue = NULL;

PHidP_MaxUsageListLength fHidP_MaxUsageListLength = NULL;

BOOLEAN
Read (
   PHID_DEVICE    HidDevice
   )
/*++
RoutineDescription:
   Given a struct _HID_DEVICE, obtain a read report and unpack the values
   into the InputData array.
--*/
{
    DWORD    bytesRead;

    if (!ReadFile (HidDevice->HidDevice,
                  HidDevice->InputReportBuffer,
                  HidDevice->Caps.InputReportByteLength,
                  &bytesRead,
                  NULL)) 
    {
        return FALSE;
    }

    ASSERT (bytesRead == HidDevice->Caps.InputReportByteLength);
    if (bytesRead != HidDevice->Caps.InputReportByteLength)
    {
        return FALSE;
    }

    return UnpackReport (HidDevice->InputReportBuffer,
                         HidDevice->Caps.InputReportByteLength,
                         HidP_Input,
                         HidDevice->InputData,
                         HidDevice->InputDataLength,
                         HidDevice->Ppd);
}

BOOLEAN
ReadOverlapped (
    PHID_DEVICE     HidDevice,
    HANDLE          CompletionEvent
   )
{
    static OVERLAPPED  overlap;
    DWORD       bytesRead;
    BOOL        readStatus;

    memset(&overlap, 0, sizeof(OVERLAPPED));
    
    overlap.hEvent = CompletionEvent;
    
    readStatus = ReadFile ( HidDevice -> HidDevice,
                            HidDevice -> InputReportBuffer,
                            HidDevice -> Caps.InputReportByteLength,
                            &bytesRead,
                            &overlap);
                          
    if (!readStatus) 
    {
		DWORD status = GetLastError();
		ms_message("read error: %i", status);
        return (ERROR_IO_PENDING == status);
    }
    else 
    {
        SetEvent(CompletionEvent);
        return (TRUE);
    }
}

BOOLEAN
Write (
   PHID_DEVICE    HidDevice,
   HANDLE         CompletionEvent,
   ULONG ReportID
)
{
    DWORD     bytesWritten;
    PHID_DATA pData;
    ULONG     Index;
    BOOLEAN   Status;
    BOOLEAN   WriteStatus;

    pData = HidDevice -> OutputData;

    for (Index = 0; Index < HidDevice -> OutputDataLength; Index++, pData++) 
    {
        pData -> IsDataSet = FALSE;
    }

    Status = TRUE;

    pData = HidDevice -> OutputData;
    for (Index = 0; Index < HidDevice -> OutputDataLength; Index++, pData++) 
    {

        if (!pData -> IsDataSet) 
        {
            PackReport (HidDevice->OutputReportBuffer,
                     HidDevice->Caps.OutputReportByteLength,
                     HidP_Output,
                     pData,
                     HidDevice->OutputDataLength - Index,
                     HidDevice->Ppd);

			static OVERLAPPED  overlap;
			memset(&overlap, 0, sizeof(OVERLAPPED));
		    overlap.hEvent = CompletionEvent;
			overlap.Offset = 0;
			overlap.OffsetHigh = 0;
			ResetEvent(CompletionEvent);
			WriteStatus = WriteFile (HidDevice->HidDevice,
								  HidDevice->OutputReportBuffer,
								  HidDevice->Caps.OutputReportByteLength,
								  &bytesWritten,
								  &overlap);
			if (!WriteStatus)
			{
				DWORD ret = GetLastError();
				if(ret == ERROR_IO_PENDING)
				{
					ret = WaitForSingleObject(CompletionEvent, 1000);
					if(ret == WAIT_TIMEOUT)
					{
						CancelIo(HidDevice->HidDevice);
						ms_message("WAIT_TIMEOUT: set hid failed");
					}
					else if(ret == WAIT_FAILED)
					{
						ms_message("WAIT_TIMEOUT: set hid failed");
					}
					else
					{
						GetOverlappedResult(HidDevice->HidDevice, &overlap, 
								&bytesWritten, FALSE);
						if (bytesWritten == HidDevice -> Caps.OutputReportByteLength)
							WriteStatus = 1;
						else
							ms_message("GetOverlappedResult: %i?=%i", bytesWritten, HidDevice -> Caps.OutputReportByteLength);
					}
				}
				else
					ms_message("set hid failed (%i)", ret);

			}

			Status = Status && WriteStatus;
        }
    }
    return (Status);
}

BOOLEAN
WriteOneOutputButton (
   PHID_DEVICE    HidDevice,
   HANDLE         CompletionEvent,
   ULONG ReportID,
   ULONG usage_page, ULONG usage_id
)
{
    DWORD     bytesWritten;
    PHID_DATA pData;
    ULONG     Index;
    BOOLEAN   Status;
    BOOLEAN   WriteStatus;

    pData = HidDevice -> OutputData;

    for (Index = 0; Index < HidDevice -> OutputDataLength; Index++, pData++) 
    {
        pData -> IsDataSet = TRUE;
		if (pData->IsButtonData
			&& pData->UsagePage == usage_page
			&& pData->ButtonData.UsageMin==usage_id)
		{
	        pData -> IsDataSet = FALSE;
		}
		else if (!pData->IsButtonData
			&& pData->UsagePage == usage_page
			&& pData->ValueData.Usage==usage_id)

		{
	        pData -> IsDataSet = FALSE;
		}
    }

    Status = TRUE;

    pData = HidDevice -> OutputData;
    for (Index = 0; Index < HidDevice -> OutputDataLength; Index++, pData++) 
    {

        if (!pData -> IsDataSet) 
        {
            PackReport (HidDevice->OutputReportBuffer,
                     HidDevice->Caps.OutputReportByteLength,
                     HidP_Output,
                     pData,
                     HidDevice->OutputDataLength - Index,
                     HidDevice->Ppd);

			static OVERLAPPED  overlap;
			memset(&overlap, 0, sizeof(OVERLAPPED));
		    overlap.hEvent = CompletionEvent;
			overlap.Offset = 0;
			overlap.OffsetHigh = 0;
			ResetEvent(CompletionEvent);
			WriteStatus = WriteFile (HidDevice->HidDevice,
								  HidDevice->OutputReportBuffer,
								  HidDevice->Caps.OutputReportByteLength,
								  &bytesWritten,
								  &overlap);
			if (!WriteStatus)
			{
				DWORD ret = GetLastError();
				if(ret == ERROR_IO_PENDING)
				{
					ret = WaitForSingleObject(CompletionEvent, 1000);
					if(ret == WAIT_TIMEOUT)
					{
						CancelIo(HidDevice->HidDevice);
						ms_message("WAIT_TIMEOUT: set hid failed");
					}
					else if(ret == WAIT_FAILED)
					{
						ms_message("WAIT_TIMEOUT: set hid failed");
					}
					else
					{
						GetOverlappedResult(HidDevice->HidDevice, &overlap, 
								&bytesWritten, FALSE);
						if (bytesWritten == HidDevice -> Caps.OutputReportByteLength)
							WriteStatus = 1;
						else
							ms_message("GetOverlappedResult: %i?=%i", bytesWritten, HidDevice -> Caps.OutputReportByteLength);
					}
				}
				else
					ms_message("set hid failed (%i)", ret);

			}

			Status = Status && WriteStatus;
        }
    }
    return (Status);
}

BOOLEAN
SetFeature (
    PHID_DEVICE    HidDevice,
	ULONG ReportID
)
{
    PHID_DATA pData;
    ULONG     Index;
    BOOLEAN   Status;
    BOOLEAN   FeatureStatus;

    pData = HidDevice -> FeatureData;

    for (Index = 0; Index < HidDevice -> FeatureDataLength; Index++, pData++) 
    {
        pData -> IsDataSet = FALSE;
    }

    Status = TRUE;

    pData = HidDevice -> FeatureData;
    for (Index = 0; Index < HidDevice -> FeatureDataLength; Index++, pData++) 
    {
		if (pData->ReportID == ReportID)
		{
			if (!pData -> IsDataSet) 
			{
				PackReport (HidDevice->FeatureReportBuffer,
						 HidDevice->Caps.FeatureReportByteLength,
						 HidP_Feature,
						 pData,
						 HidDevice->FeatureDataLength - Index,
						 HidDevice->Ppd);

				FeatureStatus =(fHidD_SetFeature (HidDevice->HidDevice,
											  HidDevice->FeatureReportBuffer,
											  HidDevice->Caps.FeatureReportByteLength));

				Status = Status && FeatureStatus;
			}
		}
    }
    return (Status);
}

BOOLEAN
GetFeature (
   PHID_DEVICE    HidDevice
)
{
    ULONG     Index;
    PHID_DATA pData;
    BOOLEAN   FeatureStatus;
    BOOLEAN   Status;

    pData = HidDevice -> FeatureData;

    for (Index = 0; Index < HidDevice -> FeatureDataLength; Index++, pData++) 
    {
        pData -> IsDataSet = FALSE;
    }

    Status = TRUE; 
    pData = HidDevice -> FeatureData;

    for (Index = 0; Index < HidDevice -> FeatureDataLength; Index++, pData++) 
    {
        if (!pData -> IsDataSet) 
        {
            memset(HidDevice -> FeatureReportBuffer, 0x00, HidDevice->Caps.FeatureReportByteLength);

            HidDevice -> FeatureReportBuffer[0] = (UCHAR) pData -> ReportID;

            FeatureStatus = fHidD_GetFeature (HidDevice->HidDevice,
                                              HidDevice->FeatureReportBuffer,
                                              HidDevice->Caps.FeatureReportByteLength);


            if (FeatureStatus) 
            {
                FeatureStatus = UnpackReport ( HidDevice->FeatureReportBuffer,
                                           HidDevice->Caps.FeatureReportByteLength,
                                           HidP_Feature,
                                           HidDevice->FeatureData,
                                           HidDevice->FeatureDataLength,
                                           HidDevice->Ppd);
            }

            Status = Status && FeatureStatus;
        }
   }

   return (Status);
}


BOOLEAN
UnpackReport (
   __in_bcount(ReportBufferLength)PCHAR ReportBuffer,
   IN       USHORT               ReportBufferLength,
   IN       HIDP_REPORT_TYPE     ReportType,
   IN OUT   PHID_DATA            Data,
   IN       ULONG                DataLength,
   IN       PHIDP_PREPARSED_DATA Ppd
)
{
    ULONG       numUsages; // Number of usages returned from GetUsages.
    ULONG       i;
    UCHAR       reportID;
    ULONG       Index;
    ULONG       nextUsage;

    reportID = ReportBuffer[0];

    for (i = 0; i < DataLength; i++, Data++) 
    {
        if (reportID == Data->ReportID) 
        {
            if (Data->IsButtonData) 
            {
                numUsages = Data->ButtonData.MaxUsageLength;

                Data->Status = fHidP_GetUsages (ReportType,
                                               Data->UsagePage,
                                               0, // All collections
                                               Data->ButtonData.Usages,
                                               &numUsages,
                                               Ppd,
                                               ReportBuffer,
                                               ReportBufferLength);


                for (Index = 0, nextUsage = 0; Index < numUsages; Index++) 
                {
                    if (Data -> ButtonData.UsageMin <= Data -> ButtonData.Usages[Index] &&
                            Data -> ButtonData.Usages[Index] <= Data -> ButtonData.UsageMax) 
                    {
                        Data -> ButtonData.Usages[nextUsage++] = Data -> ButtonData.Usages[Index];
                        
                    }
                }

                if (nextUsage < Data -> ButtonData.MaxUsageLength) 
                {
                    Data->ButtonData.Usages[nextUsage] = 0;
                }
            }
            else 
            {
                Data->Status = fHidP_GetUsageValue (
                                                ReportType,
                                                Data->UsagePage,
                                                0,               // All Collections.
                                                Data->ValueData.Usage,
                                                &Data->ValueData.Value,
                                                Ppd,
                                                ReportBuffer,
                                                ReportBufferLength);

                if (HIDP_STATUS_SUCCESS != Data->Status)
                {
                    return (FALSE);
                }

                Data->Status = fHidP_GetScaledUsageValue (
                                                       ReportType,
                                                       Data->UsagePage,
                                                       0, // All Collections.
                                                       Data->ValueData.Usage,
                                                       &Data->ValueData.ScaledValue,
                                                       Ppd,
                                                       ReportBuffer,
                                                       ReportBufferLength);
            } 
            Data -> IsDataSet = TRUE;
        }
    }
    return (TRUE);
}


BOOLEAN
PackReport (
   __out_bcount(ReportBufferLength)PCHAR ReportBuffer,
   IN  USHORT               ReportBufferLength,
   IN  HIDP_REPORT_TYPE     ReportType,
   IN  PHID_DATA            Data,
   IN  ULONG                DataLength,
   IN  PHIDP_PREPARSED_DATA Ppd
   )
{
    ULONG       numUsages; // Number of usages to set for a given report.
    ULONG       i;
    ULONG       CurrReportID;
	PHID_DATA   Data2;

    memset (ReportBuffer, (UCHAR) 0, ReportBufferLength);
    ReportBuffer[0]=(CHAR)Data->ReportID;

	Data2 = Data;
    CurrReportID = Data -> ReportID;

    for (i = 0; i < DataLength; i++, Data++) 
    {
        if (Data -> ReportID == CurrReportID) 
        {
            if (Data->IsButtonData) 
            {
                numUsages = Data->ButtonData.MaxUsageLength;
                Data->Status = fHidP_SetUsages (ReportType,
                                               Data->UsagePage,
                                               0, // All collections
                                               Data->ButtonData.Usages,
                                               &numUsages,
                                               Ppd,
                                               ReportBuffer,
                                               ReportBufferLength);
            }
            else
            {
                Data->Status = fHidP_SetUsageValue (ReportType,
                                                   Data->UsagePage,
                                                   0, // All Collections.
                                                   Data->ValueData.Usage,
                                                   Data->ValueData.Value,
                                                   Ppd,
                                                   ReportBuffer,
                                                   ReportBufferLength);
            }

            if (HIDP_STATUS_SUCCESS != Data->Status)
            {
                return FALSE;
            }
        }
    }   

    for (i = 0; i < DataLength; i++, Data2++) 
    {
        if (CurrReportID == Data2 -> ReportID)
        {
            Data2 -> IsDataSet = TRUE;
        }
    }
    return (TRUE);
}

BOOLEAN
OpenHidDevice (
    __in     LPSTR          DevicePath,
    __in     BOOL           HasReadAccess,
    __in     BOOL           HasWriteAccess,
    __in     BOOL           IsOverlapped,
    __in     BOOL           IsExclusive,
    __inout  PHID_DEVICE    HidDevice
)
/*++
RoutineDescription:
    Given the HardwareDeviceInfo, representing a handle to the plug and
    play information, and deviceInfoData, representing a specific hid device,
    open that device and fill in all the relivant information in the given
    HID_DEVICE structure.

    return if the open and initialization was successfull or not.

--*/
{
    DWORD   accessFlags = 0;
    DWORD   sharingFlags = 0;
    BOOLEAN bSuccess;
    INT     iDevicePathSize;

    iDevicePathSize = strlen(DevicePath) + 1;
    
    if (HasReadAccess)
    {
        accessFlags |= GENERIC_READ;
    }

    if (HasWriteAccess)
    {
        accessFlags |= GENERIC_WRITE;
    }

    if (!IsExclusive)
    {
        sharingFlags = FILE_SHARE_READ | FILE_SHARE_WRITE;
    }
	
	HidDevice->HidDevice = CreateFile (DevicePath,
                                       accessFlags,
                                       sharingFlags,
                                       NULL,        // no SECURITY_ATTRIBUTES structure
                                       OPEN_EXISTING, // No special create flags
                                       0,   // Open device as non-overlapped so we can get data
                                       NULL);       // No template file

    if (INVALID_HANDLE_VALUE == HidDevice->HidDevice) 
    {
        return FALSE;
    }

    HidDevice -> OpenedForRead = HasReadAccess;
    HidDevice -> OpenedForWrite = HasWriteAccess;
    HidDevice -> OpenedOverlapped = IsOverlapped;
    HidDevice -> OpenedExclusive = IsExclusive;
    
	if (!fHidD_GetPreparsedData (HidDevice->HidDevice, &HidDevice->Ppd)) 
	{
		CloseHandle(HidDevice -> HidDevice);
		HidDevice -> HidDevice = INVALID_HANDLE_VALUE ;
		return FALSE;
	}

	if (!fHidD_GetAttributes (HidDevice->HidDevice, &HidDevice->Attributes)) 
	{
		CloseHandle(HidDevice -> HidDevice);
		HidDevice -> HidDevice = INVALID_HANDLE_VALUE;
		fHidD_FreePreparsedData (HidDevice->Ppd);
		HidDevice->Ppd = NULL;

		return FALSE;
	}

	if (!fHidP_GetCaps (HidDevice->Ppd, &HidDevice->Caps))
	{
		CloseHandle(HidDevice -> HidDevice);
		HidDevice -> HidDevice = INVALID_HANDLE_VALUE;
		fHidD_FreePreparsedData (HidDevice->Ppd);
		HidDevice->Ppd = NULL;

		return FALSE;
	}

	bSuccess = FillDeviceInfo(HidDevice);

	if (FALSE == bSuccess)
	{
		CloseHidDevice(HidDevice);
		return (FALSE);
	}
    
	if (IsOverlapped)
	{
		CloseHandle(HidDevice->HidDevice);

	    HidDevice->HidDevice = CreateFile (DevicePath,
                                       accessFlags,
                                       sharingFlags,
                                       NULL,        // no SECURITY_ATTRIBUTES structure
                                       OPEN_EXISTING, // No special create flags
                                       FILE_FLAG_OVERLAPPED, // Now we open the device as overlapped
                                       NULL);       // No template file
	
	    if (INVALID_HANDLE_VALUE == HidDevice->HidDevice) 
		{
			CloseHidDevice(HidDevice);
			return FALSE;
		}
	}

    return (TRUE);
}

BOOLEAN
FillDeviceInfo(
    IN  PHID_DEVICE HidDevice
)
{
    USHORT              numValues;
    USHORT              numCaps;
    PHIDP_BUTTON_CAPS   buttonCaps;
    PHIDP_VALUE_CAPS    valueCaps;
    PHID_DATA           data;
    ULONG               i;
    USAGE               usage;

	if (HidDevice->Caps.InputReportByteLength==0)
		return (FALSE);

    HidDevice->InputReportBuffer = (PCHAR) 
        calloc (HidDevice->Caps.InputReportByteLength, sizeof (CHAR));

    if (NULL == HidDevice->InputReportBuffer)
    {
        return (FALSE);
    }

	if (HidDevice->Caps.NumberInputButtonCaps>0) {
		HidDevice->InputButtonCaps = buttonCaps = (PHIDP_BUTTON_CAPS)
			calloc (HidDevice->Caps.NumberInputButtonCaps, sizeof (HIDP_BUTTON_CAPS));

		if (NULL == buttonCaps)
		{
			return (FALSE);
		}

		numCaps = HidDevice->Caps.NumberInputButtonCaps;

		fHidP_GetButtonCaps (HidP_Input,
							buttonCaps,
							&numCaps,
							HidDevice->Ppd);
	}

	numValues = 0;
	if (HidDevice->Caps.NumberInputValueCaps>0) {
		HidDevice->InputValueCaps = valueCaps = (PHIDP_VALUE_CAPS)
			calloc (HidDevice->Caps.NumberInputValueCaps, sizeof (HIDP_VALUE_CAPS));

		if (NULL == valueCaps)
		{
			return(FALSE);
		}

		numCaps = HidDevice->Caps.NumberInputValueCaps;

		fHidP_GetValueCaps (HidP_Input,
						   valueCaps,
						   &numCaps,
						   HidDevice->Ppd);


		for (i = 0; i < HidDevice->Caps.NumberInputValueCaps; i++, valueCaps++) 
		{
			if (valueCaps->IsRange) 
			{
				numValues += valueCaps->Range.UsageMax - valueCaps->Range.UsageMin + 1;
			}
			else
			{
				numValues++;
			}
		}
	}

	HidDevice->InputDataLength = HidDevice->Caps.NumberInputButtonCaps + numValues;
	if (HidDevice->InputDataLength>0) {

		HidDevice->InputData = data = (PHID_DATA)
			calloc (HidDevice->InputDataLength, sizeof (HID_DATA));

		if (NULL == data)
		{
			return (FALSE);
		}

		for (i = 0;
			 i < HidDevice->Caps.NumberInputButtonCaps;
			 i++, data++, buttonCaps++) 
		{
			data->IsButtonData = TRUE;
			data->Status = HIDP_STATUS_SUCCESS;
			data->UsagePage = buttonCaps->UsagePage;
			if (buttonCaps->IsRange) 
			{
				data->ButtonData.UsageMin = buttonCaps -> Range.UsageMin;
				data->ButtonData.UsageMax = buttonCaps -> Range.UsageMax;
			}
			else
			{
				data -> ButtonData.UsageMin = data -> ButtonData.UsageMax = buttonCaps -> NotRange.Usage;
			}
        
			data->ButtonData.MaxUsageLength = fHidP_MaxUsageListLength (
													HidP_Input,
													buttonCaps->UsagePage,
													HidDevice->Ppd);
			data->ButtonData.Usages = (PUSAGE)
				calloc (data->ButtonData.MaxUsageLength, sizeof (USAGE));

			data->ReportID = buttonCaps -> ReportID;
		}

		valueCaps = HidDevice->InputValueCaps;

		for (i = 0; i < HidDevice->Caps.NumberInputValueCaps ; i++, valueCaps++)
		{
			if (valueCaps->IsRange) 
			{
				for (usage = valueCaps->Range.UsageMin;
					 usage <= valueCaps->Range.UsageMax;
					 usage++) 
				{
					data->IsButtonData = FALSE;
					data->Status = HIDP_STATUS_SUCCESS;
					data->UsagePage = valueCaps->UsagePage;
					data->ValueData.Usage = usage;
					data->ReportID = valueCaps -> ReportID;
					data++;
				}
			} 
			else
			{
				data->IsButtonData = FALSE;
				data->Status = HIDP_STATUS_SUCCESS;
				data->UsagePage = valueCaps->UsagePage;
				data->ValueData.Usage = valueCaps->NotRange.Usage;
				data->ReportID = valueCaps -> ReportID;
				data++;
			}
		}
	}

	if (HidDevice->Caps.OutputReportByteLength>0) {
		HidDevice->OutputReportBuffer = (PCHAR)
			calloc (HidDevice->Caps.OutputReportByteLength, sizeof (CHAR));
	}

	if (HidDevice->Caps.NumberOutputButtonCaps>0) {
		HidDevice->OutputButtonCaps = buttonCaps = (PHIDP_BUTTON_CAPS)
			calloc (HidDevice->Caps.NumberOutputButtonCaps, sizeof (HIDP_BUTTON_CAPS));

		if (NULL == buttonCaps)
		{
			return (FALSE);
		}    

		numCaps = HidDevice->Caps.NumberOutputButtonCaps;
		fHidP_GetButtonCaps (HidP_Output,
							buttonCaps,
							&numCaps,
							HidDevice->Ppd);
	}

	if (HidDevice->Caps.NumberOutputValueCaps>0) {
		HidDevice->OutputValueCaps = valueCaps = (PHIDP_VALUE_CAPS)
			calloc (HidDevice->Caps.NumberOutputValueCaps, sizeof (HIDP_VALUE_CAPS));

		if (NULL == valueCaps)
		{
			return (FALSE);
		}


		numCaps = HidDevice->Caps.NumberOutputValueCaps;
		fHidP_GetValueCaps (HidP_Output,
						   valueCaps,
						   &numCaps,
						   HidDevice->Ppd);

		numValues = 0;
		for (i = 0; i < HidDevice->Caps.NumberOutputValueCaps; i++, valueCaps++) 
		{
			if (valueCaps->IsRange) 
			{
				numValues += valueCaps->Range.UsageMax
						   - valueCaps->Range.UsageMin + 1;
			} 
			else
			{
				numValues++;
			}
		}
	}


    HidDevice->OutputDataLength = HidDevice->Caps.NumberOutputButtonCaps + numValues;
	if (HidDevice->OutputDataLength>0) {
		HidDevice->OutputData = data = (PHID_DATA)
		   calloc (HidDevice->OutputDataLength, sizeof (HID_DATA));

		if (NULL == data)
		{
			return (FALSE);
		}

		for (i = 0;
			 i < HidDevice->Caps.NumberOutputButtonCaps;
			 i++, data++, buttonCaps++) 
		{
			data->IsButtonData = TRUE;
			data->Status = HIDP_STATUS_SUCCESS;
			data->UsagePage = buttonCaps->UsagePage;

			if (buttonCaps->IsRange)
			{
				data->ButtonData.UsageMin = buttonCaps -> Range.UsageMin;
				data->ButtonData.UsageMax = buttonCaps -> Range.UsageMax;
			}
			else
			{
				data -> ButtonData.UsageMin = data -> ButtonData.UsageMax = buttonCaps -> NotRange.Usage;
			}

			data->ButtonData.MaxUsageLength = fHidP_MaxUsageListLength (
													   HidP_Output,
													   buttonCaps->UsagePage,
													   HidDevice->Ppd);

			data->ButtonData.Usages = (PUSAGE)
				calloc (data->ButtonData.MaxUsageLength, sizeof (USAGE));

			data->ReportID = buttonCaps -> ReportID;
		}

		valueCaps = HidDevice->OutputValueCaps;

		for (i = 0; i < HidDevice->Caps.NumberOutputValueCaps ; i++, valueCaps++)
		{
			if (valueCaps->IsRange)
			{
				for (usage = valueCaps->Range.UsageMin;
					 usage <= valueCaps->Range.UsageMax;
					 usage++) 
				{
					data->IsButtonData = FALSE;
					data->Status = HIDP_STATUS_SUCCESS;
					data->UsagePage = valueCaps->UsagePage;
					data->ValueData.Usage = usage;
					data->ReportID = valueCaps -> ReportID;
					data++;
				}
			}
			else
			{
				data->IsButtonData = FALSE;
				data->Status = HIDP_STATUS_SUCCESS;
				data->UsagePage = valueCaps->UsagePage;
				data->ValueData.Usage = valueCaps->NotRange.Usage;
				data->ReportID = valueCaps -> ReportID;
				data++;
			}
		}

	}

	if (HidDevice->Caps.FeatureReportByteLength>0) {
		HidDevice->FeatureReportBuffer = (PCHAR)
			   calloc (HidDevice->Caps.FeatureReportByteLength, sizeof (CHAR));
	}

	if (HidDevice->Caps.NumberFeatureButtonCaps>0) {
		HidDevice->FeatureButtonCaps = buttonCaps = (PHIDP_BUTTON_CAPS)
			calloc (HidDevice->Caps.NumberFeatureButtonCaps, sizeof (HIDP_BUTTON_CAPS));

		if (NULL == buttonCaps)
		{
			return (FALSE);
		}

		numCaps = HidDevice->Caps.NumberFeatureButtonCaps;
		fHidP_GetButtonCaps (HidP_Feature,
							buttonCaps,
							&numCaps,
							HidDevice->Ppd);
	}

	if (HidDevice->Caps.NumberFeatureValueCaps>0) {
		HidDevice->FeatureValueCaps = valueCaps = (PHIDP_VALUE_CAPS)
			calloc (HidDevice->Caps.NumberFeatureValueCaps, sizeof (HIDP_VALUE_CAPS));

		if (NULL == valueCaps)
		{
			return (FALSE);
		}


		numCaps = HidDevice->Caps.NumberFeatureValueCaps;
		fHidP_GetValueCaps (HidP_Feature,
						   valueCaps,
						   &numCaps,
						   HidDevice->Ppd);

		numValues = 0;
		for (i = 0; i < HidDevice->Caps.NumberFeatureValueCaps; i++, valueCaps++) 
		{
			if (valueCaps->IsRange) 
			{
				numValues += valueCaps->Range.UsageMax
						   - valueCaps->Range.UsageMin + 1;
			}
			else
			{
				numValues++;
			}
		}
	}

    HidDevice->FeatureDataLength = HidDevice->Caps.NumberFeatureButtonCaps + numValues;
	if (HidDevice->FeatureDataLength>0) {
		HidDevice->FeatureData = data = (PHID_DATA)
			calloc (HidDevice->FeatureDataLength, sizeof (HID_DATA));

		if (NULL == data)
		{
			return (FALSE);
		}

		for (i = 0;
			 i < HidDevice->Caps.NumberFeatureButtonCaps;
			 i++, data++, buttonCaps++) 
		{
			data->IsButtonData = TRUE;
			data->Status = HIDP_STATUS_SUCCESS;
			data->UsagePage = buttonCaps->UsagePage;

			if (buttonCaps->IsRange)
			{
				data->ButtonData.UsageMin = buttonCaps -> Range.UsageMin;
				data->ButtonData.UsageMax = buttonCaps -> Range.UsageMax;
			}
			else
			{
				data -> ButtonData.UsageMin = data -> ButtonData.UsageMax = buttonCaps -> NotRange.Usage;
			}
        
			data->ButtonData.MaxUsageLength = fHidP_MaxUsageListLength (
													HidP_Feature,
													buttonCaps->UsagePage,
													HidDevice->Ppd);
			data->ButtonData.Usages = (PUSAGE)
				 calloc (data->ButtonData.MaxUsageLength, sizeof (USAGE));

			data->ReportID = buttonCaps -> ReportID;
		}

	    valueCaps = HidDevice->FeatureValueCaps;

		for (i = 0; i < HidDevice->Caps.NumberFeatureValueCaps ; i++, valueCaps++) 
		{
			if (valueCaps->IsRange)
			{
				for (usage = valueCaps->Range.UsageMin;
					 usage <= valueCaps->Range.UsageMax;
					 usage++)
				{
					data->IsButtonData = FALSE;
					data->Status = HIDP_STATUS_SUCCESS;
					data->UsagePage = valueCaps->UsagePage;
					data->ValueData.Usage = usage;
					data->ReportID = valueCaps -> ReportID;
					data++;
				}
			} 
			else
			{
				data->IsButtonData = FALSE;
				data->Status = HIDP_STATUS_SUCCESS;
				data->UsagePage = valueCaps->UsagePage;
				data->ValueData.Usage = valueCaps->NotRange.Usage;
				data->ReportID = valueCaps -> ReportID;
				data++;
			}
		}
	}

    return (TRUE);
}

VOID
CloseHidDevice (
    IN PHID_DEVICE HidDevice
)
{
    if (INVALID_HANDLE_VALUE != HidDevice -> HidDevice)
    {
        CloseHandle(HidDevice -> HidDevice);
		HidDevice -> HidDevice = INVALID_HANDLE_VALUE;
    }
    
    if (NULL != HidDevice -> Ppd)
    {
        fHidD_FreePreparsedData(HidDevice -> Ppd);
		HidDevice -> Ppd = NULL;
    }

    if (NULL != HidDevice -> InputReportBuffer)
    {
        free(HidDevice -> InputReportBuffer);
		HidDevice -> InputReportBuffer = NULL;
    }

    if (NULL != HidDevice -> InputData)
    {
        free(HidDevice -> InputData);
		HidDevice -> InputData = NULL;
    }

    if (NULL != HidDevice -> InputButtonCaps)
    {
        free(HidDevice -> InputButtonCaps);
		HidDevice -> InputButtonCaps = NULL;
    }

    if (NULL != HidDevice -> InputValueCaps)
    {
        free(HidDevice -> InputValueCaps);
		HidDevice -> InputValueCaps = NULL;
    }

    if (NULL != HidDevice -> OutputReportBuffer)
    {
        free(HidDevice -> OutputReportBuffer);
		HidDevice -> OutputReportBuffer = NULL;
    }

    if (NULL != HidDevice -> OutputData)
    {
        free(HidDevice -> OutputData);
		HidDevice -> OutputData = NULL;
    }

    if (NULL != HidDevice -> OutputButtonCaps) 
    {
        free(HidDevice -> OutputButtonCaps);
		HidDevice -> OutputButtonCaps = NULL;
    }

    if (NULL != HidDevice -> OutputValueCaps)
    {
        free(HidDevice -> OutputValueCaps);
		HidDevice -> OutputValueCaps = NULL;
    }

    if (NULL != HidDevice -> FeatureReportBuffer)
    {
        free(HidDevice -> FeatureReportBuffer);
		HidDevice -> FeatureReportBuffer = NULL;
    }

    if (NULL != HidDevice -> FeatureData) 
    {
        free(HidDevice -> FeatureData);
		HidDevice -> FeatureData = NULL;
    }

    if (NULL != HidDevice -> FeatureButtonCaps) 
    {
        free(HidDevice -> FeatureButtonCaps);
		HidDevice -> FeatureButtonCaps = NULL;
    }

    if (NULL != HidDevice -> FeatureValueCaps) 
    {
        free(HidDevice -> FeatureValueCaps);
		HidDevice -> FeatureValueCaps = NULL;
    }

     return;
}

ULONG ResetAllButtonUsage(PHID_DATA pData, ULONG DataLength)
{
	ULONG ReportID=0x00;
	for (unsigned int i = 0; i < DataLength; i++, pData++) 
	{
		if (pData->IsButtonData)
		{
			pData->ButtonData.Usages[0]=0x0;
		}
		else
		{
			pData->ValueData.Value=0x0;
		}
	}
	return ReportID;
}

ULONG ResetButtonUsage(PHID_DATA pData, ULONG DataLength, ULONG usage_page, ULONG usage_id)
{
	ULONG ReportID=0x00;
	for (unsigned int i = 0; i < DataLength; i++, pData++) 
	{
		if (pData->IsButtonData
			&& pData->UsagePage==usage_page
			&& pData->ButtonData.UsageMin==usage_id)
		{
			pData->ButtonData.Usages[0]=0x0;
			ReportID=pData->ReportID;
		}
		//else if (pData->IsButtonData)
		//{
		//	pData->ButtonData.Usages[0]=0x0;
		//}
	}
	return ReportID;
}

ULONG SetButtonUsage(PHID_DATA pData, ULONG DataLength, ULONG usage_page, ULONG usage_id, USHORT val)
{
	ULONG ReportID=0x00;
	for (unsigned int i = 0; i < DataLength; i++, pData++) 
	{
		if (pData->IsButtonData
			&& pData->UsagePage==usage_page
			&& pData->ButtonData.UsageMin==usage_id)
		{
			pData->ButtonData.Usages[0]=val;
			ReportID=pData->ReportID;
		}
	}
	return ReportID;
}


ULONG SetOutputValue(PHID_DATA pData, ULONG DataLength, ULONG usage_page, ULONG usage_id, USHORT val)
{
	ULONG ReportID=0x00;
	for (unsigned int i = 0; i < DataLength; i++, pData++) 
	{
		if (!pData->IsButtonData
			&& pData->UsagePage==usage_page
			&& pData->ValueData.Usage==usage_id)
		{
			//pData->ValueData.Usages[0]=val;
			ReportID=pData->ReportID;
			pData->ValueData.Value=val;
		}
	}
	return ReportID;
}

#endif
