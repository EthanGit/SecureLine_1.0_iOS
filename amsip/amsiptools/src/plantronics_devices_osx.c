
#include <amsiptools/hid_hooks.h>

#if defined(__APPLE__) && defined(ENABLE_HID)

#include "hid_devices.h"

#define PLANTRONICS_VENDOR_ID 0x47f
#define PLANTRONICS_PRODUCT_ID_CS60 0x410
#define PLANTRONICS_PRODUCT_ID_SAVIOFFICE 0x0411
#define PLANTRONICS_PRODUCT_ID_DA45 0xDA45
#define PLANTRONICS_PRODUCT_ID_BTADAPTER 0x4254

#define PLANTRONICS_PRODUCT_ID_C420_2 0xAA14
#define PLANTRONICS_PRODUCT_ID_A478USB 0xC011
#define PLANTRONICS_PRODUCT_ID_BT300 0x0415

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

static int hid_setinputfeature(hid_hooks_t *ph, uint32_t _usagePage, uint32_t _usage, int enable)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	if ( ph->jdevice->gElementsCFArrayRef ) {
		CFIndex idx, cnt = CFArrayGetCount( ph->jdevice->gElementsCFArrayRef );
		for ( idx = 0; idx < cnt; idx++ ) {
			IOHIDElementRef tIOHIDElementRef = ( IOHIDElementRef ) CFArrayGetValueAtIndex( ph->jdevice->gElementsCFArrayRef, idx );
			if ( tIOHIDElementRef ) {
				IOHIDElementType eleType = IOHIDElementGetType( tIOHIDElementRef );	
				if ( eleType != kIOHIDElementTypeFeature ) {
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
							ms_message("HID: setinputfeature: 0x%02x:0x%02x %s", usagePage, usage, enable?"ON":"OFF");
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

static int plantronics_set_sendcalls(hid_hooks_t *ph, int enable)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	return -1;
}

static int plantronics_set_messagewaiting(hid_hooks_t *ph, int enable)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	return -1;
}

static int plantronics_set_ringer(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	ms_message("HID: CS60: cs60_set_ringer %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0x000B, 0x009E, enable);
	if (i==0)
		ph->jdevice->ringer=enable;
	return i;	
}

static int cs60_set_audioenabled(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	ms_message("HID: CS60: cs60_set_audioenabled %s", enable?"ON":"OFF");
	i=hid_setinputfeature(ph, 0xFFA0, 0x00A1, enable);
	if (i==0)
		ph->jdevice->audioenabled=enable;
	return i;
}


static int plantronics_set_mute(hid_hooks_t *ph, int enable)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	return -1;
}

static int plantronics_get_mute(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return FALSE;
	return ph->jdevice->mute;
}

static int plantronics_get_audioenabled(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return FALSE;
	return ph->jdevice->audioenabled;
}

static int plantronics_get_isattached(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return FALSE;
	return ph->jdevice->isAttached;
}

static int cs60_get_events(hid_hooks_t *ph)
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
	ms_message("CS60-USB EVENT 0x%04X:0x%04X %i", usagePage, usage, logical);
	if (usagePage==0xFFA0 && usage==0x00A1)
	{
		if (ph->jdevice->audioenabled==0
			&& logical==1)
		{
			ph->jdevice->audioenabled=1;
			return HID_EVENTS_HOOK;
		}
		if (ph->jdevice->audioenabled>0
			&& logical==0)
		{
			ph->jdevice->audioenabled=0;
			return HID_EVENTS_HANGUP;
		}
	}
	if (usagePage==0xFFA0 && usage==0x00B7)
	{
		if (logical==1)
		{
			if (ph->jdevice->audioenabled<=0)
				return 0; /* already hanged */
			ms_message("CS60-USB EVENT HID_EVENTS_TALK");
			return HID_EVENTS_TALK;
		}
	}
	if (usagePage==0xFFA0 && usage==0x00B6)
	{
		if (logical==1)
		{
			ms_message("CS60-USB EVENT SMART (LONG MUTE)");
			return HID_EVENTS_SMART;
		}
	}
	if (usagePage==0xFFA0 && usage==0x00B5)
	{
		if (logical==1)
		{
			if (ph->jdevice->audioenabled<=0)
				return 0; /* already hanged */
			if (ph->jdevice->mute>0)
			{
				ms_message("CS60-USB EVENT UNMUTE");
				ph->jdevice->mute=0;
				return HID_EVENTS_UNMUTE;
			}else{
				ms_message("CS60-USB EVENT MUTE");
				ph->jdevice->mute=1;
				return HID_EVENTS_MUTE;
			}
		}
	}
	if (usagePage==0xFFA0 && usage==0x00B4)
	{
		if (logical==1)
		{
			ms_message("CS60-USB EVENT FLASH (LONG VOL DOWN)");
			return HID_EVENTS_FLASHDOWN;
		}
	}
	if (usagePage==0xFFA0 && usage==0x00B3)
	{
		if (logical==1)
		{
			ms_message("CS60-USB EVENT FLASH (LONG VOL UP)");
			return HID_EVENTS_FLASHUP;
		}
	}
	if (usagePage==0xFFA0 && usage==0x00B2)
	{
		if (logical==1)
		{
			ms_message("CS60-USB EVENT VOLUME UP/DOWN");
			return HID_EVENTS_VOLUP;
		}
	}
	if (usagePage==0xFFA0 && usage==0x00B1)
	{
		if (logical==1)
		{
			ms_message("CS60-USB EVENT VOLUME UP/DOWN");
			return HID_EVENTS_VOLDOWN;
		}
	}
	return hid_event;
}

static int hd_init(hid_hooks_t *ph)
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
				ms_message("HID listening to: %4x:%4x", usagePage, usage);

				if ( eleType != kIOHIDElementTypeInput_Button ) {
					continue;	// skip non-input element types
				}

				ms_message("HID button to: %4x:%4x", usagePage, usage);
        
				if (usagePage==0xFFA0 && usage==0x00A1)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0xFFA0 && usage==0x00B7)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0xFFA0 && usage==0x00B6)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0xFFA0 && usage==0x00B5)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0xFFA0 && usage==0x00B4)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0xFFA0 && usage==0x00B3)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0xFFA0 && usage==0x00B2)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0xFFA0 && usage==0x00B1)
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

