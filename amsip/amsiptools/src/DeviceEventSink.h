/****************************************************************************
 * Copyright 2007 Plantronics, Inc.  All rights reserved.                   *
 * This code is confidential and proprietary information belonging          *
 * to Plantronics, Inc. and may not be copied, modified or distributed      *
 * without the express written consent of Plantronics, Inc.                 *
 *                                                                          *
   $Author$
   $Change$
   $DateTime$
   $Id$
 *                                                                          *
 ****************************************************************************/
#pragma once
#ifdef EVENTSINK_STATIC
#define EVENTSINK_API
#else
#ifdef EVENTSINK_EXPORTS
#define EVENTSINK_API __declspec(dllexport)
#else
#define EVENTSINK_API __declspec(dllimport)
#endif
#endif

class DeviceEventSinkImpl;
struct IUnknown;
struct IDispatch;
enum Button;

class EVENTSINK_API DeviceEventSink
{
public:
	virtual ~DeviceEventSink();

	// Returns true if sink is hooked to event source
	bool IsHooked() const;

	// Pass a pointer to an object supporting IDeviceEvents to hook
	void Hook(IUnknown *pUnkSender) throw();
	// Destructor is calling Unhook internally, so it is optional to call it
	void Unhook();

protected:
	// Must inherit!
	DeviceEventSink();

	// Override these methods to get event notifications
	virtual void OnAudioEnabledChanged(IDispatch* pSender, VARIANT_BOOL bAudioEnabled) {}
	virtual void OnTalkPressed(IDispatch* pSender) {}
	virtual void OnFlashPressed(IDispatch* pSender, LONG nButton) {}
	virtual void OnSmartPressed(IDispatch* pSender) {}
	virtual void OnButtonPressed(IDispatch* pSender, Button button) {}
	virtual void OnDeviceAttached(IDispatch* pSender) {}
	virtual void OnDeviceDetached(IDispatch* pSender) {}
	virtual void OnMuteChanged(IDispatch* pSender, VARIANT_BOOL bMute) {}

private:
	// not allowed
	DeviceEventSink(const DeviceEventSink& sink);
	DeviceEventSink& operator = (const DeviceEventSink& sink);

	DeviceEventSinkImpl* m_pImpl;
	friend class DeviceEventSinkImpl;
};


