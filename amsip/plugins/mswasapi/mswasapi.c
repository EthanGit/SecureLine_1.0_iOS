/** Copyright 2009 Americ Moizard, all rights reserved **/

#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msticker.h"

#include <mmsystem.h>
#ifdef _MSC_VER
#include <mmreg.h>
#endif
#include <msacm.h>

#include <mmreg.h>
#include <strsafe.h>
#include <MMDeviceApi.h> 
#include <avrt.h>
#include <audioclient.h>
#include <ksmedia.h>
#include <functiondiscoverykeys.h>
#include <propidl.h>
#include <initguid.h>
#include <delayimp.h>
#include <Audiopolicy.h>
#include <Endpointvolume.h>

#include <math.h>

#define MS2_AUDCLNT_SHAREMODE AUDCLNT_SHAREMODE_SHARED
#define MS2_AUDCLNT_STREAMFLAGS AUDCLNT_STREAMFLAGS_EVENTCALLBACK
/* #define MS2_AUDCLNT_SHAREMODE AUDCLNT_SHAREMODE_EXCLUSIVE */
/* #define MS2_AUDCLNT_STREAMFLAGS 0 */

#define wasapi_NBUFS 10
#define wasapi_OUT_NBUFS 20
#define wasapi_NSAMPLES 320
#define wasapi_MINIMUMBUFFER 5

static MSFilter *ms_wasapi_read_new(MSSndCard *card);
static MSFilter *ms_wasapi_write_new(MSSndCard *card);

typedef struct WasapiCard{
	char devname[256];
	char in_deviceid[256];
	char out_deviceid[256];
}WasapiCard;

#define SAFE_RELEASE(punk)  \
	if ((punk) != NULL)  \
{ (punk)->Release(); (punk) = NULL; }


class CMMNotificationClient : public IMMNotificationClient
{
	LONG _cRef;
	IMMDeviceEnumerator *_pEnumerator;

	// Private function to print device-friendly name
	IMMDevice *GetOutputDevice(LPCWSTR  pwstrId, char *szName, int szName_size, char *szDeviceid, int szDeviceid_size);
	IMMDevice *GetMicDevice(LPCWSTR  pwstrId, char *szName, int szName_size, char *szDeviceid, int szDeviceid_size);

public:
	CMMNotificationClient() :
	  _cRef(1),
		  _pEnumerator(NULL)
	  {
	  }

	  ~CMMNotificationClient()
	  {
		  SAFE_RELEASE(_pEnumerator)
	  }

	  // IUnknown methods -- AddRef, Release, and QueryInterface

	  ULONG STDMETHODCALLTYPE AddRef()
	  {
		  return InterlockedIncrement(&_cRef);
	  }

	  ULONG STDMETHODCALLTYPE Release()
	  {
		  ULONG ulRef = InterlockedDecrement(&_cRef);
		  if (0 == ulRef)
		  {
			  delete this;
		  }
		  return ulRef;
	  }

	  HRESULT STDMETHODCALLTYPE QueryInterface(
		  REFIID riid, VOID **ppvInterface)
	  {
		  if (IID_IUnknown == riid)
		  {
			  AddRef();
			  *ppvInterface = (IUnknown*)this;
		  }
		  else if (__uuidof(IMMNotificationClient) == riid)
		  {
			  AddRef();
			  *ppvInterface = (IMMNotificationClient*)this;
		  }
		  else
		  {
			  *ppvInterface = NULL;
			  return E_NOINTERFACE;
		  }
		  return S_OK;
	  }

	  // Callback methods for device-event notifications.

	  HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(
		  EDataFlow flow, ERole role,
		  LPCWSTR pwstrDeviceId);

	  HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId);

	  HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId);

	  HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(
		  LPCWSTR pwstrDeviceId,
		  DWORD dwNewState);

	  HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(
		  LPCWSTR pwstrDeviceId,
		  const PROPERTYKEY key);
};


static void WasapiCard_set_level(MSSndCard *card, MSSndCardMixerElem e, int percent){
	WasapiCard *d=(WasapiCard*)card->data;

	MMRESULT mr = MMSYSERR_NOERROR;
	DWORD dwVolume = 0xFFFF;
	dwVolume = ((0xFFFF) * percent) / 100;

	CoInitialize(NULL);
	switch(e){
		case MS_SND_CARD_MASTER:
			{
				IMMDeviceEnumerator*       enumerator      = NULL; 
				IMMDeviceCollection*       devicesList     = NULL; 
				IMMDevice *pOutputDevice;
				IAudioEndpointVolume *pAudioEndpointVolume;

				UINT                       count_out;
				HRESULT                    hr; 
				unsigned int item=0;

				hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), 
					NULL, 
					CLSCTX_ALL, 
					__uuidof(IMMDeviceEnumerator), 
					reinterpret_cast<void **>(&enumerator)); 
				hr = enumerator->EnumAudioEndpoints(eRender,DEVICE_STATE_ACTIVE, 
					&devicesList);
				hr = devicesList->GetCount(&count_out); 

				pOutputDevice=NULL;
				for (item = 0; item < count_out; item++) 
				{ 
					IPropertyStore *props = NULL; 
					hr = devicesList->Item(item, &pOutputDevice); 
					hr = pOutputDevice->OpenPropertyStore(STGM_READ, &props); 
					PROPVARIANT varName; 
					PropVariantInit(&varName); 
					hr = props->GetValue(PKEY_Device_FriendlyName, &varName); 
					char szName[256];
					WideCharToMultiByte(CP_UTF8,0,varName.pwszVal,-1,szName,256,0,0);
					PropVariantClear(&varName); 
					props->Release(); 
					if (strcmp(szName, card->name)==0)
						break;
					pOutputDevice->Release(); 
					pOutputDevice=NULL;
				} 

				if (devicesList)
					devicesList->Release();
				devicesList=NULL;

				if (enumerator)
					enumerator->Release();
				enumerator=NULL;

				if (pOutputDevice==NULL)
				{
					ms_error("WasapiCard_set_level: Cannot find WASAPI device.");
					CoUninitialize();
					return ;
				}

				hr = pOutputDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void **) &pAudioEndpointVolume);
				if (FAILED(hr)) {
					ms_error("WasapiCard_set_level: Cannot activate output.");
					pOutputDevice->Release();
					pOutputDevice=NULL;
					CoUninitialize();
					return;
				}

				hr = pAudioEndpointVolume->SetMasterVolumeLevelScalar((float)1.0*percent/(float)100.0, &GUID_NULL);
				if (FAILED(hr)) {
					ms_error("WasapiCard_set_level: Cannot set volume.");
					pAudioEndpointVolume->Release();
					pAudioEndpointVolume=NULL;
					pOutputDevice->Release();
					pOutputDevice=NULL;
					CoUninitialize();
					return;
				}
				
				//hr = pAudioEndpointVolume->SetMute(false, &GUID_NULL);
				//if (FAILED(hr)) {
				//	ms_error("WasapiCard_set_level: Cannot unmute.");
				//	pAudioEndpointVolume->Release();
				//	pAudioEndpointVolume=NULL;
				//	pOutputDevice->Release();
				//	pOutputDevice=NULL;
				//	CoUninitialize();
				//	return;
				//}
				pAudioEndpointVolume->Release();
				pAudioEndpointVolume=NULL;

				pOutputDevice->Release();
				pOutputDevice=NULL;
			}
			break;
		case MS_SND_CARD_PLAYBACK:
			{
				IMMDeviceEnumerator*       enumerator      = NULL; 
				IMMDeviceCollection*       devicesList     = NULL; 
				IMMDevice *pOutputDevice;
				IAudioSessionManager *pAudioSessionManager;
				ISimpleAudioVolume *pSimpleAudioVolume;

				UINT                       count_out;
				HRESULT                    hr; 
				unsigned int item=0;


				hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), 
					NULL, 
					CLSCTX_ALL, 
					__uuidof(IMMDeviceEnumerator), 
					reinterpret_cast<void **>(&enumerator)); 
				hr = enumerator->EnumAudioEndpoints(eRender,DEVICE_STATE_ACTIVE, 
					&devicesList);
				hr = devicesList->GetCount(&count_out); 

				pOutputDevice=NULL;
				for (item = 0; item < count_out; item++) 
				{ 
					IPropertyStore *props = NULL; 
					hr = devicesList->Item(item, &pOutputDevice); 
					hr = pOutputDevice->OpenPropertyStore(STGM_READ, &props); 
					PROPVARIANT varName; 
					PropVariantInit(&varName); 
					hr = props->GetValue(PKEY_Device_FriendlyName, &varName); 
					char szName[256];
					WideCharToMultiByte(CP_UTF8,0,varName.pwszVal,-1,szName,256,0,0);
					PropVariantClear(&varName); 
					props->Release(); 
					if (strcmp(szName, card->name)==0)
						break;
					pOutputDevice->Release(); 
					pOutputDevice=NULL;
				} 

				if (devicesList)
					devicesList->Release();
				devicesList=NULL;

				if (enumerator)
					enumerator->Release();
				enumerator=NULL;

				if (pOutputDevice==NULL)
				{
					ms_error("WasapiCard_set_level: Cannot find WASAPI device.");
					CoUninitialize();
					return ;
				}

				hr = pOutputDevice->Activate( __uuidof(IAudioSessionManager), 
					CLSCTX_INPROC_SERVER, 
					NULL, 
					(void**) &pAudioSessionManager);
				if (FAILED(hr)) {
					ms_error("WasapiCard_set_level: Cannot get IAudioSessionManager.");
					pOutputDevice->Release();
					pOutputDevice=NULL;				
					CoUninitialize();
					return;
				}

				hr = pAudioSessionManager->GetSimpleAudioVolume(
					&GUID_NULL, 0, &pSimpleAudioVolume);
				if (FAILED(hr)) {
					ms_error("WasapiCard_set_level: Cannot get ISimpleAudioVolume.");
					pAudioSessionManager->Release();
					pAudioSessionManager=NULL;
					pOutputDevice->Release();
					pOutputDevice=NULL;				
					CoUninitialize();
					return;
				}

				hr = pSimpleAudioVolume->SetMasterVolume((float)1.0*percent/(float)100.0, &GUID_NULL);
				if (FAILED(hr)) {
					ms_error("WasapiCard_set_level: Cannot set volume.");
					pSimpleAudioVolume->Release();
					pSimpleAudioVolume=NULL;
					pAudioSessionManager->Release();
					pAudioSessionManager=NULL;
					pOutputDevice->Release();
					pOutputDevice=NULL;				
					CoUninitialize();
					return;
				}

				//hr = pSimpleAudioVolume->SetMute(false, &GUID_NULL);
				//if (FAILED(hr)) {
				//	ms_error("WasapiCard_set_level: Cannot unmute.");
				//	pSimpleAudioVolume->Release();
				//	pSimpleAudioVolume=NULL;
				//	pAudioSessionManager->Release();
				//	pAudioSessionManager=NULL;
				//	pOutputDevice->Release();
				//	pOutputDevice=NULL;				
				//	CoUninitialize();
				//	return;
				//}

				pSimpleAudioVolume->Release();
				pSimpleAudioVolume=NULL;
				pAudioSessionManager->Release();
				pAudioSessionManager=NULL;
				pOutputDevice->Release();
				pOutputDevice=NULL;				
			}
			break;
		case MS_SND_CARD_CAPTURE:
			{
				IMMDeviceEnumerator*       enumerator      = NULL; 
				IMMDeviceCollection*       devicesList     = NULL; 
				IMMDevice *pMicDevice;
				IAudioEndpointVolume *pAudioEndpointVolume;
				UINT                       count_in; 
				HRESULT                    hr; 
				unsigned int item=0;

				hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), 
					NULL, 
					CLSCTX_ALL, 
					__uuidof(IMMDeviceEnumerator), 
					reinterpret_cast<void **>(&enumerator)); 
				hr = enumerator->EnumAudioEndpoints(eCapture,DEVICE_STATE_ACTIVE, 
					&devicesList);
				hr = devicesList->GetCount(&count_in); 

				pMicDevice=NULL;
				for (item = 0; item < count_in; item++) 
				{ 
					IPropertyStore *props = NULL; 
					hr = devicesList->Item(item, &pMicDevice); 
					hr = pMicDevice->OpenPropertyStore(STGM_READ, &props); 
					PROPVARIANT varName; 
					PropVariantInit(&varName); 
					hr = props->GetValue(PKEY_Device_FriendlyName, &varName); 
					char szName[256];
					WideCharToMultiByte(CP_UTF8,0,varName.pwszVal,-1,szName,256,0,0);
					PropVariantClear(&varName); 
					props->Release(); 
					if (strcmp(szName, card->name)==0)
						break;
					pMicDevice->Release(); 
					pMicDevice=NULL;
				} 

				if (devicesList)
					devicesList->Release();
				devicesList=NULL;

				if (enumerator)
					enumerator->Release();
				enumerator=NULL;

				if (pMicDevice==NULL)
				{
					ms_error("WasapiCard_set_level: Cannot find WASAPI device.");
					CoUninitialize();
					return;
				}

				hr = pMicDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void **) &pAudioEndpointVolume);
				if (FAILED(hr)) {
					ms_error("WasapiCard_set_level: Cannot activate mic.");
					pMicDevice->Release();
					pMicDevice=NULL;
					CoUninitialize();
					return;
				}
				hr = pAudioEndpointVolume->SetMasterVolumeLevelScalar((float)1.0*percent/(float)100.0, &GUID_NULL);
				if (FAILED(hr)) {
					ms_error("WasapiCard_set_level: Cannot set volume.");
					pAudioEndpointVolume->Release();
					pAudioEndpointVolume=NULL;
					pMicDevice->Release();
					pMicDevice=NULL;
					CoUninitialize();
					return;
				}

				//hr = pAudioEndpointVolume->SetMute(false, &GUID_NULL);
				//if (FAILED(hr)) {
				//	ms_error("WasapiCard_set_level: Cannot unmute.");
				//	pAudioEndpointVolume->Release();
				//	pAudioEndpointVolume=NULL;
				//	pMicDevice->Release();
				//	pMicDevice=NULL;
				//	CoUninitialize();
				//	return;
				//}

				pAudioEndpointVolume->Release();
				pAudioEndpointVolume=NULL;
				pMicDevice->Release();
				pMicDevice=NULL;

				break;
			}
		default:
			ms_warning("WasapiCard_set_level: unsupported command.");
	}
	CoUninitialize();
}