static int hd_uninit(hid_hooks_t *ph)
{
	IOHIDQueueStop( ph->jdevice->queueref );
	CFRelease(ph->jdevice->queueref);
	ph->jdevice->queueref=NULL;
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
	plantronics_set_ringer,
	cs60_set_audioenabled,
	plantronics_set_mute,
	plantronics_set_sendcalls,
	plantronics_set_messagewaiting,
	plantronics_get_mute,
	plantronics_get_audioenabled,
	plantronics_get_isattached,
	cs60_get_events,
};

struct hid_device_desc plantronics_cs60 = {
	PLANTRONICS_VENDOR_ID,
	PLANTRONICS_PRODUCT_ID_CS60,
  
	hd_init,
	hd_uninit,
  
	NULL,
	plantronics_set_ringer,
	cs60_set_audioenabled,
	plantronics_set_mute,
	plantronics_set_sendcalls,
	plantronics_set_messagewaiting,
	plantronics_get_mute,
	plantronics_get_audioenabled,
	plantronics_get_isattached,
	cs60_get_events,
};

static int A478USB_set_ringer(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
  if (ph->jdevice->ringer==enable)
    return 0;
  
	ms_message("HID: A478USB: A478USB_set_ringer %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0xFFA0, 0x009E, enable);
	if (i==0)
		ph->jdevice->ringer=enable;
	return i;	
}

static int A478USB_set_audioenabled(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
  if (ph->jdevice->audioenabled==enable)
    return 0;
  
	//i=hid_setinputbutton(ph, 0x0008, 0x0018, 0);
	//if (i==0)
	//	ph->jdevice->ringer=0;

 	ms_message("HID: A478USB: A478USB_set_audioenabled %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0xFFA0, 0x00DC, enable);
	if (i==0)
		ph->jdevice->audioenabled=enable;
	return i;
}

static int A478USB_get_events(hid_hooks_t *ph)
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
	ms_message("A478USB EVENT 0x%04X:0x%04X %i", usagePage, usage, logical);
	if (usagePage==0xFFA0 && usage==0x00AB)
	{
    ms_message("A478USB EVENT 0x00AB");
	}
	if (usagePage==0xFFA0 && usage==0x009E)
	{
    ms_message("A478USB EVENT 0x009E");
  }
	if (usagePage==0xFFA0 && usage==0x00B7)
	{
		if (ph->jdevice->audioenabled==0
        && logical==1)
		{
			hid_hooks_set_audioenabled(ph, 1);
			return HID_EVENTS_HOOK;
		}
		if (ph->jdevice->audioenabled>0
        && logical==1)
		{
			hid_hooks_set_audioenabled(ph, 0);
			return HID_EVENTS_HANGUP;
		}
	}
	if (usagePage==0xFFA0 && usage==0x00B5)
	{
    if (ph->jdevice->audioenabled<=0)
      return 0; /* already hanged */
		if (ph->jdevice->mute==0
        && logical==1)
		{
      ms_message("A478USB EVENT HID_EVENTS_MUTE");
      ph->jdevice->mute=1;
      return HID_EVENTS_MUTE;
		}
		if (ph->jdevice->mute>0
        && logical==0)
		{
      ms_message("A478USB EVENT HID_EVENTS_UNMUTE");
      ph->jdevice->mute=0;
      return HID_EVENTS_UNMUTE;
		}    
	}
	if (usagePage==0xFFA0 && usage==0x00B2)
	{
		if (logical==1)
		{
			ms_message("A478USB EVENT VOLUME UP/DOWN");
			return HID_EVENTS_VOLUP;
		}
	}
	if (usagePage==0xFFA0 && usage==0x00B1)
	{
		if (logical==1)
		{
			ms_message("A478USB EVENT VOLUME UP/DOWN");
			return HID_EVENTS_VOLDOWN;
		}
	}
	return hid_event;
}

static int A478USB_init(hid_hooks_t *ph)
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
				ms_message("HID listening to: %4x:%4x", usagePage, usage);
        
				if ( eleType != kIOHIDElementTypeInput_Button ) {
					continue;	// skip non-input element types
				}
        
				ms_message("HID button to: %4x:%4x", usagePage, usage);
        
				if (usagePage==0xFFA0 && usage==0x00AB)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0xFFA0 && usage==0x009E)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0xFFA0 && usage==0x00B7)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0xFFA0 && usage==0x00B5)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0xFFA0 && usage==0x00B2)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0xFFA0 && usage==0x00B1)
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

