
#include <amsiptools/hid_hooks.h>

#if defined(__APPLE__) && defined(ENABLE_HID)

#include "hid_devices.h"

#define GNNETCOM_VENDOR_ID 0x0B0E
#define GNNETCOM_PRODUCT_ID_A330 0xA330
#define GNNETCOM_PRODUCT_ID_BIZ620USB 0x0930
#define GNNETCOM_PRODUCT_ID_GO6470 0x1003
#define GNNETCOM_PRODUCT_ID_GO9470 0x1041
#define GNNETCOM_PRODUCT_ID_LINK280 0x0910
#define GNNETCOM_PRODUCT_ID_LINK350OC 0xA340
#define GNNETCOM_PRODUCT_ID_BIZ2400USB 0x091C

#define GNNETCOM_PRODUCT_ID_M5390 0xA335
#define GNNETCOM_PRODUCT_ID_M5390USB 0xA338
#define GNNETCOM_PRODUCT_ID_BIZ2400 0x090A
#define GNNETCOM_PRODUCT_ID_GN9350 0x9350
#define GNNETCOM_PRODUCT_ID_GN9330 0x9330
#define GNNETCOM_PRODUCT_ID_GN8120 0x8120
#define GNNETCOM_PRODUCT_ID_DIAL520 0x0520

#define GNNETCOM_PRODUCT_ID_UC250 0x0341
#define GNNETCOM_PRODUCT_ID_UC550DUO 0x0030
#define GNNETCOM_PRODUCT_ID_UC550MONO 0x0031
#define GNNETCOM_PRODUCT_ID_UC150MONO 0x0043
#define GNNETCOM_PRODUCT_ID_UC150DUO 0x0041
#define GNNETCOM_PRODUCT_ID_BIZ2400MONOUSB 0x2400
#define GNNETCOM_PRODUCT_ID_PRO930 0x1016
#define GNNETCOM_PRODUCT_ID_PRO9450 0x1022

static int hid_setinputbutton(hid_hooks_t *ph, uint32_t _usagePage, uint32_t _usage, int enable)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	if ( ph->jdevice->gElementsCFArrayRef ) {
		CFIndex idx, cnt = CFArrayGetCount( ph->jdevice->gElementsCFArrayRef );
		for ( idx = 0; idx < cnt; idx++ ) {
			IOHIDElementRef tIOHIDElementRef = ( IOHIDElementRef ) CFArrayGetValueAtIndex( ph->jdevice->gElementsCFArrayRef, idx );
			if ( tIOHIDElementRef ) {
				IOHIDElementType eleType = IOHIDElementGetType( tIOHIDElementRef );	
				if ( eleType != kIOHIDElementTypeOutput ) {
					continue;	// skip non-output element types
				}
				
				uint32_t usagePage = IOHIDElementGetUsagePage( tIOHIDElementRef );
				uint32_t usage = IOHIDElementGetUsage( tIOHIDElementRef );
				if (usagePage==_usagePage && usage==_usage)
				{
					uint64_t timestamp = 0; // create the IO HID Value to be sent to this LED element
					IOHIDValueRef tIOHIDValueRef = IOHIDValueCreateWithIntegerValue( kCFAllocatorDefault, tIOHIDElementRef, timestamp, enable );
					if ( tIOHIDValueRef ) {
						IOReturn tIOReturn = IOHIDDeviceSetValue( ph->jdevice->gCurrentIOHIDDeviceRef, tIOHIDElementRef, tIOHIDValueRef );
						CFRelease( tIOHIDValueRef );
						if ( kIOReturnSuccess == tIOReturn) {
							ms_message("HID: setinputbutton: 0x%02x:0x%02x %s", usagePage, usage, enable?"ON":"OFF");
							return 0;
						}
					}
					return -1;
				}
			}
		}
	}
	
	return -1;
}