static int WasapiCard_get_level(MSSndCard *card, MSSndCardMixerElem e){
	WasapiCard *d=(WasapiCard*)card->data;

	MMRESULT mr = MMSYSERR_NOERROR;

	int percent = 100;
	float val;

	CoInitialize(NULL);
	switch(e){
		case MS_SND_CARD_MASTER:
			{
				IMMDeviceEnumerator*       enumerator      = NULL; 
				IMMDeviceCollection*       devicesList     = NULL; 
				IMMDevice *pOutputDevice;
				IAudioEndpointVolume *pAudioEndpointVolume;

				UINT                       count_out;
				HRESULT                    hr; 
				unsigned int item=0;

				hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), 
					NULL, 
					CLSCTX_ALL, 
					__uuidof(IMMDeviceEnumerator), 
					reinterpret_cast<void **>(&enumerator)); 
				hr = enumerator->EnumAudioEndpoints(eRender,DEVICE_STATE_ACTIVE, 
					&devicesList);
				hr = devicesList->GetCount(&count_out); 

				pOutputDevice=NULL;
				for (item = 0; item < count_out; item++) 
				{ 
					IPropertyStore *props = NULL; 
					hr = devicesList->Item(item, &pOutputDevice); 
					hr = pOutputDevice->OpenPropertyStore(STGM_READ, &props); 
					PROPVARIANT varName; 
					PropVariantInit(&varName); 
					hr = props->GetValue(PKEY_Device_FriendlyName, &varName); 
					char szName[256];
					WideCharToMultiByte(CP_UTF8,0,varName.pwszVal,-1,szName,256,0,0);
					PropVariantClear(&varName); 
					props->Release(); 
					if (strcmp(szName, card->name)==0)
						break;
					pOutputDevice->Release(); 
					pOutputDevice=NULL;
				} 

				if (devicesList)
					devicesList->Release();
				devicesList=NULL;

				if (enumerator)
					enumerator->Release();
				enumerator=NULL;

				if (pOutputDevice==NULL)
				{
					ms_error("WasapiCard_get_level: Cannot find WASAPI device.");
					CoUninitialize();
					return -1;
				}

				hr = pOutputDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void **) &pAudioEndpointVolume);
				if (FAILED(hr)) {
					ms_error("WasapiCard_get_level: Cannot activate output.");
					pOutputDevice->Release();
					pOutputDevice=NULL;
					CoUninitialize();
					return -1;
				}

				hr = pAudioEndpointVolume->GetMasterVolumeLevelScalar(&val);
				if (FAILED(hr)) {
					ms_error("WasapiCard_get_level: Cannot set volume.");
					pAudioEndpointVolume->Release();
					pAudioEndpointVolume=NULL;
					pOutputDevice->Release();
					pOutputDevice=NULL;				
					CoUninitialize();
					return -1;
				}
				percent = (int)(100.0*val);

				pAudioEndpointVolume->Release();
				pAudioEndpointVolume=NULL;

				pOutputDevice->Release();
				pOutputDevice=NULL;				
			}
			CoUninitialize();
			return percent;
		case MS_SND_CARD_PLAYBACK:
			{
				IMMDeviceEnumerator*       enumerator      = NULL; 
				IMMDeviceCollection*       devicesList     = NULL; 
				IMMDevice *pOutputDevice;
				IAudioSessionManager *pAudioSessionManager;
				ISimpleAudioVolume *pSimpleAudioVolume;

				UINT                       count_out;
				HRESULT                    hr; 
				unsigned int item=0;


				hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), 
					NULL, 
					CLSCTX_ALL, 
					__uuidof(IMMDeviceEnumerator), 
					reinterpret_cast<void **>(&enumerator)); 
				hr = enumerator->EnumAudioEndpoints(eRender,DEVICE_STATE_ACTIVE, 
					&devicesList);
				hr = devicesList->GetCount(&count_out); 

				pOutputDevice=NULL;
				for (item = 0; item < count_out; item++) 
				{ 
					IPropertyStore *props = NULL; 
					hr = devicesList->Item(item, &pOutputDevice); 
					hr = pOutputDevice->OpenPropertyStore(STGM_READ, &props); 
					PROPVARIANT varName; 
					PropVariantInit(&varName); 
					hr = props->GetValue(PKEY_Device_FriendlyName, &varName); 
					char szName[256];
					WideCharToMultiByte(CP_UTF8,0,varName.pwszVal,-1,szName,256,0,0);
					PropVariantClear(&varName); 
					props->Release(); 
					if (strcmp(szName, card->name)==0)
						break;
					pOutputDevice->Release(); 
					pOutputDevice=NULL;
				} 

				if (devicesList)
					devicesList->Release();
				devicesList=NULL;

				if (enumerator)
					enumerator->Release();
				enumerator=NULL;

				if (pOutputDevice==NULL)
				{
					ms_error("WasapiCard_get_level: Cannot find WASAPI device.");
					CoUninitialize();
					return -1;
				}

				hr = pOutputDevice->Activate( __uuidof(IAudioSessionManager), 
					CLSCTX_INPROC_SERVER, 
					NULL, 
					(void**) &pAudioSessionManager);
				if (FAILED(hr)) {
					ms_error("WasapiCard_get_level: Cannot get IAudioSessionManager.");
					pOutputDevice->Release();
					pOutputDevice=NULL;
					CoUninitialize();
					return -1;
				}

				hr = pAudioSessionManager->GetSimpleAudioVolume(
					&GUID_NULL, 0, &pSimpleAudioVolume);
				if (FAILED(hr)) {
					ms_error("WasapiCard_get_level: Cannot get ISimpleAudioVolume.");
					pAudioSessionManager->Release();
					pAudioSessionManager=NULL;
					pOutputDevice->Release();
					pOutputDevice=NULL;
					CoUninitialize();
					return -1;
				}

				hr = pSimpleAudioVolume->GetMasterVolume(&val);
				if (FAILED(hr)) {
					ms_error("WasapiCard_get_level: Cannot set volume.");
					pSimpleAudioVolume->Release();
					pSimpleAudioVolume=NULL;
					pAudioSessionManager->Release();
					pAudioSessionManager=NULL;
					pOutputDevice->Release();
					pOutputDevice=NULL;
					CoUninitialize();
					return -1;
				}
				percent = (int)(100.0*val);

				pSimpleAudioVolume->Release();
				pSimpleAudioVolume=NULL;
				pAudioSessionManager->Release();
				pAudioSessionManager=NULL;
				pOutputDevice->Release();
				pOutputDevice=NULL;
			}
			CoUninitialize();
			return percent;
		case MS_SND_CARD_CAPTURE:
			{
				IMMDeviceEnumerator*       enumerator      = NULL; 
				IMMDeviceCollection*       devicesList     = NULL; 
				IMMDevice *pMicDevice;
				IAudioEndpointVolume *pAudioEndpointVolume;
				UINT                       count_in; 
				HRESULT                    hr; 
				unsigned int item=0;

				hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), 
					NULL, 
					CLSCTX_ALL, 
					__uuidof(IMMDeviceEnumerator), 
					reinterpret_cast<void **>(&enumerator)); 
				hr = enumerator->EnumAudioEndpoints(eCapture,DEVICE_STATE_ACTIVE, 
					&devicesList);
				hr = devicesList->GetCount(&count_in); 

				pMicDevice=NULL;
				for (item = 0; item < count_in; item++) 
				{ 
					IPropertyStore *props = NULL; 
					hr = devicesList->Item(item, &pMicDevice); 
					hr = pMicDevice->OpenPropertyStore(STGM_READ, &props); 
					PROPVARIANT varName; 
					PropVariantInit(&varName); 
					hr = props->GetValue(PKEY_Device_FriendlyName, &varName); 
					char szName[256];
					WideCharToMultiByte(CP_UTF8,0,varName.pwszVal,-1,szName,256,0,0);
					PropVariantClear(&varName); 
					props->Release(); 
					if (strcmp(szName, card->name)==0)
						break;
					pMicDevice->Release(); 
					pMicDevice=NULL;
				} 

				if (devicesList)
					devicesList->Release();
				devicesList=NULL;

				if (enumerator)
					enumerator->Release();
				enumerator=NULL;

				if (pMicDevice==NULL)
				{
					ms_error("WasapiCard_get_level: Cannot find WASAPI device.");
					CoUninitialize();
					return -1;
				}

				hr = pMicDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void **) &pAudioEndpointVolume);
				if (FAILED(hr)) {
					ms_error("WasapiCard_get_level: Cannot activate mic.");
					pMicDevice->Release();
					pMicDevice=NULL;
					CoUninitialize();
					return -1;
				}
				hr = pAudioEndpointVolume->GetMasterVolumeLevelScalar(&val);
				if (FAILED(hr)) {
					ms_error("WasapiCard_get_level: Cannot set volume.");
					pAudioEndpointVolume->Release();
					pAudioEndpointVolume=NULL;
					pMicDevice->Release();
					pMicDevice=NULL;
					CoUninitialize();
					return -1;
				}
				percent = (int)(100.0*val);

				pAudioEndpointVolume->Release();
				pAudioEndpointVolume=NULL;
				pMicDevice->Release();
				pMicDevice=NULL;

			}
			CoUninitialize();
			return percent;
		default:
			ms_warning("WasapiCard_get_level: unsupported command.");
	}
	CoUninitialize();
	return -1;
}

static void WasapiCard_set_source(MSSndCard *card, MSSndCardCapture source){

	switch(source){
		case MS_SND_CARD_MIC:
			break;
		case MS_SND_CARD_LINE:
			break;
	}	
}

