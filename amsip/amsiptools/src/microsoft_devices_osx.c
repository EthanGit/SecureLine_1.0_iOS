
#include <amsiptools/hid_hooks.h>

#if defined(__APPLE__) && defined(ENABLE_HID)

#include "hid_devices.h"

#define MICROSOFT_VENDOR_ID 0x045e
#define MICROSOFT_PRODUCT_ID_CATALINA 0xffca


static int catalina_set_presenceindicator(hid_hooks_t *ph, int value)
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
				if (usagePage==0xFF99 && usage==0xFF18)
				{
					uint64_t timestamp = 0; // create the IO HID Value to be sent to this LED element
					IOHIDValueRef tIOHIDValueRef = IOHIDValueCreateWithIntegerValue( kCFAllocatorDefault, tIOHIDElementRef, timestamp, value );
					if ( tIOHIDValueRef ) {
						IOReturn tIOReturn = IOHIDDeviceSetValue( ph->jdevice->gCurrentIOHIDDeviceRef, tIOHIDElementRef, tIOHIDValueRef );
						CFRelease( tIOHIDValueRef );
						if ( kIOReturnSuccess == tIOReturn) {
							ms_message("CATALINA -> Device: led presence indicator (0x%0,2X)", value);
							return 0;
						}
					}
				}
			}
		}
	}
	
	return -1;
}