static int JABRA_set_ringer_0B_9E(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	ms_message("HID: JABRA_set_ringer_0B_9E %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0x000B, 0x009E, enable);
	if (i==0)
		ph->jdevice->ringer=enable;
	return i;
}

static int A330_set_audioenabled(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	ms_message("HID: A330: A330_set_audioenabled %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0x0008, 0x0017, enable);
	if (i==0)
		ph->jdevice->audioenabled=enable;
	return i;
}


static int A330_set_mute(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	ms_message("HID: A330: A330_set_mute %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0x0008, 0x0009, enable);
	if (i==0)
		ph->jdevice->mute=enable;
	return i;	
}

static int gnetcom_get_mute(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return 0;
	return ph->jdevice->mute;
}

static int gnetcom_get_audioenabled(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return 0;
	return ph->jdevice->audioenabled;
}

static int gnetcom_get_isattached(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return 0;
	return ph->jdevice->isAttached;
}

static int A330_get_events(hid_hooks_t *ph)
{
	int hid_event=-1;

	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	IOHIDValueRef tIOHIDValueRef = IOHIDQueueCopyNextValueWithTimeout( ph->jdevice->queueref, 0 );
	if (!tIOHIDValueRef)
		return hid_event;
	
	IOHIDElementRef tIOHIDElementRef = IOHIDValueGetElement(tIOHIDValueRef);
	uint32_t usagePage = IOHIDElementGetUsagePage( tIOHIDElementRef );
	uint32_t usage = IOHIDElementGetUsage( tIOHIDElementRef );
	if (usagePage==0x000B && usage==0x0020)
	{
		CFIndex logical = 0;
		logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
		if (ph->jdevice->audioenabled==0
			&& logical==1)
		{
			ms_message("A330 EVENT 0x0020 HID_EVENTS_HOOK");
			hid_hooks_set_audioenabled(ph, 1);
			return HID_EVENTS_HOOK;
		}
		if (ph->jdevice->audioenabled>0
			&& logical==0)
		{
			ms_message("A330 EVENT 0x0020 HID_EVENTS_HANGUP");
			hid_hooks_set_audioenabled(ph, 0);
			return HID_EVENTS_HANGUP;
		}
	}
	if (usagePage==0x000B && usage==0x002F)
	{
		CFIndex logical = 0;
		logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
		if (logical==1)
		{
			//only show when button is pressed... not state of button???
			if (ph->jdevice->mute>0)
			{
				ms_message("A330 EVENT 0x002F HID_EVENTS_UNMUTE");
				ph->jdevice->mute=0;
				return HID_EVENTS_UNMUTE;
			}else{
				ms_message("A330 EVENT 0x002F HID_EVENTS_MUTE");
				ph->jdevice->mute=1;
				return HID_EVENTS_MUTE;
			}
		}
	}
	if (usagePage==0x000B && usage==0x002A)
	{
		CFIndex logical = 0;
		logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
		if (logical==1)
		{
			ms_message("A330 EVENT 0x2A");
		}
	}
	if (usagePage==0x000B && usage==0x0097)
	{
		CFIndex logical = 0;
		logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
		if (logical==1)
		{
			ms_message("A330 EVENT 0x97");
		}
	}
	return hid_event;
}

static int A330_init(hid_hooks_t *ph)
{
	IOOptionBits inOptions = 0;
	IOHIDQueueRef inIOHIDQueueRef = IOHIDQueueCreate(kCFAllocatorDefault,
											  ph->jdevice->gCurrentIOHIDDeviceRef,
											  10, // the maximum number of values to queue
											  inOptions);

	if ( ph->jdevice->gElementsCFArrayRef ) {
		CFIndex idx, cnt = CFArrayGetCount( ph->jdevice->gElementsCFArrayRef );
		for ( idx = 0; idx < cnt; idx++ ) {
			IOHIDElementRef tIOHIDElementRef = ( IOHIDElementRef ) CFArrayGetValueAtIndex( ph->jdevice->gElementsCFArrayRef, idx );
			if ( tIOHIDElementRef ) {
				IOHIDElementType eleType = IOHIDElementGetType( tIOHIDElementRef );	
				if ( eleType != kIOHIDElementTypeInput_Button ) {
					continue;	// skip non-input element types
				}
				
				uint32_t usagePage = IOHIDElementGetUsagePage( tIOHIDElementRef );
				uint32_t usage = IOHIDElementGetUsage( tIOHIDElementRef );
				if (usagePage==0x000B && usage==0x0020)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0x000B && usage==0x002F)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0x000B && usage==0x002A)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0x000B && usage==0x0097)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
			}
		}
	}

	IOHIDQueueStart( inIOHIDQueueRef );
	
	ph->jdevice->queueref = inIOHIDQueueRef;
	
	return 0;
}

static int gnetcom_uninit(hid_hooks_t *ph)
{
	IOHIDQueueStop( ph->jdevice->queueref );
	CFRelease(ph->jdevice->queueref);
	ph->jdevice->queueref=NULL;
	return 0;
}

struct hid_device_desc gnetcom_A330 = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_A330,

	A330_init,
	gnetcom_uninit,
	
	NULL,
	JABRA_set_ringer_0B_9E,
	A330_set_audioenabled,
	A330_set_mute,
	NULL,
	NULL,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	A330_get_events,
};

static int BIZ620USB_set_audioenabled(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	ms_message("HID: BIZ620USB: BIZ620USB_set_audioenabled %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0x0008, 0x0017, enable);
	if (i==0)
		ph->jdevice->audioenabled=enable;
	return i;
}


static int BIZ620USB_set_mute(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	ms_message("HID: BIZ620USB: BIZ620USB_set_mute %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0x0008, 0x0009, enable);
	if (i==0)
		ph->jdevice->mute=enable;
	if (enable>0)
		ph->jdevice->mute=2;
	return i;	
}

static int BIZ620USB_get_events(hid_hooks_t *ph)
{
	int hid_event=-1;
	
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	IOHIDValueRef tIOHIDValueRef = IOHIDQueueCopyNextValueWithTimeout( ph->jdevice->queueref, 0 );
	if (!tIOHIDValueRef)
		return hid_event;
	
	IOHIDElementRef tIOHIDElementRef = IOHIDValueGetElement(tIOHIDValueRef);
	uint32_t usagePage = IOHIDElementGetUsagePage( tIOHIDElementRef );
	uint32_t usage = IOHIDElementGetUsage( tIOHIDElementRef );
	if (usagePage==0x000B && usage==0x0020)
	{
		CFIndex logical = 0;
		logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
		if (ph->jdevice->audioenabled==0
			&& logical==1)
		{
			ms_message("BIZ620USB EVENT 0x0020 HID_EVENTS_HOOK");
			hid_hooks_set_audioenabled(ph, 1);
			return HID_EVENTS_HOOK;
		}
		if (ph->jdevice->audioenabled>0
			&& logical==0)
		{
			ms_message("BIZ620USB EVENT 0x0020 HID_EVENTS_HANGUP");
			hid_hooks_set_audioenabled(ph, 0);
			return HID_EVENTS_HANGUP;
		}
	}
	if (usagePage==0x000B && usage==0x002F)
	{
		CFIndex logical = 0;
		logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
		if (logical==0 && ph->jdevice->mute>0)
		{
			ms_message("BIZ620USB EVENT 0x002F HID_EVENTS_UNMUTE");
			ph->jdevice->mute=0;
			return HID_EVENTS_UNMUTE;
		} else if (logical==1 && ph->jdevice->mute==0)
		{
			
			ms_message("BIZ620USB EVENT 0x002F HID_EVENTS_MUTE");
			ph->jdevice->mute=1;
			return HID_EVENTS_MUTE;
		}
	}
	return hid_event;
}

static int BIZ620USB_init(hid_hooks_t *ph)
{
	IOOptionBits inOptions = 0;
	IOHIDQueueRef inIOHIDQueueRef = IOHIDQueueCreate(kCFAllocatorDefault,
													 ph->jdevice->gCurrentIOHIDDeviceRef,
													 10, // the maximum number of values to queue
													 inOptions);
	
	if ( ph->jdevice->gElementsCFArrayRef ) {
		CFIndex idx, cnt = CFArrayGetCount( ph->jdevice->gElementsCFArrayRef );
		for ( idx = 0; idx < cnt; idx++ ) {
			IOHIDElementRef tIOHIDElementRef = ( IOHIDElementRef ) CFArrayGetValueAtIndex( ph->jdevice->gElementsCFArrayRef, idx );
			if ( tIOHIDElementRef ) {
				IOHIDElementType eleType = IOHIDElementGetType( tIOHIDElementRef );	
				if ( eleType != kIOHIDElementTypeInput_Button ) {
					continue;	// skip non-input element types
				}
				
				uint32_t usagePage = IOHIDElementGetUsagePage( tIOHIDElementRef );
				uint32_t usage = IOHIDElementGetUsage( tIOHIDElementRef );
				if (usagePage==0x000B && usage==0x0020)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0x000B && usage==0x002F)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
			}
		}
	}
	
	IOHIDQueueStart( inIOHIDQueueRef );
	
	ph->jdevice->queueref = inIOHIDQueueRef;
	
	hid_hooks_set_mute(ph, 0);
	return 0;
}

//Working fine: no ringer.
struct hid_device_desc gnetcom_BIZ620USB = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_BIZ620USB,
	
	BIZ620USB_init,
	gnetcom_uninit,
	
	NULL,
	NULL,
	BIZ620USB_set_audioenabled,
	BIZ620USB_set_mute,
	NULL,
	NULL,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	BIZ620USB_get_events,
};

