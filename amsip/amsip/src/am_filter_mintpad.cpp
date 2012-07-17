/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
  Copyright (C) 2003-2011  Aymeric MOIZARD - <amoizard@gmail.com>
*/

#if defined(_WIN32_WCE) && defined(ENABLE_VIDEO)

#include "amsip/am_options.h"
#include <osipparser2/osip_port.h>

#include <math.h>

#include <mediastreamer2/msfilter.h>
#include <mediastreamer2/rfc3984.h>
#include <mediastreamer2/msvideo.h>
#include <mediastreamer2/msticker.h>

#if defined(__cplusplus)
#define B64_NO_NAMESPACE
#endif
#include <ortp/b64.h>

#include <TCHAR.h>
#include <errors.h>
#include <streams.h>
#include <initguid.h>
#include <strmif.h>
#include <stdio.h>
#include <basetsd.h>
#include <ddraw.h>
#include <mpconfig.h>
#include <vptype.h>
#include <vpnotify.h>
//#include <atlbase.h>

#ifdef ENABLED_MX31

#include "FSLGuids.h"
#include "dxifilter.h"

DEFINE_GUID(GUID_NULL, 0x00000000L, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00);

typedef struct
{
	IGraphBuilder *m_pGraph;
	CDataInputFilter *mpInputDataFilter;
	IBaseFilter *mpTCCH264DecFilter;
	IBaseFilter *mpTCCVrendFilter;

} DecodeSession;

typedef struct _DecData{
	mblk_t *yuv_msg;
	mblk_t *sps,*pps;
	Rfc3984Context unpacker;
	MSPicture outbuf;
	struct SwsContext *sws_ctx;
	unsigned int packet_num;
	uint8_t *bitstream;
	int bitstream_size;

	//VrenderModeFlag flag;
	DecodeSession state;

}DecData;


static int tcc_init(DecData *d){
	// Initialize COM
	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	// get a Graph
	HRESULT hr= CoCreateInstance (CLSID_FilterGraph,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IGraphBuilder,
		(void **)&d->state.m_pGraph);
	if(FAILED(hr))
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: CLSID_FilterGraph failed\r\n"));
		return -1;
	}

	hr = CoCreateInstance((REFCLSID) CLSID_H264DecoderWrapper, NULL, CLSCTX_INPROC, (REFIID) IID_IBaseFilter,
		(void **) &d->state.mpTCCH264DecFilter);
	if(FAILED(hr))
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: CLSID_H264DecoderWrapper failed\r\n"));
		return -2;
	}
	hr = d->state.m_pGraph->AddFilter(d->state.mpTCCH264DecFilter, L"FSL- Direct Rending H264 Decoder Wrapper");
	if (FAILED(hr))
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: AddFilter CLSID_H264DecoderWrapper failed\r\n"));
		return -2;
	}

	d->state.mpInputDataFilter = new CDataInputFilter(NULL, &hr);
	if(FAILED(hr))
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: CLSID_InputDataFilter_Filter failed\r\n"));
		return -2;
	}
	hr = d->state.m_pGraph->AddFilter(d->state.mpInputDataFilter, L"Input Data Filter");
	if (FAILED(hr))
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: AddFilter CLSID_InputDataFilter_Filter failed\r\n"));
		return -2;
	}

	hr = CoCreateInstance((REFCLSID) CLSID_FSLVDEC_DR_FILTER, NULL, CLSCTX_INPROC, (REFIID) IID_IBaseFilter,
		(void **) &d->state.mpTCCVrendFilter);
	if(FAILED(hr))
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: CLSID_FSLVDEC_DR_FILTER failed\r\n"));
		return -2;
	}
	hr = d->state.m_pGraph->AddFilter(d->state.mpTCCVrendFilter, L"FSL Generic Video Decoder Renderer Filter");
	if (FAILED(hr))
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: AddFilter CLSID_FSLVDEC_DR_FILTER failed\r\n"));
		return -2;
	}

	IPin*    p_outpin = NULL;
	IPin*    p_inpin = NULL;

	//InputDataFilter -> H264
	p_outpin = MyFindPin(d->state.mpInputDataFilter, PINDIR_OUTPUT, "Out", FALSE);
	if (p_outpin==NULL)
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: output pin for mpInputDataFilter not found\r\n"));
		return -2;
	}

	p_inpin = MyFindPin(d->state.mpTCCH264DecFilter, PINDIR_INPUT, "XForm In", FALSE);
	if (p_inpin==NULL)
	{
		p_outpin->Release();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: input pin for mpTCCH264DecFilter not found\r\n"));
		return -2;
	}

	hr = d->state.m_pGraph->Connect(p_outpin, p_inpin);
	p_outpin->Release();
	p_inpin->Release();
	if (FAILED(hr))
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: Connect InputDataFilter -> H264 failed\r\n"));
		return -2;
	}

	// H264 -> VRend
	p_outpin = MyFindPin(d->state.mpTCCH264DecFilter, PINDIR_OUTPUT, "XForm Out", FALSE);
	if (p_outpin==NULL)
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: output pin for mpTCCH264DecFilter not found\r\n"));
		return -2;
	}
	p_inpin = MyFindPin(d->state.mpTCCVrendFilter, PINDIR_INPUT, "Input", FALSE);
	if (p_inpin==NULL)
	{
		p_outpin->Release();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: input pin for mpTCCVrendFilter not found\r\n"));
		return -2;
	}
	hr = d->state.m_pGraph->Connect(p_outpin, p_inpin);
	p_outpin->Release();
	p_inpin->Release();
	if (FAILED(hr))
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: Connect H264 -> VRend failed\r\n"));
		return -2;
	}

	return 0;
}

