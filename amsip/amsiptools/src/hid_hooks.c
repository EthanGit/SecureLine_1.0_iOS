
#include <amsiptools/hid_hooks.h>

#define MICROSOFT_VENDOR_ID 0x045e
#define MICROSOFT_PRODUCT_ID_CATALINA 0xffca

#define POLYCOM_VENDOR_ID 0x095D
#define POLYCOM_PRODUCT_ID_COMMUNICATOR 0x0005

#define ZMM_VENDOR_ID 0x1994
#define ZMM_PRODUCT_ID_USBPHONE 0x2004

#define PLANTRONICS_VENDOR_ID 0x047f
#define PLANTRONICS_PRODUCT_ID_CS60 0x0410
#define PLANTRONICS_PRODUCT_ID_SAVIOFFICE 0x0411
#define PLANTRONICS_PRODUCT_ID_SAVI7XX 0xAC01 //TODO: MACOSX
#define PLANTRONICS_PRODUCT_ID_BTADAPTER 0x4254 //ONLY HOOK & TALK buttons
#define PLANTRONICS_PRODUCT_ID_DA45 0xDA45
#define PLANTRONICS_PRODUCT_ID_MCD100 0xD101
#define PLANTRONICS_PRODUCT_ID_BUA200 0x0715
#define PLANTRONICS_PRODUCT_ID_C420 0xAA10
#define PLANTRONICS_PRODUCT_ID_P540M 0x0A11

#define PLANTRONICS_PRODUCT_ID_C420_2 0xAA14 //TODO: MACOSX
#define PLANTRONICS_PRODUCT_ID_A478USB 0xC011 //TODO: MACOSX
#define PLANTRONICS_PRODUCT_ID_BT300 0x0415 //TODO: MACOSX


#define GNNETCOM_VENDOR_ID 0x0B0E
#define GNNETCOM_PRODUCT_ID_A330 0xA330
#define GNNETCOM_PRODUCT_ID_BIZ620USB 0x0930
#define GNNETCOM_PRODUCT_ID_DIAL520 0x0520 //MACOSX: MISSING # KEY ON MACOSX?
#define GNNETCOM_PRODUCT_ID_LINK280 0x0910
#define GNNETCOM_PRODUCT_ID_LINK350OC 0xA340
#define GNNETCOM_PRODUCT_ID_BIZ2400USB 0x091C
#define GNNETCOM_PRODUCT_ID_GO6470 0x1003
#define GNNETCOM_PRODUCT_ID_GO9470 0x1041 //MACOSX: AUDIO PROBLEM?

#define GNNETCOM_PRODUCT_ID_M5390 0xA335
#define GNNETCOM_PRODUCT_ID_M5390USB 0xA338
#define GNNETCOM_PRODUCT_ID_BIZ2400 0x090A
#define GNNETCOM_PRODUCT_ID_GN9350 0x9350
#define GNNETCOM_PRODUCT_ID_GN9330 0x9330
#define GNNETCOM_PRODUCT_ID_GN8120 0x8120 //MACOSX: NOT WORKING?

#define GNNETCOM_PRODUCT_ID_UC250 0x0341 //TODO: RINGER DOES NOT WORK ON MACOSX //AUDIO BROKEN (with itunes too...)
#define GNNETCOM_PRODUCT_ID_UC550DUO 0x0030
#define GNNETCOM_PRODUCT_ID_UC550MONO 0x0031
#define GNNETCOM_PRODUCT_ID_UC150MONO 0x0043 //TODO: RINGER DOES NOT WORK ON MACOSX //AUDIO BROKEN (with itunes too...) //OTHER ISSUE WITH HOOK/HANG?
#define GNNETCOM_PRODUCT_ID_UC150DUO 0x0041 //TODO: RINGER DOES NOT WORK ON MACOSX //AUDIO BROKEN (with itunes too...) //OTHER ISSUE WITH HOOK/HANG?
#define GNNETCOM_PRODUCT_ID_BIZ2400MONOUSB 0x2400
#define GNNETCOM_PRODUCT_ID_PRO930 0x1016
#define GNNETCOM_PRODUCT_ID_PRO9450 0x1022

#define USAGEPAGE_TELEPHONY 0xB
#define USAGEPAGE_LED 0x8
#define USAGEPAGE_GNNETCOM 0xFF30

#define USAGEID_TELEPHONY_HOOK_SWITCH 0x20
#define USAGEID_TELEPHONY_PHONE_MUTE 0x20
#define USAGEID_TELEPHONY_RINGER 0x20

#define USAGEID_LED_OFFHOOK 0x17
#define USAGEID_LED_RING 0x18
#define USAGEID_LED_MICROPHONE 0x21
#define USAGEID_LED_ONLINE 0x2A

#define USAGEID_GNNETCOM_REJECTCALL 0xFFFD

#include "hid_devices.h"

#if defined(WIN32) && defined(ENABLE_HID)

static struct hid_device jdevice;

extern hid_device_desc_t microsoft_catalina;
extern hid_device_desc_t polycom_communicator;
extern hid_device_desc_t zmm_usbphone;
extern hid_device_desc_t plantronics_cs60;
extern hid_device_desc_t plantronics_savioffice;
extern hid_device_desc_t plantronics_savi7xx;
extern hid_device_desc_t plantronics_btadapter; //ONLY HOOK & TALK buttons
extern hid_device_desc_t plantronics_da45;
extern hid_device_desc_t plantronics_mcd100;
extern hid_device_desc_t plantronics_bua200;
extern hid_device_desc_t plantronics_c420;

