
#include <amsiptools/plantronics_hooks.h>

//#define ENABLE_PLANTRONICS 1

#if defined(WIN32) && defined(ENABLE_PLANTRONICS)
#include "stdafx.h"
#include "DeviceEventHandler.h"

#include <mediastreamer2/mscommon.h>

[module(name="TestDevice")];

typedef CComGITPtr<IDevice> IDeviceGITPtr;

struct plantronics_device {
	IDevicePtr spDevice;
	IDeviceGITPtr spgitDevice;
	DeviceEventHandler eventHander;
};

static struct plantronics_device pdevice;
#endif


int plantronics_hooks_start(plantronics_hooks_t *ph)
{
#if defined(WIN32) && defined(ENABLE_PLANTRONICS)
	//CoInitialize(NULL);
	//AMD IDevicePtr spDevice;
	ph->pdevice = &pdevice;
	HRESULT hr = pdevice.spDevice.CreateInstance(__uuidof(CDevice));
	if (FAILED(hr))
	{
		ms_message("Plantronics Device driver not installed");
		ph->pdevice=NULL;
		return -1;
	}

	//AMD DeviceEventHandler eventHander;
	pdevice.eventHander.HookEvents((IUnknown*)pdevice.spDevice);
	if (FAILED(hr))
	{
		ph->pdevice=NULL;
		pdevice.spDevice.Release();
		return -1;
	}

	try {
		pdevice.spDevice->StartupPNP();
	}
	catch ( ... )
	{
		// Unsubscribe the events
		ph->pdevice->eventHander.UnhookEvents();
		ph->pdevice=NULL;
		pdevice.spDevice.Release();
		return -1;
	}
	return 0;
#else
	return -1;
#endif
}

int plantronics_hooks_stop(plantronics_hooks_t *ph)
{
#if defined(WIN32) && defined(ENABLE_PLANTRONICS)
	if (ph->pdevice==NULL)
		return -1;
	ph->pdevice->spDevice->Ringer = false;
	// Unsubscribe the events
	ph->pdevice->eventHander.UnhookEvents();
	// Shutdown the PNP listener and detach from the device
	ph->pdevice->spDevice->ShutdownPNP();

	pdevice.spDevice.Release();
	ph->pdevice=NULL;
	return 0;
#else
	return -1;
#endif
}

int plantronics_hooks_set_ringer(plantronics_hooks_t *ph, bool enable)
{
#if defined(WIN32) && defined(ENABLE_PLANTRONICS)
	if (ph->pdevice==NULL)
		return -1;
	ph->pdevice->spDevice->Ringer = enable;
	return 0;
#else
	return -1;
#endif
}

int plantronics_hooks_set_audioenabled(plantronics_hooks_t *ph, bool enable)
{
#if defined(WIN32) && defined(ENABLE_PLANTRONICS)
	if (ph->pdevice==NULL)
		return -1;
	ph->pdevice->spDevice->AudioEnabled = enable;
	return 0;
#else
	return -1;
#endif
}

bool plantronics_hooks_get_mute(plantronics_hooks_t *ph)
{
#if defined(WIN32) && defined(ENABLE_PLANTRONICS)
	if (ph->pdevice==NULL)
		return FALSE;
	if (ph->pdevice->spDevice->IsAttached == VARIANT_FALSE)
		return FALSE;
	return (ph->pdevice->spDevice->Mute != VARIANT_FALSE);
#else
	return false;
#endif
}

bool plantronics_hooks_get_audioenabled(plantronics_hooks_t *ph)
{
#if defined(WIN32) && defined(ENABLE_PLANTRONICS)
	if (ph->pdevice==NULL)
		return FALSE;
	if (ph->pdevice->spDevice->IsAttached == VARIANT_FALSE)
		return FALSE;
	return (ph->pdevice->spDevice->AudioEnabled != VARIANT_FALSE);
#else
	return false;
#endif
}

bool plantronics_hooks_get_isattached(plantronics_hooks_t *ph)
{
#if defined(WIN32) && defined(ENABLE_PLANTRONICS)
	if (ph->pdevice==NULL)
		return FALSE;
	return (ph->pdevice->spDevice->IsAttached != VARIANT_FALSE);
#else
	return false;
#endif
}

int plantronics_hooks_get_events(plantronics_hooks_t *ph)
{
#if defined(WIN32) && defined(ENABLE_PLANTRONICS)
	return 0;
#else
	return -1;
#endif
}