static int tcc_uninit(DecData *d){

	IPin*    p_outpin = NULL;
	IPin*    p_inpin = NULL;
	HRESULT  hr;

	if (d->state.m_pGraph==NULL)
		return 0;

	if (d->state.mpInputDataFilter!=NULL && d->state.mpTCCH264DecFilter!=NULL) {
		//InputDataFilter -> H264
		p_outpin = MyFindPin(d->state.mpInputDataFilter, PINDIR_OUTPUT, "Out", FALSE);
		if (p_outpin!=NULL)
		{
			hr = d->state.m_pGraph->Disconnect(p_outpin);
			p_inpin = MyFindPin(d->state.mpTCCH264DecFilter, PINDIR_INPUT, "XForm In", FALSE);
			if (p_inpin!=NULL)
			{
				hr = d->state.m_pGraph->Disconnect(p_inpin);
				p_inpin->Release();
			}
			p_outpin->Release();
		}
	}
	if (d->state.mpTCCH264DecFilter!=NULL && d->state.mpTCCVrendFilter!=NULL) {
		// H264 -> VRend
		p_outpin = MyFindPin(d->state.mpTCCH264DecFilter, PINDIR_OUTPUT, "XForm Out", FALSE);
		if (p_outpin!=NULL)
		{
			hr = d->state.m_pGraph->Disconnect(p_outpin);
			p_inpin = MyFindPin(d->state.mpTCCVrendFilter, PINDIR_INPUT, "Input", FALSE);
			if (p_inpin!=NULL)
			{
				hr = d->state.m_pGraph->Disconnect(p_inpin);
				p_inpin->Release();
			}
			p_outpin->Release();
		}
	}


	if (d->state.mpTCCVrendFilter!=NULL)
	{
		hr = d->state.m_pGraph->RemoveFilter(d->state.mpTCCVrendFilter);
		if (FAILED(hr))
		{
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
				"tcc_uninit: RemoveFilter mpTCCVrendFilter failed\r\n"));
			return -2;
		}
		d->state.mpTCCVrendFilter->Release();
		d->state.mpTCCVrendFilter = NULL;
	}

	if (d->state.mpTCCH264DecFilter!=NULL)
	{
		hr = d->state.m_pGraph->RemoveFilter(d->state.mpTCCH264DecFilter);
		if (FAILED(hr))
		{
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
				"tcc_uninit: RemoveFilter mpTCCH264DecFilter failed\r\n"));
			return -2;
		}
		d->state.mpTCCH264DecFilter->Release();
		d->state.mpTCCH264DecFilter = NULL;
	}

	if (d->state.mpInputDataFilter!=NULL)
	{
		hr = d->state.m_pGraph->RemoveFilter(d->state.mpInputDataFilter);
		if (FAILED(hr))
		{
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
				"tcc_uninit: RemoveFilter mpInputDataFilter failed\r\n"));
			return -2;
		}
		delete d->state.mpInputDataFilter;
		d->state.mpInputDataFilter = NULL;
	}

	d->state.m_pGraph->Release();
	d->state.m_pGraph = NULL;

	return 0;
}

static void dec_close(DecData *d){

	HRESULT hr;

	if (d->state.m_pGraph==NULL)
		return;

	if (d->state.mpInputDataFilter!=NULL)
		d->state.mpInputDataFilter->PutData(NULL, -1);

	IMediaControl* pMC = NULL;
	hr = d->state.m_pGraph->QueryInterface(IID_IMediaControl, (void **)&pMC);
	if (SUCCEEDED(hr))
	{
		hr = pMC->Stop();
		pMC->Release();
	}

}