static int WasapiCard_set_control(MSSndCard *card, MSSndCardControlElem e, int val){
	WasapiCard *d=(WasapiCard*)card->data;

	MMRESULT mr = MMSYSERR_NOERROR;

	CoInitialize(NULL);
	switch(e){
		case MS_SND_CARD_MASTER_MUTE:
			{
				IMMDeviceEnumerator*       enumerator      = NULL; 
				IMMDeviceCollection*       devicesList     = NULL; 
				IMMDevice *pOutputDevice;
				IAudioEndpointVolume *pAudioEndpointVolume;

				UINT                       count_out;
				HRESULT                    hr; 
				unsigned int item=0;

				hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), 
					NULL, 
					CLSCTX_ALL, 
					__uuidof(IMMDeviceEnumerator), 
					reinterpret_cast<void **>(&enumerator)); 
				hr = enumerator->EnumAudioEndpoints(eRender,DEVICE_STATE_ACTIVE, 
					&devicesList);
				hr = devicesList->GetCount(&count_out); 

				pOutputDevice=NULL;
				for (item = 0; item < count_out; item++) 
				{ 
					IPropertyStore *props = NULL; 
					hr = devicesList->Item(item, &pOutputDevice); 
					hr = pOutputDevice->OpenPropertyStore(STGM_READ, &props); 
					PROPVARIANT varName; 
					PropVariantInit(&varName); 
					hr = props->GetValue(PKEY_Device_FriendlyName, &varName); 
					char szName[256];
					WideCharToMultiByte(CP_UTF8,0,varName.pwszVal,-1,szName,256,0,0);
					PropVariantClear(&varName); 
					props->Release(); 
					if (strcmp(szName, card->name)==0)
						break;
					pOutputDevice->Release(); 
					pOutputDevice=NULL;
				} 

				if (devicesList)
					devicesList->Release();
				devicesList=NULL;

				if (enumerator)
					enumerator->Release();
				enumerator=NULL;

				if (pOutputDevice==NULL)
				{
					ms_error("WasapiCard_set_control: Cannot find WASAPI device.");
					CoUninitialize();
					return -1;
				}

				hr = pOutputDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void **) &pAudioEndpointVolume);
				if (FAILED(hr)) {
					ms_error("WasapiCard_set_control: Cannot activate output.");
					pOutputDevice->Release();
					pOutputDevice=NULL;
					CoUninitialize();
					return -1;
				}

				if (val>0)
					hr = pAudioEndpointVolume->SetMute(true, &GUID_NULL);
				else
					hr = pAudioEndpointVolume->SetMute(false, &GUID_NULL);
				if (FAILED(hr)) {
					ms_error("WasapiCard_set_control: Cannot unmute.");
					pAudioEndpointVolume->Release();
					pAudioEndpointVolume=NULL;
					pOutputDevice->Release();
					pOutputDevice=NULL;
					CoUninitialize();
					return -1;
				}
				pAudioEndpointVolume->Release();
				pAudioEndpointVolume=NULL;

				pOutputDevice->Release();
				pOutputDevice=NULL;
				CoUninitialize();
				return 0;
			}
			break;
		case MS_SND_CARD_PLAYBACK_MUTE:
			{
				IMMDeviceEnumerator*       enumerator      = NULL; 
				IMMDeviceCollection*       devicesList     = NULL; 
				IMMDevice *pOutputDevice;
				IAudioSessionManager *pAudioSessionManager;
				ISimpleAudioVolume *pSimpleAudioVolume;

				UINT                       count_out;
				HRESULT                    hr; 
				unsigned int item=0;


				hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), 
					NULL, 
					CLSCTX_ALL, 
					__uuidof(IMMDeviceEnumerator), 
					reinterpret_cast<void **>(&enumerator)); 
				hr = enumerator->EnumAudioEndpoints(eRender,DEVICE_STATE_ACTIVE, 
					&devicesList);
				hr = devicesList->GetCount(&count_out); 

				pOutputDevice=NULL;
				for (item = 0; item < count_out; item++) 
				{ 
					IPropertyStore *props = NULL; 
					hr = devicesList->Item(item, &pOutputDevice); 
					hr = pOutputDevice->OpenPropertyStore(STGM_READ, &props); 
					PROPVARIANT varName; 
					PropVariantInit(&varName); 
					hr = props->GetValue(PKEY_Device_FriendlyName, &varName); 
					char szName[256];
					WideCharToMultiByte(CP_UTF8,0,varName.pwszVal,-1,szName,256,0,0);
					PropVariantClear(&varName); 
					props->Release(); 
					if (strcmp(szName, card->name)==0)
						break;
					pOutputDevice->Release(); 
					pOutputDevice=NULL;
				} 

				if (devicesList)
					devicesList->Release();
				devicesList=NULL;

				if (enumerator)
					enumerator->Release();
				enumerator=NULL;

				if (pOutputDevice==NULL)
				{
					ms_error("WasapiCard_set_control: Cannot find WASAPI device.");
					CoUninitialize();
					return -1;
				}

				hr = pOutputDevice->Activate( __uuidof(IAudioSessionManager), 
					CLSCTX_INPROC_SERVER, 
					NULL, 
					(void**) &pAudioSessionManager);
				if (FAILED(hr)) {
					ms_error("WasapiCard_set_control: Cannot get IAudioSessionManager.");
					pOutputDevice->Release();
					pOutputDevice=NULL;				
					CoUninitialize();
					return -1;
				}

				hr = pAudioSessionManager->GetSimpleAudioVolume(
					&GUID_NULL, 0, &pSimpleAudioVolume);
				if (FAILED(hr)) {
					ms_error("WasapiCard_set_control: Cannot get ISimpleAudioVolume.");
					pAudioSessionManager->Release();
					pAudioSessionManager=NULL;
					pOutputDevice->Release();
					pOutputDevice=NULL;				
					CoUninitialize();
					return -1;
				}

				if (val>0)
					hr = pSimpleAudioVolume->SetMute(true, &GUID_NULL);
				else
					hr = pSimpleAudioVolume->SetMute(false, &GUID_NULL);
				if (FAILED(hr)) {
					ms_error("WasapiCard_set_control: Cannot unmute.");
					pSimpleAudioVolume->Release();
					pSimpleAudioVolume=NULL;
					pAudioSessionManager->Release();
					pAudioSessionManager=NULL;
					pOutputDevice->Release();
					pOutputDevice=NULL;				
					CoUninitialize();
					return -1;
				}

				pSimpleAudioVolume->Release();
				pSimpleAudioVolume=NULL;
				pAudioSessionManager->Release();
				pAudioSessionManager=NULL;
				pOutputDevice->Release();
				pOutputDevice=NULL;				
				CoUninitialize();
				return 0;
			}
			break;
		case MS_SND_CARD_CAPTURE_MUTE:
			{
				IMMDeviceEnumerator*       enumerator      = NULL; 
				IMMDeviceCollection*       devicesList     = NULL; 
				IMMDevice *pMicDevice;
				IAudioEndpointVolume *pAudioEndpointVolume;
				UINT                       count_in; 
				HRESULT                    hr; 
				unsigned int item=0;

				hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), 
					NULL, 
					CLSCTX_ALL, 
					__uuidof(IMMDeviceEnumerator), 
					reinterpret_cast<void **>(&enumerator)); 
				hr = enumerator->EnumAudioEndpoints(eCapture,DEVICE_STATE_ACTIVE, 
					&devicesList);
				hr = devicesList->GetCount(&count_in); 

				pMicDevice=NULL;
				for (item = 0; item < count_in; item++) 
				{ 
					IPropertyStore *props = NULL; 
					hr = devicesList->Item(item, &pMicDevice); 
					hr = pMicDevice->OpenPropertyStore(STGM_READ, &props); 
					PROPVARIANT varName; 
					PropVariantInit(&varName); 
					hr = props->GetValue(PKEY_Device_FriendlyName, &varName); 
					char szName[256];
					WideCharToMultiByte(CP_UTF8,0,varName.pwszVal,-1,szName,256,0,0);
					PropVariantClear(&varName); 
					props->Release(); 
					if (strcmp(szName, card->name)==0)
						break;
					pMicDevice->Release(); 
					pMicDevice=NULL;
				} 

				if (devicesList)
					devicesList->Release();
				devicesList=NULL;

				if (enumerator)
					enumerator->Release();
				enumerator=NULL;

				if (pMicDevice==NULL)
				{
					ms_error("WasapiCard_set_control: Cannot find WASAPI device.");
					CoUninitialize();
					return -1;
				}

				hr = pMicDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void **) &pAudioEndpointVolume);
				if (FAILED(hr)) {
					ms_error("WasapiCard_set_control: Cannot activate mic.");
					pMicDevice->Release();
					pMicDevice=NULL;
					CoUninitialize();
					return -1;
				}

				if (val>0)
					hr = pAudioEndpointVolume->SetMute(true, &GUID_NULL);
				else
					hr = pAudioEndpointVolume->SetMute(false, &GUID_NULL);
				if (FAILED(hr)) {
					ms_error("WasapiCard_set_control: Cannot unmute.");
					pAudioEndpointVolume->Release();
					pAudioEndpointVolume=NULL;
					pMicDevice->Release();
					pMicDevice=NULL;
					CoUninitialize();
					return -1;
				}

				pAudioEndpointVolume->Release();
				pAudioEndpointVolume=NULL;
				pMicDevice->Release();
				pMicDevice=NULL;

				CoUninitialize();
				return 0;
			}
		default:
			ms_warning("WasapiCard_set_control: unsupported command.");
	}
	CoUninitialize();
	return -1;
}

static int WasapiCard_get_control(MSSndCard *card, MSSndCardControlElem e){
	WasapiCard *d=(WasapiCard*)card->data;
	return -1;
}

static void WasapiCard_init(MSSndCard *card){
	WasapiCard *c=(WasapiCard *)ms_new(WasapiCard,1);
	card->data=c;
}

static void WasapiCard_uninit(MSSndCard *card){
	ms_free(card->data);
}

static void WasapiCard_unload(MSSndCardManager *obj){
}

static void WasapiCard_detect(MSSndCardManager *m);
static  MSSndCard *WasapiCard_dup(MSSndCard *obj);

MSSndCardDesc wasapi_card_desc={
	"WASAPI",
	WasapiCard_detect,
	WasapiCard_init,
	WasapiCard_set_level,
	WasapiCard_get_level,
	WasapiCard_set_source,
	WasapiCard_set_control,
	WasapiCard_get_control,
	ms_wasapi_read_new,
	ms_wasapi_write_new,
	WasapiCard_uninit,
	WasapiCard_dup,
	WasapiCard_unload
};

static  MSSndCard *WasapiCard_dup(MSSndCard *obj){
	MSSndCard *card=ms_snd_card_new(&wasapi_card_desc);
	card->name=ms_strdup(obj->name);
	/* card->data=ms_new(WasapiCard,1); MEMORY LEAK */
	memcpy(card->data,obj->data,sizeof(WasapiCard));
	return card;
}

static MSSndCard *WasapiCard_new(const char *name, const char *deviceid, unsigned cap){
	MSSndCard *card=ms_snd_card_new(&wasapi_card_desc);
	WasapiCard *d=(WasapiCard*)card->data;
	card->name=ms_strdup(name);
	strncpy(d->devname, name, sizeof(d->devname));
	if (cap & MS_SND_CARD_CAP_CAPTURE)
		strncpy(d->in_deviceid, deviceid, sizeof(d->in_deviceid));
	if (cap & MS_SND_CARD_CAP_PLAYBACK)
		strncpy(d->out_deviceid, deviceid, sizeof(d->out_deviceid));
	card->capabilities=cap;
	return card;
}

static void add_or_update_card(MSSndCardManager *m, const char *name, const char *deviceid, unsigned int capability){
	MSSndCard *card;
	const MSList *elem=ms_snd_card_manager_get_list(m);
	for(;elem!=NULL;elem=elem->next){
		card=(MSSndCard*)elem->data;
		if (strcmp(card->desc->driver_type, wasapi_card_desc.driver_type)==0
			&& strcmp(card->name,name)==0){
			/*update already entered card */
			WasapiCard *d=(WasapiCard*)card->data;
			if (capability & MS_SND_CARD_CAP_CAPTURE)
				strncpy(d->in_deviceid, deviceid, sizeof(d->in_deviceid));
			if (capability & MS_SND_CARD_CAP_PLAYBACK)
				strncpy(d->out_deviceid, deviceid, sizeof(d->out_deviceid));

			card->capabilities|=capability;
			/* already exist */
			return;
		}
	}
	ms_snd_card_manager_add_card(m,WasapiCard_new(name, deviceid, capability));
}

static void WasapiCard_detect(MSSndCardManager *m){
	IMMDeviceEnumerator*       enumerator      = NULL; 
	IMMDeviceCollection*       devicesList     = NULL; 
	UINT                       count_in; 
	UINT                       count_out; 
	HRESULT                    hr; 
	unsigned int item=0;

	CoInitialize(NULL);

	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), 
		NULL, 
		CLSCTX_ALL, 
		__uuidof(IMMDeviceEnumerator), 
		reinterpret_cast<void **>(&enumerator)); 
	if (hr!=MMSYSERR_NOERROR)
		return;
	hr = enumerator->EnumAudioEndpoints(eCapture,DEVICE_STATE_ACTIVE, 
		&devicesList);
	if (hr!=MMSYSERR_NOERROR)
	{
		enumerator->Release();
		return;
	}
	hr = devicesList->GetCount(&count_in); 
	if (hr!=MMSYSERR_NOERROR)
	{
		enumerator->Release();
		devicesList->Release();
		return;
	}
	for (item = 0; item < count_in; item++) 
	{ 
		IPropertyStore *props = NULL; 
		IMMDevice *endpoint = NULL;
		hr = devicesList->Item(item, &endpoint); 
		hr = endpoint->OpenPropertyStore(STGM_READ, &props); 

		LPWSTR deviceID=NULL;
		endpoint->GetId(&deviceID);
		char szDeviceID[256];
		WideCharToMultiByte(CP_UTF8,0,deviceID,-1,szDeviceID,256,0,0);
		CoTaskMemFree(deviceID);

		PROPVARIANT varName; 
		PropVariantInit(&varName); 
		hr = props->GetValue(PKEY_Device_FriendlyName, &varName); 
		char szName[256];
		WideCharToMultiByte(CP_UTF8,0,varName.pwszVal,-1,szName,256,0,0);
		add_or_update_card(m,szName,szDeviceID,MS_SND_CARD_CAP_CAPTURE);

		PropVariantClear(&varName); 
		props->Release(); 
		endpoint->Release(); 
	} 
	if (devicesList)
		devicesList->Release();

	hr = enumerator->EnumAudioEndpoints(eRender,DEVICE_STATE_ACTIVE, 
		&devicesList);
	if (hr!=MMSYSERR_NOERROR)
	{
		enumerator->Release();
		return;
	}
	hr = devicesList->GetCount(&count_out); 
	if (hr!=MMSYSERR_NOERROR)
	{
		enumerator->Release();
		devicesList->Release();
		return;
	}
	for (item = 0; item < count_out; item++) 
	{ 
		IPropertyStore *props = NULL; 
		IMMDevice *endpoint = NULL;
		hr = devicesList->Item(item, &endpoint); 
		hr = endpoint->OpenPropertyStore(STGM_READ, &props); 

		LPWSTR deviceID=NULL;
		endpoint->GetId(&deviceID);
		char szDeviceID[256];
		WideCharToMultiByte(CP_UTF8,0,deviceID,-1,szDeviceID,256,0,0);
		CoTaskMemFree(deviceID);

		PROPVARIANT varName; 
		PropVariantInit(&varName); 
		hr = props->GetValue(PKEY_Device_FriendlyName, &varName); 
		char szName[256];
		WideCharToMultiByte(CP_UTF8,0,varName.pwszVal,-1,szName,256,0,0);
		add_or_update_card(m,szName,szDeviceID, MS_SND_CARD_CAP_PLAYBACK);
		PropVariantClear(&varName); 
		props->Release(); 
		endpoint->Release(); 
	} 

	if (devicesList)
		devicesList->Release();

	//Keep looking at new device.
	static CMMNotificationClient *notify = new CMMNotificationClient();
	hr = enumerator->RegisterEndpointNotificationCallback(notify);
	if (hr!=MMSYSERR_NOERROR)
	{
		enumerator->Release();
		return;
	}

	if (enumerator)
		enumerator->Release();
}


