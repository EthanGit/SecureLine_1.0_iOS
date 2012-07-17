/*
  The amsip program is a modular SIP softphone (SIP -rfc3261-)
  Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>
*/

#ifdef WIN32
#include <dshow.h>
#endif

#include <amsiptools/webcam_settings.h>

int amsiptools_open_webcam_settings(char *name)
{
#ifdef WIN32
  // Initialize COM
  IGraphBuilder *m_pGraph=NULL;
  ICaptureGraphBuilder2 *m_pBuilder=NULL;
  IMediaControl *m_pControl=NULL;
  IBaseFilter *m_pDeviceFilter=NULL;

  CoInitialize(NULL);

  // get a Graph
  HRESULT hr= CoCreateInstance (CLSID_FilterGraph,
	  NULL,
	  CLSCTX_INPROC_SERVER,
	  IID_IGraphBuilder, //IID_IBaseFilter,
	  (void **)&m_pGraph);
  if(FAILED(hr))
  {
	  CoUninitialize();
	  return -1;
  }

  // get a CaptureGraphBuilder2
  hr= CoCreateInstance (CLSID_CaptureGraphBuilder2,
	  NULL,
	  CLSCTX_INPROC_SERVER,
	  IID_ICaptureGraphBuilder2, //IID_IBaseFilter,
	  (void **)&m_pBuilder);
  if(FAILED(hr))
  {
	  if (m_pGraph)
		  m_pGraph->Release();

	  m_pDeviceFilter=NULL;
	  m_pBuilder=NULL;
	  m_pGraph=NULL;

	  CoUninitialize();
	  return -1;
  }

  // connect capture graph builder with the graph
  m_pBuilder->SetFiltergraph(m_pGraph);

  // get mediacontrol so we can start and stop the filter graph
  hr=m_pGraph->QueryInterface (IID_IMediaControl, (void **)&m_pControl);
  if(FAILED(hr))
  {
	  if (m_pBuilder)
		  m_pBuilder->Release();
	  if (m_pControl)
		  m_pControl->Release();
	  if (m_pGraph)
		  m_pGraph->Release();

	  m_pDeviceFilter=NULL;
	  m_pBuilder=NULL;
	  m_pGraph=NULL;

	  CoUninitialize();
	  return -1;
  }


  ICreateDevEnum *pCreateDevEnum = NULL;
  IEnumMoniker *pEnumMoniker = NULL;
  IMoniker *pMoniker = NULL;

  ULONG nFetched = 0;

  hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, 
	  IID_ICreateDevEnum, (PVOID *)&pCreateDevEnum);
  if(FAILED(hr))
  {
	  if (m_pBuilder)
		  m_pBuilder->Release();
	  if (m_pControl)
		  m_pControl->Release();
	  if (m_pGraph)
		  m_pGraph->Release();

	  m_pDeviceFilter=NULL;
	  m_pBuilder=NULL;
	  m_pControl=NULL;
	  m_pGraph=NULL;

	  CoUninitialize();
	  return -1;
  }

  hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
	  &pEnumMoniker, 0);
  if (FAILED(hr) || pEnumMoniker == NULL) {
	  pCreateDevEnum->Release();

	  if (m_pBuilder)
		  m_pBuilder->Release();
	  if (m_pControl)
		  m_pControl->Release();
	  if (m_pGraph)
		  m_pGraph->Release();

	  m_pDeviceFilter=NULL;
	  m_pBuilder=NULL;
	  m_pControl=NULL;
	  m_pGraph=NULL;

	  CoUninitialize();
	  return -1;
  }

  pEnumMoniker->Reset();

  int pos=0;
  while(S_OK == pEnumMoniker->Next(1, &pMoniker, &nFetched) )
  {
	  IPropertyBag *pBag;
	  hr = pMoniker->BindToStorage( 0, 0, IID_IPropertyBag, (void**) &pBag );
	  if( hr != S_OK )
		  continue; 

	  VARIANT var;
	  VariantInit(&var);
	  hr = pBag->Read( L"FriendlyName", &var, NULL ); 
	  if( hr != S_OK )
	  {
		  pMoniker->Release();
		  continue;
	  }
	  //USES_CONVERSION;
	  char szName[256];

	  WideCharToMultiByte(CP_UTF8,0,var.bstrVal,-1,szName,256,0,0);
	  VariantClear(&var); 

	  if (strcmp(szName, name)==0)
		  break;

	  pMoniker->Release();
	  pBag->Release();
	  pMoniker=NULL;
	  pBag=NULL;
  }

  if(pMoniker==NULL)
  {
	  pEnumMoniker->Release();
	  pCreateDevEnum->Release();

	  if (m_pBuilder)
		  m_pBuilder->Release();
	  if (m_pControl)
		  m_pControl->Release();
	  if (m_pGraph)
		  m_pGraph->Release();

	  m_pDeviceFilter=NULL;
	  m_pBuilder=NULL;
	  m_pControl=NULL;
	  m_pGraph=NULL;

	  CoUninitialize();
	  return -1;
  }

  hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&m_pDeviceFilter );

  pMoniker->Release();
  pEnumMoniker->Release();
  pCreateDevEnum->Release();

  if(FAILED(hr))
  {
	  if (m_pBuilder)
		  m_pBuilder->Release();
	  if (m_pControl)
		  m_pControl->Release();
	  if (m_pGraph)
		  m_pGraph->Release();

	  m_pDeviceFilter=NULL;
	  m_pBuilder=NULL;
	  m_pControl=NULL;
	  m_pGraph=NULL;

	  CoUninitialize();
	  return -1;
  }

  if (m_pDeviceFilter!=NULL)
  {
	  ISpecifyPropertyPages *pSpec;
	  CAUUID cauuid;
	  HRESULT hr;

	  hr = m_pDeviceFilter->QueryInterface(IID_ISpecifyPropertyPages,
		  (void **)&pSpec);
	  if(hr == S_OK)
	  {
		  hr = pSpec->GetPages(&cauuid);

		  hr = OleCreatePropertyFrame(NULL, 30, 30, NULL, 1,
			  (IUnknown **)&m_pDeviceFilter, cauuid.cElems,
			  (GUID *)cauuid.pElems, 0, 0, NULL);

		  CoTaskMemFree(cauuid.pElems);
		  pSpec->Release();
	  }
  }

  if (m_pBuilder)
	  m_pBuilder->Release();
  if (m_pControl)
	  m_pControl->Release();
  if (m_pGraph)
	  m_pGraph->Release();

  m_pDeviceFilter=NULL;
  m_pBuilder=NULL;
  m_pControl=NULL;
  m_pGraph=NULL;

  CoUninitialize();
#endif

  return 0;
}