static int dec_open(MSFilter *f, DecData *d){

	HRESULT hr;

	if (d->state.m_pGraph==NULL)
	{
		return -1;
	}

#if 0
	// Start Display YUV layer
	d->flag.mode = 0; // alpha and chromakey mode for overlay
	d->flag.alpha = 0; //60; // alpha value
	d->flag.rotate = 3; // rotate vlaue (defalut 3)
	d->flag.left = 17; // display x point
	d->flag.top = 16; // display y point
	d->flag.width = MS_VIDEO_SIZE_QCIF_W; // 320; // display width
	d->flag.height = MS_VIDEO_SIZE_QCIF_H; //240; // display height

	/* ++ TELECHIPS SHKANG 070126 : */
	ITccVren* pIVren = NULL;
	hr = d->state.mpTCCVrendFilter->QueryInterface(IID_ITccVren, (void**)&pIVren);
	if (FAILED(hr))
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"dec_open: failed to configure CLSID_TCCVREND\r\n"));
		return -1;
	}

	if (pIVren) {
		hr = pIVren->SetModeFlag(d->flag);
		pIVren->Release();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_INFO2, NULL,
			"dec_open: CLSID_TCCVREND configured\r\n"));
	}
#endif

	IMediaControl* pMC = NULL;
	hr = d->state.m_pGraph->QueryInterface(IID_IMediaControl, (void **)&pMC);
	if (FAILED(hr))
	{
		return -1;
	}

	hr = pMC->Run();
	if (FAILED(hr))
	{
		pMC->Release();
		return -1;
	}

	pMC->Release();
	return 0;
}

static void dec_init(MSFilter *f){
	DecData *d=(DecData*)ms_new(DecData,1);
	int i;

	memset(&d->state, 0, sizeof(d->state));
	//memset(&d->flag, 0, sizeof(d->flag));
	i = tcc_init(d);
	if (i!=0)
	{
		printf("dec_init: failed to initialize H264 decoder");
		tcc_uninit(d);
		memset(&d->state, 0, sizeof(d->state)); /* just to be sure */
	}
	i = dec_open(f, d);
	if (i!=0)
	{
		printf("dec_init: failed to configure H264 decoder");
		dec_close(d);
		tcc_uninit(d);
		memset(&d->state, 0, sizeof(d->state)); /* just to be sure */
	}

	d->yuv_msg=NULL;
	d->sps=NULL;
	d->pps=NULL;

	d->sws_ctx=NULL;
	rfc3984_init(&d->unpacker);
	d->packet_num=0;
	d->outbuf.w=0;
	d->outbuf.h=0;
	d->bitstream_size=65536;
	d->bitstream=(uint8_t*)ms_malloc0(d->bitstream_size);
	f->data=d;
}

static void dec_reinit(MSFilter *f, DecData *d){

	return;
}

static void dec_uninit(MSFilter *f){
	DecData *d=(DecData*)f->data;
	rfc3984_uninit(&d->unpacker);
	dec_close(d);
	tcc_uninit(d);
	if (d->yuv_msg) freemsg(d->yuv_msg);
	if (d->sps) freemsg(d->sps);
	if (d->pps) freemsg(d->pps);
	ms_free(d->bitstream);
	ms_free(d);
}

static void update_sps(DecData *d, mblk_t *sps){
	if (d->sps)
		freemsg(d->sps);
	d->sps=NULL;
	if (sps)
		d->sps=dupb(sps);
}

static void update_pps(DecData *d, mblk_t *pps){
	if (d->pps)
		freemsg(d->pps);
	d->pps=NULL;
	if (pps)
		d->pps=dupb(pps);
}

static bool_t check_sps_pps_change(DecData *d, mblk_t *sps, mblk_t *pps){
	bool_t ret1=FALSE,ret2=FALSE;
	if (d->sps){
		if (sps){
			ret1=(msgdsize(sps)!=msgdsize(d->sps)) || (memcmp(d->sps->b_rptr,sps->b_rptr,msgdsize(sps))!=0);
			if (ret1) {
				update_sps(d,sps);
				ms_message("SPS changed !");
				update_pps(d,NULL);
			}
		}
	}else if (sps) {
		ms_message("Receiving first SPS");
		update_sps(d,sps);
	}
	if (d->pps){
		if (pps){
			ret2=(msgdsize(pps)!=msgdsize(d->pps)) || (memcmp(d->pps->b_rptr,pps->b_rptr,msgdsize(pps))!=0);
			if (ret2) {
				update_pps(d,pps);
				ms_message("PPS changed ! %i,%i",msgdsize(pps),msgdsize(d->pps));
			}
		}
	}else if (pps) {
		ms_message("Receiving first PPS");
		update_pps(d,pps);
	}
	return ret1 || ret2;
}

static void enlarge_bitstream(DecData *d, int new_size){
	d->bitstream_size=new_size;
	d->bitstream=(uint8_t*)ms_realloc(d->bitstream,d->bitstream_size);
}