typedef struct WinSnd{
	char devname[256];
	char deviceid[256];

	/* Input device information */
	HANDLE hMmMicThread;
	IMMDevice *pMicDevice;
	IAudioClient *pMicAudioClient;
	IAudioCaptureClient *pMicCaptureClient;
	HANDLE hMicEvent;
	UINT32 micBufferFrameCount;
	REFERENCE_TIME output_delay;

	HANDLE hMmOutputThread;
	IMMDevice *pEchoDevice;
	IAudioClient *pEchoAudioClient;
	IAudioCaptureClient *pEchoCaptureClient;
	HANDLE hOutputEvent;
	UINT32 outputBufferFrameCount;

	WAVEFORMATEX wfx;

	IMMDevice *pOutputDevice;
	IAudioClient *pOutputAudioClient;
	IAudioRenderClient *pOutputRenderClient;

	queue_t rq;
	ms_mutex_t mutex;
	unsigned int nbufs_playing;
	bool_t running;

	int32_t stat_input;
	int32_t stat_output;
	int32_t stat_notplayed;

	int32_t stat_minimumbuffer;

	int ready;
}WinSnd;

#define _TRUE_TIME
#ifndef _TRUE_TIME
static uint64_t wasapi_get_cur_time( void *data){
	WinSnd *d=(WinSnd*)data;
	uint64_t curtime=((uint64_t)d->bytes_read*1000)/(uint64_t)d->wfx.nAvgBytesPerSec;
	/* ms_debug("wasapi_get_cur_time: bytes_read=%u return %lu\n",d->bytes_read,(unsigned long)curtime); */
	return curtime;
}
#endif


static void wasapi_init(MSFilter *f){
	WinSnd *d=(WinSnd *)ms_new0(WinSnd,1);

	d->wfx.wFormatTag = WAVE_FORMAT_PCM;
	d->wfx.nChannels = 1;
	d->wfx.nSamplesPerSec = 8000;
	d->wfx.wBitsPerSample = 16;
	d->wfx.nBlockAlign = d->wfx.nChannels * d->wfx.wBitsPerSample / 8; 
	d->wfx.nAvgBytesPerSec = d->wfx.nSamplesPerSec*d->wfx.nBlockAlign; 
	d->wfx.cbSize = 0;

	d->ready=0;
	ms_mutex_init(&d->mutex,NULL);
	f->data=d;

	d->stat_input=0;
	d->stat_output=0;
	d->stat_notplayed=0;
	d->stat_minimumbuffer=wasapi_MINIMUMBUFFER;
}

static void wasapi_uninit(MSFilter *f){
	WinSnd *d=(WinSnd*)f->data;
	d->ready=0;
	ms_mutex_destroy(&d->mutex);
	ms_free(f->data);
}

static int wasapi_read_checkformat(WinSnd *d, int rate, int channel, bool initialize)
{
	WAVEFORMATEX *micpwfx = NULL;
	WAVEFORMATEXTENSIBLE *micpwfxe = NULL;
	HRESULT hr; 
	REFERENCE_TIME hnsRequestedDuration;

	d->wfx.wFormatTag = WAVE_FORMAT_PCM;
	d->wfx.nChannels = channel; 
	d->wfx.nSamplesPerSec = (DWORD)rate; 
	d->wfx.wBitsPerSample = 16;
	d->wfx.nBlockAlign = d->wfx.nChannels * d->wfx.wBitsPerSample / 8; 
	d->wfx.nAvgBytesPerSec = d->wfx.nSamplesPerSec*d->wfx.nBlockAlign; 
	d->wfx.cbSize = 0;

	hnsRequestedDuration = d->output_delay * 20 * 10000;

	hr = d->pMicAudioClient->IsFormatSupported(MS2_AUDCLNT_SHAREMODE, &d->wfx, &micpwfx);
	if (FAILED(hr)) {
		ms_error("wasapi_read_checkformat: IsFormatSupported failed");
		return -1;
	}
	else if (micpwfx!=NULL)
	{
		micpwfxe = reinterpret_cast<WAVEFORMATEXTENSIBLE *>(micpwfx);

		if (FAILED(hr) || (micpwfx->wBitsPerSample != (sizeof(float) * 8)))
		{
			ms_warning("wasapi_read_checkformat: Mic Subformat is not IEEE Float");
			if (micpwfx)
				CoTaskMemFree(micpwfx);
			return -1;
		} else if (micpwfxe->SubFormat != KSDATAFORMAT_SUBTYPE_IEEE_FLOAT
			|| micpwfxe->Format.wFormatTag != WAVE_FORMAT_IEEE_FLOAT )
			 {
			ms_warning("wasapi_read_checkformat: Mic Subformat is not IEEE Float");
			if (micpwfx)
				CoTaskMemFree(micpwfx);
			return -1;
		}

		memcpy(&d->wfx, micpwfx, sizeof(WAVEFORMATEX));
		
		hr=S_OK;
		if (initialize==true)
		{
			hr = d->pMicAudioClient->Initialize(MS2_AUDCLNT_SHAREMODE, MS2_AUDCLNT_STREAMFLAGS,
				hnsRequestedDuration, 0, micpwfx, NULL);
		}
	}
	else
	{
		hr=S_OK;
		if (initialize==true)
		{
			hr = d->pMicAudioClient->Initialize(MS2_AUDCLNT_SHAREMODE, MS2_AUDCLNT_STREAMFLAGS,
				hnsRequestedDuration, 0, &d->wfx, NULL);
		}
	}

	if (micpwfx)
		CoTaskMemFree(micpwfx);
	if (hr==S_OK)
		return 0;
	ms_error("wasapi_read_checkformat: failed");
	return -1;
}

static void wasapi_read_preprocess(MSFilter *f){
	WinSnd *d=(WinSnd*)f->data;

	IMMDeviceEnumerator*       enumerator      = NULL; 
	IMMDeviceCollection*       devicesList     = NULL; 
#if 0
	UINT                       count_in; 
#endif
	HRESULT                    hr; 
	unsigned int item=0;

	int bsize;

	CoInitialize(NULL);

	d->output_delay = 10; /* ?? */

	d->stat_input=0;
	d->stat_output=0;
	d->stat_notplayed=0;
	d->stat_minimumbuffer=wasapi_MINIMUMBUFFER;

	d->wfx.wFormatTag = WAVE_FORMAT_PCM; 
	d->wfx.nChannels = 1; 
	d->wfx.nSamplesPerSec = (DWORD)44100; 
	d->wfx.wBitsPerSample = 16;
	d->wfx.nBlockAlign = d->wfx.nChannels * d->wfx.wBitsPerSample / 8; 
	d->wfx.nAvgBytesPerSec = d->wfx.nSamplesPerSec*d->wfx.nBlockAlign; 
	d->wfx.cbSize = 0;

	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), 
		NULL, 
		CLSCTX_ALL, 
		__uuidof(IMMDeviceEnumerator), 
		reinterpret_cast<void **>(&enumerator));

	WCHAR wUnicodeName[1024];
	MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)d->deviceid, -1, wUnicodeName, 1024);

	hr = enumerator->GetDevice(wUnicodeName, &d->pMicDevice);
#if 0
	hr = enumerator->EnumAudioEndpoints(eCapture,DEVICE_STATE_ACTIVE, 
		&devicesList);
	hr = devicesList->GetCount(&count_in); 

	d->pMicDevice=NULL;
	for (item = 0; item < count_in; item++) 
	{ 
		IPropertyStore *props = NULL; 
		hr = devicesList->Item(item, &d->pMicDevice); 
		hr = d->pMicDevice->OpenPropertyStore(STGM_READ, &props); 
		PROPVARIANT varName; 
		PropVariantInit(&varName); 
		hr = props->GetValue(PKEY_Device_FriendlyName, &varName); 
		char szName[256];
		WideCharToMultiByte(CP_UTF8,0,varName.pwszVal,-1,szName,256,0,0);
		PropVariantClear(&varName); 
		props->Release(); 
		if (strcmp(szName, d->devname)==0)
			break;
		d->pMicDevice->Release(); 
		d->pMicDevice=NULL;
	} 

	if (devicesList)
		devicesList->Release();
	devicesList=NULL;
#endif

	if (d->pMicDevice==NULL)
	{
		ms_error("wasapi_read_preprocess: Cannot find WASAPI device.");
		hr = enumerator->GetDefaultAudioEndpoint(eCapture, eCommunications, &d->pMicDevice);
	}

	if (enumerator)
		enumerator->Release();
	enumerator=NULL;

	if (d->pMicDevice==NULL)
	{
		ms_error("wasapi_read_preprocess: Cannot find default WASAPI device.");
		CoUninitialize();
		return ;
	}

	hr = d->pMicDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void **) &d->pMicAudioClient);
	if (FAILED(hr)) {
		ms_error("wasapi_read_preprocess: Cannot activate mic.");
		d->pMicDevice->Release();
		d->pMicDevice=NULL;
		CoUninitialize();
		return;
	}

	hr=S_FALSE;
	if (wasapi_read_checkformat(d, 48000, 1, true)==0){
		hr=S_OK;
	}else if (wasapi_read_checkformat(d, 44100, 1, true)==0){
		hr=S_OK;
	}else if (wasapi_read_checkformat(d, 32000, 1, true)==0){
		hr=S_OK;
	}else if (wasapi_read_checkformat(d, 16000, 1, true)==0){
		hr=S_OK;
	}else if (wasapi_read_checkformat(d, 8000, 1, true)==0){
		hr=S_OK;
	}else if (wasapi_read_checkformat(d, 48000, 2, true)==0){
		hr=S_OK;
	}else if (wasapi_read_checkformat(d, 44100, 2, true)==0){
		hr=S_OK;
	}else if (wasapi_read_checkformat(d, 32000, 2, true)==0){
		hr=S_OK;
	}else if (wasapi_read_checkformat(d, 16000, 2, true)==0){
		hr=S_OK;
	}else if (wasapi_read_checkformat(d, 8000, 2, true)==0){
		hr=S_OK;
	}
	if (FAILED(hr)) {
		ms_error("wasapi_read_preprocess: No compatible supported rate/bit");
		if (d->pMicAudioClient) {
			d->pMicAudioClient->Release();
		}
		d->pMicAudioClient=NULL;
		if (d->pMicCaptureClient)
			d->pMicCaptureClient->Release();
		d->pMicCaptureClient=NULL;
		if (d->pMicDevice)
			d->pMicDevice->Release();
		d->pMicDevice=NULL;
		CoUninitialize();
		return;
	}

	hr = d->pMicAudioClient->GetBufferSize(&d->micBufferFrameCount);
	if (FAILED(hr)) {
		ms_error("wasapi_read_preprocess: GetBufferSize failed");
		if (d->pMicAudioClient) {
			d->pMicAudioClient->Release();
		}
		d->pMicAudioClient=NULL;
		if (d->pMicCaptureClient)
			d->pMicCaptureClient->Release();
		d->pMicCaptureClient=NULL;
		if (d->pMicDevice)
			d->pMicDevice->Release();
		d->pMicDevice=NULL;
		CoUninitialize();
		return;
	}

	hr = d->pMicAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&d->pMicCaptureClient);
	if (FAILED(hr)) {
		ms_error("wasapi_read_preprocess: Mic GetService failed");
		if (d->pMicAudioClient) {
			d->pMicAudioClient->Release();
		}
		d->pMicAudioClient=NULL;
		if (d->pMicCaptureClient)
			d->pMicCaptureClient->Release();
		d->pMicCaptureClient=NULL;
		if (d->pMicDevice)
			d->pMicDevice->Release();
		d->pMicDevice=NULL;
		CoUninitialize();
		return;
	}

	d->hMicEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	d->pMicAudioClient->SetEventHandle(d->hMicEvent);
	if (FAILED(hr)) {
		ms_error("wasapi_read_preprocess: Failed to set mic event");
		if (d->pMicAudioClient) {
			d->pMicAudioClient->Release();
		}
		d->pMicAudioClient=NULL;
		if (d->pMicCaptureClient)
			d->pMicCaptureClient->Release();
		d->pMicCaptureClient=NULL;
		if (d->pMicDevice)
			d->pMicDevice->Release();
		d->pMicDevice=NULL;
		if (d->hMicEvent != NULL)
			CloseHandle(d->hMicEvent);
		d->hMicEvent=NULL;
		CoUninitialize();
		return;
	}

	hr = d->pMicAudioClient->Start();
	if (FAILED(hr)) {
		ms_error("wasapi_read_preprocess: Failed to start mic");
		if (d->pMicAudioClient) {
			d->pMicAudioClient->Stop();
			d->pMicAudioClient->Release();
		}
		d->pMicAudioClient=NULL;
		if (d->pMicCaptureClient)
			d->pMicCaptureClient->Release();
		d->pMicCaptureClient=NULL;
		if (d->pMicDevice)
			d->pMicDevice->Release();
		d->pMicDevice=NULL;
		if (d->hMicEvent != NULL)
			CloseHandle(d->hMicEvent);
		d->hMicEvent=NULL;
		CoUninitialize();
		return;
	}

	bsize=wasapi_NSAMPLES*d->wfx.nAvgBytesPerSec/8000;
	ms_debug("Using input buffers of %i bytes",bsize);
	d->running=TRUE;
