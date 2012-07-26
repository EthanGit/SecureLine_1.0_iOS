
#include <amsiptools/hid_hooks.h>

#if defined(__APPLE__) && defined(ENABLE_HID)

#include "hid_devices.h"

#define ZMM_VENDOR_ID 0x1994
#define ZMM_PRODUCT_ID_USBPHONE 0x2004

static int usbphone_set_audioenabled(hid_hooks_t *ph, int enable)
{
	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	ph->jdevice->audioenabled=enable;
	if (enable)
		ms_message("CS60-USB -> Device: led audio enabled");
	else
		ms_message("CS60-USB -> Device: led audio enabled OFF");
	
	return 0;
}

static int usbphone_get_audioenabled(hid_hooks_t *ph)
{
	if (ph->jdevice==NULL)
		return FALSE;
	return ph->jdevice->audioenabled;
}

static int usbphone_get_events(hid_hooks_t *ph)
{
	int hid_event=-1;

	if (ph->jdevice==NULL)
		return HID_ERROR_NODEVICEFOUND;

	IOHIDValueRef tIOHIDValueRef = IOHIDQueueCopyNextValueWithTimeout( ph->jdevice->queueref, 0 );
	if (!tIOHIDValueRef)
		return hid_event;
	
	CFIndex logical = 0;
	logical = IOHIDValueGetIntegerValue( tIOHIDValueRef );
	if (logical==2 && ph->jdevice->audioenabled==0)
	{
		ph->jdevice->audioenabled=1;
		return HID_EVENTS_HOOK;
	}
	if (logical==258 && ph->jdevice->audioenabled>0)
	{
		ph->jdevice->audioenabled=0;
		return HID_EVENTS_HANGUP;
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
				ms_message("HID: %x:%x type=%i", usagePage, usage, eleType);
				if ( eleType != kIOHIDElementTypeInput_Button ) {
					continue;	// skip non-input element types
				}
				
				ms_message("HID: %x:%x type=button", usagePage, usage, eleType);
				IOHIDQueueAddElement( inIOHIDQueueRef, tIOHIDElementRef );
				break;
			}
		}
	}

	IOHIDQueueStart( inIOHIDQueueRef );
	
	ph->jdevice->queueref = inIOHIDQueueRef;

	return 0;
}

static int hd_uninit(hid_hooks_t *ph)
{
	return 0;
}

struct hid_device_desc zmm_usbphone = {
	ZMM_VENDOR_ID,
	ZMM_PRODUCT_ID_USBPHONE,

	hd_init,
	hd_uninit,

	NULL,
	NULL,
	usbphone_set_audioenabled,
	NULL,
	NULL,
	NULL,
	NULL,
	usbphone_get_audioenabled,
	NULL,
	usbphone_get_events,
};

#endif
