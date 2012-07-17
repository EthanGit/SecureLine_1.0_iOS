
#include <amsiptools/hid_hooks.h>

#if defined(__APPLE__) && defined(ENABLE_HID)

#include "hid_devices.h"

#define POLYCOM_VENDOR_ID 0x095D
#define POLYCOM_PRODUCT_ID_COMMUNICATOR 0x0005

int current_presencecolor = 0x07;

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

static int cx100_set_color(hid_hooks_t *ph, int value)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	if (value<0)
		value=0x00;
	if (value>0x7F)
		value=0x7F;
	
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
				if (usagePage==0xffA1 && usage==0x0005)
				{
					uint64_t timestamp = 0; // create the IO HID Value to be sent to this LED element
					IOHIDValueRef tIOHIDValueRef = IOHIDValueCreateWithIntegerValue( kCFAllocatorDefault, tIOHIDElementRef, timestamp, 0x05 );
					if ( tIOHIDValueRef ) {
						IOReturn tIOReturn = IOHIDDeviceSetValue( ph->jdevice->gCurrentIOHIDDeviceRef, tIOHIDElementRef, tIOHIDValueRef );
						CFRelease( tIOHIDValueRef );
						if ( kIOReturnSuccess == tIOReturn) {

							IOHIDValueRef tIOHIDValueRef = IOHIDValueCreateWithIntegerValue( kCFAllocatorDefault, tIOHIDElementRef, timestamp, value );
							if ( tIOHIDValueRef ) {
								IOReturn tIOReturn = IOHIDDeviceSetValue( ph->jdevice->gCurrentIOHIDDeviceRef, tIOHIDElementRef, tIOHIDValueRef );
								CFRelease( tIOHIDValueRef );
								if ( kIOReturnSuccess == tIOReturn) {
									ms_message("cx100 -> Device: led color (0x%0,2X)", value);
									return 0;
								}
							}
						}
					}
					ms_error("cx100 -> Device: error setting led color (0x%0,2X)", value);
					return -1;
				}
			}
		}
	}
	
	return -1;
}

static int cx100_set_presenceindicator(hid_hooks_t *ph, int value)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	//this color will be used when audioenabled==false                                        
	current_presencecolor = value;
	if (ph->jdevice->audioenabled==0)
		cx100_set_color(ph, current_presencecolor);
	
	return -1;
}

static int cx100_set_sendcalls(hid_hooks_t *ph, int enable)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	return cx100_set_color(ph, 0x15);	
}

static int cx100_set_messagewaiting(hid_hooks_t *ph, int enable)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	return cx100_set_color(ph, 0x15);	
}

static int cx100_set_ringer(hid_hooks_t *ph, int enable)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	if (ph->jdevice->ringer==enable)
		return 0;
	
	cx100_set_color(ph, 0x14);
	if (enable)
		ms_message("cx100 -> Device: led ringer");
	else
		ms_message("cx100 -> Device: led ringer OFF");
	ph->jdevice->ringer = enable;
	
	return -1;
}

static int cx100_set_audioenabled(hid_hooks_t *ph, int enable)
{
	if (ph->jdevice==NULL)
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
	//ph->jdevice->mute = 0;
	cx100_set_presenceindicator(ph, current_presencecolor);
	
	return -1;
}


static int cx100_set_mute(hid_hooks_t *ph, int enable)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;
	
	if (ph->jdevice->mute==enable)
		return 0;
	
	ms_message("cx100 PC -> Device: led mute/unmute: not implemented");
	return -1;
}

static int cx100_get_mute(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return FALSE;
	return ph->jdevice->mute;
}

static int cx100_get_audioenabled(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return FALSE;
	return ph->jdevice->audioenabled;
}

static int cx100_get_isattached(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return FALSE;
	return ph->jdevice->isAttached;
}

static int cx100_get_events(hid_hooks_t *ph)
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
	ms_message("EVENT: usagepage = %x usageid = %x (logical=%i)", usagePage, usage, logical);
	
	if (logical==0x01)
	{
		ms_message("Polycom Communicator HID_EVENTS_VOLUP");
		if (ph->jdevice->mute==1)
			cx100_set_color(ph, 0x04);
		else if (ph->jdevice->audioenabled>0)
			cx100_set_color(ph, 0x06);
		else
			cx100_set_color(ph, current_presencecolor);
		return HID_EVENTS_VOLUP;
	}
	else if (logical==0x02)
	{
		ms_message("Polycom Communicator HID_EVENTS_VOLDOWN");
		if (ph->jdevice->mute==1)
			cx100_set_color(ph, 0x04);
		else if (ph->jdevice->audioenabled>0)
			cx100_set_color(ph, 0x06);
		else
			cx100_set_color(ph, current_presencecolor);
		return HID_EVENTS_VOLDOWN;
	}
	else if (logical==0x03)
	{
		ms_message("Polycom Communicator HID_EVENTS_SMART");
		if (ph->jdevice->mute==1)
			cx100_set_color(ph, 0x04);
		else if (ph->jdevice->audioenabled>0)
			cx100_set_color(ph, 0x06);
		else
			cx100_set_color(ph, current_presencecolor);
		return HID_EVENTS_SMART;
	}
	else if (logical==0x04)
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
			//ph->jdevice->mute=0;
			return HID_EVENTS_HANGUP;
		}
	}
	else if (logical==0x05)
	{
		if (ph->jdevice->mute==0)
		{
			ms_message("Polycom Communicator HID_EVENTS_MUTE");
			ph->jdevice->mute=1;
			cx100_set_color(ph, 0x04);
			if (ph->jdevice->audioenabled==0)
				cx100_set_color(ph, current_presencecolor);
			return HID_EVENTS_MUTE;
		}
		else
		{
			ms_message("Polycom Communicator HID_EVENTS_UNMUTE");
			ph->jdevice->mute=0;
			if (ph->jdevice->audioenabled>0)
				cx100_set_color(ph, 0x06);
			else
				cx100_set_color(ph, current_presencecolor);
			return HID_EVENTS_UNMUTE;
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

				uint32_t usagePage = IOHIDElementGetUsagePage( tIOHIDElementRef );
				uint32_t usage = IOHIDElementGetUsage( tIOHIDElementRef );
				if ( eleType != kIOHIDElementTypeInput_Misc ) {
					continue;	// skip non-input element types
				}
				ms_message("Add support for usagepage = %x usageid = %x", usagePage, usage);
				if (usagePage==0xffa1 && usage==0x0003)
				{
					IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				}
			}
		}
	}

	IOHIDQueueStart( inIOHIDQueueRef );
	
	ph->jdevice->queueref = inIOHIDQueueRef;
	
	cx100_set_color(ph, 0x04);
	return 0;
}

static int hd_uninit(hid_hooks_t *ph)
{
	
	IOHIDQueueStop( ph->jdevice->queueref );
	CFRelease(ph->jdevice->queueref);
	ph->jdevice->queueref=NULL;
	cx100_set_color(ph, 0x07);
	return 0;
}

struct hid_device_desc polycom_communicator = {
	POLYCOM_VENDOR_ID,
	POLYCOM_PRODUCT_ID_COMMUNICATOR,

	hd_init,
	hd_uninit,
	
	cx100_set_presenceindicator,
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