static int nalusToFrame(DecData *d, MSQueue *naluq, bool_t *new_sps_pps){
	mblk_t *im;
	uint8_t *dst=d->bitstream,*src,*end;
	int nal_len;
	bool_t start_picture=TRUE;
	uint8_t nalu_type;
	*new_sps_pps=FALSE;
	end=d->bitstream+d->bitstream_size;
	while((im=ms_queue_get(naluq))!=NULL){
		src=im->b_rptr;
		nal_len=im->b_wptr-src;
		if (dst+nal_len+100>end){
			int pos=dst-d->bitstream;
			enlarge_bitstream(d, d->bitstream_size+nal_len+100);
			dst=d->bitstream+pos;
			end=d->bitstream+d->bitstream_size;
		}
		nalu_type=(*src) & ((1<<5)-1);
		if (nalu_type==7)
			*new_sps_pps=check_sps_pps_change(d,im,NULL) || *new_sps_pps;
		if (nalu_type==8)
			*new_sps_pps=check_sps_pps_change(d,NULL,im) || *new_sps_pps;
		if (start_picture || nalu_type==7/*SPS*/ || nalu_type==8/*PPS*/ ){
			*dst++=0;
			start_picture=FALSE;
		}
		/*prepend nal marker*/
		*dst++=0;
		*dst++=0;
		*dst++=1;
		*dst++=*src++;
		while(src<(im->b_wptr-3)){
			if (src[0]==0 && src[1]==0 && src[2]<3){
				*dst++=0;
				*dst++=0;
				*dst++=3;
				src+=2;
			}
			*dst++=*src++;
		}
		*dst++=*src++;
		*dst++=*src++;
		*dst++=*src++;
		freemsg(im);
	}
	return dst-d->bitstream;
}

static void dec_process(MSFilter *f){
	DecData *d=(DecData*)f->data;
	mblk_t *im;
	MSQueue nalus;
	//AVFrame orig;

	ms_queue_init(&nalus);
	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		uint32_t ts = mblk_get_timestamp_info(im);
		/*push the sps/pps given in sprop-parameter-sets if any*/
		if (d->packet_num==0 && d->sps && d->pps){
			mblk_set_timestamp_info(d->sps,mblk_get_timestamp_info(im));
			mblk_set_timestamp_info(d->pps,mblk_get_timestamp_info(im));
			rfc3984_unpack(&d->unpacker,d->sps,&nalus);
			rfc3984_unpack(&d->unpacker,d->pps,&nalus);
			d->sps=NULL;
			d->pps=NULL;
		}
		rfc3984_unpack(&d->unpacker,im,&nalus);
		if (!ms_queue_empty(&nalus)){
			int size;
			uint8_t *p,*end;
			bool_t need_reinit=FALSE;

			size=nalusToFrame(d,&nalus,&need_reinit);
			if (need_reinit)
			{

			}
			p=d->bitstream;
			end=d->bitstream+size;

			while (end-p>0) {
				//CUresult err;
				//CUVIDSOURCEDATAPACKET pkt;
				//pkt.flags = CUVID_PKT_TIMESTAMP;
				//pkt.payload_size = end-p;
				//pkt.payload = p;
				//pkt.timestamp = ts;  // not using timestamps
				//err = cuvidParseVideoData(d->state.cuParser, &pkt);

				//if (err<0) {
				//	ms_error("cuvidParseVideoData: error %i.",err);
				//	break;
				//}
				//do it in callback ms_queue_put(f->outputs[0],get_as_yuvmsg(f,d,&orig));



				d->state.mpInputDataFilter->PutData(p, end-p);

				//InputDataFilter_PushData(d->state.mpInputDataFilter, end-p, p);

				p+=end-p;
			}
		}
		d->packet_num++;
	}
}


static int dec_add_fmtp(MSFilter *f, void *arg){
	DecData *d=(DecData*)f->data;
	const char *fmtp=(const char *)arg;
	char value[256];
	if (fmtp_get_value(fmtp,"sprop-parameter-sets",value,sizeof(value))){
		char * b64_sps=value;
		char * b64_pps=strchr(value,',');
		if (b64_pps){
			*b64_pps='\0';
			++b64_pps;
			ms_message("Got sprop-parameter-sets : sps=%s , pps=%s",b64_sps,b64_pps);
			d->sps=allocb(sizeof(value),0);
			d->sps->b_wptr+=b64_decode((const char *)b64_sps,(size_t)strlen(b64_sps),(void *)d->sps->b_wptr,(size_t)sizeof(value));
			d->pps=allocb(sizeof(value),0);
			d->pps->b_wptr+=b64_decode(b64_pps,strlen(b64_pps),d->pps->b_wptr,sizeof(value));
		}
	}
	return 0;
}

static MSFilterMethod  h264_dec_methods[]={
	{	MS_FILTER_ADD_FMTP	,	dec_add_fmtp	},
	{	0			,	NULL	}
};

