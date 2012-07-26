/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
  Copyright (C) 2003-2011  Aymeric MOIZARD - <amoizard@gmail.com>
*/

#if defined(_WIN32_WCE) && defined(ENABLE_VIDEO)

#include "streams.h"
#include "dxifilter.h"
#include "stdio.h"
#include <tchar.h>

#include "initguid.h"
//Missing definition
DEFINE_GUID(IID_IPersist, 0x0000010c, 0x0000, 0x0000, 0xC0, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x46);

IPin* MyFindPin(IBaseFilter* pFilter, PIN_DIRECTION Dir, LPCSTR pszPinName, 
				BOOL wildMatch)
{
	IPin*          p_pin = NULL;
	IEnumPins*     p_pins = NULL;
	PIN_DIRECTION  pin_dir;
	BOOL           pin_found = FALSE;
	ULONG          n;
	HRESULT        hr;
	PIN_INFO       pi;

	WCHAR w_pin_name[128];
	MultiByteToWideChar(CP_ACP, 0, pszPinName, -1, w_pin_name, 128);
	BOOL           ananymous_find = (wcslen(w_pin_name) == 0);

	if (SUCCEEDED(pFilter->EnumPins(&p_pins))) {
		hr = p_pins->Reset();

		while (!pin_found && (S_OK == p_pins->Next(1, &p_pin, &n))) {
			if (S_OK == p_pin->QueryDirection(&pin_dir)) {
				if (pin_dir == Dir) {
					if (ananymous_find)
						pin_found = TRUE;
					else if (SUCCEEDED(p_pin->QueryPinInfo(&pi))) {
						if (wildMatch)
							pin_found = (wcsstr(pi.achName, w_pin_name) != NULL);
						else
							pin_found = (_wcsicmp(w_pin_name, pi.achName) == 0);
						pi.pFilter->Release();
						pi.pFilter=NULL;
					}
				}
			}
			if (!pin_found) 
			{
				p_pin->Release();
				p_pin=NULL;
			}
		}
		p_pins->Release();
		p_pins=NULL;
	}
	if (pin_found) {
		return p_pin;
	}
	return NULL;
}

//
// Constructor
//
CDataInputFilter::CDataInputFilter(LPUNKNOWN pUnk, HRESULT *phr)
: CSource(NAME("CDataInputFilter"), pUnk, CLSID_DataInputFilter)
{
	CAutoLock cAutoLock(&m_cStateLock);
	new CDataInputStream(phr, this, L"Out");
}
CDataInputFilter::~CDataInputFilter()
{
}

int CDataInputFilter::PutData(unsigned char *payload, int size)
{
	int i=-1;
	CDataInputStream *ss = NULL;
	IPin *pin=NULL;
	pin = MyFindPin(this, PINDIR_OUTPUT, "Out", FALSE);
	if (pin==NULL)
		return i;
	ss = (CDataInputStream *)pin;
	i = ss->PutData(payload, size);
	ss->Release();
	return i;
}

CDataInputStream::CDataInputStream(HRESULT *phr, CDataInputFilter *pParent, LPCWSTR pPinName)
: CSourceStream(NAME("CDataInputStream"), phr, pParent, pPinName)
, m_llFrameCount(0)

{
	CAutoLock cAutoLock(m_pFilter->pStateLock());
	ms_queue_init(&h264pkts);
	eos = 0;
	ms_mutex_init(&h264lock,NULL);
}

CDataInputStream::~CDataInputStream()
{
	ms_queue_flush(&h264pkts);
	ms_mutex_destroy(&h264lock);
}

HRESULT CDataInputStream::Notify(IBaseFilter * pSender, Quality q)
{
	return S_OK;
}

int CDataInputStream::PutData(unsigned char *payload, int size)
{
	//CAutoLock lock(m_pFilter->pStateLock());

	if (size==-1)
	{
		eos = 1;
		return 0;
	}

	mblk_t *om = allocb(size, 0);
	memcpy(om->b_wptr, payload, size);
	om->b_wptr += size;

	ms_mutex_lock(&h264lock);
	ms_queue_put(&h264pkts, om);
	ms_mutex_unlock(&h264lock);

	return 0;
}


