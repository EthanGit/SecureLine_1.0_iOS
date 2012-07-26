
#if !defined(DXIFILTER_H)
#define DXIFILTER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <mediastreamer2/msqueue.h>
#include <mediastreamer2/msfilter.h>
#include <mediastreamer2/mscommon.h>

DEFINE_GUID(CLSID_DataInputFilter, 
0x92e9bf2a, 0x4385, 0x4a08, 0x82, 0x36, 0x39, 0x74, 0x78, 0xeb, 0x34, 0xb8);

IPin* MyFindPin(IBaseFilter* pFilter, PIN_DIRECTION Dir, LPCSTR pszPinName, 
                           BOOL wildMatch);

class CDataInputStream;

class CDataInputFilter : public CSource
{
	friend class CDataInputStream;

public:
	CDataInputFilter(LPUNKNOWN pUnk, HRESULT *phr);
	virtual ~CDataInputFilter(void);
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);
	int PutData(unsigned char *payload, int size);

protected:
	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
	{
		return CSource::NonDelegatingQueryInterface(riid, ppv);
	}

};


class CDataInputStream : public CSourceStream
{
public:
	CDataInputStream(HRESULT *phr, CDataInputFilter *pParent, LPCWSTR pPinName);
	virtual ~CDataInputStream(void);
	HRESULT Notify(IBaseFilter * pSender, Quality q);

	int PutData(unsigned char *payload, int size);

public:
	MSQueue h264pkts;
	int eos;
	ms_mutex_t h264lock;

	LONGLONG			m_llFrameCount;

	BITMAPINFO		m_bmpInfo;
	void			*m_pPaintBuffer;
	HDC				m_dcPaint;
	int				m_nLastX;
	int				m_nLastY;
	int				m_nScoreBoardHeight;
	int				m_nSnakeBlockHeight;
	int				m_nSnakeBlockWidth;
	int				m_nNumberSnakeBlocks;
	int				m_nSpaceBetweenBlock;

protected:
	virtual HRESULT GetMediaType(CMediaType *pMediaType);
	virtual HRESULT DecideBufferSize(IMemAllocator *pMemAlloc, ALLOCATOR_PROPERTIES *pProperties);
	virtual HRESULT FillBuffer(IMediaSample *pSample);

	virtual HRESULT OnThreadCreate(void);
	virtual HRESULT OnThreadDestroy(void);
	virtual HRESULT OnThreadStartPlay(void);

	//virtual HRESULT DoBufferProcessingLoop(void);

};


#endif // !defined(DXIFILTER_H)