static MSFilterDesc mintpad_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSH264MINTPADDec",
	"A H264 hardware decoder based on MINTPAD.",
	MS_FILTER_DECODER,
	"H264",
	1,
	1,
	dec_init,
	NULL,
	dec_process,
	NULL,
	dec_uninit,
	h264_dec_methods
};


extern "C" void am_filter_register_h264(void)
{
	ms_filter_register(&mintpad_dec_desc);
}

#else

#include "FilterGuid.h"
#include "Typedef.h"

#include "iTccTune.h"
#include "iTccH264.h"
#include "VrendModeFlag.h"

#include "dxifilter.h"

DEFINE_GUID(GUID_NULL, 0x00000000L, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00);

typedef struct
{
	IGraphBuilder *m_pGraph;
	CDataInputFilter *mpInputDataFilter;
	IBaseFilter *mpTCCH264DecFilter;
	IBaseFilter *mpTCCVrendFilter;

} DecodeSession;

typedef struct _DecData{
	mblk_t *yuv_msg;
	mblk_t *sps,*pps;
	Rfc3984Context unpacker;
	MSPicture outbuf;
	struct SwsContext *sws_ctx;
	unsigned int packet_num;
	uint8_t *bitstream;
	int bitstream_size;

	VrenderModeFlag flag;
	DecodeSession state;

}DecData;


static int tcc_init(DecData *d){
	// Initialize COM
	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	// get a Graph
	HRESULT hr= CoCreateInstance (CLSID_FilterGraph,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IGraphBuilder,
		(void **)&d->state.m_pGraph);
	if(FAILED(hr))
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: CLSID_FilterGraph failed\r\n"));
		return -1;
	}

	hr = CoCreateInstance((REFCLSID) CLSID_H264DEC, NULL, CLSCTX_INPROC, (REFIID) IID_IBaseFilter,
		(void **) &d->state.mpTCCH264DecFilter);
	if(FAILED(hr))
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: CLSID_H264DEC failed\r\n"));
		return -2;
	}
	hr = d->state.m_pGraph->AddFilter(d->state.mpTCCH264DecFilter, L"TCC H264Dec Filter");
	if (FAILED(hr))
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: AddFilter CLSID_H264DEC failed\r\n"));
		return -2;
	}

	d->state.mpInputDataFilter = new CDataInputFilter(NULL, &hr);
	if(FAILED(hr))
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: CLSID_InputDataFilter_Filter failed\r\n"));
		return -2;
	}
	hr = d->state.m_pGraph->AddFilter(d->state.mpInputDataFilter, L"Input Data Filter");
	if (FAILED(hr))
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: AddFilter CLSID_InputDataFilter_Filter failed\r\n"));
		return -2;
	}

	hr = CoCreateInstance((REFCLSID) CLSID_TCCVREND, NULL, CLSCTX_INPROC, (REFIID) IID_IBaseFilter,
		(void **) &d->state.mpTCCVrendFilter);
	if(FAILED(hr))
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: CLSID_TCCVREND failed\r\n"));
		return -2;
	}
	hr = d->state.m_pGraph->AddFilter(d->state.mpTCCVrendFilter, L"TCC Video Renderer");
	if (FAILED(hr))
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: AddFilter CLSID_TCCVREND failed\r\n"));
		return -2;
	}

	IPin*    p_outpin = NULL;
	IPin*    p_inpin = NULL;

	//InputDataFilter -> H264
	p_outpin = MyFindPin(d->state.mpInputDataFilter, PINDIR_OUTPUT, "Out", FALSE);
	if (p_outpin==NULL)
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: output pin for mpInputDataFilter not found\r\n"));
		return -2;
	}

	p_inpin = MyFindPin(d->state.mpTCCH264DecFilter, PINDIR_INPUT, "XForm In", FALSE);
	if (p_inpin==NULL)
	{
		p_outpin->Release();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: input pin for mpTCCH264DecFilter not found\r\n"));
		return -2;
	}

	hr = d->state.m_pGraph->Connect(p_outpin, p_inpin);
	p_outpin->Release();
	p_inpin->Release();
	if (FAILED(hr))
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: Connect InputDataFilter -> H264 failed\r\n"));
		return -2;
	}

	// H264 -> VRend
	p_outpin = MyFindPin(d->state.mpTCCH264DecFilter, PINDIR_OUTPUT, "XForm Out", FALSE);
	if (p_outpin==NULL)
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: output pin for mpTCCH264DecFilter not found\r\n"));
		return -2;
	}
	p_inpin = MyFindPin(d->state.mpTCCVrendFilter, PINDIR_INPUT, "Input", FALSE);
	if (p_inpin==NULL)
	{
		p_outpin->Release();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: input pin for mpTCCVrendFilter not found\r\n"));
		return -2;
	}
	hr = d->state.m_pGraph->Connect(p_outpin, p_inpin);
	p_outpin->Release();
	p_inpin->Release();
	if (FAILED(hr))
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"tcc_init: Connect H264 -> VRend failed\r\n"));
		return -2;
	}

	return 0;
}