struct hid_device_desc plantronics_A478USB = {
	PLANTRONICS_VENDOR_ID,
	PLANTRONICS_PRODUCT_ID_A478USB,
  
	A478USB_init,
	hd_uninit,
  
	NULL,
	A478USB_set_ringer,
	A478USB_set_audioenabled,
	plantronics_set_mute,
	plantronics_set_sendcalls,
	plantronics_set_messagewaiting,
	plantronics_get_mute,
	plantronics_get_audioenabled,
	plantronics_get_isattached,
	A478USB_get_events,
};

static int savi_set_ringer(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
  
	ms_message("HID: SAVI: savi_set_ringer %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0x0008, 0x0018, enable);
	if (i==0)
		ph->jdevice->ringer=enable;
	return i;
}

static int savi_set_audioenabled(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	 
	ms_message("HID: SAVI: savi_set_audioenabled %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0x0008, 0x0017, enable);
	if (i==0)
		ph->jdevice->audioenabled=enable;
	return i;
}


static int savi_set_mute(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	ms_message("HID: SAVI: savi_set_mute %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0x0008, 0x0009, enable);
	if (i==0)
		ph->jdevice->mute=enable;
	return i;
}

static int savi_get_events(hid_hooks_t *ph)
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

	ms_message("SAVI EVENT %04X:%04X val=%i", usagePage, usage, logical);
	if (usagePage==0x000B && usage==0x0020)
	{
		if (ph->jdevice->audioenabled==0
			&& logical==1)
		{
			ms_message("SAVI EVENT HID_EVENTS_HOOK");
			hid_hooks_set_audioenabled(ph, 1);
			return HID_EVENTS_HOOK;
		}
		if (ph->jdevice->audioenabled>0
			&& logical==0)
		{
			ms_message("SAVI EVENT HID_EVENTS_HANGUP");
			hid_hooks_set_audioenabled(ph, 0);
			return HID_EVENTS_HANGUP;
		}
	}
	if (usagePage==0x000B && usage==0x002F)
	{
		if (logical==0 && ph->jdevice->mute>0)
		{
			ms_message("SAVI EVENT HID_EVENTS_UNMUTE");
			ph->jdevice->mute=0;
			return HID_EVENTS_UNMUTE;
		}
		if (logical==1 && ph->jdevice->mute==0)
		{
			ms_message("SAVI EVENT HID_EVENTS_MUTE");
			ph->jdevice->mute=1;
			return HID_EVENTS_MUTE;
		}
	}	
	if (usagePage==0xFFA0 && usage==0x00B2)
	{
		if (logical==1)
		{
			ms_message("SAVI EVENT HID_EVENTS_VOLUP");
			return HID_EVENTS_VOLUP;
		}
	}
	if (usagePage==0xFFA0 && usage==0x00B1)
	{
		if (logical==1)
		{
			ms_message("SAVI EVENT HID_EVENTS_VOLDOWN");
			return HID_EVENTS_VOLDOWN;
		}
	}

	
	return hid_event;
}

static int savi_init(hid_hooks_t *ph)
{
	IOOptionBits inOptions = 0;
	IOHIDQueueRef inIOHIDQueueRef = IOHIDQueueCreate(kCFAllocatorDefault,
													 ph->jdevice->gCurrentIOHIDDeviceRef,
													 20, // the maximum number of values to queue
													 inOptions);
	
	if ( ph->jdevice->gElementsCFArrayRef ) {
		CFIndex idx, cnt = CFArrayGetCount( ph->jdevice->gElementsCFArrayRef );
		for ( idx = 0; idx < cnt; idx++ ) {
			IOHIDElementRef tIOHIDElementRef = ( IOHIDElementRef ) CFArrayGetValueAtIndex( ph->jdevice->gElementsCFArrayRef, idx );
			if ( tIOHIDElementRef ) {
				IOHIDElementType eleType = IOHIDElementGetType( tIOHIDElementRef );	
				
				uint32_t usagePage = IOHIDElementGetUsagePage( tIOHIDElementRef );
				uint32_t usage = IOHIDElementGetUsage( tIOHIDElementRef );
				ms_message("HID listening to: %4x:%4x", usagePage, usage);
				
				if ( eleType != kIOHIDElementTypeInput_Button ) {
					continue;	// skip non-input element types
				}
				ms_message("HID button to: %4x:%4x", usagePage, usage);
				
				if (usagePage==0x000B)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0xFFA0 && usage==0x00B2)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0xFFA0 && usage==0x00B1)
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
//savi_set_audioenabled(0) -> doesn't clear the green led.
//                         -> button must be used to clear the green led.
struct hid_device_desc plantronics_savioffice = {
	PLANTRONICS_VENDOR_ID,
	PLANTRONICS_PRODUCT_ID_SAVIOFFICE,
	
	savi_init,
	hd_uninit,
	
	NULL,
	savi_set_ringer,
	savi_set_audioenabled,
	savi_set_mute,
	plantronics_set_sendcalls,
	plantronics_set_messagewaiting,
	plantronics_get_mute,
	plantronics_get_audioenabled,
	plantronics_get_isattached,
	savi_get_events,
};

static int da45_set_ringer(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	if (enable>0 && ph->jdevice->audioenabled>0)
	{
		ms_warning("disable audio first");
		hid_hooks_set_audioenabled(ph, 0);
	}
	
	ms_message("HID: DA45: da45_set_ringer %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0x0008, 0x001E, enable);
	if (i==0)
		ph->jdevice->ringer=enable;
	return i;
}

static int da45_set_audioenabled(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	if ((enable>0 && ph->jdevice->audioenabled>0)
      ||(enable==0 && ph->jdevice->audioenabled==0))
	{
    return 0; /* already done */
  }
  
	ms_message("HID: DA45: da45_set_audioenabled %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0x0008, 0x0017, enable);
	if (i==0)
		ph->jdevice->audioenabled=enable;
	return i;
}


static int da45_set_mute(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	ms_message("HID: DA45: da45_set_mute %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0x0008, 0x0009, enable);
	if (i==0)
		ph->jdevice->mute=enable;
	return i;
}

static int da45_get_events(hid_hooks_t *ph)
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
	
	ms_message("DA45 EVENT %04X:%04X val=%i", usagePage, usage, logical);
	if (usagePage==0x000B && usage==0x0020)
	{
		if (ph->jdevice->audioenabled==0
			&& logical==1)
		{
			ms_message("DA45 EVENT HID_EVENTS_HOOK");
			hid_hooks_set_audioenabled(ph, 1);
			return HID_EVENTS_HOOK;
		}
		if (ph->jdevice->audioenabled>0
			&& logical==0)
		{
			ms_message("DA45 EVENT HID_EVENTS_HANGUP");
			hid_hooks_set_audioenabled(ph, 0);
			return HID_EVENTS_HANGUP;
		}
	}
	if (usagePage==0x000B && usage==0x0021 && logical==1)
	{
		ms_message("DA45 EVENT HID_EVENTS_FLASHUP");
		return HID_EVENTS_FLASHUP;
	}
	if (usagePage==0x000B && usage==0x002F)
	{
		if (logical==0 && ph->jdevice->mute>0)
		{
			ms_message("DA45 EVENT HID_EVENTS_UNMUTE");
			ph->jdevice->mute=0;
			return HID_EVENTS_UNMUTE;
		}
		if (logical==1 && ph->jdevice->mute==0)
		{
			ms_message("DA45 EVENT HID_EVENTS_MUTE");
			ph->jdevice->mute=1;
			return HID_EVENTS_MUTE;
		}
	}	
	if (usagePage==0x000C && usage==0x00E9)
	{
		if (logical==1)
		{
			ms_message("DA45 EVENT HID_EVENTS_VOLUP");
			return HID_EVENTS_VOLUP;
		}
	}
	if (usagePage==0x000C && usage==0x00EA)
	{
		if (logical==1)
		{
			ms_message("DA45 EVENT HID_EVENTS_VOLDOWN");
			return HID_EVENTS_VOLDOWN;
		}
	}
	
	
	return hid_event;
}

static int da45_init(hid_hooks_t *ph)
{
	IOOptionBits inOptions = 0;
	IOHIDQueueRef inIOHIDQueueRef = IOHIDQueueCreate(kCFAllocatorDefault,
													 ph->jdevice->gCurrentIOHIDDeviceRef,
													 20, // the maximum number of values to queue
													 inOptions);
	
	if ( ph->jdevice->gElementsCFArrayRef ) {
		CFIndex idx, cnt = CFArrayGetCount( ph->jdevice->gElementsCFArrayRef );
		for ( idx = 0; idx < cnt; idx++ ) {
			IOHIDElementRef tIOHIDElementRef = ( IOHIDElementRef ) CFArrayGetValueAtIndex( ph->jdevice->gElementsCFArrayRef, idx );
			if ( tIOHIDElementRef ) {
				IOHIDElementType eleType = IOHIDElementGetType( tIOHIDElementRef );	
				
				uint32_t usagePage = IOHIDElementGetUsagePage( tIOHIDElementRef );
				uint32_t usage = IOHIDElementGetUsage( tIOHIDElementRef );
				ms_message("HID listening to: %4x:%4x", usagePage, usage);
				        
				if ( eleType != kIOHIDElementTypeInput_Button) {
					continue;	// skip non-input element types
				}
        
				if (usagePage==0x000B)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0x000C)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0x000B)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0xFFA0 && usage==0x00B2)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
				if (usagePage==0xFFA0 && usage==0x00B1)
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

struct hid_device_desc plantronics_da45 = {
	PLANTRONICS_VENDOR_ID,
	PLANTRONICS_PRODUCT_ID_DA45,
	
	da45_init,
	hd_uninit,
	
	NULL,
	da45_set_ringer,
	da45_set_audioenabled,
	da45_set_mute,
	plantronics_set_sendcalls,
	plantronics_set_messagewaiting,
	plantronics_get_mute,
	plantronics_get_audioenabled,
	plantronics_get_isattached,
	da45_get_events,
};

static int plantronics_set_ringer_08_18(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	if (enable>0 && ph->jdevice->audioenabled>0)
	{
		ms_warning("disable audio first");
		hid_hooks_set_audioenabled(ph, 0);
	}
	
	ms_message("HID: plantronics_set_ringer_08_18 %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0x0008, 0x0018, enable);
	if (i==0)
		ph->jdevice->ringer=enable;
	return i;
}

static int c420_2_set_audioenabled(hid_hooks_t *ph, int enable)
{
	int i;
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	if ((enable>0 && ph->jdevice->audioenabled>0)
      ||(enable==0 && ph->jdevice->audioenabled==0))
	{
    return 0; /* already done */
  }
  //disable ringer
  if (ph->jdevice->ringer>0) {
    i=hid_setinputbutton(ph, 0x0008, 0x0018, 0);
    if (i==0)
      ph->jdevice->ringer=0;
  }
  
	ms_message("HID: c420_2: c420_2_set_audioenabled %s", enable?"ON":"OFF");
	i=hid_setinputbutton(ph, 0x0008, 0x0017, enable);
	if (i==0)
		ph->jdevice->audioenabled=enable;
	return i;
}

static int c420_2_get_events(hid_hooks_t *ph)
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
  
	ms_message("c420_2 EVENT %04X:%04X val=%i", usagePage, usage, logical);
  
#if 0
  CFIndex length = IOHIDValueGetLength( tIOHIDValueRef );
  if (length!=1)
  {
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    memcpy(buf, IOHIDValueGetBytePtr(tIOHIDValueRef), length);
    ms_message("c420_2 EVENT receiving cellular call?");
  } else {
  }

	if (usagePage==0xFFA0 && usage==0x0033) {
    ms_message("c420_2 EVENT receiving cellular call?");
  }
	if (usagePage==0xFFA0 && usage==0x0089) {
    ms_message("c420_2 EVENT receiving cellular call?");
  }
  if (usagePage==0xFFA0 && usage==0x00F9) {
    ms_message("c420_2 EVENT receiving cellular call?");
  }
  if (usagePage==0xFFA0 && usage==0x00F8) {
    ms_message("c420_2 EVENT receiving cellular call?");
  }
  if (usagePage==0xFFA0 && usage==0x008A) {
    ms_message("c420_2 EVENT receiving cellular call?");
  }
  if (usagePage==0xFFA0 && usage==0x0089) {
    ms_message("c420_2 EVENT receiving cellular call?");
  }
#endif
  
	if (usagePage==0x000B && usage==0x0020)
	{
		if (ph->jdevice->audioenabled==0
        && logical==1)
		{
			ms_message("c420_2 EVENT HID_EVENTS_HOOK");
			hid_hooks_set_audioenabled(ph, 1);
			return HID_EVENTS_HOOK;
		}
		if (ph->jdevice->audioenabled>0
        && logical==0)
		{
			ms_message("c420_2 EVENT HID_EVENTS_HANGUP");
			hid_hooks_set_audioenabled(ph, 0);
			return HID_EVENTS_HANGUP;
		}
	}
	if (usagePage==0x000B && usage==0x0021 && logical==1)
	{
		ms_warning("c420_2 *****DISCARED***** EVENT HID_EVENTS_FLASHUP");
		//return HID_EVENTS_FLASHUP;
	}
	if (usagePage==0x000B && usage==0x002F)
	{
		if (logical==0 && ph->jdevice->mute>0)
		{
			ms_message("c420_2 EVENT HID_EVENTS_UNMUTE");
			ph->jdevice->mute=0;
			return HID_EVENTS_UNMUTE;
		}
		if (logical==1 && ph->jdevice->mute==0)
		{
			ms_message("c420_2 EVENT HID_EVENTS_MUTE");
			ph->jdevice->mute=1;
			return HID_EVENTS_MUTE;
		}
	}	
	if (usagePage==0x000C && usage==0x00E9)
	{
		if (logical==1)
		{
			ms_message("c420_2 EVENT HID_EVENTS_VOLUP");
			return HID_EVENTS_VOLUP;
		}
	}
	if (usagePage==0x000C && usage==0x00EA)
	{
		if (logical==1)
		{
			ms_message("c420_2 EVENT HID_EVENTS_VOLDOWN");
			return HID_EVENTS_VOLDOWN;
		}
	}
	
	return hid_event;
}

struct hid_device_desc plantronics_c420_2 = {
	PLANTRONICS_VENDOR_ID,
	PLANTRONICS_PRODUCT_ID_C420_2,
	
	da45_init,
	hd_uninit,
	
	NULL,
	plantronics_set_ringer_08_18,
	c420_2_set_audioenabled,
	da45_set_mute,
	plantronics_set_sendcalls,
	plantronics_set_messagewaiting,
	plantronics_get_mute,
	plantronics_get_audioenabled,
	plantronics_get_isattached,
	c420_2_get_events,
};

struct hid_device_desc plantronics_BT300 = {
	PLANTRONICS_VENDOR_ID,
	PLANTRONICS_PRODUCT_ID_BT300,
	
	da45_init,
	hd_uninit,
	
	NULL,
	plantronics_set_ringer_08_18,
	c420_2_set_audioenabled,
	da45_set_mute,
	plantronics_set_sendcalls,
	plantronics_set_messagewaiting,
	plantronics_get_mute,
	plantronics_get_audioenabled,
	plantronics_get_isattached,
	c420_2_get_events,
};

#endif