extern hid_device_desc_t plantronics_c420_2;
extern hid_device_desc_t plantronics_A478USB;
extern hid_device_desc_t plantronics_BT300;

extern hid_device_desc_t gnetcom_A330;
extern hid_device_desc_t gnetcom_BIZ620USB;
extern hid_device_desc_t gnetcom_DIAL520;
extern hid_device_desc_t gnetcom_LINK280;
extern hid_device_desc_t gnetcom_LINK350OC;
extern hid_device_desc_t gnetcom_BIZ2400USB;
extern hid_device_desc_t gnetcom_GO6470;
extern hid_device_desc_t gnetcom_GO9470;

//TO BE TESTED AGAIN:
extern hid_device_desc_t gnetcom_M5390;
extern hid_device_desc_t gnetcom_M5390USB;
extern hid_device_desc_t gnetcom_BIZ2400;
extern hid_device_desc_t gnetcom_GN9350;
extern hid_device_desc_t gnetcom_GN9330;
extern hid_device_desc_t gnetcom_GN8120;

extern hid_device_desc_t gnetcom_UC250;
extern hid_device_desc_t gnetcom_UC550DUO;
extern hid_device_desc_t gnetcom_UC550MONO;
extern hid_device_desc_t gnetcom_UC150MONO;
extern hid_device_desc_t gnetcom_UC150DUO;
extern hid_device_desc_t gnetcom_BIZ2400MONOUSB;
extern hid_device_desc_t gnetcom_PRO930;
extern hid_device_desc_t gnetcom_PRO9450;

#elif defined(__APPLE__) && defined(ENABLE_HID)

static struct hid_device jdevice;

extern hid_device_desc_t microsoft_catalina;
extern hid_device_desc_t polycom_communicator;
extern hid_device_desc_t zmm_usbphone;
extern hid_device_desc_t plantronics_cs60;
extern hid_device_desc_t plantronics_savioffice;
extern hid_device_desc_t plantronics_btadapter; //ONLY HOOK & TALK buttons
extern hid_device_desc_t plantronics_da45;

extern hid_device_desc_t plantronics_c420_2;
extern hid_device_desc_t plantronics_A478USB;
extern hid_device_desc_t plantronics_BT300;

extern hid_device_desc_t gnetcom_A330;
extern hid_device_desc_t gnetcom_BIZ620USB;
extern hid_device_desc_t gnetcom_DIAL520; //MACOSX: MISSING # KEY ON MACOSX?
extern hid_device_desc_t gnetcom_LINK280;
extern hid_device_desc_t gnetcom_LINK350OC;
extern hid_device_desc_t gnetcom_BIZ2400USB;
extern hid_device_desc_t gnetcom_GO6470;
extern hid_device_desc_t gnetcom_GO9470; //MACOSX: AUDIO PROBLEM?

extern hid_device_desc_t gnetcom_GN8120; //MACOSX: NOT WORKING?

extern hid_device_desc_t gnetcom_UC250;
extern hid_device_desc_t gnetcom_UC550DUO;
extern hid_device_desc_t gnetcom_UC550MONO;
extern hid_device_desc_t gnetcom_UC150MONO;
extern hid_device_desc_t gnetcom_UC150DUO;
extern hid_device_desc_t gnetcom_BIZ2400MONOUSB;
extern hid_device_desc_t gnetcom_PRO930;
extern hid_device_desc_t gnetcom_PRO9450;

static void CFSetApplierFunctionCopyToCFArray(const void *value, void *context)
{
	CFArrayAppendValue( ( CFMutableArrayRef ) context, value );
}	// CFSetApplierFunctionCopyToCFArray

static CFComparisonResult CFDeviceArrayComparatorFunction(const void *val1, const void *val2, void *context)
{
#pragma unused( context )
	CFComparisonResult result = kCFCompareEqualTo;
	
	long loc1 = IOHIDDevice_GetLocationID( ( IOHIDDeviceRef ) val1 );
	long loc2 = IOHIDDevice_GetLocationID( ( IOHIDDeviceRef ) val2 );
	
	if ( loc1 < loc2 ) {
		result = kCFCompareLessThan;
	} else if ( loc1 > loc2 ) {
		result = kCFCompareGreaterThan;
	}
	return result;
}	// CFDeviceArrayComparatorFunction