static int tcc_uninit(DecData *d){

	IPin*    p_outpin = NULL;
	IPin*    p_inpin = NULL;
	HRESULT  hr;

	if (d->state.m_pGraph==NULL)
		return 0;

	if (d->state.mpInputDataFilter!=NULL && d->state.mpTCCH264DecFilter!=NULL) {
		//InputDataFilter -> H264
		p_outpin = MyFindPin(d->state.mpInputDataFilter, PINDIR_OUTPUT, "Out", FALSE);
		if (p_outpin!=NULL)
		{
			hr = d->state.m_pGraph->Disconnect(p_outpin);
			p_inpin = MyFindPin(d->state.mpTCCH264DecFilter, PINDIR_INPUT, "XForm In", FALSE);
			if (p_inpin!=NULL)
			{
				hr = d->state.m_pGraph->Disconnect(p_inpin);
				p_inpin->Release();
			}
			p_outpin->Release();
		}
	}
	if (d->state.mpTCCH264DecFilter!=NULL && d->state.mpTCCVrendFilter!=NULL) {
		// H264 -> VRend
		p_outpin = MyFindPin(d->state.mpTCCH264DecFilter, PINDIR_OUTPUT, "XForm Out", FALSE);
		if (p_outpin!=NULL)
		{
			hr = d->state.m_pGraph->Disconnect(p_outpin);
			p_inpin = MyFindPin(d->state.mpTCCVrendFilter, PINDIR_INPUT, "Input", FALSE);
			if (p_inpin!=NULL)
			{
				hr = d->state.m_pGraph->Disconnect(p_inpin);
				p_inpin->Release();
			}
			p_outpin->Release();
		}
	}


	if (d->state.mpTCCVrendFilter!=NULL)
	{
		hr = d->state.m_pGraph->RemoveFilter(d->state.mpTCCVrendFilter);
		if (FAILED(hr))
		{
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
				"tcc_uninit: RemoveFilter mpTCCVrendFilter failed\r\n"));
			return -2;
		}
		d->state.mpTCCVrendFilter->Release();
		d->state.mpTCCVrendFilter = NULL;
	}

	if (d->state.mpTCCH264DecFilter!=NULL)
	{
		hr = d->state.m_pGraph->RemoveFilter(d->state.mpTCCH264DecFilter);
		if (FAILED(hr))
		{
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
				"tcc_uninit: RemoveFilter mpTCCH264DecFilter failed\r\n"));
			return -2;
		}
		d->state.mpTCCH264DecFilter->Release();
		d->state.mpTCCH264DecFilter = NULL;
	}

	if (d->state.mpInputDataFilter!=NULL)
	{
		hr = d->state.m_pGraph->RemoveFilter(d->state.mpInputDataFilter);
		if (FAILED(hr))
		{
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
				"tcc_uninit: RemoveFilter mpInputDataFilter failed\r\n"));
			return -2;
		}
		delete d->state.mpInputDataFilter;
		d->state.mpInputDataFilter = NULL;
	}

	d->state.m_pGraph->Release();
	d->state.m_pGraph = NULL;

	return 0;
}

static void dec_close(DecData *d){

	HRESULT hr;

	if (d->state.m_pGraph==NULL)
		return;

	if (d->state.mpInputDataFilter!=NULL)
		d->state.mpInputDataFilter->PutData(NULL, -1);

	IMediaControl* pMC = NULL;
	hr = d->state.m_pGraph->QueryInterface(IID_IMediaControl, (void **)&pMC);
	if (SUCCEEDED(hr))
	{
		hr = pMC->Stop();
		pMC->Release();
	}

}

static int dec_open(MSFilter *f, DecData *d){

	HRESULT hr;

	if (d->state.m_pGraph==NULL)
	{
		return -1;
	}

	// Start Display YUV layer
	d->flag.mode = 0; // alpha and chromakey mode for overlay
	d->flag.alpha = 0; //60; // alpha value
	d->flag.rotate = 3; // rotate vlaue (defalut 3)
	d->flag.left = 17; // display x point
	d->flag.top = 16; // display y point
	d->flag.width = MS_VIDEO_SIZE_QCIF_W; // 320; // display width
	d->flag.height = MS_VIDEO_SIZE_QCIF_H; //240; // display height

	/* ++ TELECHIPS SHKANG 070126 : */
	ITccVren* pIVren = NULL;
	hr = d->state.mpTCCVrendFilter->QueryInterface(IID_ITccVren, (void**)&pIVren);
	if (FAILED(hr))
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_ERROR, NULL,
			"dec_open: failed to configure CLSID_TCCVREND\r\n"));
		return -1;
	}

	if (pIVren) {
		hr = pIVren->SetModeFlag(d->flag);
		pIVren->Release();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, (osip_trace_level_t)OSIP_INFO2, NULL,
			"dec_open: CLSID_TCCVREND configured\r\n"));
	}