static int JABRA_set_ringer_08_18(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	ms_message("HID: JABRA_set_ringer_08_18 %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0x0008, 0x0018, enable);
	if (i==0)
		ph->jdevice->ringer=enable;
	return i;
}

static int GO6470_set_audioenabled(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	hid_hooks_set_ringer(ph, 0);
	
	ms_message("HID: GO6470: GO6470_set_audioenabled %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0x0008, 0x0017, enable);
	i=hid_setinputbutton(ph, 0x0008, 0x002A, enable);
	if (i==0)
		ph->jdevice->audioenabled=enable;
	return i;
}


static int GO6470_set_mute(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	ms_message("HID: GO6470: GO6470_set_mute %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0x0008, 0x0009, enable);
	if (i==0)
		ph->jdevice->mute=enable;
	return i;
}

static static int GO6470_init(hid_hooks_t *ph)
{
	IOOptionBits inOptions = 0;
	IOHIDQueueRef inIOHIDQueueRef = IOHIDQueueCreate(kCFAllocatorDefault,
													 ph->jdevice->gCurrentIOHIDDeviceRef,
													 10, // the maximum number of values to queue
													 inOptions);
	
	if ( ph->jdevice->gElementsCFArrayRef ) {
		CFIndex idx, cnt = CFArrayGetCount( ph->jdevice->gElementsCFArrayRef );
		for ( idx = 0; idx < cnt; idx++ ) {
			IOHIDElementRef tIOHIDElementRef = ( IOHIDElementRef ) CFArrayGetValueAtIndex( ph->jdevice->gElementsCFArrayRef, idx );
			if ( tIOHIDElementRef ) {
				IOHIDElementType eleType = IOHIDElementGetType( tIOHIDElementRef );	
				if ( eleType != kIOHIDElementTypeInput_Button ) {
					continue;	// skip non-input element types
				}
				
				uint32_t usagePage = IOHIDElementGetUsagePage( tIOHIDElementRef );
				uint32_t usage = IOHIDElementGetUsage( tIOHIDElementRef );
				if (usagePage==0x000B && usage==0x0020)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0x000B && usage==0x002F)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				//if (usagePage==0x000B && usage==0x002A)
				//{
				//	IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				//}
				if (usagePage==0x000B && usage==0x0097)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
			}
		}
	}
	
	IOHIDQueueStart( inIOHIDQueueRef );
	
	ph->jdevice->queueref = inIOHIDQueueRef;
	
	return 0;
}

static int GO6470_get_events(hid_hooks_t *ph)
{
	int hid_event=-1;
	
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	IOHIDValueRef tIOHIDValueRef = IOHIDQueueCopyNextValueWithTimeout( ph->jdevice->queueref, 0 );
	if (!tIOHIDValueRef)
		return hid_event;
	
	IOHIDElementRef tIOHIDElementRef = IOHIDValueGetElement(tIOHIDValueRef);
	uint32_t usagePage = IOHIDElementGetUsagePage( tIOHIDElementRef );
	uint32_t usage = IOHIDElementGetUsage( tIOHIDElementRef );

	if (usagePage==0x000B && usage==0x0020)
	{
		CFIndex logical = 0;
		logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
		if (ph->jdevice->audioenabled==0
			&& logical==1)
		{
			ms_message("GO6470 EVENT 0x0020 HID_EVENTS_HOOK");
			hid_hooks_set_audioenabled(ph, 1);
			return HID_EVENTS_HOOK;
		}
		if (ph->jdevice->audioenabled>0
			&& logical==0)
		{
			ms_message("GO6470 EVENT 0x0020 HID_EVENTS_HANGUP");
			hid_hooks_set_audioenabled(ph, 0);
			return HID_EVENTS_HANGUP;
		}
	}
	if (usagePage==0x000B && usage==0x002F)
	{
		CFIndex logical = 0;
		logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
		if (logical==1)
		{
			//only show when button is pressed... not state of button???
			if (ph->jdevice->mute>0)
			{
				ms_message("GO6470 EVENT 0x002F HID_EVENTS_UNMUTE");
				hid_hooks_set_mute(ph, 0);
				return HID_EVENTS_UNMUTE;
			}else{
				ms_message("GO6470 EVENT 0x002F HID_EVENTS_MUTE");
				hid_hooks_set_mute(ph, 1);
				return HID_EVENTS_MUTE;
			}
		}
	}
	if (usagePage==0x000B && usage==0x002A)
	{
		CFIndex logical = 0;
		logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
		if (logical==1)
		{
			ms_message("GO6470 EVENT 0x2A");
		}
	}
	if (usagePage==0x000B && usage==0x0097)
	{
		CFIndex logical = 0;
		logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
		if (logical==1)
		{
			ms_message("GO6470 EVENT 0x97");
		}
	}
	return hid_event;
}

struct hid_device_desc gnetcom_GO6470 = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_GO6470,

	GO6470_init,
	gnetcom_uninit,

	NULL,
	JABRA_set_ringer_08_18,
	GO6470_set_audioenabled,
	GO6470_set_mute,
	NULL,
	NULL,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	GO6470_get_events,
};

static int GO9470_set_audioenabled(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	if (ph->jdevice->audioenabled==0x2A)
	{
		ms_message("GO9470 on going HOOKSWITCH/pls, retry later!");
		return 0; /* skip! */
	}
	
	if (ph->jdevice->audioenabled==enable)
		return 0;
	
	hid_hooks_set_ringer(ph, 0);
	
	ms_message("HID: GO9470: GO6470_set_audioenabled %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0x0008, 0x0017, (enable>0)?1:0);
	i=hid_setinputbutton(ph, 0x0008, 0x002A, (enable>0)?1:0);
	if (i==0)
		ph->jdevice->audioenabled=enable;
	return i;
}

static int GO9470_get_events(hid_hooks_t *ph)
{
	int hid_event=-1;
	
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	IOHIDValueRef tIOHIDValueRef = IOHIDQueueCopyNextValueWithTimeout( ph->jdevice->queueref, 0 );
	if (!tIOHIDValueRef)
		return hid_event;
	
	IOHIDElementRef tIOHIDElementRef = IOHIDValueGetElement(tIOHIDValueRef);
	uint32_t usagePage = IOHIDElementGetUsagePage( tIOHIDElementRef );
	uint32_t usage = IOHIDElementGetUsage( tIOHIDElementRef );
	
	CFIndex logical = 0;
	logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
	ms_message("GO9470 ---> %02X:%02X %i", usagePage, usage, logical);
	if (usagePage==0x000B && usage==0x0020)
	{
		CFIndex logical = 0;
		logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
		if (ph->jdevice->audioenabled==0
			&& logical==1)
		{
			ms_message("GO9470 EVENT 0x0020 HID_EVENTS_HOOK");
			hid_hooks_set_audioenabled(ph, 0x2A);
			return HID_EVENTS_HOOK;
		}
		if (ph->jdevice->audioenabled>0
			&& logical==0)
		{
			if (ph->jdevice->audioenabled==0x2A)
			{
				ms_message("GO9470 EVENT 0x0020 skip event");
				return 0;
			}
			ms_message("GO9470 EVENT 0x0020 HID_EVENTS_HANGUP");
			hid_hooks_set_audioenabled(ph, 0);
			return HID_EVENTS_HANGUP;
		}
		if (ph->jdevice->audioenabled==0x2A && logical==1)
		{
			ms_message("GO9470 HOOK CONFIRMED");
			ph->jdevice->audioenabled=1;
			return 0;
		}
	}
	if (usagePage==0x000B && usage==0x002F)
	{
		CFIndex logical = 0;
		logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
		if (logical==1)
		{
			//only show when button is pressed... not state of button???
			if (ph->jdevice->mute>0)
			{
				ms_message("GO9470 EVENT 0x002F HID_EVENTS_UNMUTE");
				hid_hooks_set_mute(ph, 0);
				return HID_EVENTS_UNMUTE;
			}else{
				ms_message("GO9470 EVENT 0x002F HID_EVENTS_MUTE");
				hid_hooks_set_mute(ph, 1);
				return HID_EVENTS_MUTE;
			}
		}
	}
	if (usagePage==0x000B && usage==0x002A)
	{
		CFIndex logical = 0;
		logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
		if (logical==1)
		{
			if (ph->jdevice->audioenabled==0x2A)
			{
				ms_message("GO9470 HOOK CONFIRMED");
				ph->jdevice->audioenabled=1;
			}
		}
	}
	if (usagePage==0x000B && usage==0x0097)
	{
		CFIndex logical = 0;
		logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
		if (logical==1)
		{
			ms_message("GO9470 EVENT 0x97");
		}
	}
	return hid_event;
}

struct hid_device_desc gnetcom_GO9470 = {
GNNETCOM_VENDOR_ID,
GNNETCOM_PRODUCT_ID_GO9470,

GO6470_init,
gnetcom_uninit,

NULL,
JABRA_set_ringer_08_18,
GO9470_set_audioenabled,
GO6470_set_mute,
NULL,
NULL,
gnetcom_get_mute,
gnetcom_get_audioenabled,
gnetcom_get_isattached,
GO9470_get_events,
};

static int LINK280_set_audioenabled(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	if (ph->jdevice->audioenabled==enable)
		return 0;
	
	hid_hooks_set_ringer(ph, 0);
	
	ms_message("HID: LINK280: LINK280_set_audioenabled %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0x0008, 0x0017, (enable>0)?1:0);
	if (i==0)
		ph->jdevice->audioenabled=enable;
	return i;
}

static int LINK280_set_mute(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	ms_message("HID: LINK280: LINK280_set_mute %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0x0008, 0x0009, enable);
	if (i==0)
		ph->jdevice->mute=enable;
	return i;
}

static int LINK280_get_events(hid_hooks_t *ph)
{
	int hid_event=-1;
	
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	IOHIDValueRef tIOHIDValueRef = IOHIDQueueCopyNextValueWithTimeout( ph->jdevice->queueref, 0 );
	if (!tIOHIDValueRef)
		return hid_event;
	
	IOHIDElementRef tIOHIDElementRef = IOHIDValueGetElement(tIOHIDValueRef);
	uint32_t usagePage = IOHIDElementGetUsagePage( tIOHIDElementRef );
	uint32_t usage = IOHIDElementGetUsage( tIOHIDElementRef );
	
	CFIndex logical = 0;
	logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
	ms_message("LINK280 ---> %02X:%02X %i", usagePage, usage, logical);
	if (usagePage==0x000B && usage==0x0020)
	{
		CFIndex logical = 0;
		logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
		if (ph->jdevice->audioenabled==0
			&& logical==1)
		{
			ms_message("LINK280 EVENT 0x0020 HID_EVENTS_HOOK");
			hid_hooks_set_audioenabled(ph, 1);
			return HID_EVENTS_HOOK;
		}
		if (ph->jdevice->audioenabled>0
			&& logical==0)
		{
			ms_message("LINK280 EVENT 0x0020 HID_EVENTS_HANGUP");
			hid_hooks_set_audioenabled(ph, 0);
			return HID_EVENTS_HANGUP;
		}
	}
	if (usagePage==0x000B && usage==0x002F)
	{
		CFIndex logical = 0;
		logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
		if (logical==1)
		{
			//only show when button is pressed... not state of button???
			if (ph->jdevice->mute>0)
			{
				ms_message("LINK280 EVENT 0x002F HID_EVENTS_UNMUTE");
				hid_hooks_set_mute(ph, 0);
				return HID_EVENTS_UNMUTE;
			}else{
				ms_message("LINK280 EVENT 0x002F HID_EVENTS_MUTE");
				hid_hooks_set_mute(ph, 1);
				return HID_EVENTS_MUTE;
			}
		}
	}
	return hid_event;
}

static int LINK280_init(hid_hooks_t *ph)
{
	IOOptionBits inOptions = 0;
	IOHIDQueueRef inIOHIDQueueRef = IOHIDQueueCreate(kCFAllocatorDefault,
													 ph->jdevice->gCurrentIOHIDDeviceRef,
													 10, // the maximum number of values to queue
													 inOptions);
	
	if ( ph->jdevice->gElementsCFArrayRef ) {
		CFIndex idx, cnt = CFArrayGetCount( ph->jdevice->gElementsCFArrayRef );
		for ( idx = 0; idx < cnt; idx++ ) {
			IOHIDElementRef tIOHIDElementRef = ( IOHIDElementRef ) CFArrayGetValueAtIndex( ph->jdevice->gElementsCFArrayRef, idx );
			if ( tIOHIDElementRef ) {
				IOHIDElementType eleType = IOHIDElementGetType( tIOHIDElementRef );	
				if ( eleType != kIOHIDElementTypeInput_Button ) {
					continue;	// skip non-input element types
				}
				
				uint32_t usagePage = IOHIDElementGetUsagePage( tIOHIDElementRef );
				uint32_t usage = IOHIDElementGetUsage( tIOHIDElementRef );
				if (usagePage==0x000B && usage==0x0020)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0x000B && usage==0x002F)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
			}
		}
	}
	
	IOHIDQueueStart( inIOHIDQueueRef );
	
	ph->jdevice->queueref = inIOHIDQueueRef;
	
	return 0;
}

struct hid_device_desc gnetcom_LINK280 = {
GNNETCOM_VENDOR_ID,
GNNETCOM_PRODUCT_ID_LINK280,

LINK280_init,
gnetcom_uninit,

NULL,
JABRA_set_ringer_08_18,
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

LINK280_init,
gnetcom_uninit,

NULL,
JABRA_set_ringer_08_18,
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

LINK280_init,
gnetcom_uninit,

NULL,
JABRA_set_ringer_08_18,
LINK280_set_audioenabled,
LINK280_set_mute,
NULL,
NULL,
gnetcom_get_mute,
gnetcom_get_audioenabled,
gnetcom_get_isattached,
LINK280_get_events,
};

struct hid_device_desc gnetcom_UC250 = {
  GNNETCOM_VENDOR_ID,
  GNNETCOM_PRODUCT_ID_UC250,
  
  LINK280_init,
  gnetcom_uninit,
  
  NULL,
  JABRA_set_ringer_08_18, //ringer doesn't work??
  LINK280_set_audioenabled,
  LINK280_set_mute,
  NULL,
  NULL,
  gnetcom_get_mute,
  gnetcom_get_audioenabled,
  gnetcom_get_isattached,
  LINK280_get_events,
};

struct hid_device_desc gnetcom_UC550DUO = {
  GNNETCOM_VENDOR_ID,
  GNNETCOM_PRODUCT_ID_UC550DUO,
  
  LINK280_init,
  gnetcom_uninit,
  
  NULL,
  JABRA_set_ringer_08_18, //ringer doesn't work??
  LINK280_set_audioenabled,
  LINK280_set_mute,
  NULL,
  NULL,
  gnetcom_get_mute,
  gnetcom_get_audioenabled,
  gnetcom_get_isattached,
  LINK280_get_events,
};

struct hid_device_desc gnetcom_UC550MONO = {
  GNNETCOM_VENDOR_ID,
  GNNETCOM_PRODUCT_ID_UC550MONO,
  
  LINK280_init,
  gnetcom_uninit,
  
  NULL,
  JABRA_set_ringer_08_18, //ringer doesn't work??
  LINK280_set_audioenabled,
  LINK280_set_mute,
  NULL,
  NULL,
  gnetcom_get_mute,
  gnetcom_get_audioenabled,
  gnetcom_get_isattached,
  LINK280_get_events,
};

struct hid_device_desc gnetcom_UC150MONO = {
  GNNETCOM_VENDOR_ID,
  GNNETCOM_PRODUCT_ID_UC150MONO,
  
  LINK280_init,
  gnetcom_uninit,
  
  NULL,
  JABRA_set_ringer_08_18, //ringer doesn't work??
  LINK280_set_audioenabled,
  LINK280_set_mute,
  NULL,
  NULL,
  gnetcom_get_mute,
  gnetcom_get_audioenabled,
  gnetcom_get_isattached,
  LINK280_get_events,
};

struct hid_device_desc gnetcom_UC150DUO = {
  GNNETCOM_VENDOR_ID,
  GNNETCOM_PRODUCT_ID_UC150DUO,
  
  LINK280_init,
  gnetcom_uninit,
  
  NULL,
  JABRA_set_ringer_08_18, //ringer doesn't work??
  LINK280_set_audioenabled,
  LINK280_set_mute,
  NULL,
  NULL,
  gnetcom_get_mute,
  gnetcom_get_audioenabled,
  gnetcom_get_isattached,
  LINK280_get_events,
};

struct hid_device_desc gnetcom_BIZ2400MONOUSB = {
  GNNETCOM_VENDOR_ID,
  GNNETCOM_PRODUCT_ID_BIZ2400MONOUSB,
  
  LINK280_init,
  gnetcom_uninit,
  
  NULL,
  JABRA_set_ringer_08_18, //ringer doesn't work??
  LINK280_set_audioenabled,
  LINK280_set_mute,
  NULL,
  NULL,
  gnetcom_get_mute,
  gnetcom_get_audioenabled,
  gnetcom_get_isattached,
  LINK280_get_events,
};

struct hid_device_desc gnetcom_PRO930 = {
  GNNETCOM_VENDOR_ID,
  GNNETCOM_PRODUCT_ID_PRO930,
  
  LINK280_init,
  gnetcom_uninit,
  
  NULL,
  JABRA_set_ringer_08_18, //ringer doesn't work??
  LINK280_set_audioenabled,
  LINK280_set_mute,
  NULL,
  NULL,
  gnetcom_get_mute,
  gnetcom_get_audioenabled,
  gnetcom_get_isattached,
  LINK280_get_events,
};

struct hid_device_desc gnetcom_PRO9450 = {
  GNNETCOM_VENDOR_ID,
  GNNETCOM_PRODUCT_ID_PRO9450,
  
  LINK280_init,
  gnetcom_uninit,
  
  NULL,
  JABRA_set_ringer_08_18, //ringer doesn't work??
  LINK280_set_audioenabled,
  LINK280_set_mute,
  NULL,
  NULL,
  gnetcom_get_mute,
  gnetcom_get_audioenabled,
  gnetcom_get_isattached,
  LINK280_get_events,
};



static int GN8120_set_val(hid_hooks_t *ph, int val)
{
	if ( ph->jdevice->gElementsCFArrayRef ) {
		CFIndex idx, cnt = CFArrayGetCount( ph->jdevice->gElementsCFArrayRef );
		for ( idx = 0; idx < cnt; idx++ ) {
			IOHIDElementRef tIOHIDElementRef = ( IOHIDElementRef ) CFArrayGetValueAtIndex( ph->jdevice->gElementsCFArrayRef, idx );
			if ( tIOHIDElementRef ) {
				IOHIDElementType eleType = IOHIDElementGetType( tIOHIDElementRef );	
				if ( eleType != kIOHIDElementTypeOutput ) {
					continue;	// skip non-output element types
				}
				
				uint32_t usagePage = IOHIDElementGetUsagePage( tIOHIDElementRef );
				uint32_t usage = IOHIDElementGetUsage( tIOHIDElementRef );
				if (usagePage==0x0008 && usage==0xFFFFFFFF)
				{
					uint64_t timestamp = 0; // create the IO HID Value to be sent to this LED element
					const uint8_t *bytes;
					uint8_t bytes_copy[8];
					IOHIDValueRef tIOHIDValueRef;
					IOReturn tIOReturn = IOHIDDeviceGetValue( ph->jdevice->gCurrentIOHIDDeviceRef, tIOHIDElementRef, &tIOHIDValueRef );
					if ( kIOReturnSuccess != tIOReturn)
						break;
					bytes =	IOHIDValueGetBytePtr(tIOHIDValueRef);
					
					if (bytes==NULL)
					{
						break;
					}
					memcpy(bytes_copy, bytes, 8);
					
					bytes_copy[0] = val;
					tIOHIDValueRef = IOHIDValueCreateWithBytes( kCFAllocatorDefault, tIOHIDElementRef, timestamp, bytes_copy, 8 );
					if ( tIOHIDValueRef ) {
						tIOReturn = IOHIDDeviceSetValue( ph->jdevice->gCurrentIOHIDDeviceRef, tIOHIDElementRef, tIOHIDValueRef );
						CFRelease( tIOHIDValueRef );
						if ( kIOReturnSuccess == tIOReturn) {
							ms_message("HID: GN8120: GN8120_set_val: 0x%02x:0x%02x val=%02x", usagePage, usage, val);
							return 0;
						}
					}
					return -1;
				}
			}
		}
	}
	
	return -1;
}

static int GN8120_set_ringer(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	ms_message("HID: GN8120: GN8120_set_ringer %s", enable?"ON":"OFF");
	i=GN8120_set_val(ph, (enable>0)?0x24:0x23);
	if (i==0)
		ph->jdevice->ringer=enable;
	return i;
}

static int GN8120_set_audioenabled(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	if (ph->jdevice->audioenabled==enable)
		return 0;
	
	hid_hooks_set_ringer(ph, 0);

	ms_message("HID: GN8120: GN8120_set_audioenabled %s", enable?"ON":"OFF");
	i=GN8120_set_val(ph, (enable>0)?0x01:0x02);
	if (i==0)
		ph->jdevice->audioenabled=enable;
	return i;
}


static int GN8120_set_mute(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	ms_message("HID: GN8120: GN8120_set_mute %s", enable?"ON":"OFF");
	i=GN8120_set_val(ph, (enable>0)?0x09:0x0A);
	if (i==0)
		ph->jdevice->mute=enable;
	return i;
}

static int GN8120_get_events(hid_hooks_t *ph)
{
	int hid_event=-1;
	
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	IOHIDValueRef tIOHIDValueRef = IOHIDQueueCopyNextValueWithTimeout( ph->jdevice->queueref, 0 );
	if (!tIOHIDValueRef)
		return hid_event;
	
	IOHIDElementRef tIOHIDElementRef = IOHIDValueGetElement(tIOHIDValueRef);
	uint32_t usagePage = IOHIDElementGetUsagePage( tIOHIDElementRef );
	uint32_t usage = IOHIDElementGetUsage( tIOHIDElementRef );
	
	CFIndex logical = 0;
	logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
	ms_message("GN8120 ---> %02X:%02X %i", usagePage, usage, logical);
	if (usagePage==0x000B && usage==0x0020)
	{
		CFIndex logical = 0;
		logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
		if (ph->jdevice->audioenabled==0
			&& logical==1)
		{
			ms_message("GN8120 EVENT 0x0020 HID_EVENTS_HOOK");
			hid_hooks_set_audioenabled(ph, 1);
			return HID_EVENTS_HOOK;
		}
		if (ph->jdevice->audioenabled>0
			&& logical==0)
		{
			ms_message("GN8120 EVENT 0x0020 HID_EVENTS_HANGUP");
			hid_hooks_set_audioenabled(ph, 0);
			return HID_EVENTS_HANGUP;
		}
	}
	if (usagePage==0x000B && usage==0x002F)
	{
		CFIndex logical = 0;
		logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
		if (logical==1)
		{
			//only show when button is pressed... not state of button???
			if (ph->jdevice->mute>0)
			{
				ms_message("GN8120 EVENT 0x002F HID_EVENTS_UNMUTE");
				hid_hooks_set_mute(ph, 0);
				return HID_EVENTS_UNMUTE;
			}else{
				ms_message("GN8120 EVENT 0x002F HID_EVENTS_MUTE");
				hid_hooks_set_mute(ph, 1);
				return HID_EVENTS_MUTE;
			}
		}
	}
	return hid_event;
}

static int GN8120_init(hid_hooks_t *ph)
{
	IOOptionBits inOptions = 0;
	IOHIDQueueRef inIOHIDQueueRef = IOHIDQueueCreate(kCFAllocatorDefault,
													 ph->jdevice->gCurrentIOHIDDeviceRef,
													 10, // the maximum number of values to queue
													 inOptions);
	
	if ( ph->jdevice->gElementsCFArrayRef ) {
		CFIndex idx, cnt = CFArrayGetCount( ph->jdevice->gElementsCFArrayRef );
		for ( idx = 0; idx < cnt; idx++ ) {
			IOHIDElementRef tIOHIDElementRef = ( IOHIDElementRef ) CFArrayGetValueAtIndex( ph->jdevice->gElementsCFArrayRef, idx );
			if ( tIOHIDElementRef ) {
				IOHIDElementType eleType = IOHIDElementGetType( tIOHIDElementRef );	

				uint32_t usagePage = IOHIDElementGetUsagePage( tIOHIDElementRef );
				uint32_t usage = IOHIDElementGetUsage( tIOHIDElementRef );
				ms_message("GN8120 ---> %02X:%02X %i", usagePage, usage, eleType);
				HIDDumpElementInfo(tIOHIDElementRef);
				if ( eleType != kIOHIDElementTypeInput_Button ) {
					continue;	// skip non-input element types
				}
				
				ms_message("GN8120 ---> BUTTON: %02X:%02X %i", usagePage, usage);
				if (usagePage==0x000B)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0x000C)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
			}
		}
	}
	
	IOHIDQueueStart( inIOHIDQueueRef );
	
	ph->jdevice->queueref = inIOHIDQueueRef;
	
	return 0;
}

//TODO:
//GN8120 ringer led doesn't work?
//GN8120 buttons doesn't work as expected
// -> when hitting a button, it does not have the desired
//    effect, then if you click another one, it will have
//    the desired effect.
struct hid_device_desc gnetcom_GN8120 = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_GN8120,
	
	GN8120_init,
	gnetcom_uninit,
	
	NULL,
	GN8120_set_ringer,
	GN8120_set_audioenabled,
	GN8120_set_mute,
	NULL,
	NULL,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	GN8120_get_events,
};

static int DIAL520_set_audioenabled(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	if (ph->jdevice->audioenabled==enable)
		return 0;
	
	hid_hooks_set_ringer(ph, 0);
	
	ms_message("HID: DIAL520: DIAL520_set_audioenabled %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0x0008, 0x0017, (enable>0)?1:0);
	if (i==0)
		ph->jdevice->audioenabled=enable;
	return i;
}

static int DIAL520_set_mute(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	ms_message("HID: DIAL520: DIAL520_set_mute %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0x0008, 0x0009, enable);
	if (i==0)
		ph->jdevice->mute=enable;
	return i;
}

static int DIAL520_get_events(hid_hooks_t *ph)
{
	int hid_event=-1;
	
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	IOHIDValueRef tIOHIDValueRef = IOHIDQueueCopyNextValueWithTimeout( ph->jdevice->queueref, 0 );
	if (!tIOHIDValueRef)
		return hid_event;
	
	IOHIDElementRef tIOHIDElementRef = IOHIDValueGetElement(tIOHIDValueRef);
	uint32_t usagePage = IOHIDElementGetUsagePage( tIOHIDElementRef );
	uint32_t usage = IOHIDElementGetUsage( tIOHIDElementRef );
	
	CFIndex logical = 0;
	logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
	ms_message("DIAL520 ---> %02X:%02X %i", usagePage, usage, logical);
	if (usagePage==0x000B && usage==0x0020)
	{
		if (ph->jdevice->audioenabled==0
			&& logical==1)
		{
			ms_message("DIAL520 EVENT 0x0020 HID_EVENTS_HOOK");
			hid_hooks_set_audioenabled(ph, 1);
			return HID_EVENTS_HOOK;
		}
		if (ph->jdevice->audioenabled>0
			&& logical==0)
		{
			ms_message("DIAL520 EVENT 0x0020 HID_EVENTS_HANGUP");
			hid_hooks_set_audioenabled(ph, 0);
			return HID_EVENTS_HANGUP;
		}
	}
	if (usagePage==0x000B && usage==0x002F)
	{
		if (logical==1)
		{
			//only show when button is pressed... not state of button???
			if (ph->jdevice->mute>0)
			{
				ms_message("DIAL520 EVENT 0x002F HID_EVENTS_UNMUTE");
				hid_hooks_set_mute(ph, 0);
				return HID_EVENTS_UNMUTE;
			}else{
				ms_message("DIAL520 EVENT 0x002F HID_EVENTS_MUTE");
				hid_hooks_set_mute(ph, 1);
				return HID_EVENTS_MUTE;
			}
		}
	}
	if (usagePage==0x000B && usage==0x0021)
	{
		if (logical==1)
		{
			ms_message("DIAL520 EVENT HID_EVENTS_FLASHUP");
			return HID_EVENTS_FLASHUP;
		}
	}
	
	if (usagePage==0x000B && usage==0x00B1 && logical==1)
	{
		ms_message("DIAL520 EVENT HID_EVENTS_KEY0");
		return HID_EVENTS_KEY0;
	}
	if (usagePage==0x000B && usage==0x00B2 && logical==1)
	{
		ms_message("DIAL520 EVENT HID_EVENTS_KEY1");
		return HID_EVENTS_KEY1;
	}
	if (usagePage==0x000B && usage==0x00B3 && logical==1)
	{
		ms_message("DIAL520 EVENT HID_EVENTS_KEY2");
		return HID_EVENTS_KEY2;
	}
	if (usagePage==0x000B && usage==0x00B4 && logical==1)
	{
		ms_message("DIAL520 EVENT HID_EVENTS_KEY3");
		return HID_EVENTS_KEY3;
	}
	if (usagePage==0x000B && usage==0x00B5 && logical==1)
	{
		ms_message("DIAL520 EVENT HID_EVENTS_KEY4");
		return HID_EVENTS_KEY4;
	}
	if (usagePage==0x000B && usage==0x00B6 && logical==1)
	{
		ms_message("DIAL520 EVENT HID_EVENTS_KEY5");
		return HID_EVENTS_KEY5;
	}
	if (usagePage==0x000B && usage==0x00B7 && logical==1)
	{
		ms_message("DIAL520 EVENT HID_EVENTS_KEY6");
		return HID_EVENTS_KEY6;
	}
	if (usagePage==0x000B && usage==0x00B8 && logical==1)
	{
		ms_message("DIAL520 EVENT HID_EVENTS_KEY7");
		return HID_EVENTS_KEY7;
	}
	if (usagePage==0x000B && usage==0x00B9 && logical==1)
	{
		ms_message("DIAL520 EVENT HID_EVENTS_KEY8");
		return HID_EVENTS_KEY8;
	}
	if (usagePage==0x000B && usage==0x00BA && logical==1)
	{
		ms_message("DIAL520 EVENT HID_EVENTS_KEY9");
		return HID_EVENTS_KEY9;
	}
	if (usagePage==0x000B && usage==0x00BB && logical==1)
	{
		ms_message("DIAL520 EVENT HID_EVENTS_KEYSTAR");
		return HID_EVENTS_KEYSTAR;
	}
	//if (usagePage==0x000B && usage==0x00BC && logical==1)
	//{
	//	ms_message("DIAL520 EVENT HID_EVENTS_KEYPOUND");
	//	return HID_EVENTS_KEYPOUND;
	//}
	if (usagePage==0x000B && usage==0x0007 && logical==1)
	{
		ms_message("DIAL520 EVENT HID_EVENTS_KEYERASE");
		return HID_EVENTS_KEYERASE;
	}
	
	
	return hid_event;
}

static int DIAL520_init(hid_hooks_t *ph)
{
	IOOptionBits inOptions = 0;
	IOHIDQueueRef inIOHIDQueueRef = IOHIDQueueCreate(kCFAllocatorDefault,
													 ph->jdevice->gCurrentIOHIDDeviceRef,
													 10, // the maximum number of values to queue
													 inOptions);
	
	if ( ph->jdevice->gElementsCFArrayRef ) {
		CFIndex idx, cnt = CFArrayGetCount( ph->jdevice->gElementsCFArrayRef );
		for ( idx = 0; idx < cnt; idx++ ) {
			IOHIDElementRef tIOHIDElementRef = ( IOHIDElementRef ) CFArrayGetValueAtIndex( ph->jdevice->gElementsCFArrayRef, idx );
			if ( tIOHIDElementRef ) {
				IOHIDElementType eleType = IOHIDElementGetType( tIOHIDElementRef );	
				
				uint32_t usagePage = IOHIDElementGetUsagePage( tIOHIDElementRef );
				uint32_t usage = IOHIDElementGetUsage( tIOHIDElementRef );
				ms_message("DIAL520 ---> %02X:%02X %i", usagePage, usage, eleType);
				HIDDumpElementInfo(tIOHIDElementRef);
				if ( eleType != kIOHIDElementTypeInput_Button ) {
					continue;	// skip non-input element types
				}
				
				ms_message("DIAL520 ---> BUTTON: %02X:%02X %i", usagePage, usage);
				if (usagePage==0x000B && usage!=0xFFFFFFFF && usage!=0xB0)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0x0009)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
			}
		}
	}
	
	IOHIDQueueStart( inIOHIDQueueRef );
	
	ph->jdevice->queueref = inIOHIDQueueRef;
	
	return 0;
}

struct hid_device_desc gnetcom_DIAL520 = {
	GNNETCOM_VENDOR_ID,
	GNNETCOM_PRODUCT_ID_DIAL520,
	
	DIAL520_init,
	gnetcom_uninit,
	
	NULL,
	JABRA_set_ringer_08_18,
	DIAL520_set_audioenabled,
	DIAL520_set_mute,
	NULL,
	NULL,
	gnetcom_get_mute,
	gnetcom_get_audioenabled,
	gnetcom_get_isattached,
	DIAL520_get_events,
};

#endif