static CFStringRef Copy_DeviceName( IOHIDDeviceRef inDeviceRef ) {
	CFStringRef result = NULL;
	if ( inDeviceRef ) {
		CFStringRef manCFStringRef = IOHIDDevice_GetManufacturer( inDeviceRef );
		if ( manCFStringRef ) {
			// make a copy that we can CFRelease later
			CFMutableStringRef tCFStringRef = CFStringCreateMutableCopy( kCFAllocatorDefault, 0, manCFStringRef );
			// trim off any trailing spaces
			while ( CFStringHasSuffix( tCFStringRef, CFSTR( " " ) ) ) {
				CFIndex cnt = CFStringGetLength( tCFStringRef );
				if ( !cnt ) break;
				CFStringDelete( tCFStringRef, CFRangeMake( cnt - 1, 1 ) );
			}
			manCFStringRef = tCFStringRef;
		} else {
			// try the vendor ID source
			manCFStringRef = IOHIDDevice_GetVendorIDSource( inDeviceRef );
		}
		if ( !manCFStringRef ) {
			// use the vendor ID to make a manufacturer string
			long vendorID = IOHIDDevice_GetVendorID( inDeviceRef );
			manCFStringRef = CFStringCreateWithFormat( kCFAllocatorDefault, NULL, CFSTR("vendor: %d"), vendorID );
		}
		
		CFStringRef prodCFStringRef = IOHIDDevice_GetProduct( inDeviceRef );
		if ( prodCFStringRef ) {
			// make a copy that we can CFRelease later
			prodCFStringRef = CFStringCreateCopy( kCFAllocatorDefault, prodCFStringRef );
		} else {
			// use the product ID
			long productID = IOHIDDevice_GetProductID( inDeviceRef );
			// to make a product string
			prodCFStringRef = CFStringCreateWithFormat( kCFAllocatorDefault, NULL, CFSTR("%@ - product id %d"), manCFStringRef, productID );
		}
		assert( prodCFStringRef );
		
		// if the product name begins with the manufacturer string...
		if ( CFStringHasPrefix( prodCFStringRef, manCFStringRef ) ) {
			// then just use the product name
			result = CFStringCreateCopy( kCFAllocatorDefault, prodCFStringRef );
		} else {	// otherwise
			// append the product name to the manufacturer
			result = CFStringCreateWithFormat( kCFAllocatorDefault, NULL, CFSTR("%@ - %@"), manCFStringRef, prodCFStringRef );
		}
		
		if ( manCFStringRef ) {
			CFRelease( manCFStringRef );
		}
		if ( prodCFStringRef ) {
			CFRelease( prodCFStringRef );
		}
	}
	return result;
}	// Copy_DeviceName

#endif