#if 0
	ITccH264 *pIH264 = NULL;
	hr = d->state.mpTCCH264DecFilter->QueryInterface(IID_ITccH264, (void**)&pIH264);
	if( SUCCEEDED(hr) && pIH264 )
	{
		int videomode=1; // DMB mode : 1 , File Play Mode 0 (Default)
		pIH264->SetInfo(ITCCH264_MODE, &videomode);
	}
	pIH264->Release();
#endif

	IMediaControl* pMC = NULL;
	hr = d->state.m_pGraph->QueryInterface(IID_IMediaControl, (void **)&pMC);
	if (FAILED(hr))
	{
		return -1;
	}

	hr = pMC->Run();
	if (FAILED(hr))
	{
		pMC->Release();
		return -1;
	}

	pMC->Release();
	return 0;
}

static void dec_init(MSFilter *f){
	DecData *d=(DecData*)ms_new(DecData,1);
	int i;

	memset(&d->state, 0, sizeof(d->state));
	memset(&d->flag, 0, sizeof(d->flag));
	i = tcc_init(d);
	if (i!=0)
	{
		printf("dec_init: failed to initialize H264 decoder");
		tcc_uninit(d);
		memset(&d->state, 0, sizeof(d->state)); /* just to be sure */
	}
	i = dec_open(f, d);
	if (i!=0)
	{
		printf("dec_init: failed to configure H264 decoder");
		dec_close(d);
		tcc_uninit(d);
		memset(&d->state, 0, sizeof(d->state)); /* just to be sure */
	}

	d->yuv_msg=NULL;
	d->sps=NULL;
	d->pps=NULL;

	d->sws_ctx=NULL;
	rfc3984_init(&d->unpacker);
	d->packet_num=0;
	d->outbuf.w=0;
	d->outbuf.h=0;
	d->bitstream_size=65536;
	d->bitstream=(uint8_t*)ms_malloc0(d->bitstream_size);
	f->data=d;
}

static void dec_reinit(MSFilter *f, DecData *d){

	return;
}

static void dec_uninit(MSFilter *f){
	DecData *d=(DecData*)f->data;
	rfc3984_uninit(&d->unpacker);
	dec_close(d);
	tcc_uninit(d);
	if (d->yuv_msg) freemsg(d->yuv_msg);
	if (d->sps) freemsg(d->sps);
	if (d->pps) freemsg(d->pps);
	ms_free(d->bitstream);
	ms_free(d);
}

static void update_sps(DecData *d, mblk_t *sps){
	if (d->sps)
		freemsg(d->sps);
	d->sps=NULL;
	if (sps)
		d->sps=dupb(sps);
}

static void update_pps(DecData *d, mblk_t *pps){
	if (d->pps)
		freemsg(d->pps);
	d->pps=NULL;
	if (pps)
		d->pps=dupb(pps);
}

static bool_t check_sps_pps_change(DecData *d, mblk_t *sps, mblk_t *pps){
	bool_t ret1=FALSE,ret2=FALSE;
	if (d->sps){
		if (sps){
			ret1=(msgdsize(sps)!=msgdsize(d->sps)) || (memcmp(d->sps->b_rptr,sps->b_rptr,msgdsize(sps))!=0);
			if (ret1) {
				update_sps(d,sps);
				ms_message("SPS changed !");
				update_pps(d,NULL);
			}
		}
	}else if (sps) {
		ms_message("Receiving first SPS");
		update_sps(d,sps);
	}
	if (d->pps){
		if (pps){
			ret2=(msgdsize(pps)!=msgdsize(d->pps)) || (memcmp(d->pps->b_rptr,pps->b_rptr,msgdsize(pps))!=0);
			if (ret2) {
				update_pps(d,pps);
				ms_message("PPS changed ! %i,%i",msgdsize(pps),msgdsize(d->pps));
			}
		}
	}else if (pps) {
		ms_message("Receiving first PPS");
		update_pps(d,pps);
	}
	return ret1 || ret2;
}

static void enlarge_bitstream(DecData *d, int new_size){
	d->bitstream_size=new_size;
	d->bitstream=(uint8_t*)ms_realloc(d->bitstream,d->bitstream_size);
}