HRESULT CDataInputStream::GetMediaType(CMediaType *pMediaType)
{
	CAutoLock lock(m_pFilter->pStateLock());
	ZeroMemory(pMediaType, sizeof(CMediaType));
	{
		VIDEOINFO *pvi = (VIDEOINFO *)pMediaType->AllocFormatBuffer(sizeof(VIDEOINFO));
		if (NULL == pvi) 
			return E_OUTOFMEMORY;

		ZeroMemory(pvi, sizeof(VIDEOINFO));

		pvi->bmiHeader.biCompression	= BI_RGB;
		pvi->bmiHeader.biBitCount		= 24;
		pvi->bmiHeader.biSize			= sizeof(BITMAPINFOHEADER);
		pvi->bmiHeader.biWidth			= 320;
		pvi->bmiHeader.biHeight			= 240;
		pvi->bmiHeader.biPlanes			= 1;
		pvi->bmiHeader.biSizeImage		= GetBitmapSize(&pvi->bmiHeader);
		pvi->bmiHeader.biClrImportant	= 0;

		SetRectEmpty(&(pvi->rcSource));	// we want the whole image area rendered.
		SetRectEmpty(&(pvi->rcTarget));	// no particular destination rectangle

		pMediaType->SetType(&MEDIATYPE_Video);
		pMediaType->SetFormatType(&FORMAT_VideoInfo);
		pMediaType->SetTemporalCompression(FALSE);

		const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
		pMediaType->SetSubtype(&SubTypeGUID);
		pMediaType->SetSampleSize(pvi->bmiHeader.biSizeImage);

		m_bmpInfo.bmiHeader = pvi->bmiHeader;
	}

	return S_OK;
}

HRESULT CDataInputStream::DecideBufferSize(IMemAllocator *pMemAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
	CAutoLock cAutoLock(m_pFilter->pStateLock());
	ASSERT(pMemAlloc);
	ASSERT(pProperties);

	HRESULT hr;

	// TODO: set the properties
	{
		VIDEOINFO *pvi = (VIDEOINFO *)m_mt.Format();
		pProperties->cBuffers = 3;
		pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;
	}

	// Ask the allocator to reserve us some sample memory, NOTE the function
	// can succeed (that is return NOERROR) but still not have allocated the
	// memory that we requested, so we must check we got whatever we wanted

	ALLOCATOR_PROPERTIES Actual;
	hr = pMemAlloc->SetProperties(pProperties, &Actual);
	if (FAILED(hr)) 
	{
		return hr;
	}

	// Is this allocator unsuitable
	if (Actual.cbBuffer < pProperties->cbBuffer) 
	{
		return E_FAIL;
	}

	return S_OK;
}

int nFrameRate = 0;


HRESULT CDataInputStream::FillBuffer(IMediaSample *pSample)
{
	mblk_t *im;
	ms_mutex_lock(&h264lock);
	im=ms_queue_get(&h264pkts);
	ms_mutex_unlock(&h264lock);
	while (im==NULL && eos==0) {
		Sleep(30);
		ms_mutex_lock(&h264lock);
		im=ms_queue_get(&h264pkts);
		ms_mutex_unlock(&h264lock);
	}

	if (eos==1)
	{
		freemsg(im);
		return S_FALSE;
	}

	CAutoLock lock(m_pFilter->pStateLock());

	HRESULT hr;
	BYTE *pBuffer;

	hr = pSample->GetPointer(&pBuffer);
	if (SUCCEEDED(hr))
	{
		if (pSample->GetSize()>msgdsize(im))
		{
			printf("received H264 data %i %i %i\n", msgdsize(im), pSample->GetSize(), pSample->GetActualDataLength());

			CopyMemory(pBuffer,im->b_rptr,msgdsize(im));
			pSample->SetActualDataLength(msgdsize(im));
		}
		else
		{
			printf("no H264 data %i %i\n", pSample->GetSize(), pSample->GetActualDataLength());
			pSample->SetActualDataLength(0);
		}
	}
	freemsg(im);

	return S_OK;
}

HRESULT CDataInputStream::OnThreadCreate(void)
{
	return CSourceStream::OnThreadCreate();
}

HRESULT CDataInputStream::OnThreadDestroy(void)
{
	return CSourceStream::OnThreadDestroy();
}

HRESULT CDataInputStream::OnThreadStartPlay(void)
{
	return CSourceStream::OnThreadStartPlay();
}

#endif