static int catalina_set_sendcalls(hid_hooks_t *ph, int enable)
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
				if (usagePage==0x0008 && usage==0x0024)
				{
					uint64_t timestamp = 0; // create the IO HID Value to be sent to this LED element
					IOHIDValueRef tIOHIDValueRef = IOHIDValueCreateWithIntegerValue( kCFAllocatorDefault, tIOHIDElementRef, timestamp, enable );
					if ( tIOHIDValueRef ) {
						IOReturn tIOReturn = IOHIDDeviceSetValue( ph->jdevice->gCurrentIOHIDDeviceRef, tIOHIDElementRef, tIOHIDValueRef );
						CFRelease( tIOHIDValueRef );
						if ( kIOReturnSuccess == tIOReturn) {
							if (enable)
								ms_message("CATALINA PC -> Device: led send calls");
							else
								ms_message("CATALINA PC -> Device: led send calls OFF");
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

static int catalina_set_messagewaiting(hid_hooks_t *ph, int enable)
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
				if (usagePage==0x0008 && usage==0x0019)
				{
					uint64_t timestamp = 0; // create the IO HID Value to be sent to this LED element
					IOHIDValueRef tIOHIDValueRef = IOHIDValueCreateWithIntegerValue( kCFAllocatorDefault, tIOHIDElementRef, timestamp, enable );
					if ( tIOHIDValueRef ) {
						IOReturn tIOReturn = IOHIDDeviceSetValue( ph->jdevice->gCurrentIOHIDDeviceRef, tIOHIDElementRef, tIOHIDValueRef );
						CFRelease( tIOHIDValueRef );
						if ( kIOReturnSuccess == tIOReturn) {
							if (enable)
								ms_message("CATALINA PC -> Device: led message waiting");
							else
								ms_message("CATALINA PC -> Device: led message waiting OFF");
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

static int catalina_set_ringer(hid_hooks_t *ph, int enable)
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
				if (usagePage==0x0008 && usage==0x0018)
				{
					uint64_t timestamp = 0; // create the IO HID Value to be sent to this LED element
					IOHIDValueRef tIOHIDValueRef = IOHIDValueCreateWithIntegerValue( kCFAllocatorDefault, tIOHIDElementRef, timestamp, enable );
					if ( tIOHIDValueRef ) {
						IOReturn tIOReturn = IOHIDDeviceSetValue( ph->jdevice->gCurrentIOHIDDeviceRef, tIOHIDElementRef, tIOHIDValueRef );
						CFRelease( tIOHIDValueRef );
						if ( kIOReturnSuccess == tIOReturn) {
							if (enable)
								ms_message("CATALINA -> Device: led ringer");
							else
								ms_message("CATALINA -> Device: led ringer OFF");
							ph->jdevice->ringer=enable;
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

static int catalina_set_audioenabled(hid_hooks_t *ph, int enable)
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
				if (usagePage==0x0008 && usage==0x0017)
				{
					uint64_t timestamp = 0; // create the IO HID Value to be sent to this LED element
					IOHIDValueRef tIOHIDValueRef = IOHIDValueCreateWithIntegerValue( kCFAllocatorDefault, tIOHIDElementRef, timestamp, enable );
					if ( tIOHIDValueRef ) {
						IOReturn tIOReturn = IOHIDDeviceSetValue( ph->jdevice->gCurrentIOHIDDeviceRef, tIOHIDElementRef, tIOHIDValueRef );
						CFRelease( tIOHIDValueRef );
						if ( kIOReturnSuccess == tIOReturn) {
							ph->jdevice->audioenabled=enable;
							if (enable)
								ms_message("CATALINA PC -> Device: led audiolink & off hook/active call led");
							else
								ms_message("CATALINA PC -> Device: led audiolink & off hook/active call led OFF");
							return 0;
						}
					}
				}
			}
		}
	}
	
	return -1;
}


static int catalina_set_mute(hid_hooks_t *ph, int enable)
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
				if (usagePage==0x0008 && usage==0x0009)
				{
					uint64_t timestamp = 0; // create the IO HID Value to be sent to this LED element
					IOHIDValueRef tIOHIDValueRef = IOHIDValueCreateWithIntegerValue( kCFAllocatorDefault, tIOHIDElementRef, timestamp, enable );
					if ( tIOHIDValueRef ) {
						IOReturn tIOReturn = IOHIDDeviceSetValue( ph->jdevice->gCurrentIOHIDDeviceRef, tIOHIDElementRef, tIOHIDValueRef );
						CFRelease( tIOHIDValueRef );
						if ( kIOReturnSuccess == tIOReturn) {
							ph->jdevice->audioenabled=enable;
							if (enable)
								ms_message("CATALINA PC -> Device: led mute");
							else
								ms_message("CATALINA PC -> Device: led mute OFF");
							return 0;
						}
					}
				}
			}
		}
	}
	
	return -1;
}

static int catalina_get_mute(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return FALSE;
	return ph->jdevice->mute;
}

static int catalina_get_audioenabled(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return FALSE;
	return ph->jdevice->audioenabled;
}

static int catalina_get_isattached(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return FALSE;
	return ph->jdevice->isAttached;
}

static int catalina_get_events(hid_hooks_t *ph)
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
	if (usagePage==0x000B && usage==0x002F)
	{
		CFIndex logical = 0;
		logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
		if (logical==1)
		{
			//only show when button is pressed... not state of button???
			if (ph->jdevice->mute>0)
			{
				ph->jdevice->mute=0;
				return HID_EVENTS_UNMUTE;
			}else{
				ph->jdevice->mute=1;
				return HID_EVENTS_MUTE;
			}
		}
	}
	return hid_event;
	
#if 0
	//Check HOOK state
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
					CFIndex logical = 0;
					double_t physical = 0.0f;
					double_t calibrated = 0.0f;
					IOHIDValueRef tIOHIDValueRef;
					if ( kIOReturnSuccess == IOHIDDeviceGetValue( ph->jdevice->gCurrentIOHIDDeviceRef, tIOHIDElementRef, &tIOHIDValueRef ) ) {
						logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
						physical = IOHIDValueGetScaledValue( tIOHIDValueRef, kIOHIDValueScaleTypePhysical );
						calibrated = IOHIDValueGetScaledValue( tIOHIDValueRef, kIOHIDValueScaleTypeCalibrated );
					}
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
			}
		}
	}
	
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
				if (usagePage==0x000B && usage==0x002F)
				{
					CFIndex logical = 0;
					double_t physical = 0.0f;
					double_t calibrated = 0.0f;
					IOHIDValueRef tIOHIDValueRef;
					if ( kIOReturnSuccess == IOHIDDeviceGetValue( ph->jdevice->gCurrentIOHIDDeviceRef, tIOHIDElementRef, &tIOHIDValueRef ) ) {
						logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
						physical = IOHIDValueGetScaledValue( tIOHIDValueRef, kIOHIDValueScaleTypePhysical );
						calibrated = IOHIDValueGetScaledValue( tIOHIDValueRef, kIOHIDValueScaleTypeCalibrated );
					}
					if (logical==1)
					{
						//only show when button is pressed... not state of button???
						if (ph->jdevice->mute>0)
						{
							ph->jdevice->mute=0;
							return HID_EVENTS_UNMUTE;
						}else{
							ph->jdevice->mute=1;
							return HID_EVENTS_MUTE;
						}
					}
				}
			}
		}
	}

	return hid_event;
#endif
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
	
	catalina_set_presenceindicator(ph, 0x0F);
	return 0;
}

static int hd_uninit(hid_hooks_t *ph)
{
	
	IOHIDQueueStop( ph->jdevice->queueref );
	CFRelease(ph->jdevice->queueref);
	ph->jdevice->queueref=NULL;
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