static int nalusToFrame(DecData *d, MSQueue *naluq, bool_t *new_sps_pps){
	mblk_t *im;
	uint8_t *dst=d->bitstream,*src,*end;
	int nal_len;
	bool_t start_picture=TRUE;
	uint8_t nalu_type;
	*new_sps_pps=FALSE;
	end=d->bitstream+d->bitstream_size;
	while((im=ms_queue_get(naluq))!=NULL){
		src=im->b_rptr;
		nal_len=im->b_wptr-src;
		if (dst+nal_len+100>end){
			int pos=dst-d->bitstream;
			enlarge_bitstream(d, d->bitstream_size+nal_len+100);
			dst=d->bitstream+pos;
			end=d->bitstream+d->bitstream_size;
		}
		nalu_type=(*src) & ((1<<5)-1);
		if (nalu_type==7)
			*new_sps_pps=check_sps_pps_change(d,im,NULL) || *new_sps_pps;
		if (nalu_type==8)
			*new_sps_pps=check_sps_pps_change(d,NULL,im) || *new_sps_pps;
		if (start_picture || nalu_type==7/*SPS*/ || nalu_type==8/*PPS*/ ){
			*dst++=0;
			start_picture=FALSE;
		}
		/*prepend nal marker*/
		*dst++=0;
		*dst++=0;
		*dst++=1;
		*dst++=*src++;
		while(src<(im->b_wptr-3)){
			if (src[0]==0 && src[1]==0 && src[2]<3){
				*dst++=0;
				*dst++=0;
				*dst++=3;
				src+=2;
			}
			*dst++=*src++;
		}
		*dst++=*src++;
		*dst++=*src++;
		*dst++=*src++;
		freemsg(im);
	}
	return dst-d->bitstream;
}

static void dec_process(MSFilter *f){
	DecData *d=(DecData*)f->data;
	mblk_t *im;
	MSQueue nalus;
	//AVFrame orig;

	ms_queue_init(&nalus);
	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		uint32_t ts = mblk_get_timestamp_info(im);
		/*push the sps/pps given in sprop-parameter-sets if any*/
		if (d->packet_num==0 && d->sps && d->pps){
			mblk_set_timestamp_info(d->sps,mblk_get_timestamp_info(im));
			mblk_set_timestamp_info(d->pps,mblk_get_timestamp_info(im));
			rfc3984_unpack(&d->unpacker,d->sps,&nalus);
			rfc3984_unpack(&d->unpacker,d->pps,&nalus);
			d->sps=NULL;
			d->pps=NULL;
		}
		rfc3984_unpack(&d->unpacker,im,&nalus);
		if (!ms_queue_empty(&nalus)){
			int size;
			uint8_t *p,*end;
			bool_t need_reinit=FALSE;

			size=nalusToFrame(d,&nalus,&need_reinit);
			if (need_reinit)
			{

			}
			p=d->bitstream;
			end=d->bitstream+size;

			while (end-p>0) {
				//CUresult err;
				//CUVIDSOURCEDATAPACKET pkt;
				//pkt.flags = CUVID_PKT_TIMESTAMP;
				//pkt.payload_size = end-p;
				//pkt.payload = p;
				//pkt.timestamp = ts;  // not using timestamps
				//err = cuvidParseVideoData(d->state.cuParser, &pkt);

				//if (err<0) {
				//	ms_error("cuvidParseVideoData: error %i.",err);
				//	break;
				//}
				//do it in callback ms_queue_put(f->outputs[0],get_as_yuvmsg(f,d,&orig));



				d->state.mpInputDataFilter->PutData(p, end-p);

				//InputDataFilter_PushData(d->state.mpInputDataFilter, end-p, p);

				p+=end-p;
			}
		}
		d->packet_num++;
	}
}


static int dec_add_fmtp(MSFilter *f, void *arg){
	DecData *d=(DecData*)f->data;
	const char *fmtp=(const char *)arg;
	char value[256];
	if (fmtp_get_value(fmtp,"sprop-parameter-sets",value,sizeof(value))){
		char * b64_sps=value;
		char * b64_pps=strchr(value,',');
		if (b64_pps){
			*b64_pps='\0';
			++b64_pps;
			ms_message("Got sprop-parameter-sets : sps=%s , pps=%s",b64_sps,b64_pps);
			d->sps=allocb(sizeof(value),0);
			d->sps->b_wptr+=b64_decode((const char *)b64_sps,(size_t)strlen(b64_sps),(void *)d->sps->b_wptr,(size_t)sizeof(value));
			d->pps=allocb(sizeof(value),0);
			d->pps->b_wptr+=b64_decode(b64_pps,strlen(b64_pps),d->pps->b_wptr,sizeof(value));
		}
	}
	return 0;
}

static MSFilterMethod  h264_dec_methods[]={
	{	MS_FILTER_ADD_FMTP	,	dec_add_fmtp	},
	{	0			,	NULL	}
};

static MSFilterDesc mintpad_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSH264MINTPADDec",
	"A H264 hardware decoder based on MINTPAD.",
	MS_FILTER_DECODER,
	"H264",
	1,
	1,
	dec_init,
	NULL,
	dec_process,
	NULL,
	dec_uninit,
	h264_dec_methods
};


extern "C" void am_filter_register_h264(void)
{
	ms_filter_register(&mintpad_dec_desc);
}

#endif

#endif