#ifndef _TRUE_TIME
	ms_ticker_set_time_func(f->ticker,wasapi_get_cur_time,d);
#endif

	CoUninitialize();
}

static void wasapi_read_postprocess(MSFilter *f){
	WinSnd *d=(WinSnd*)f->data;

	CoInitialize(NULL);

#ifndef _TRUE_TIME
	ms_ticker_set_time_func(f->ticker,NULL,NULL);
#endif
	d->running=FALSE;

	if (d->pMicAudioClient) {
		d->pMicAudioClient->Stop();
		d->pMicAudioClient->Release();
	}
	d->pMicAudioClient=NULL;
	if (d->pMicCaptureClient)
		d->pMicCaptureClient->Release();
	d->pMicCaptureClient=NULL;
	if (d->pMicDevice)
		d->pMicDevice->Release();
	d->pMicDevice=NULL;
	if (d->hMicEvent != NULL)
		CloseHandle(d->hMicEvent);
	d->hMicEvent=NULL;

	ms_message("wasapi_read_postprocess: device closed (playing: %i) (input-output: %i) (notplayed: %i)",
		d->nbufs_playing, d->stat_input, d->stat_notplayed);

	if (d->hMmMicThread != NULL)
	{
		AvRevertMmThreadCharacteristics(d->hMmMicThread);
		d->hMmMicThread=NULL;
	}

	CoUninitialize();
}

static void wasapi_read_process(MSFilter *f){
	WinSnd *d=(WinSnd*)f->data;
	HRESULT hr; 
	uint32_t micPacketLength = 0;

	UINT32 numFramesAvailable;
	UINT64 devicePosition;
	UINT64 qpcPosition;
	BYTE *pData;
	DWORD flags;
	DWORD dwTaskIndex = 0;

	if (d->pMicCaptureClient==NULL)
		return;

	CoInitialize(NULL);

#if 0
	if (d->hMmMicThread == NULL) {
		d->hMmMicThread = AvSetMmThreadCharacteristics("Pro Audio", &dwTaskIndex);
		if (d->hMmMicThread == NULL) {
			ms_warning("wasapi_read_process: Failed to set Pro Audio thread priority");
		}
	}
#endif

	hr=S_OK;
	while (!FAILED(hr)) {
		micPacketLength=0;
		hr = d->pMicCaptureClient->GetNextPacketSize(&micPacketLength);
		if (hr==AUDCLNT_E_DEVICE_INVALIDATED)
		{
			/* device has been disconnected */
			CoUninitialize();
			return;
		}
		if (FAILED(hr))
			break;

		if (micPacketLength>0)
		{
			uint32_t i;
			hr = d->pMicCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, &devicePosition, &qpcPosition);

			if (FAILED(hr))
				break;

			mblk_t *m=allocb(numFramesAvailable * sizeof(int16_t),0);
			if (d->wfx.wBitsPerSample==16)
			{
				int16_t *pTmp;
				pTmp = (int16_t*)pData;
				for (i = 0; i < micPacketLength; i++) {
					// copy first channel
					*((int16_t*)m->b_wptr)=pTmp[i * (d->wfx.nChannels) + 0];
					m->b_wptr+=2;
				}
			}
			else
			{
				float *pTmp;
				pTmp = (float*)pData;
				for (i = 0; i < micPacketLength; i++) {
					// copy first channel
					*((int16_t*)m->b_wptr)=(int16_t)(pTmp[i * (d->wfx.nChannels) + 0]*32767 + 0.5);
					m->b_wptr+=2;
				}
			}
			//memcpy(m->b_wptr, pData, numFramesAvailable * d->wfx.nChannels * sizeof(float));
			//m->b_wptr=m->b_wptr+numFramesAvailable * d->wfx.nChannels * sizeof(float);

			hr = d->pMicCaptureClient->ReleaseBuffer(numFramesAvailable);
			if (FAILED(hr))
			{
				freemsg(m);
				break;
			}
			ms_queue_put(f->outputs[0],m);
		}
		else
			break;
	}

	CoUninitialize();
}


static int wasapi_write_checkformat(WinSnd *d, int rate, int channel, bool initialize)
{
	WAVEFORMATEX *pwfx = NULL;
	WAVEFORMATEXTENSIBLE *pwfxe = NULL;
	HRESULT hr; 
	REFERENCE_TIME hnsRequestedDuration;
#if 0
	REFERENCE_TIME def, min;
#endif

	d->wfx.wFormatTag = WAVE_FORMAT_PCM; 
	d->wfx.nChannels = channel; 
	d->wfx.nSamplesPerSec = (DWORD)rate; 
	d->wfx.wBitsPerSample = 16;
	d->wfx.nBlockAlign = d->wfx.nChannels * d->wfx.wBitsPerSample / 8; 
	d->wfx.nAvgBytesPerSec = d->wfx.nSamplesPerSec*d->wfx.nBlockAlign; 
	d->wfx.cbSize = 0;

	hnsRequestedDuration = d->output_delay * 20 * 10000;

#if 0
	d->pOutputAudioClient->GetDevicePeriod(&def, &min);
	ms_message("WASAPIOutput: Periods %lld %lld", def / 10, min / 10);
	if (def < min)
		def = min;
	if (hnsRequestedDuration < (2 * def))
		hnsRequestedDuration = 2 * def;
#endif

	hr = d->pOutputAudioClient->IsFormatSupported(MS2_AUDCLNT_SHAREMODE, &d->wfx, &pwfx);
	if (FAILED(hr)) {
		ms_error("wasapi_write_checkformat: Output Initialize failed");
		return -1;
	}
	else if (pwfx!=NULL)
	{
		pwfxe = reinterpret_cast<WAVEFORMATEXTENSIBLE *>(pwfx);

		if (FAILED(hr) || (pwfx->wBitsPerSample != (sizeof(float) * 8)) || (pwfxe->SubFormat != KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
			ms_warning("wasapi_write_checkformat: Mic Subformat is not IEEE Float");
			if (pwfx)
				CoTaskMemFree(pwfx);
			return -1;
		}
		memcpy(&d->wfx, pwfx, sizeof(WAVEFORMATEX));
		hr=S_OK;
		if (initialize==true)
		{
			hr = d->pOutputAudioClient->Initialize(MS2_AUDCLNT_SHAREMODE, MS2_AUDCLNT_STREAMFLAGS,
				hnsRequestedDuration, 0, pwfx, NULL);
		}
	}
	else
	{
		hr=S_OK;
		if (initialize==true)
		{
			hr = d->pOutputAudioClient->Initialize(MS2_AUDCLNT_SHAREMODE, MS2_AUDCLNT_STREAMFLAGS,
				hnsRequestedDuration, 0, &d->wfx, NULL);
		}
	}

	if (pwfx)
		CoTaskMemFree(pwfx);
	if (hr==S_OK)
		return 0;
	return -1;
}

static void wasapi_write_preprocess(MSFilter *f){
	WinSnd *d=(WinSnd*)f->data;

	IMMDeviceEnumerator*       enumerator      = NULL; 
	IMMDeviceCollection*       devicesList     = NULL; 
#if 0
	UINT                       count_out;
	UINT                       count_in;
#endif
	HRESULT                    hr; 
	unsigned int item=0;

	CoInitialize(NULL);

	d->output_delay = 10; /* ?? */

	d->stat_input=0;
	d->stat_output=0;
	d->stat_notplayed=0;
	d->stat_minimumbuffer=wasapi_MINIMUMBUFFER;

	d->wfx.wFormatTag = WAVE_FORMAT_PCM; 
	d->wfx.nChannels = 1; 
	d->wfx.nSamplesPerSec = (DWORD)44100; 
	d->wfx.wBitsPerSample = 16;
	d->wfx.nBlockAlign = d->wfx.nChannels * d->wfx.wBitsPerSample / 8; 
	d->wfx.nAvgBytesPerSec = d->wfx.nSamplesPerSec*d->wfx.nBlockAlign; 
	d->wfx.cbSize = 0;

#if 0
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), 
		NULL, 
		CLSCTX_ALL, 
		__uuidof(IMMDeviceEnumerator), 
		reinterpret_cast<void **>(&enumerator)); 
	hr = enumerator->EnumAudioEndpoints(eCapture,DEVICE_STATE_ACTIVE, 
		&devicesList);
	hr = devicesList->GetCount(&count_in); 

	if (devicesList)
		devicesList->Release();
	devicesList=NULL;
	if (enumerator)
		enumerator->Release();
	enumerator=NULL;
#endif

	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), 
		NULL, 
		CLSCTX_ALL, 
		__uuidof(IMMDeviceEnumerator), 
		reinterpret_cast<void **>(&enumerator)); 

	WCHAR wUnicodeName[1024];
	MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)d->deviceid, -1, wUnicodeName, 1024);

	hr = enumerator->GetDevice(wUnicodeName, &d->pOutputDevice);

#if 0
	hr = enumerator->EnumAudioEndpoints(eRender,DEVICE_STATE_ACTIVE, 
		&devicesList);
	hr = devicesList->GetCount(&count_out); 

	d->pOutputDevice=NULL;
	for (item = 0; item < count_out; item++) 
	{ 
		IPropertyStore *props = NULL; 
		hr = devicesList->Item(item, &d->pOutputDevice); 
		hr = d->pOutputDevice->OpenPropertyStore(STGM_READ, &props); 
		PROPVARIANT varName; 
		PropVariantInit(&varName); 
		hr = props->GetValue(PKEY_Device_FriendlyName, &varName); 
		char szName[256];
		WideCharToMultiByte(CP_UTF8,0,varName.pwszVal,-1,szName,256,0,0);
		PropVariantClear(&varName); 
		props->Release(); 
		if (strcmp(szName, d->devname)==0)
			break;
		d->pOutputDevice->Release(); 
		d->pOutputDevice=NULL;
	} 

	if (devicesList)
		devicesList->Release();
	devicesList=NULL;