static int _hid_hooks_start(hid_hooks_t *ph, int vendor, int product)
{
#if defined(WIN32) && defined(ENABLE_HID)
	HINSTANCE hHID = NULL;
	hHID = LoadLibrary("HID.DLL");

	if (hHID==NULL)
	{
		ms_error("hid.dll is not available");
		return HID_ERROR_DEVICEFAILURE;
	}

	memset(&jdevice, 0, sizeof(struct hid_device));
	ph->jdevice=NULL;
	ph->desc=NULL;

	fHidD_GetProductString = (PHidD_GetProductString)
	GetProcAddress(hHID, "HidD_GetProductString");
	fHidD_GetHidGuid = (PHidD_GetHidGuid)
	GetProcAddress(hHID, "HidD_GetHidGuid");
	fHidD_GetAttributes = (PHidD_GetAttributes)
	GetProcAddress(hHID, "HidD_GetAttributes");
	fHidD_SetFeature = (PHidD_SetFeature)
	GetProcAddress(hHID, "HidD_SetFeature");
	fHidD_GetFeature = (PHidD_GetFeature)
	GetProcAddress(hHID, "HidD_GetFeature");
	fHidD_GetInputReport = (PHidD_GetInputReport)
	GetProcAddress(hHID, "HidD_GetInputReport");
	
	fHidD_GetPreparsedData = (PHidD_GetPreparsedData)
	GetProcAddress(hHID, "HidD_GetPreparsedData");
	fHidD_FreePreparsedData = (PHidD_FreePreparsedData)
	GetProcAddress(hHID, "HidD_FreePreparsedData");	
	fHidP_GetCaps = (PHidP_GetCaps)
	GetProcAddress(hHID, "HidP_GetCaps");
	fHidP_GetValueCaps = (PHidP_GetValueCaps)
	GetProcAddress(hHID, "HidP_GetValueCaps");
	fHidP_GetButtonCaps = (PHidP_GetButtonCaps)
	GetProcAddress(hHID, "HidP_GetButtonCaps");

	fHidP_GetUsageValue = (PHidP_GetUsageValue)
	GetProcAddress(hHID, "HidP_GetUsageValue");
	fHidP_GetScaledUsageValue = (PHidP_GetScaledUsageValue)
	GetProcAddress(hHID, "HidP_GetScaledUsageValue");
	fHidP_GetUsages = (PHidP_GetUsages)
	GetProcAddress(hHID, "HidP_GetUsages");
	fHidP_SetUsages = (PHidP_SetUsages)
	GetProcAddress(hHID, "HidP_SetUsages");
	fHidP_SetUsageValue = (PHidP_SetUsageValue)
	GetProcAddress(hHID, "HidP_SetUsageValue");

	fHidP_MaxUsageListLength = (PHidP_MaxUsageListLength)
	GetProcAddress(hHID, "HidP_MaxUsageListLength");

	if (fHidD_GetProductString==NULL
		|| fHidD_GetHidGuid==NULL
		|| fHidD_GetAttributes==NULL
		|| fHidD_SetFeature==NULL
		|| fHidD_GetFeature==NULL
		|| fHidD_GetInputReport==NULL
		|| fHidD_GetPreparsedData==NULL
		|| fHidD_FreePreparsedData==NULL
		|| fHidP_GetCaps==NULL
		|| fHidP_GetValueCaps==NULL
		|| fHidP_GetButtonCaps==NULL
		|| fHidP_GetUsageValue==NULL
		|| fHidP_GetScaledUsageValue==NULL
		|| fHidP_GetUsages==NULL
		|| fHidP_SetUsages==NULL
		|| fHidP_SetUsageValue==NULL
		|| fHidP_MaxUsageListLength==NULL
		)
	{
		ms_error("hid.dll is not compatible");
		return HID_ERROR_DEVICEFAILURE;
	}

	GUID HidGuid;
	(fHidD_GetHidGuid)(&HidGuid);
	HDEVINFO hDevInfo;
	//Get information about HIDs
	try {
		hDevInfo = SetupDiGetClassDevs
			(&HidGuid,NULL,NULL,DIGCF_PRESENT|DIGCF_INTERFACEDEVICE);
		if (hDevInfo == INVALID_HANDLE_VALUE)
			return HID_ERROR_DEVICEFAILURE;
	} catch (...) {
			return HID_ERROR_DEVICEFAILURE;
	}

	//Identify each HID interface
	SP_DEVICE_INTERFACE_DATA devInfoData;
	devInfoData.cbSize = sizeof(devInfoData);
	DWORD MemberIndex = 0;
	int Result;
	while (1)
	{
		try {
			Result = SetupDiEnumDeviceInterfaces
				(hDevInfo,0,&HidGuid,MemberIndex,&devInfoData);
			if (!Result)
			{
				SetupDiDestroyDeviceInfoList(hDevInfo);
				return HID_ERROR_DEVICEFAILURE; //No more devices found
			}
			MemberIndex++;
		} catch (...) {
			SetupDiDestroyDeviceInfoList (hDevInfo);
			return HID_ERROR_DEVICEFAILURE;
		}

		//Get the Pathname of the current device
		PSP_DEVICE_INTERFACE_DETAIL_DATA detailData;
		DWORD Reguired = 0;

		Result = SetupDiGetDeviceInterfaceDetail
			(hDevInfo,&devInfoData,NULL,0,&Reguired,NULL);
		if (Reguired<=0)
			continue;

		detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(Reguired);

#if defined(_WIN64)
		detailData->cbSize = 8;
#else
		detailData->cbSize = 5;
#endif
		try {
			Result = SetupDiGetDeviceInterfaceDetail
				(hDevInfo,&devInfoData,detailData,256,&Reguired,NULL);
			if (!Result)
			{
				free(detailData);
				continue;
			}
		} catch (...) {
			free(detailData);
			continue;
		}
		try {
			if (!OpenHidDevice(detailData->DevicePath,
				TRUE,
				TRUE,
				TRUE,
				FALSE,
				&jdevice.hid_dev))
			{
				free(detailData);
				continue;
			}
		} catch (...) {
			free(detailData);
			continue;
		}

		if (vendor!=0
			&& ((jdevice.hid_dev.Attributes.VendorID != vendor)
			|| (jdevice.hid_dev.Attributes.ProductID != product)))
		{
			CloseHidDevice(&jdevice.hid_dev);
			memset(&jdevice.hid_dev, 0, sizeof(HID_DEVICE));
			free(detailData);
			continue;
		}

		if ((jdevice.hid_dev.Attributes.VendorID == MICROSOFT_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == MICROSOFT_PRODUCT_ID_CATALINA))
		{
			ph->desc = &microsoft_catalina;
			ph->jdevice = &jdevice;
			break;
		}
		else if ((jdevice.hid_dev.Attributes.VendorID == POLYCOM_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == POLYCOM_PRODUCT_ID_COMMUNICATOR))
		{
			ph->desc = &polycom_communicator;
			ph->jdevice = &jdevice;
			break;
		}
		else if ((jdevice.hid_dev.Attributes.VendorID == PLANTRONICS_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == PLANTRONICS_PRODUCT_ID_CS60))
		{
			ph->desc = &plantronics_cs60;
			ph->jdevice = &jdevice;
			break;
		}
		else if ((jdevice.hid_dev.Attributes.VendorID == PLANTRONICS_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == PLANTRONICS_PRODUCT_ID_SAVIOFFICE))
		{
			if (jdevice.hid_dev.Caps.UsagePage==HID_USAGE_PAGE_TELEPHONY)
			{
				ph->desc = &plantronics_savioffice;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ((jdevice.hid_dev.Attributes.VendorID == PLANTRONICS_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == PLANTRONICS_PRODUCT_ID_SAVI7XX))
		{
			if (jdevice.hid_dev.Caps.UsagePage==HID_USAGE_PAGE_TELEPHONY)
			{
				ph->desc = &plantronics_savi7xx;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ((jdevice.hid_dev.Attributes.VendorID == PLANTRONICS_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == PLANTRONICS_PRODUCT_ID_BTADAPTER))
		{
			if (jdevice.hid_dev.Caps.NumberInputButtonCaps == 8
				&& jdevice.hid_dev.Caps.NumberOutputButtonCaps == 1)
			{
				ph->desc = &plantronics_btadapter;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ((jdevice.hid_dev.Attributes.VendorID == PLANTRONICS_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == PLANTRONICS_PRODUCT_ID_DA45))
		{
			if (jdevice.hid_dev.Caps.NumberInputButtonCaps == 3
				&& jdevice.hid_dev.Caps.NumberOutputButtonCaps == 5)
			{
				ph->desc = &plantronics_da45;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ((jdevice.hid_dev.Attributes.VendorID == PLANTRONICS_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == PLANTRONICS_PRODUCT_ID_MCD100))
		{
			if (jdevice.hid_dev.Caps.NumberInputButtonCaps == 3
				&& jdevice.hid_dev.Caps.NumberOutputButtonCaps == 0)
			{
				ph->desc = &plantronics_mcd100;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ((jdevice.hid_dev.Attributes.VendorID == PLANTRONICS_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == PLANTRONICS_PRODUCT_ID_BUA200))
		{
			if (jdevice.hid_dev.Caps.UsagePage==HID_USAGE_PAGE_TELEPHONY)
			{
				ph->desc = &plantronics_bua200;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ((jdevice.hid_dev.Attributes.VendorID == PLANTRONICS_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == PLANTRONICS_PRODUCT_ID_C420))
		{
			if (jdevice.hid_dev.Caps.UsagePage==HID_USAGE_PAGE_TELEPHONY)
			{
				ph->desc = &plantronics_c420;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ((jdevice.hid_dev.Attributes.VendorID == PLANTRONICS_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == PLANTRONICS_PRODUCT_ID_C420_2))
		{
			if (jdevice.hid_dev.Caps.UsagePage==HID_USAGE_PAGE_TELEPHONY)
			{
				ph->desc = &plantronics_c420_2;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ((jdevice.hid_dev.Attributes.VendorID == PLANTRONICS_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == PLANTRONICS_PRODUCT_ID_A478USB))
		{
			if (jdevice.hid_dev.Caps.UsagePage==0xFFA0)
			{
				ph->desc = &plantronics_A478USB;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ((jdevice.hid_dev.Attributes.VendorID == PLANTRONICS_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == PLANTRONICS_PRODUCT_ID_BT300))
		{
			if (jdevice.hid_dev.Caps.UsagePage==HID_USAGE_PAGE_TELEPHONY)
			{
				ph->desc = &plantronics_BT300;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ((jdevice.hid_dev.Attributes.VendorID == ZMM_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == ZMM_PRODUCT_ID_USBPHONE))
		{
			ph->desc = &zmm_usbphone;
			ph->jdevice = &jdevice;
			break;
		}
		else if ( (jdevice.hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
			((jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_M5390) ||
			(jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_M5390USB) ||
			(jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_BIZ2400)) )
		{
		}
		else if ( (jdevice.hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_A330))
		{
			if (jdevice.hid_dev.Caps.NumberInputButtonCaps == 4
				&& jdevice.hid_dev.Caps.NumberOutputButtonCaps == 6)
			{
				ph->desc = &gnetcom_A330;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ( (jdevice.hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_DIAL520))
		{
			ph->desc = &gnetcom_DIAL520;
			ph->jdevice = &jdevice;
			break;
		}
		else if ( (jdevice.hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_GO9470))
		{
			if (jdevice.hid_dev.Caps.UsagePage==HID_USAGE_PAGE_TELEPHONY)
			{
				ph->desc = &gnetcom_GO9470;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ( (jdevice.hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_GO6470))
		{
			if (jdevice.hid_dev.Caps.UsagePage==HID_USAGE_PAGE_TELEPHONY)
			{
				ph->desc = &gnetcom_GO6470;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ( (jdevice.hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_BIZ2400USB))
		{
			if (jdevice.hid_dev.Caps.UsagePage==HID_USAGE_PAGE_TELEPHONY)
			{
				ph->desc = &gnetcom_BIZ2400USB;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ( (jdevice.hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_LINK280))
		{
			if (jdevice.hid_dev.Caps.UsagePage==HID_USAGE_PAGE_TELEPHONY)
			{
				ph->desc = &gnetcom_LINK280;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ( (jdevice.hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_LINK350OC))
		{
			if (jdevice.hid_dev.Caps.UsagePage==HID_USAGE_PAGE_TELEPHONY)
			{
				ph->desc = &gnetcom_LINK350OC;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ( (jdevice.hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_BIZ620USB))
		{
			if (jdevice.hid_dev.Caps.UsagePage==HID_USAGE_PAGE_TELEPHONY)
			{
				ph->desc = &gnetcom_BIZ620USB;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ( (jdevice.hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_GN9330))
		{
			ph->desc = &gnetcom_GN9330;
			ph->jdevice = &jdevice;
			break;
		}
		else if ( (jdevice.hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
			((jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_GN9350) ||
			(jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_GN9330)) )
		{
			ph->desc = &gnetcom_GN9350;
			ph->jdevice = &jdevice;
			break;
		}
		else if ( (jdevice.hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_GN8120) )
		{
			ph->desc = &gnetcom_GN8120;
			ph->jdevice = &jdevice;
			break;
		}
		else if ( (jdevice.hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_UC250) )
		{
			if (jdevice.hid_dev.Caps.UsagePage==HID_USAGE_PAGE_TELEPHONY)
			{
				ph->desc = &gnetcom_UC250;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ( (jdevice.hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_UC550DUO) )
		{
			if (jdevice.hid_dev.Caps.UsagePage==HID_USAGE_PAGE_TELEPHONY)
			{
				ph->desc = &gnetcom_UC550DUO;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ( (jdevice.hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_UC550MONO) )
		{
			if (jdevice.hid_dev.Caps.UsagePage==HID_USAGE_PAGE_TELEPHONY)
			{
				ph->desc = &gnetcom_UC550MONO;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ( (jdevice.hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_UC150MONO) )
		{
			if (jdevice.hid_dev.Caps.UsagePage==HID_USAGE_PAGE_TELEPHONY)
			{
				ph->desc = &gnetcom_UC150MONO;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ( (jdevice.hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_UC150DUO) )
		{
			if (jdevice.hid_dev.Caps.UsagePage==HID_USAGE_PAGE_TELEPHONY)
			{
				ph->desc = &gnetcom_UC150DUO;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ( (jdevice.hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_BIZ2400MONOUSB) )
		{
			if (jdevice.hid_dev.Caps.UsagePage==HID_USAGE_PAGE_TELEPHONY)
			{
				ph->desc = &gnetcom_BIZ2400MONOUSB;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ( (jdevice.hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_PRO930) )
		{
			if (jdevice.hid_dev.Caps.UsagePage==HID_USAGE_PAGE_TELEPHONY)
			{
				ph->desc = &gnetcom_PRO930;
				ph->jdevice = &jdevice;
				break;
			}
		}
		else if ( (jdevice.hid_dev.Attributes.VendorID == GNNETCOM_VENDOR_ID) &&
			(jdevice.hid_dev.Attributes.ProductID == GNNETCOM_PRODUCT_ID_PRO9450) )
		{
			if (jdevice.hid_dev.Caps.UsagePage==HID_USAGE_PAGE_TELEPHONY)
			{
				ph->desc = &gnetcom_PRO9450;
				ph->jdevice = &jdevice;
				break;
			}
		}

		CloseHidDevice(&jdevice.hid_dev);
		memset(&jdevice.hid_dev, 0, sizeof(HID_DEVICE));
		free(detailData);
		
	} //end of while

	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->desc==NULL)
		return HID_ERROR_NODEVICEFOUND;

	jdevice.hEvent=NULL;
	jdevice.hEvent = CreateEvent(NULL, TRUE, TRUE, ""); 
	if (jdevice.hEvent==NULL)
	{
		ph->jdevice=NULL;
		CloseHidDevice(&jdevice.hid_dev);
		memset(&jdevice.hid_dev, 0, sizeof(HID_DEVICE));
		SetupDiDestroyDeviceInfoList (hDevInfo);
		return HID_ERROR_DEVICEFAILURE;
	}

	ph->jdevice->ringer = false;
	ph->jdevice->mute = false;
	ph->jdevice->audioenabled = true;

	if (ph->desc->hd_init!=NULL)
		ph->desc->hd_init(ph);

	hid_hooks_set_audioenabled(ph, false);

	SetupDiDestroyDeviceInfoList (hDevInfo);
	return 0;

#elif defined(__APPLE__) && defined(ENABLE_HID)
	
	memset(&jdevice, 0, sizeof(struct hid_device));
	ph->jdevice=NULL;
	ph->desc=NULL;

	if ( !gIOHIDManagerRef ) {
		// create the manager
		gIOHIDManagerRef = IOHIDManagerCreate( kCFAllocatorDefault, 0L );
	}
	if ( gIOHIDManagerRef ) {
		// open it
		IOReturn tIOReturn = IOHIDManagerOpen( gIOHIDManagerRef, 0L);
		if ( kIOReturnSuccess != tIOReturn ) {
			ms_error("%s: Couldn’t open IOHIDManager.", __PRETTY_FUNCTION__ );
		} else {
			IOHIDManagerSetDeviceMatching( gIOHIDManagerRef, NULL );
			CFSetRef devCFSetRef = IOHIDManagerCopyDevices( gIOHIDManagerRef );
			if ( devCFSetRef ) {
				if ( gDeviceCFArrayRef ) {
					CFRelease( gDeviceCFArrayRef );
				}
				gDeviceCFArrayRef = CFArrayCreateMutable( kCFAllocatorDefault, 0, & kCFTypeArrayCallBacks );
				CFSetApplyFunction( devCFSetRef, CFSetApplierFunctionCopyToCFArray, gDeviceCFArrayRef );
				CFIndex cnt = CFArrayGetCount( gDeviceCFArrayRef );
				CFArraySortValues( gDeviceCFArrayRef, CFRangeMake( 0, cnt ), CFDeviceArrayComparatorFunction, NULL );
				CFRelease( devCFSetRef );
			}
		}
	} else {
		ms_error("%s: Couldn’t create a IOHIDManager.", __PRETTY_FUNCTION__ );
		return -1;
	}
	
	CFIndex idx, cnt = CFArrayGetCount( gDeviceCFArrayRef );
	for ( idx = 0; idx < cnt; idx++ ) {
		IOHIDDeviceRef tIOHIDDeviceRef = ( IOHIDDeviceRef ) CFArrayGetValueAtIndex( gDeviceCFArrayRef, idx );
		if ( tIOHIDDeviceRef ) {
			CFStringRef tCFStringRef = Copy_DeviceName( tIOHIDDeviceRef );
			if ( tCFStringRef ) {
				char devname[256];

				//status = AppendMenuItemTextWithCFString( tMenuHdl, tCFStringRef, kMenuItemAttrIgnoreMeta, kDeviceMenuCommand, NULL );
				//if ( noErr == status ) {
				//	numItems++;
				//}
				//SetMenuItemProperty( tMenuHdl, numItems, gPropertyCreator, gPropertyTagDeviceRef, sizeof( IOHIDDeviceRef ), &tIOHIDDeviceRef );
				CFStringGetCString(tCFStringRef, devname, 256,
								   CFStringGetSystemEncoding());
				CFRelease( tCFStringRef );
				
				long vendorID = IOHIDDevice_GetVendorID( tIOHIDDeviceRef );
				long productID = IOHIDDevice_GetProductID( tIOHIDDeviceRef );

				ms_message( "%s: %04x:%04x\n", __PRETTY_FUNCTION__, vendorID, productID);
				
				if (vendor!=0 && ((vendorID != vendor) || (productID != product)))
				{
					continue;
				}
				
				if ((vendorID == POLYCOM_VENDOR_ID) &&
					(productID == POLYCOM_PRODUCT_ID_COMMUNICATOR))
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &polycom_communicator;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==MICROSOFT_VENDOR_ID
					&&productID==MICROSOFT_PRODUCT_ID_CATALINA)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &microsoft_catalina;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==PLANTRONICS_VENDOR_ID
						 &&productID==PLANTRONICS_PRODUCT_ID_CS60)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &plantronics_cs60;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==PLANTRONICS_VENDOR_ID
						 &&productID==PLANTRONICS_PRODUCT_ID_SAVIOFFICE)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &plantronics_savioffice;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==PLANTRONICS_VENDOR_ID
						 &&productID==PLANTRONICS_PRODUCT_ID_BTADAPTER)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &plantronics_btadapter;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==PLANTRONICS_VENDOR_ID
                 &&productID==PLANTRONICS_PRODUCT_ID_DA45)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &plantronics_da45;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==PLANTRONICS_VENDOR_ID
                 &&productID==PLANTRONICS_PRODUCT_ID_C420_2)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &plantronics_c420_2;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==PLANTRONICS_VENDOR_ID
                 &&productID==PLANTRONICS_PRODUCT_ID_A478USB)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &plantronics_A478USB;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==PLANTRONICS_VENDOR_ID
                 &&productID==PLANTRONICS_PRODUCT_ID_BT300)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &plantronics_BT300;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
        
				else if (vendorID==ZMM_VENDOR_ID
						 &&productID==ZMM_PRODUCT_ID_USBPHONE)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &zmm_usbphone;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==GNNETCOM_VENDOR_ID
						 &&productID==GNNETCOM_PRODUCT_ID_A330)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &gnetcom_A330;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==GNNETCOM_VENDOR_ID
						 &&productID==GNNETCOM_PRODUCT_ID_GO6470)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &gnetcom_GO6470;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==GNNETCOM_VENDOR_ID
						 &&productID==GNNETCOM_PRODUCT_ID_GO9470)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &gnetcom_GO9470;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==GNNETCOM_VENDOR_ID
						 &&productID==GNNETCOM_PRODUCT_ID_LINK280)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &gnetcom_LINK280;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==GNNETCOM_VENDOR_ID
						 &&productID==GNNETCOM_PRODUCT_ID_LINK350OC)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &gnetcom_LINK350OC;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==GNNETCOM_VENDOR_ID
						 &&productID==GNNETCOM_PRODUCT_ID_BIZ620USB)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &gnetcom_BIZ620USB;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==GNNETCOM_VENDOR_ID
						 &&productID==GNNETCOM_PRODUCT_ID_BIZ2400USB)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &gnetcom_BIZ2400USB;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==GNNETCOM_VENDOR_ID
						 &&productID==GNNETCOM_PRODUCT_ID_GN8120)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &gnetcom_GN8120;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==GNNETCOM_VENDOR_ID
                 &&productID==GNNETCOM_PRODUCT_ID_DIAL520)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &gnetcom_DIAL520;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==GNNETCOM_VENDOR_ID
                 &&productID==GNNETCOM_PRODUCT_ID_UC250)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &gnetcom_UC250;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==GNNETCOM_VENDOR_ID
                 &&productID==GNNETCOM_PRODUCT_ID_UC550DUO)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &gnetcom_UC550DUO;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==GNNETCOM_VENDOR_ID
                 &&productID==GNNETCOM_PRODUCT_ID_UC550MONO)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &gnetcom_UC550MONO;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==GNNETCOM_VENDOR_ID
                 &&productID==GNNETCOM_PRODUCT_ID_UC150MONO)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &gnetcom_UC150MONO;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==GNNETCOM_VENDOR_ID
                 &&productID==GNNETCOM_PRODUCT_ID_UC150DUO)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &gnetcom_UC150DUO;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==GNNETCOM_VENDOR_ID
                 &&productID==GNNETCOM_PRODUCT_ID_BIZ2400MONOUSB)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &gnetcom_BIZ2400MONOUSB;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==GNNETCOM_VENDOR_ID
                 &&productID==GNNETCOM_PRODUCT_ID_PRO930)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &gnetcom_PRO930;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
				else if (vendorID==GNNETCOM_VENDOR_ID
                 &&productID==GNNETCOM_PRODUCT_ID_PRO9450)
				{
					ph->jdevice = &jdevice;
					ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
					ph->desc = &gnetcom_PRO9450;
					ph->jdevice->gElementsCFArrayRef = IOHIDDeviceCopyMatchingElements( ph->jdevice->gCurrentIOHIDDeviceRef, NULL, 0 );
					ph->jdevice->queueref=NULL;
					break;
				}
			}
      
      
			//long orgDevLocID = 0;
			//orgDevLocID = IOHIDDevice_GetLocationID( tIOHIDDeviceRef );
			//ph->jdevice->gCurrentIOHIDDeviceRef = tIOHIDDeviceRef;
		}
	}
	
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->desc==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	ph->jdevice->ringer = false;
	ph->jdevice->mute = false;
	ph->jdevice->audioenabled = true;
	
	if (ph->desc->hd_init!=NULL)
		ph->desc->hd_init(ph);

	hid_hooks_set_audioenabled(ph, false);

	return 0;
#else
	return HID_ERROR_NOTCOMPILED;
#endif
}

int hid_hooks_start_device(hid_hooks_t *ph, int vendor, int product)
{
	return _hid_hooks_start(ph, vendor, product);
}

int hid_hooks_start(hid_hooks_t *ph)
{
	return _hid_hooks_start(ph, 0, 0);
}

int hid_hooks_stop(hid_hooks_t *ph)
{
#if defined(WIN32) && defined(ENABLE_HID)
	if (ph->jdevice==NULL)
		return -1;

	if (ph->desc->hd_uninit!=NULL)
		ph->desc->hd_uninit(ph);

	ph->jdevice=NULL;
	CloseHidDevice(&jdevice.hid_dev);
	memset(&jdevice.hid_dev, 0, sizeof(HID_DEVICE));
	if (jdevice.hEvent!=NULL)
	{
		CloseHandle(jdevice.hEvent);
		jdevice.hEvent=NULL;
	}
	ph->desc=NULL;
	return 0;
	
#elif defined(__APPLE__) && defined(ENABLE_HID)
	if (ph->jdevice==NULL)
		return -1;

	if (ph->desc->hd_uninit!=NULL)
		ph->desc->hd_uninit(ph);

	if ( ph->jdevice->gElementsCFArrayRef ) {
		CFRelease( ph->jdevice->gElementsCFArrayRef );
		ph->jdevice->gElementsCFArrayRef = NULL;
	}
	
	if ( gElementCFArrayRef ) {
		CFRelease( gElementCFArrayRef );
		gElementCFArrayRef = NULL;
	}
	
	if ( gDeviceCFArrayRef ) {
		CFRelease( gDeviceCFArrayRef );
		gDeviceCFArrayRef = NULL;
	}
	
	if ( gIOHIDManagerRef ) {
		IOHIDManagerClose( gIOHIDManagerRef, 0 );
		gIOHIDManagerRef = NULL;
	}
	return 0;
#else
	return -1;
#endif
}

int hid_hooks_set_presence_indicator(hid_hooks_t *ph, int value)
{
#if defined(ENABLE_HID)
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->desc==NULL)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->desc->hd_set_presenceindicator!=NULL)
		return ph->desc->hd_set_presenceindicator(ph, value);
	return HID_ERROR_NOTIMPLEMENTED;
#else
	return -1;
#endif
}

int hid_hooks_set_ringer(hid_hooks_t *ph, int enable)
{
#if defined(ENABLE_HID)
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->desc==NULL)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->desc->hd_set_ringer!=NULL)
		return ph->desc->hd_set_ringer(ph, enable);
	return HID_ERROR_NOTIMPLEMENTED;
#else
	return -1;
#endif
}

int hid_hooks_set_audioenabled(hid_hooks_t *ph, int enable)
{
#if defined(ENABLE_HID)
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->desc==NULL)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->desc->hd_set_audioenabled!=NULL)
		return ph->desc->hd_set_audioenabled(ph, enable);
	return HID_ERROR_NOTIMPLEMENTED;
#else
	return -1;
#endif
}


int hid_hooks_set_mute(hid_hooks_t *ph, int enable)
{
#if defined(ENABLE_HID)
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->desc==NULL)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->desc->hd_set_mute!=NULL)
		return ph->desc->hd_set_mute(ph, enable);
	return HID_ERROR_NOTIMPLEMENTED;
#else
	return -1;
#endif
}

int hid_hooks_set_sendcalls(hid_hooks_t *ph, int enable)
{
#if defined(ENABLE_HID)
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->desc==NULL)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->desc->hd_set_sendcalls!=NULL)
		return ph->desc->hd_set_sendcalls(ph, enable);
	return HID_ERROR_NOTIMPLEMENTED;
#else
	return -1;
#endif
}

int hid_hooks_set_messagewaiting(hid_hooks_t *ph, int enable)
{
#if defined(ENABLE_HID)
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->desc==NULL)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->desc->hd_set_messagewaiting!=NULL)
		return ph->desc->hd_set_messagewaiting(ph, enable);
	return HID_ERROR_NOTIMPLEMENTED;
#else
	return -1;
#endif
}

int hid_hooks_get_mute(hid_hooks_t *ph)
{
#if defined(ENABLE_HID)
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->desc==NULL)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->desc->hd_get_mute!=NULL)
		return ph->desc->hd_get_mute(ph);
	return ph->jdevice->mute;
#else
	return HID_ERROR_NOTIMPLEMENTED;
#endif
}

int hid_hooks_get_audioenabled(hid_hooks_t *ph)
{
#if defined(ENABLE_HID)
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->desc==NULL)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->desc->hd_get_audioenabled!=NULL)
		return ph->desc->hd_get_audioenabled(ph);
	return ph->jdevice->audioenabled;
#else
	return HID_ERROR_NOTIMPLEMENTED;
#endif
}

int hid_hooks_get_events(hid_hooks_t *ph)
{
#if defined(ENABLE_HID)
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->desc==NULL)
		return HID_ERROR_NODEVICEFOUND;
	if (ph->desc->hd_get_events!=NULL)
		return ph->desc->hd_get_events(ph);
	return HID_ERROR_NOTIMPLEMENTED;
#else
	return HID_ERROR_NOTCOMPILED;
#endif
}
