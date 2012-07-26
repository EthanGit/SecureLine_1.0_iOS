#pragma once

// Import the PlantronicsDevice COM type library
#import "_PlantronicsDevice.tlb" no_namespace

// IDeviceEvents event sink helper
#include "DeviceEventSink.h"

// Declare event handler
class DeviceEventHandler : public DeviceEventSink
{
public:
	DeviceEventHandler();
	~DeviceEventHandler();

	void HookEvents(CComPtr<IUnknown> spUnkSource);
	void UnhookEvents();

protected:
	// Override event handlers
	virtual void OnAudioEnabledChanged(IDispatch* pSender, VARIANT_BOOL bAudioEnabled);
	virtual void OnTalkPressed(IDispatch* pSender);
	virtual void OnFlashPressed(IDispatch* pSender, LONG nButton);
	virtual void OnSmartPressed(IDispatch* pSender);
	virtual void OnButtonPressed(IDispatch* pSender, Button button);
	virtual void OnDeviceAttached(IDispatch* pSender);
	virtual void OnDeviceDetached(IDispatch* pSender);
	virtual void OnMuteChanged(IDispatch* pSender, VARIANT_BOOL bMute);

private:
	void Log( LPCTSTR format, ... );

};