#endif

	if (d->pOutputDevice==NULL)
	{
		ms_error("wasapi_write_preprocess: Cannot find WASAPI device.");
		hr = enumerator->GetDefaultAudioEndpoint(eRender, eCommunications, &d->pOutputDevice);
	}

	if (enumerator)
		enumerator->Release();
	enumerator=NULL;

	if (d->pOutputDevice==NULL)
	{
		ms_error("wasapi_write_preprocess: Cannot find default WASAPI device.");
		CoUninitialize();
		return ;
	}

	hr = d->pOutputDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void **) &d->pOutputAudioClient);
	if (FAILED(hr)) {
		ms_error("wasapi_write_preprocess: Cannot activate output.");
		d->pOutputDevice->Release();
		d->pOutputDevice=NULL;
		CoUninitialize();
		return;
	}

	hr=S_FALSE;
	if (wasapi_write_checkformat(d, 48000, 1, true)==0){
		hr=S_OK;
	}else if (wasapi_write_checkformat(d, 44100, 1, true)==0){
		hr=S_OK;
	}else if (wasapi_write_checkformat(d, 32000, 1, true)==0){
		hr=S_OK;
	}else if (wasapi_write_checkformat(d, 16000, 1, true)==0){
		hr=S_OK;
	}else if (wasapi_write_checkformat(d, 8000, 1, true)==0){
		hr=S_OK;
	}else if (wasapi_write_checkformat(d, 96000, 1, true)==0){
		hr=S_OK;
	}else if (wasapi_write_checkformat(d, 192000, 1, true)==0){
		hr=S_OK;
	}else if (wasapi_write_checkformat(d, 48000, 2, true)==0){
		hr=S_OK;
	}else if (wasapi_write_checkformat(d, 44100, 2, true)==0){
		hr=S_OK;
	}else if (wasapi_write_checkformat(d, 32000, 2, true)==0){
		hr=S_OK;
	}else if (wasapi_write_checkformat(d, 16000, 2, true)==0){
		hr=S_OK;
	}else if (wasapi_write_checkformat(d, 8000, 2, true)==0){
		hr=S_OK;
	}else if (wasapi_write_checkformat(d, 96000, 2, true)==0){
		hr=S_OK;
	}else if (wasapi_write_checkformat(d, 192000, 2, true)==0){
		hr=S_OK;
	}
	if (FAILED(hr)) {
		ms_error("wasapi_write_preprocess: No compatible supported rate/bit");
		if (d->pOutputAudioClient) {
			d->pOutputAudioClient->Release();
		}
		d->pOutputAudioClient=NULL;
		if (d->pOutputDevice)
			d->pOutputDevice->Release();
		d->pOutputDevice=NULL;
		CoUninitialize();
		return;
	}
	
	hr = d->pOutputAudioClient->GetBufferSize(&d->outputBufferFrameCount);
	if (FAILED(hr)) {
		ms_error("wasapi_write_preprocess: GetBufferSize failed");
		if (d->pOutputAudioClient) {
			d->pOutputAudioClient->Release();
		}
		d->pOutputAudioClient=NULL;
		if (d->pOutputDevice)
			d->pOutputDevice->Release();
		d->pOutputDevice=NULL;
		CoUninitialize();
		return;
	}

	hr = d->pOutputAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&d->pOutputRenderClient);
	if (FAILED(hr)) {
		ms_error("wasapi_write_preprocess: GetService failed");
		if (d->pOutputAudioClient) {
			d->pOutputAudioClient->Release();
		}
		d->pOutputAudioClient=NULL;
		if (d->pOutputDevice)
			d->pOutputDevice->Release();
		d->pOutputDevice=NULL;
		CoUninitialize();
		return;
	}

	d->hOutputEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	d->pOutputAudioClient->SetEventHandle(d->hOutputEvent);
	if (FAILED(hr)) {
		ms_error("wasapi_write_preprocess: Failed to set event");
		if (d->pOutputAudioClient) {
			d->pOutputAudioClient->Release();
		}
		d->pOutputAudioClient=NULL;
		if (d->pOutputDevice)
			d->pOutputDevice->Release();
		d->pOutputDevice=NULL;
		if (d->hOutputEvent != NULL)
			CloseHandle(d->hOutputEvent);
		d->hOutputEvent=NULL;
		CoUninitialize();
		return;
	}

	hr = d->pOutputAudioClient->Start();
	if (FAILED(hr)) {
		ms_error("wasapi_write_preprocess: Failed to start");
		if (d->pOutputAudioClient) {
			d->pOutputAudioClient->Stop();
			d->pOutputAudioClient->Release();
		}
		d->pOutputAudioClient=NULL;
		if (d->pOutputRenderClient)
			d->pOutputRenderClient->Release();
		d->pOutputRenderClient=NULL;
		if (d->pOutputDevice)
			d->pOutputDevice->Release();
		d->pOutputDevice=NULL;
		if (d->hOutputEvent != NULL)
			CloseHandle(d->hOutputEvent);
		d->hOutputEvent=NULL;
		CoUninitialize();
		return;
	}
	CoUninitialize();
}

static void wasapi_write_postprocess(MSFilter *f){
	WinSnd *d=(WinSnd*)f->data;

	CoInitialize(NULL);
	if (d->pOutputAudioClient) {
		d->pOutputAudioClient->Stop();
		d->pOutputAudioClient->Release();
	}
	d->pOutputAudioClient=NULL;
	if (d->pOutputRenderClient)
		d->pOutputRenderClient->Release();
	d->pOutputRenderClient=NULL;
	if (d->pOutputDevice)
		d->pOutputDevice->Release();
	d->pOutputDevice=NULL;
	if (d->hOutputEvent != NULL)
		CloseHandle(d->hOutputEvent);
	d->hOutputEvent=NULL;
	ms_message("wasapi_write_postprocess: device closed (playing: %i) (input-output: %i) (notplayed: %i)",
		d->nbufs_playing, d->stat_output, d->stat_notplayed);

	if (d->hMmOutputThread != NULL)
	{
		AvRevertMmThreadCharacteristics(d->hMmOutputThread);
		d->hMmOutputThread=NULL;
	}
	CoUninitialize();
}

static void wasapi_write_process(MSFilter *f){
	WinSnd *d=(WinSnd*)f->data;
	mblk_t *m;
	int i;
	int discarded=0;

	UINT32 numFramesAvailable;
	UINT32 packetLength = 0;
	UINT32 bsize;
	BYTE *pData;
	int16_t *pSrc;
	HRESULT hr; 
	DWORD dwTaskIndex = 0;

	if (d->pOutputAudioClient==NULL)
	{
		ms_queue_flush(f->inputs[0]);
		return;
	}

	CoInitialize(NULL);

	if (d->hMmOutputThread == NULL) {
		d->hMmOutputThread = AvSetMmThreadCharacteristics("Pro Audio", &dwTaskIndex);
		if (d->hMmOutputThread == NULL) {
			ms_warning("wasapi_write_process: Failed to set Pro Audio thread priority");
		}
	}

	//bsize = lroundf(ceilf((320 * d->wfx.nSamplesPerSec) / (44100 * 1.0f)));
	bsize = wasapi_NSAMPLES*d->wfx.nAvgBytesPerSec/8000;

	while((m=ms_queue_get(f->inputs[0]))!=NULL){

		bsize = msgdsize(m)/2;
		hr=S_OK;
		hr = d->pOutputAudioClient->GetCurrentPadding(&numFramesAvailable);
		if (hr==AUDCLNT_E_DEVICE_INVALIDATED)
		{
			/* device has been disconnected */
			ms_queue_flush(f->inputs[0]);
			CoUninitialize();
			return;
		}
		if (FAILED(hr))
		{
			freemsg(m);
			discarded++;
			continue;
		}
		if (numFramesAvailable==0)
		{
			mblk_t *tmp=allocb(bsize,0);
			memset(tmp->b_wptr,0,bsize);
			tmp->b_wptr+=bsize;
			ms_queue_put(f->inputs[0], tmp);
			ms_queue_put(f->inputs[0], dupb(tmp));
			ms_queue_put(f->inputs[0], dupb(tmp));
			ms_warning("wasapi_write_process: under run detected");
		}

		packetLength = d->outputBufferFrameCount - numFramesAvailable;

		if (packetLength >= bsize) {

			hr = d->pOutputRenderClient->GetBuffer(bsize, &pData);
			if (FAILED(hr))
			{
				freemsg(m);
				discarded++;
				continue;
			}

			pSrc = (int16_t*)m->b_rptr;
			if (d->wfx.wBitsPerSample==16)
			{
				int16_t *pTmp;
				pTmp = (int16_t*)pData;
				for (i = 0; i < (int)bsize; i++) {
					// copy first channel
					int chan;
					for (chan = 0;chan<d->wfx.nChannels;chan++)
						pTmp[i * (d->wfx.nChannels) + chan] = *pSrc;
					pSrc+=1;
				}
			}
			else
			{
				float *pTmp;
				pTmp = (float*)pData;
				for (i = 0; i < (int)bsize; i++) {
					// copy first channel
					int chan;
					for (chan = 0;chan<d->wfx.nChannels;chan++)
						pTmp[i * (d->wfx.nChannels) + chan] = ((float)(*pSrc)/32767);
					pSrc+=1;
				}
			}
			//memcpy(pData, m->b_rptr, msgdsize(m));
			//bool mixed = mix(reinterpret_cast<float *>(pData), bsize);
			//if (mixed)
			hr = d->pOutputRenderClient->ReleaseBuffer(bsize, 0);
			//else
			//	hr = d->pOutputRenderClient->ReleaseBuffer(bsize, AUDCLNT_BUFFERFLAGS_SILENT);
			if (FAILED(hr))
			{
				discarded++;
				freemsg(m);
				continue;
			}
		}
		else
		{
			ms_warning("wasapi_write_process: over run detected");
			discarded++;
		}

		freemsg(m);
	}
	if (discarded>0)
		ms_message("wasapi_write_process: %i buffer removed", discarded);

	CoUninitialize();
}

static int wasapi_read_get_rate(MSFilter *f, void *arg){
	WinSnd *d=(WinSnd*)f->data;

	ms_message("wasapi_read_get_rate: entering");
	CoInitialize(NULL);
	ms_message("wasapi_read_get_rate: entered");

	*((int*)arg) = d->wfx.nSamplesPerSec;
	if (d->pOutputAudioClient==NULL)
	{
		IMMDeviceEnumerator*       enumerator      = NULL; 
		IMMDeviceCollection*       devicesList     = NULL; 
#if 0
		UINT                       count_in; 
#endif
		HRESULT                    hr; 
		unsigned int item=0;

		d->output_delay = 10; /* ?? */

		d->stat_input=0;
		d->stat_output=0;
		d->stat_notplayed=0;
		d->stat_minimumbuffer=wasapi_MINIMUMBUFFER;

		d->wfx.wFormatTag = WAVE_FORMAT_PCM; 
		d->wfx.nChannels = 2; 
		d->wfx.nSamplesPerSec = (DWORD)44100; 
		d->wfx.wBitsPerSample = 16;
		d->wfx.nBlockAlign = d->wfx.nChannels * d->wfx.wBitsPerSample / 8; 
		d->wfx.nAvgBytesPerSec = d->wfx.nSamplesPerSec*d->wfx.nBlockAlign; 
		d->wfx.cbSize = 0;

		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), 
			NULL, 
			CLSCTX_ALL, 
			__uuidof(IMMDeviceEnumerator), 
			reinterpret_cast<void **>(&enumerator)); 

		WCHAR wUnicodeName[1024];
		MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)d->deviceid, -1, wUnicodeName, 1024);

		hr = enumerator->GetDevice(wUnicodeName, &d->pMicDevice);

#if 0
		hr = enumerator->EnumAudioEndpoints(eCapture,DEVICE_STATE_ACTIVE, 
			&devicesList);
		hr = devicesList->GetCount(&count_in); 

		d->pMicDevice=NULL;
		for (item = 0; item < count_in; item++) 
		{ 
			IPropertyStore *props = NULL; 
			hr = devicesList->Item(item, &d->pMicDevice); 
			hr = d->pMicDevice->OpenPropertyStore(STGM_READ, &props); 
			PROPVARIANT varName; 
			PropVariantInit(&varName); 
			hr = props->GetValue(PKEY_Device_FriendlyName, &varName); 
			char szName[256];
			WideCharToMultiByte(CP_UTF8,0,varName.pwszVal,-1,szName,256,0,0);
			PropVariantClear(&varName); 
			props->Release(); 
			if (strcmp(szName, d->devname)==0)
				break;
			d->pMicDevice->Release(); 
			d->pMicDevice=NULL;
		} 

		if (devicesList)
			devicesList->Release();
		devicesList=NULL;
#endif

		if (enumerator)
			enumerator->Release();
		enumerator=NULL;

		if (d->pMicDevice==NULL)
		{
			ms_error("wasapi_read_get_rate: Cannot find WASAPI device.");
			CoUninitialize();
			return -1;
		}

		ms_message("wasapi_read_get_rate: activating MicDevice");
		hr = d->pMicDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void **) &d->pMicAudioClient);
		if (FAILED(hr)) {
			ms_error("wasapi_read_get_rate: Cannot activate mic.");
			d->pMicDevice->Release();
			d->pMicDevice=NULL;
			CoUninitialize();
			return -1;
		}

		ms_message("wasapi_read_get_rate: checking rate");
		hr=S_FALSE;
		if (wasapi_read_checkformat(d, 48000, 1, false)==0){
			hr=S_OK;
		}else if (wasapi_read_checkformat(d, 44100, 1, false)==0){
			hr=S_OK;
		}else if (wasapi_read_checkformat(d, 32000, 1, false)==0){
			hr=S_OK;
		}else if (wasapi_read_checkformat(d, 16000, 1, false)==0){
			hr=S_OK;
		}else if (wasapi_read_checkformat(d, 8000, 1, false)==0){
			hr=S_OK;
		}else if (wasapi_read_checkformat(d, 48000, 2, false)==0){
			hr=S_OK;
		}else if (wasapi_read_checkformat(d, 44100, 2, false)==0){
			hr=S_OK;
		}else if (wasapi_read_checkformat(d, 32000, 2, false)==0){
			hr=S_OK;
		}else if (wasapi_read_checkformat(d, 16000, 2, false)==0){
			hr=S_OK;
		}else if (wasapi_read_checkformat(d, 8000, 2, false)==0){
			hr=S_OK;
		}
		
		if (d->pMicAudioClient) {
			d->pMicAudioClient->Release();
		}
		d->pMicAudioClient=NULL;
		if (d->pMicDevice)
			d->pMicDevice->Release();
		d->pMicDevice=NULL;

		if (FAILED(hr)) {
			ms_error("wasapi_read_get_rate: No compatible supported rate/bit");
			CoUninitialize();
			return -1;
		}
	}

	*((int*)arg) = d->wfx.nSamplesPerSec;
	ms_message("wasapi_read_get_rate: rate=%i, channels=%i, bits=%i",
		d->wfx.nSamplesPerSec, d->wfx.nChannels, d->wfx.wBitsPerSample);
	CoUninitialize();
	return 0;
}

