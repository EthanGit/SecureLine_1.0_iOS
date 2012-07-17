#if defined(WIN32) && defined(ENABLE_PLANTRONICS)

#include "StdAfx.h"
#include "DeviceEventSink.h"

#include <assert.h>

#import "_PlantronicsDevice.tlb" embedded_idl, raw_interfaces_only, no_namespace

//
// DeviceEventSinkImpl
// 

[event_receiver(com)]
class DeviceEventSinkImpl
{
public:
	DeviceEventSinkImpl(DeviceEventSink* pFacade);

	bool IsHooked() const;

	void Hook(CComPtr<IUnknown> spUnkSource) throw();
	void Unhook();

private:
	// Override these methods to call event notifications
	HRESULT OnAudioEnabledChanged(IDispatch* pSender, VARIANT_BOOL bAudioEnabled);
	HRESULT OnTalkPressed(IDispatch* pSender);
	HRESULT OnFlashPressed(IDispatch* pSender, LONG nButton);
	HRESULT OnSmartPressed(IDispatch* pSender);
	HRESULT OnButtonPressed(IDispatch* pSender, Button button);
	HRESULT OnDeviceAttached(IDispatch* pSender);
	HRESULT OnDeviceDetached(IDispatch* pSender);
	HRESULT OnMuteChanged(IDispatch* pSender, VARIANT_BOOL bMute);

	CComPtr<IUnknown> m_spUnkSource;
	DeviceEventSink* m_pFacade;
};

DeviceEventSinkImpl::DeviceEventSinkImpl(DeviceEventSink* pFacade)
: m_pFacade(pFacade)
{
	assert(pFacade != NULL);
}

bool DeviceEventSinkImpl::IsHooked() const
{
	return (m_spUnkSource != NULL);
}

void DeviceEventSinkImpl::Hook(CComPtr<IUnknown> spUnkSource) throw()
{
	if( IsHooked() )
	{
		Unhook();
	}

	HRESULT hr = __hook(&IDeviceEvents::AudioEnabledChanged, spUnkSource, 
		&DeviceEventSinkImpl::OnAudioEnabledChanged);
	if( FAILED(hr) )
	{
		_com_raise_error(hr);
	}

	hr = __hook(&IDeviceEvents::TalkPressed, spUnkSource, 
		&DeviceEventSinkImpl::OnTalkPressed);
	if( FAILED(hr) )
	{
		_com_raise_error(hr);
	}

	hr = __hook(&IDeviceEvents::FlashPressed, spUnkSource, 
		&DeviceEventSinkImpl::OnFlashPressed);
	if( FAILED(hr) )
	{
		_com_raise_error(hr);
	}

	hr = __hook(&IDeviceEvents::SmartPressed, spUnkSource, 
		&DeviceEventSinkImpl::OnSmartPressed);
	if( FAILED(hr) )
	{
		_com_raise_error(hr);
	}

	hr = __hook(&IDeviceEvents::ButtonPressed, spUnkSource, 
		&DeviceEventSinkImpl::OnButtonPressed);
	if( FAILED(hr) )
	{
		_com_raise_error(hr);
	}

	hr = __hook(&IDeviceEvents::DeviceAttached, spUnkSource, 
		&DeviceEventSinkImpl::OnDeviceAttached);
	if( FAILED(hr) )
	{
		_com_raise_error(hr);
	}

	hr = __hook(&IDeviceEvents::DeviceDetached, spUnkSource, 
		&DeviceEventSinkImpl::OnDeviceDetached);
	if( FAILED(hr) )
	{
		_com_raise_error(hr);
	}

	hr = __hook(&IDeviceEvents::MuteChanged, spUnkSource, 
		&DeviceEventSinkImpl::OnMuteChanged);
	if( FAILED(hr) )
	{
		_com_raise_error(hr);
	}

	m_spUnkSource = spUnkSource;
}

void DeviceEventSinkImpl::Unhook()
{
	if( IsHooked() )
	{
		__unhook(&IDeviceEvents::AudioEnabledChanged, m_spUnkSource, &DeviceEventSinkImpl::OnAudioEnabledChanged);
		__unhook(&IDeviceEvents::TalkPressed, m_spUnkSource, &DeviceEventSinkImpl::OnTalkPressed);
		__unhook(&IDeviceEvents::FlashPressed, m_spUnkSource, &DeviceEventSinkImpl::OnFlashPressed);
		__unhook(&IDeviceEvents::SmartPressed, m_spUnkSource, &DeviceEventSinkImpl::OnSmartPressed);
		__unhook(&IDeviceEvents::ButtonPressed, m_spUnkSource, &DeviceEventSinkImpl::OnButtonPressed);
		__unhook(&IDeviceEvents::DeviceAttached, m_spUnkSource, &DeviceEventSinkImpl::OnDeviceAttached);
		__unhook(&IDeviceEvents::DeviceDetached, m_spUnkSource, &DeviceEventSinkImpl::OnDeviceDetached);
		__unhook(&IDeviceEvents::MuteChanged, m_spUnkSource, &DeviceEventSinkImpl::OnMuteChanged);

		m_spUnkSource.Release();
		assert(m_spUnkSource == NULL);
	}
}

HRESULT DeviceEventSinkImpl::OnAudioEnabledChanged( IDispatch* pSender, VARIANT_BOOL bAudioEnabled)
{
	m_pFacade->OnAudioEnabledChanged(pSender, bAudioEnabled);
	return S_OK;
}

HRESULT DeviceEventSinkImpl::OnTalkPressed( IDispatch* pSender )
{
	m_pFacade->OnTalkPressed(pSender);
	return S_OK;
}

HRESULT DeviceEventSinkImpl::OnFlashPressed( IDispatch* pSender, LONG nButton )
{
	m_pFacade->OnFlashPressed(pSender, nButton);
	return S_OK;
}

HRESULT DeviceEventSinkImpl::OnSmartPressed( IDispatch* pSender )
{
	m_pFacade->OnSmartPressed(pSender);
	return S_OK;
}

HRESULT DeviceEventSinkImpl::OnButtonPressed( IDispatch* pSender, Button button )
{
	m_pFacade->OnButtonPressed(pSender, button);
	return S_OK;
}

HRESULT DeviceEventSinkImpl::OnDeviceAttached( IDispatch* pSender )
{
	m_pFacade->OnDeviceAttached(pSender);
	return S_OK;
}

HRESULT DeviceEventSinkImpl::OnDeviceDetached( IDispatch* pSender )
{
	m_pFacade->OnDeviceDetached(pSender);
	return S_OK;
}

HRESULT DeviceEventSinkImpl::OnMuteChanged( IDispatch* pSender, VARIANT_BOOL bMute)
{
	m_pFacade->OnMuteChanged(pSender, bMute);
	return S_OK;
}


//
// DeviceEventSink
// 

DeviceEventSink::DeviceEventSink()
{
	m_pImpl = new DeviceEventSinkImpl(this);
	assert(m_pImpl != NULL);
}

DeviceEventSink::~DeviceEventSink()
{
	delete m_pImpl;
	m_pImpl = NULL;
}

void DeviceEventSink::Hook(IUnknown *pUnkSender) throw()
{
	assert(m_pImpl != NULL);
	m_pImpl->Hook(pUnkSender);
}

void DeviceEventSink::Unhook() throw()
{
	assert(m_pImpl != NULL);
	m_pImpl->Unhook();
}

#endif
