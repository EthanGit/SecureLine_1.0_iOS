
#if defined(WIN32) && defined(ENABLE_PLANTRONICS)

#include "StdAfx.h"
#include "DeviceEventHandler.h"


DeviceEventHandler::DeviceEventHandler(void)
{
}

DeviceEventHandler::~DeviceEventHandler(void)
{
}

void DeviceEventHandler::HookEvents(CComPtr<IUnknown> spUnkSource)
{
	DeviceEventSink::Hook(spUnkSource);
}

void DeviceEventHandler::UnhookEvents()
{
	DeviceEventSink::Unhook();
}

void DeviceEventHandler::OnAudioEnabledChanged(IDispatch* pSender, VARIANT_BOOL bAudioEnabled)
{
	Log( _T("OnAudioEnabledChanged(bAudioEnabled=%s)"), 
		bAudioEnabled != VARIANT_FALSE ? _T("true") : _T("false") );
}

void DeviceEventHandler::OnTalkPressed(IDispatch* pSender)
{
	Log( _T("OnTalkPressed") );
}

void DeviceEventHandler::OnFlashPressed(IDispatch* pSender, LONG nButton)
{
	Log( _T("OnFlashPressed(nButton=%d)"), nButton );
}

void DeviceEventHandler::OnSmartPressed(IDispatch* pSender)
{
	Log( _T("OnSmartPressed") );
}

void DeviceEventHandler::OnButtonPressed(IDispatch* pSender, Button button)
{
	Log( _T("OnButtonPressed(button=%d)"), button );
}

void DeviceEventHandler::OnDeviceAttached(IDispatch* pSender)
{
	Log( _T("OnDeviceAttached") );
}

void DeviceEventHandler::OnDeviceDetached(IDispatch* pSender)
{
	Log( _T("OnDeviceDetached") );
}

void DeviceEventHandler::OnMuteChanged(IDispatch* pSender, VARIANT_BOOL bMute)
{
	Log( _T("OnMuteChanged(bMute=%s)"), 
		bMute != VARIANT_FALSE ? _T("true") : _T("false") );
}

void DeviceEventHandler::Log( LPCTSTR format, ... )
{
	va_list args;
	va_start(args, format);
	_vtprintf(format, args);
	_tprintf("\n");

/*
	#define BUF_SIZE 1024
	TCHAR buf[BUF_SIZE];

	va_list args;
	va_start(args, format);

	int nSize = _vsntprintf( buf, sizeof(buf), format, args);
*/
}

#endif