static int wasapi_write_get_rate(MSFilter *f, void *arg){
	WinSnd *d=(WinSnd*)f->data;

	CoInitialize(NULL);

	*((int*)arg) = d->wfx.nSamplesPerSec;
	if (d->pOutputAudioClient==NULL)
	{
		IMMDeviceEnumerator*       enumerator      = NULL; 
		IMMDeviceCollection*       devicesList     = NULL; 
#if 0
		UINT                       count_out;
		UINT                       count_in;
#endif
		HRESULT                    hr; 
		unsigned int item=0;

		d->output_delay = 10; /* ?? */

		d->stat_input=0;
		d->stat_output=0;
		d->stat_notplayed=0;
		d->stat_minimumbuffer=wasapi_MINIMUMBUFFER;

		d->wfx.wFormatTag = WAVE_FORMAT_PCM; 
		d->wfx.nChannels = 2; 
		d->wfx.nSamplesPerSec = (DWORD)44100; 
		d->wfx.wBitsPerSample = 16;
		d->wfx.nBlockAlign = d->wfx.nChannels * d->wfx.wBitsPerSample / 8; 
		d->wfx.nAvgBytesPerSec = d->wfx.nSamplesPerSec*d->wfx.nBlockAlign; 
		d->wfx.cbSize = 0;

#if 0
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), 
			NULL, 
			CLSCTX_ALL, 
			__uuidof(IMMDeviceEnumerator), 
			reinterpret_cast<void **>(&enumerator)); 
		hr = enumerator->EnumAudioEndpoints(eCapture,DEVICE_STATE_ACTIVE, 
			&devicesList);
		hr = devicesList->GetCount(&count_in); 

		if (devicesList)
			devicesList->Release();
		devicesList=NULL;
		if (enumerator)
			enumerator->Release();
		enumerator=NULL;
#endif

		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), 
			NULL, 
			CLSCTX_ALL, 
			__uuidof(IMMDeviceEnumerator), 
			reinterpret_cast<void **>(&enumerator)); 

		WCHAR wUnicodeName[1024];
		MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)d->deviceid, -1, wUnicodeName, 1024);

		hr = enumerator->GetDevice(wUnicodeName, &d->pOutputDevice);
#if 0
		hr = enumerator->EnumAudioEndpoints(eRender,DEVICE_STATE_ACTIVE, 
			&devicesList);
		hr = devicesList->GetCount(&count_out); 

		d->pOutputDevice=NULL;
		for (item = 0; item < count_out; item++) 
		{ 
			IPropertyStore *props = NULL; 
			hr = devicesList->Item(item, &d->pOutputDevice); 
			hr = d->pOutputDevice->OpenPropertyStore(STGM_READ, &props); 
			PROPVARIANT varName; 
			PropVariantInit(&varName); 
			hr = props->GetValue(PKEY_Device_FriendlyName, &varName); 
			char szName[256];
			WideCharToMultiByte(CP_UTF8,0,varName.pwszVal,-1,szName,256,0,0);
			PropVariantClear(&varName); 
			props->Release(); 
			if (strcmp(szName, d->devname)==0)
				break;
			d->pOutputDevice->Release(); 
			d->pOutputDevice=NULL;
		} 

		if (devicesList)
			devicesList->Release();
		devicesList=NULL;
#endif

		if (enumerator)
			enumerator->Release();
		enumerator=NULL;

		if (d->pOutputDevice==NULL)
		{
			ms_error("wasapi_write_get_rate: Cannot find WASAPI device.");
			CoUninitialize();
			return -1;
		}

		hr = d->pOutputDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void **) &d->pOutputAudioClient);
		if (FAILED(hr)) {
			ms_error("wasapi_write_get_rate: Cannot activate output.");
			d->pOutputDevice->Release();
			d->pOutputDevice=NULL;
			CoUninitialize();
			return -1;
		}

		hr=S_FALSE;
		if (wasapi_write_checkformat(d, 48000, 1, false)==0){
			hr=S_OK;
		}else if (wasapi_write_checkformat(d, 44100, 1, false)==0){
			hr=S_OK;
		}else if (wasapi_write_checkformat(d, 32000, 1, false)==0){
			hr=S_OK;
		}else if (wasapi_write_checkformat(d, 16000, 1, false)==0){
			hr=S_OK;
		}else if (wasapi_write_checkformat(d, 8000, 1, false)==0){
			hr=S_OK;
		}else if (wasapi_write_checkformat(d, 96000, 1, false)==0){
			hr=S_OK;
		}else if (wasapi_write_checkformat(d, 192000, 1, false)==0){
			hr=S_OK;
		}else if (wasapi_write_checkformat(d, 48000, 2, false)==0){
			hr=S_OK;
		}else if (wasapi_write_checkformat(d, 44100, 2, false)==0){
			hr=S_OK;
		}else if (wasapi_write_checkformat(d, 32000, 2, false)==0){
			hr=S_OK;
		}else if (wasapi_write_checkformat(d, 16000, 2, false)==0){
			hr=S_OK;
		}else if (wasapi_write_checkformat(d, 8000, 2, false)==0){
			hr=S_OK;
		}else if (wasapi_write_checkformat(d, 96000, 2, false)==0){
			hr=S_OK;
		}else if (wasapi_write_checkformat(d, 192000, 2, false)==0){
			hr=S_OK;
		}

		if (d->pOutputAudioClient) {
			d->pOutputAudioClient->Release();
		}
		d->pOutputAudioClient=NULL;
		if (d->pOutputDevice)
			d->pOutputDevice->Release();
		d->pOutputDevice=NULL;

		if (FAILED(hr)) {
			ms_error("wasapi_write_get_rate: No compatible supported rate/bit");
			CoUninitialize();
			return -1;
		}
	}

	*((int*)arg) = d->wfx.nSamplesPerSec;
	ms_message("wasapi_write_get_rate: rate=%i, channels=%i, bits=%i",
		d->wfx.nSamplesPerSec, d->wfx.nChannels, d->wfx.wBitsPerSample);
	CoUninitialize();
	return 0;
}

static int set_rate(MSFilter *f, void *arg){
	WinSnd *d=(WinSnd*)f->data;
	d->wfx.nSamplesPerSec=*((int*)arg);
	return 0;
}

static int set_nchannels(MSFilter *f, void *arg){
	WinSnd *d=(WinSnd*)f->data;
	d->wfx.nChannels=*((int*)arg);
	return 0;
}

static int wasapi_get_stat_input(MSFilter *f, void *arg){
	WinSnd *d=(WinSnd*)f->data;
	return d->stat_input;
}

static int wasapi_get_stat_ouptut(MSFilter *f, void *arg){
	WinSnd *d=(WinSnd*)f->data;
	return d->stat_output;
}

static int wasapi_get_stat_discarded(MSFilter *f, void *arg){
	WinSnd *d=(WinSnd*)f->data;
	return d->stat_notplayed;
}


static int read_get_statistics(MSFilter *f, void *arg){
	WinSnd *d=(WinSnd*)f->data;
	ms_warning("filter: %s[%i->%i]", f->desc->name,
		f->desc->ninputs, f->desc->noutputs);
	if (f->outputs[0]!=NULL) ms_warning("filter: %s out[0]=%i", f->desc->name,
		f->outputs[0]->q.q_mcount);
	ms_warning("filter: %s buf=%i", f->desc->name,
		d->rq.q_mcount);
	return 0;
}

static int write_get_statistics(MSFilter *f, void *arg){
	WinSnd *d=(WinSnd*)f->data;
	ms_warning("filter: %s[%i->%i]", f->desc->name,
		f->desc->ninputs, f->desc->noutputs);
	if (f->inputs[0]!=NULL) ms_warning("filter: %s in[0]=%i", f->desc->name,
		f->inputs[0]->q.q_mcount);
	ms_warning("filter: %s buf=%i", f->desc->name,
		d->rq.q_mcount);
	return 0;
}
static MSFilterMethod wasapi_read_methods[]={
	{	MS_FILTER_GET_SAMPLE_RATE	, wasapi_read_get_rate	},
	{	MS_FILTER_SET_SAMPLE_RATE	, set_rate	},
	{	MS_FILTER_SET_NCHANNELS		, set_nchannels	},
	{	MS_FILTER_GET_STAT_INPUT, wasapi_get_stat_input },
	{	MS_FILTER_GET_STAT_OUTPUT, wasapi_get_stat_ouptut },
	{	MS_FILTER_GET_STAT_DISCARDED, wasapi_get_stat_discarded },
	{	MS_FILTER_GET_STATISTICS, read_get_statistics },
	{	0				, NULL		}
};

MSFilterDesc wasapi_read_desc={
	MS_FILTER_PLUGIN_ID,
	"WasapiRead",
	"Wasapi Sound capture filter for Vista",
	MS_FILTER_OTHER,
	NULL,
	0,
	1,
	wasapi_init,
	wasapi_read_preprocess,
	wasapi_read_process,
	wasapi_read_postprocess,
	wasapi_uninit,
	wasapi_read_methods
};

static MSFilterMethod wasapi_write_methods[]={
	{	MS_FILTER_GET_SAMPLE_RATE	, wasapi_write_get_rate	},
	{	MS_FILTER_SET_SAMPLE_RATE	, set_rate	},
	{	MS_FILTER_SET_NCHANNELS		, set_nchannels	},
	{	MS_FILTER_GET_STAT_INPUT, wasapi_get_stat_input },
	{	MS_FILTER_GET_STAT_OUTPUT, wasapi_get_stat_ouptut },
	{	MS_FILTER_GET_STAT_DISCARDED, wasapi_get_stat_discarded },
	{	MS_FILTER_GET_STATISTICS, write_get_statistics },
	{	0				, NULL		}
};

MSFilterDesc wasapi_write_desc={
	MS_FILTER_PLUGIN_ID,
	"WasapiWrite",
	"Wasapi Sound playback filter for Vista",
	MS_FILTER_OTHER,
	NULL,
	1,
	0,
	wasapi_init,
	wasapi_write_preprocess,
	wasapi_write_process,
	wasapi_write_postprocess,
	wasapi_uninit,
	wasapi_write_methods
};

MSFilter *ms_wasapi_read_new(MSSndCard *card){
	MSFilter *f=ms_filter_new_from_desc(&wasapi_read_desc);
	WasapiCard *wc=(WasapiCard*)card->data;
	WinSnd *d=(WinSnd*)f->data;
	strncpy(d->devname, wc->devname, sizeof(d->devname));
	strncpy(d->deviceid, wc->in_deviceid, sizeof(d->deviceid));
	return f;
}


MSFilter *ms_wasapi_write_new(MSSndCard *card){
	MSFilter *f=ms_filter_new_from_desc(&wasapi_write_desc);
	WasapiCard *wc=(WasapiCard*)card->data;
	WinSnd *d=(WinSnd*)f->data;
	strncpy(d->devname, wc->devname, sizeof(d->devname));
	strncpy(d->deviceid, wc->out_deviceid, sizeof(d->deviceid));
	return f;
}

extern "C" __declspec(dllexport) void libmswasapi_init(void){
	MSSndCardManager *cm;

	OSVERSIONINFOEXA ovi;
	memset(&ovi, 0, sizeof(ovi));

	ovi.dwOSVersionInfoSize=sizeof(ovi);
	GetVersionEx(reinterpret_cast<OSVERSIONINFOA *>(&ovi));

	if ((ovi.dwMajorVersion < 6) || (ovi.dwBuildNumber < 6001)) {
		//WASAPIInit: Requires Vista SP1
		return;
	}

	HMODULE hLib = LoadLibrary("AVRT.DLL");
	if (hLib == NULL) {
		//WASAPIInit: Failed to load avrt.dll
		return;
	}
	FreeLibrary(hLib);

#if 1
	ms_snd_card_manager_destroy();
#endif

	ms_filter_register(&wasapi_read_desc);
	ms_filter_register(&wasapi_write_desc);

	cm=ms_snd_card_manager_get();
	ms_snd_card_manager_register_desc(cm,&wasapi_card_desc);
}


// Callback methods for device-event notifications.

HRESULT STDMETHODCALLTYPE CMMNotificationClient::OnDefaultDeviceChanged(
	EDataFlow flow, ERole role,
	LPCWSTR pwstrDeviceId)
{
	char  *pszFlow = "?????";
	char  *pszRole = "?????";
	char szName[256];
	char szDeviceID[256];
	IMMDevice *pOutputDevice=NULL;
	IMMDevice *pMicDevice=NULL;

	CoInitialize(NULL);
	pOutputDevice = GetOutputDevice(pwstrDeviceId, szName, sizeof(szName), szDeviceID, sizeof(szDeviceID));
	if (pOutputDevice==NULL)
		pMicDevice = GetMicDevice(pwstrDeviceId, szName, sizeof(szName), szDeviceID, sizeof(szDeviceID));

	if (pOutputDevice==NULL && pMicDevice==NULL)
	{
		CoUninitialize();
		return S_OK;
	}

	switch (flow)
	{
	case eRender:
		pszFlow = "eRender";
		break;
	case eCapture:
		pszFlow = "eCapture";
		break;
	}

	switch (role)
	{
	case eConsole:
		pszRole = "eConsole";
		break;
	case eMultimedia:
		pszRole = "eMultimedia";
		break;
	case eCommunications:
		pszRole = "eCommunications";
		break;
	}

	ms_message("  -->New default device: flow = %s, role = %s\n",
		pszFlow, pszRole);
	CoUninitialize();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CMMNotificationClient::OnDeviceAdded(LPCWSTR pwstrDeviceId)
{
	char szName[256];
	char szDeviceID[256];
	IMMDevice *pOutputDevice=NULL;
	IMMDevice *pMicDevice=NULL;

	CoInitialize(NULL);
	pOutputDevice = GetOutputDevice(pwstrDeviceId, szName, sizeof(szName), szDeviceID, sizeof(szDeviceID));
	if (pOutputDevice==NULL)
		pMicDevice = GetMicDevice(pwstrDeviceId, szName, sizeof(szName), szDeviceID, sizeof(szDeviceID));

	if (pOutputDevice==NULL && pMicDevice==NULL)
	{
		CoUninitialize();
		return S_OK;
	}

	ms_message("  -->Added device\n");
	CoUninitialize();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CMMNotificationClient::OnDeviceRemoved(LPCWSTR pwstrDeviceId)
{
	char szName[256];
	char szDeviceID[256];
	IMMDevice *pOutputDevice=NULL;
	IMMDevice *pMicDevice=NULL;

	CoInitialize(NULL);
	pOutputDevice = GetOutputDevice(pwstrDeviceId, szName, sizeof(szName), szDeviceID, sizeof(szDeviceID));
	if (pOutputDevice==NULL)
		pMicDevice = GetMicDevice(pwstrDeviceId, szName, sizeof(szName), szDeviceID, sizeof(szDeviceID));

	if (pOutputDevice==NULL && pMicDevice==NULL)
	{
		CoUninitialize();
		return S_OK;
	}

	ms_message("  -->Removed device\n");
	CoUninitialize();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CMMNotificationClient::OnDeviceStateChanged(
	LPCWSTR pwstrDeviceId,
	DWORD dwNewState)
{
	char  *pszState = "?????";
	char szName[256];
	char szDeviceID[256];

	IMMDevice *pOutputDevice=NULL;
	IMMDevice *pMicDevice=NULL;

	CoInitialize(NULL);
	pOutputDevice = GetOutputDevice(pwstrDeviceId, szName, sizeof(szName), szDeviceID, sizeof(szDeviceID));
	if (pOutputDevice==NULL)
		pMicDevice = GetMicDevice(pwstrDeviceId, szName, sizeof(szName), szDeviceID, sizeof(szDeviceID));

	if (pOutputDevice==NULL && pMicDevice==NULL)
	{
		CoUninitialize();
		return S_OK;
	}

	MSSndCard *card=NULL;
	MSSndCardManager *m=NULL;
	const MSList *elem=NULL;

	m = ms_snd_card_manager_get();
	if (m!=NULL)
		elem=ms_snd_card_manager_get_list(m);

	ms_message("  -->New device state is DEVICE_STATE_%s (0x%8.8x)\n",
		pszState, dwNewState);

	if (elem==NULL)
	{
		CoUninitialize();
		return S_OK;
	}

	switch (dwNewState)
	{
	case DEVICE_STATE_ACTIVE:

		pszState = "ACTIVE";

		for(;elem!=NULL;elem=elem->next){
			card=(MSSndCard*)elem->data;
			if (strcmp(card->desc->driver_type, wasapi_card_desc.driver_type)!=0)
				continue;
			WasapiCard *d=(WasapiCard*)card->data;
			if (strcmp(d->devname,szName)==0){
				/*update already entered card */
				if (pMicDevice!=NULL)
				{
					card->capabilities|=MS_SND_CARD_CAP_CAPTURE;
					strncpy(d->in_deviceid, szDeviceID, sizeof(d->in_deviceid));
				}
				else if (pOutputDevice!=NULL)
				{
					card->capabilities|=MS_SND_CARD_CAP_PLAYBACK;
					strncpy(d->out_deviceid, szDeviceID, sizeof(d->out_deviceid));
				}
				/* already exist */
				CoUninitialize();
				return S_OK;
			}
		}
		if (pMicDevice!=NULL)
			ms_snd_card_manager_add_card(m,WasapiCard_new(szName, szDeviceID, MS_SND_CARD_CAP_CAPTURE));
		else if (pOutputDevice!=NULL)
			ms_snd_card_manager_add_card(m,WasapiCard_new(szName, szDeviceID, MS_SND_CARD_CAP_PLAYBACK));

		break;
	case DEVICE_STATE_DISABLED:
		pszState = "DISABLED";
		break;
	case DEVICE_STATE_NOTPRESENT:
		pszState = "NOTPRESENT";
		
		for(;elem!=NULL;elem=elem->next){
			card=(MSSndCard*)elem->data;
			if (strcmp(card->desc->driver_type, wasapi_card_desc.driver_type)!=0)
				continue;
			WasapiCard *d=(WasapiCard*)card->data;
			if (strcmp(d->devname,szName)==0){
				/*update already entered card */
				if (pMicDevice!=NULL)
				{
					card->capabilities&= ~MS_SND_CARD_CAP_CAPTURE; /* turn off */
					strncpy(d->in_deviceid, szDeviceID, sizeof(d->in_deviceid));
				}
				else if (pOutputDevice!=NULL)
				{
					card->capabilities&= ~MS_SND_CARD_CAP_PLAYBACK; /* turn off */
					strncpy(d->out_deviceid, szDeviceID, sizeof(d->out_deviceid));
				}
				/* already exist */
				CoUninitialize();
				return S_OK;
			}
		}

		break;
	case DEVICE_STATE_UNPLUGGED:
		pszState = "UNPLUGGED";
		break;
	}

	CoUninitialize();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CMMNotificationClient::OnPropertyValueChanged(
	LPCWSTR pwstrDeviceId,
	const PROPERTYKEY key)
{
	char szName[256];
	char szDeviceID[256];
	IMMDevice *pOutputDevice=NULL;
	IMMDevice *pMicDevice=NULL;

	CoInitialize(NULL);
	pOutputDevice = GetOutputDevice(pwstrDeviceId, szName, sizeof(szName), szDeviceID, sizeof(szDeviceID));
	if (pOutputDevice==NULL)
		pMicDevice = GetMicDevice(pwstrDeviceId, szName, sizeof(szName), szDeviceID, sizeof(szDeviceID));

	if (pOutputDevice==NULL && pMicDevice==NULL)
	{
		CoUninitialize();
		return S_OK;
	}

	ms_message("  -->Changed device property "
		"{%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x}#%d\n",
		key.fmtid.Data1, key.fmtid.Data2, key.fmtid.Data3,
		key.fmtid.Data4[0], key.fmtid.Data4[1],
		key.fmtid.Data4[2], key.fmtid.Data4[3],
		key.fmtid.Data4[4], key.fmtid.Data4[5],
		key.fmtid.Data4[6], key.fmtid.Data4[7],
		key.pid);
	CoUninitialize();
	return S_OK;
}

// Given an endpoint ID string, print the friendly device name.
IMMDevice *CMMNotificationClient::GetOutputDevice(LPCWSTR pwstrId, char *szName, int szName_size, char *szDeviceid, int szDeviceid_size)
{
	IMMDeviceEnumerator*       enumerator      = NULL; 
	IMMDeviceCollection*       devicesList     = NULL; 
	IMMDevice *pOutputDevice;
	IMMDevice *pDevice;
	IPropertyStore *props = NULL; 

	UINT                       count_out;
	HRESULT                    hr; 
	unsigned int item=0;

	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), 
		NULL, 
		CLSCTX_ALL, 
		__uuidof(IMMDeviceEnumerator), 
		reinterpret_cast<void **>(&enumerator)); 
	if (hr!=S_OK)
	{
		return NULL;
	}

	hr = enumerator->GetDevice(pwstrId, &pDevice);
	if (hr!=S_OK)
	{
		enumerator->Release();
		return NULL;
	}
	hr = pDevice->OpenPropertyStore(STGM_READ, &props); 
	if (hr!=S_OK)
	{
		pDevice->Release();
		enumerator->Release();
		return NULL;
	}
	PROPVARIANT varName; 
	PropVariantInit(&varName); 
	hr = props->GetValue(PKEY_Device_FriendlyName, &varName); 
	if (hr!=S_OK)
	{
		PropVariantClear(&varName); 
		props->Release(); 
		pDevice->Release();
		enumerator->Release();
		return NULL;
	}
	WideCharToMultiByte(CP_UTF8,0,varName.pwszVal,-1,szName,256,0,0);
	PropVariantClear(&varName); 
	props->Release(); 

	LPWSTR deviceID=NULL;
	pDevice->GetId(&deviceID);
	WideCharToMultiByte(CP_UTF8,0,deviceID,-1,szDeviceid,szDeviceid_size,0,0);
	CoTaskMemFree(deviceID);

	pDevice->Release();

	hr = enumerator->EnumAudioEndpoints(eRender,DEVICE_STATEMASK_ALL, 
		&devicesList);
	if (hr!=S_OK)
	{
		enumerator->Release();
		return NULL;
	}
	hr = devicesList->GetCount(&count_out); 
	if (hr!=S_OK)
	{
		enumerator->Release();
		devicesList->Release();
		return NULL;
	}
	pOutputDevice=NULL;
	for (item = 0; item < count_out; item++) 
	{ 
		hr = devicesList->Item(item, &pOutputDevice);
		if (hr!=S_OK)
		{
			devicesList->Release();
			enumerator->Release();
			return NULL;
		}

		char szDeviceid2[256];
		LPWSTR deviceID2=NULL;
		pOutputDevice->GetId(&deviceID2);
		WideCharToMultiByte(CP_UTF8,0,deviceID2,-1,szDeviceid2,256,0,0);
		CoTaskMemFree(deviceID2);

		if (strcmp(szDeviceid2, szDeviceid)==0)
			break;
		pOutputDevice->Release(); 
		pOutputDevice=NULL;
	} 

	devicesList->Release();
	devicesList=NULL;

	enumerator->Release();
	enumerator=NULL;

	return pOutputDevice;
}

// Given an endpoint ID string, print the friendly device name.
IMMDevice *CMMNotificationClient::GetMicDevice(LPCWSTR pwstrId, char *szName, int szName_size, char *szDeviceid, int szDeviceid_size)
{
	IMMDeviceEnumerator*       enumerator      = NULL; 
	IMMDeviceCollection*       devicesList     = NULL; 
	IMMDevice *pMicDevice;
	IMMDevice *pDevice;
	IPropertyStore *props = NULL; 
	UINT                       count_in; 
	HRESULT                    hr; 
	unsigned int item=0;

	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), 
		NULL, 
		CLSCTX_ALL, 
		__uuidof(IMMDeviceEnumerator), 
		reinterpret_cast<void **>(&enumerator)); 
	if (hr!=S_OK)
	{
		return NULL;
	}

	hr = enumerator->GetDevice(pwstrId, &pDevice);
	if (hr!=S_OK)
	{
		enumerator->Release();
		return NULL;
	}
	hr = pDevice->OpenPropertyStore(STGM_READ, &props); 
	if (hr!=S_OK)
	{
		pDevice->Release();
		enumerator->Release();
		return NULL;
	}
	PROPVARIANT varName; 
	PropVariantInit(&varName); 
	if (hr!=S_OK)
	{
		PropVariantClear(&varName); 
		props->Release(); 
		pDevice->Release();
		enumerator->Release();
		return NULL;
	}
	hr = props->GetValue(PKEY_Device_FriendlyName, &varName); 
	WideCharToMultiByte(CP_UTF8,0,varName.pwszVal,-1,szName,256,0,0);
	PropVariantClear(&varName); 
	props->Release(); 

	LPWSTR deviceID=NULL;
	pDevice->GetId(&deviceID);
	WideCharToMultiByte(CP_UTF8,0,deviceID,-1,szDeviceid,szDeviceid_size,0,0);
	CoTaskMemFree(deviceID);

	pDevice->Release();

	hr = enumerator->EnumAudioEndpoints(eCapture,DEVICE_STATEMASK_ALL, 
		&devicesList);
	if (hr!=S_OK)
	{
		enumerator->Release();
		return NULL;
	}
	hr = devicesList->GetCount(&count_in); 
	if (hr!=S_OK)
	{
		enumerator->Release();
		devicesList->Release();
		return NULL;
	}
	pMicDevice=NULL;
	for (item = 0; item < count_in; item++) 
	{ 
		hr = devicesList->Item(item, &pMicDevice);
		if (hr!=S_OK)
		{
			devicesList->Release();
			enumerator->Release();
			return NULL;
		}

		char szDeviceid2[256];
		LPWSTR deviceID2=NULL;
		pMicDevice->GetId(&deviceID2);
		WideCharToMultiByte(CP_UTF8,0,deviceID2,-1,szDeviceid2,256,0,0);
		CoTaskMemFree(deviceID2);

		if (strcmp(szDeviceid2, szDeviceid)==0)
			break;
		pMicDevice->Release(); 
		pMicDevice=NULL;
	} 

	devicesList->Release();
	devicesList=NULL;

	enumerator->Release();
	enumerator=NULL;

	return pMicDevice;
}
