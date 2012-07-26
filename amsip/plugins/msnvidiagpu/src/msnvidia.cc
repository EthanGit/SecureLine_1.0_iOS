/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#include <math.h>

#include <mediastreamer2/msfilter.h>
#include <mediastreamer2/rfc3984.h>
#include <mediastreamer2/msvideo.h>
#include <mediastreamer2/msticker.h>

#if defined(__cplusplus)
#define B64_NO_NAMESPACE
#endif
#include <ortp/b64.h>

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <nvcuvid.h>

/* DECODER DEFINES */
#define DISPLAY_DELAY           4   /* Attempt to decode up to 4 frames ahead of display */
#define MAX_FRM_CNT             DISPLAY_DELAY+1+1
#define USE_ASYNC_COPY          0
#define USE_FLOATING_CONTEXTS   1   /* Use floating contexts */

/* ENCODER INCLUDE */
#include <NVEncoderAPI.h>
#include <NVEncodeDataTypes.h>
#include "types.h"
#include "drvapi_error_string.h"

CUdevice best_cuDevice;

/* Autolock for floating contexts */
class CAutoCtxLock
{
private:
    CUvideoctxlock m_lock;
public:
#if USE_FLOATING_CONTEXTS
    CAutoCtxLock(CUvideoctxlock lck) { m_lock=lck; cuvidCtxLock(m_lock, 0); }
    ~CAutoCtxLock() { cuvidCtxUnlock(m_lock, 0); }
#else
    CAutoCtxLock(CUvideoctxlock lck) { m_lock=lck; }
#endif
};


typedef struct _NVidiaEncState{
	NVEncoder enc;
	MSVideoSize vsize;
	int iAspectRatio[3];
	int bitrate;
	float fps;
	int mode;
	int profile_idc;
	uint64_t framenum;
	Rfc3984Context packer;
	mblk_t *sav_sps;
	mblk_t *sav_pps;
    int reset;

	unsigned char *h264_buf;
	int h264_buf_size;
}NVidiaEncState;

static void enc_init(MSFilter *f){
	NVidiaEncState *d=ms_new(NVidiaEncState,1);
	d->enc=NULL;
	d->vsize.width=MS_VIDEO_SIZE_CIF_W;
	d->vsize.height=MS_VIDEO_SIZE_CIF_H;
	d->bitrate=384000;
	d->fps=30;
	d->mode=0;
	d->profile_idc=13;
	d->framenum=0;
	d->sav_sps=NULL;
	d->sav_pps=NULL;
    d->reset=0;

	d->h264_buf_size=1024*10;
	d->h264_buf = (unsigned char*)ms_malloc(d->h264_buf_size);
	f->data=d;
}

static void enc_uninit(MSFilter *f){
	NVidiaEncState *d=(NVidiaEncState*)f->data;
	if (d->h264_buf!=NULL) {
		ms_free(d->h264_buf);
		d->h264_buf=NULL;
	}
	ms_free(d);
}

void  push_nalu(MSFilter *f, MSQueue *nalus, uint8_t *begin, uint8_t *end){
	NVidiaEncState *d=(NVidiaEncState*)f->data;

	mblk_t *m;
	uint8_t *src=begin;
	uint8_t nalu_type=(*begin) & ((1<<5)-1);
	m=allocb((int)(end-begin),0);
	memcpy(m->b_wptr,begin,end-begin);
	m->b_wptr+=(end-begin);
	if (nalu_type==5) {
		ms_message("msnvidiahw: A IDR is being sent.");
	}else if (nalu_type==7) {
		ms_message("msnvidiahw: A SPS is being sent.");
		if (d->sav_sps!=NULL)
			freeb(d->sav_sps);
		d->sav_sps=dupb(m);
	} else if (nalu_type==8) {
		ms_message("msnvidiahw: A PPS is being sent.");
		if (d->sav_pps!=NULL)
			freeb(d->sav_pps);
		d->sav_pps=dupb(m);
	}
	ms_queue_put(nalus,m);	
}

void  get_nalus(MSFilter *f, uint8_t *frame, int frame_size, MSQueue *nalus){
	int i;
	uint8_t *p,*begin=NULL;
	int zeroes=0;
	
	for(i=0,p=frame;i<frame_size;++i){
		if (*p==0){
			++zeroes;
		}else if (zeroes>=2 && *p==1 ){
			if (begin){
				push_nalu(f, nalus,begin,p-zeroes);
			}
			begin=p+1;
		}else zeroes=0;
		++p;
	}
	if (begin) push_nalu(f, nalus,begin,p);
}

/* NVCUVENC callback function to signal the start of bitstream that is to be encoded */
static unsigned char* _stdcall HandleAcquireBitStream(int *pBufferSize, void *pUserData)
{
	MSFilter *f=(MSFilter *)pUserData;
	NVidiaEncState *d=(NVidiaEncState*)f->data;

	*pBufferSize = d->h264_buf_size;
    return d->h264_buf;
}

/*NVCUVENC callback function to signal that the encoded bitstream is ready to be written to file */
static void _stdcall HandleReleaseBitStream(int nBytesInBuffer, unsigned char *cb,void *pUserData)
{
	MSFilter *f=(MSFilter *)pUserData;
	NVidiaEncState *d=(NVidiaEncState*)f->data;
	uint32_t ts=(uint32_t)(f->ticker->time*90LL);

	MSQueue nalus;
	ms_queue_init(&nalus);

	ms_filter_lock(f);
	get_nalus(f, cb, nBytesInBuffer ,&nalus);
	rfc3984_pack(&d->packer,&nalus,f->outputs[0],ts);
	ms_filter_unlock(f);
    return;
}

/*NVCUVENC callback function to signal that the encoding operation on the frame has started */
static void _stdcall HandleOnBeginFrame(const NVVE_BeginFrameInfo *pbfi, void *pUserData)
{
    return;
}

/*NVCUVENC callback function signals that the encoding operation on the frame has finished */
static void _stdcall HandleOnEndFrame(const NVVE_EndFrameInfo *pefi, void *pUserData)
{
    return;
}

static int gcd(int m, int n)
{
   if(n == 0)
     return m;
   else
     return gcd(n, m % n);
}
   
static void reduce(int *num, int *denom)
{
   int divisor = gcd(*num, *denom);
   *num /= divisor;
   *denom /= divisor;
}

#ifndef TUNE_RC_MODE
#define TUNE_RC_MODE RC_VBR
/*#define TUNE_RC_MODE RC_CBR */
#endif

static int enc_setup_param(MSFilter *f, NVEncoderParams *params)
{
	NVidiaEncState *d=(NVidiaEncState*)f->data;
	int err;
	err = NVSetCodec(d->enc, NV_CODEC_TYPE_H264);

	err = NVSetDefaultParam(d->enc);

	err = NVGetParamValue(d->enc, NVVE_GET_GPU_COUNT, &params->GPU_count);
	ms_message("msnvidiahw: NVVE_GET_GPU_COUNT=%i", params->GPU_count);

	NVVE_GPUAttributes GPUAttributes = {0};
	GPUAttributes.iGpuOrdinal = best_cuDevice;
	err = NVGetParamValue(d->enc, NVVE_GET_GPU_ATTRIBUTES, &GPUAttributes);

	params->GPUOffloadLevel= NVVE_GPU_OFFLOAD_ALL;
	params->MaxOffloadLevel = GPUAttributes.MaxGpuOffloadLevel;

	params->iUseDeviceMem=0;
	err = NVSetParamValue(d->enc, NVVE_DEVICE_MEMORY_INPUT, &params->iUseDeviceMem);

    NVVE_GPUOffloadLevel eMaxOffloadLevel = NVVE_GPU_OFFLOAD_DEFAULT;
	err = NVGetParamValue(d->enc, NVVE_GPU_OFFLOAD_LEVEL_MAX, &eMaxOffloadLevel);

	if (params->GPUOffloadLevel > eMaxOffloadLevel) {
		params->GPUOffloadLevel = eMaxOffloadLevel;
		switch (params->GPUOffloadLevel ) {
			case NVVE_GPU_OFFLOAD_DEFAULT:
				ms_message("msnvidiahw: Offload Default (CPU: PEL Processing\n)");
				break;
			case NVVE_GPU_OFFLOAD_ESTIMATORS:
				ms_message("msnvidiahw: Offload Motion Estimators\n");
				break;
			case NVVE_GPU_OFFLOAD_ALL:
				ms_message("msnvidiahw: Offload Full Encode\n)");
				break;
		}
	}

	err = NVSetParamValue(d->enc, NVVE_GPU_OFFLOAD_LEVEL, &(params->GPUOffloadLevel));
	
	params->iOutputSize[0] = d->vsize.width;
	params->iOutputSize[1] = d->vsize.height;
	params->iInputSize[0] = d->vsize.width;
	params->iInputSize[1] = d->vsize.height;

	int ratiow, ratioh;
	ratiow=d->vsize.width;
	ratioh=d->vsize.height;
	reduce(&ratiow, &ratioh);

	params->iAspectRatio[0] = ratiow;
	params->iAspectRatio[1] = ratioh;
	params->iAspectRatio[2] = 0;

	params->Fieldmode = MODE_FRAME;
	params->iP_Interval = 1;
	params->iIDR_Period = (int)(d->fps*5);
	params->iDynamicGOP = 0;

	params->RCType = TUNE_RC_MODE;

	params->iAvgBitrate = (int)(d->bitrate*0.8);
	params->iPeakBitrate = d->bitrate;
	params->iQP_Level_Intra = 25;
	params->iQP_Level_InterP = 28;
	params->iQP_Level_InterB = 31;
	params->iFrameRate[0] = (int)(d->fps*1000);
	params->iFrameRate[1] = 1000;
	params->iDeblockMode = 1;

	/* setting level_idc (0x0c=12) may fail if bitrate is not adequate? */
	params->iProfileLevel = 0xff42; /* 0x0c42; */
	params->iForceIntra = 0;
	params->iForceIDR = 0;
	params->iClearStat = 0;
	params->DIMode = DI_MEDIAN;
	params->Presets = (NVVE_PRESETS_TARGET)-1; /* ? */
	params->iDisableCabac = 1;
	params->iNaluFramingType = 0;
	params->iDisableSPSPPS = 0;

	/* this code helps to reduce packet size between IDR */
	if (d->bitrate>=1024000){
		params->iSliceCnt = 32;
	} else if (d->bitrate>=512000){
		params->iSliceCnt = 16;
	} else if (d->bitrate>=384000){
		params->iSliceCnt = 8;
	}else if (d->bitrate>=256000){
		params->iSliceCnt = 4;
	}else if (d->bitrate>=128000){
		params->iSliceCnt = 2;
	}else {
		params->iSliceCnt = 0;
	}

	err = NVSetParamValue(d->enc, NVVE_SLICE_COUNT, &(params->iSliceCnt));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }

	err = NVSetParamValue(d->enc, NVVE_OUT_SIZE, &(params->iOutputSize));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
	err = NVSetParamValue(d->enc, NVVE_IN_SIZE, &(params->iInputSize));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
    err = NVSetParamValue(d->enc, NVVE_ASPECT_RATIO, &(params->iAspectRatio));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
    err = NVSetParamValue(d->enc, NVVE_FIELD_ENC_MODE, &(params->Fieldmode));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
    err = NVSetParamValue(d->enc, NVVE_P_INTERVAL, &(params->iP_Interval));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
    err = NVSetParamValue(d->enc, NVVE_IDR_PERIOD, &(params->iIDR_Period));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
    err = NVSetParamValue(d->enc, NVVE_DYNAMIC_GOP, &(params->iDynamicGOP));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
    err = NVSetParamValue(d->enc, NVVE_RC_TYPE, &(params->RCType));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
    err = NVSetParamValue(d->enc, NVVE_AVG_BITRATE, &(params->iAvgBitrate));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
    err = NVSetParamValue(d->enc, NVVE_PEAK_BITRATE, &(params->iPeakBitrate));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
    err = NVSetParamValue(d->enc, NVVE_QP_LEVEL_INTRA, &(params->iQP_Level_Intra));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }


    err = NVSetParamValue(d->enc, NVVE_QP_LEVEL_INTER_P, &(params->iQP_Level_InterP));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
    err = NVSetParamValue(d->enc, NVVE_QP_LEVEL_INTER_B, &(params->iQP_Level_InterB));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
    err = NVSetParamValue(d->enc, NVVE_FRAME_RATE, &(params->iFrameRate));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
    err = NVSetParamValue(d->enc, NVVE_DEBLOCK_MODE, &(params->iDeblockMode));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
    err = NVSetParamValue(d->enc, NVVE_PROFILE_LEVEL, &(params->iProfileLevel));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
    err = NVSetParamValue(d->enc, NVVE_FORCE_INTRA, &(params->iForceIntra));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
    err = NVSetParamValue(d->enc, NVVE_FORCE_IDR, &(params->iForceIDR));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
    err = NVSetParamValue(d->enc, NVVE_CLEAR_STAT, &(params->iClearStat));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
    err = NVSetParamValue(d->enc, NVVE_SET_DEINTERLACE, &(params->DIMode));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
    if (params->Presets != -1)
    {
        err = NVSetParamValue(d->enc, NVVE_PRESETS, &(params->Presets));
		if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
	}
    err = NVSetParamValue(d->enc, NVVE_DISABLE_CABAC, &(params->iDisableCabac));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
    err = NVSetParamValue(d->enc, NVVE_CONFIGURE_NALU_FRAMING_TYPE, &(params->iNaluFramingType));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
    err = NVSetParamValue(d->enc, NVVE_DISABLE_SPS_PPS, &(params->iDisableSPSPPS));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }

	int iLowLatency = 1;
    err = NVSetParamValue(d->enc, NVVE_LOW_LATENCY, &(iLowLatency));
	if (err!=0) { ms_error("msnvidiahw: NVSetParamValue err = %04d", err); }
	
	NVVE_CallbackParams sCBParams      = {0};
	memset(&sCBParams,0,sizeof(NVVE_CallbackParams));
	sCBParams.pfnacquirebitstream = HandleAcquireBitStream;
	sCBParams.pfnonbeginframe     = HandleOnBeginFrame;
	sCBParams.pfnonendframe       = HandleOnEndFrame;
	sCBParams.pfnreleasebitstream = HandleReleaseBitStream;
	NVRegisterCB(d->enc, sCBParams, f);

	err = NVCreateHWEncoder(d->enc);
	if (err!=0) {
		ms_error("msnvidiahw: NVCreateHWEncoder err = %08X", err);
		return -1;
	}
	return 0;
}

static void enc_preprocess(MSFilter *f){
	NVidiaEncState *d=(NVidiaEncState*)f->data;
	NVEncoderParams params;
	rfc3984_init(&d->packer);
	if (d->mode==0) {
		d->mode=1;
		ms_warning("msnvidiahw: packetization-mode was 0, not supported by encoder -> force mode=1");
	}
	rfc3984_set_mode(&d->packer,d->mode);

	d->framenum=0;
	d->sav_pps=NULL;
	d->sav_sps=NULL;

	int err = NVCreateEncoder(&d->enc);
	if (err<0) {
		ms_error("msnvidiahw: NVCreateEncoder err = %04d", err);
		return;
	}

    err = enc_setup_param(f, &params);
	if (err<0) {
		ms_error("msnvidiahw: enc_setup_param err = %04d", err);
		NVDestroyEncoder(d->enc);
		d->enc=NULL;
		return;
	}
}

static void enc_postprocess(MSFilter *f){
	NVidiaEncState *d=(NVidiaEncState*)f->data;
	rfc3984_uninit(&d->packer);
	if (d->enc!=NULL){
		NVDestroyEncoder(d->enc);
		d->enc=NULL;
	}
	if (d->sav_sps!=NULL)
		freeb(d->sav_sps);
	d->sav_sps=NULL;
	if (d->sav_pps!=NULL)
		freeb(d->sav_pps);
	d->sav_pps=NULL;
}

static void enc_process(MSFilter *f){
	NVidiaEncState *d=(NVidiaEncState*)f->data;
	uint32_t ts=(uint32_t)(f->ticker->time*90LL);
	mblk_t *im;
	MSPicture pic;
	MSQueue nalus;
	ms_queue_init(&nalus);

	ms_filter_lock(f);

	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		if (yuv_buf_init_from_mblk(&pic,im)==0){
			int err;

            if (d->vsize.width!=pic.w || d->vsize.height!=pic.h || d->reset==1)
            {
                d->vsize.width=pic.w;
                d->vsize.height=pic.h;
                enc_postprocess(f);
                enc_preprocess(f);
                ms_message("msnvidiahw: nvidia h264 encoder reconfigured (%ix%i)", pic.w, pic.h);
                d->reset=0;
            }

			if (d->enc==NULL) {
				freemsg(im);
				continue;
			}

			int nDeviceMemPitch=0; /* used when device memory is used */
			NVVE_SurfaceFormat iSurfaceFormat=IYUV;
			NVVE_EncodeFrameParams      efparams;
			efparams.Width            = d->vsize.width;
			efparams.Height           = d->vsize.height;
			efparams.Pitch            = (nDeviceMemPitch ? nDeviceMemPitch : d->vsize.width);
			efparams.PictureStruc     = FRAME_PICTURE; 
			efparams.SurfFmt          = IYUV;
			efparams.progressiveFrame = (iSurfaceFormat == 3) ? 1 : 0;
			efparams.repeatFirstField = 0;
			efparams.topfieldfirst    = (iSurfaceFormat == 1) ? 1 : 0;

			efparams.picBuf = im->b_rptr;

			efparams.bLast = false;

			unsigned long flags = 0;
			if (d->framenum==0)
				flags |= 0x04;  /* idr */
			else if (d->framenum==(int)(d->fps*2))
				flags |= 0x04;  /* idr */
			
			err = NVEncodeFrame(d->enc, &efparams, flags, NULL);
			if (err!=0) {
				ms_error("msnvidiahw: NVEncodeFrame err = %04d", err);
			} else {
				d->framenum++;
				if (d->framenum%((int)(d->fps*5))==0)
				{
					/* resend PPS/SPS */
					if (d->sav_sps!=NULL && d->sav_pps!=NULL)
					{
						ms_queue_put(&nalus,dupb(d->sav_sps));
						ms_queue_put(&nalus,dupb(d->sav_pps));
						rfc3984_pack(&d->packer,&nalus,f->outputs[0],ts);
					}
				}
			}
		}
		freemsg(im);
	}

	ms_filter_unlock(f);
}

static int enc_set_br(MSFilter *f, void *arg){
	NVidiaEncState *d=(NVidiaEncState*)f->data;
	d->bitrate=*(int*)arg;
	if (d->bitrate>=8192000){
		d->vsize.width=MS_VIDEO_SIZE_720P_W;
		d->vsize.height=MS_VIDEO_SIZE_720P_H;
		d->fps=30;
	}else if (d->bitrate>=4096000){
		d->vsize.width=MS_VIDEO_SIZE_VGA_W;
		d->vsize.height=MS_VIDEO_SIZE_VGA_H;
		d->fps=30;
	}else if (d->bitrate>=2048000){
		d->vsize.width=MS_VIDEO_SIZE_VGA_W;
		d->vsize.height=MS_VIDEO_SIZE_VGA_H;
		d->fps=30;
	}else if (d->bitrate>=1024000){
		d->vsize.width=MS_VIDEO_SIZE_VGA_W;
		d->vsize.height=MS_VIDEO_SIZE_VGA_H;
		d->fps=30;
	}else if (d->bitrate>=512000){
		d->vsize.width=MS_VIDEO_SIZE_CIF_W;
		d->vsize.height=MS_VIDEO_SIZE_CIF_H;
		d->fps=30;
	} else if (d->bitrate>=384000){
		d->vsize.width=MS_VIDEO_SIZE_CIF_W;
		d->vsize.height=MS_VIDEO_SIZE_CIF_H;
		d->fps=25;
	}else if (d->bitrate>=256000){
		d->vsize.width=MS_VIDEO_SIZE_CIF_W;
		d->vsize.height=MS_VIDEO_SIZE_CIF_H;
		d->fps=15;
	}else if (d->bitrate>=128000){
		d->vsize.width=MS_VIDEO_SIZE_CIF_W;
		d->vsize.height=MS_VIDEO_SIZE_CIF_H;
		d->fps=15;
	}else if (d->bitrate>=64000){
		d->vsize.width=MS_VIDEO_SIZE_QCIF_W;
		d->vsize.height=MS_VIDEO_SIZE_QCIF_H;
		d->fps=10;
	}else if (d->bitrate>=20000){
		/* for static images */
		d->vsize.width=MS_VIDEO_SIZE_CIF_W;
		d->vsize.height=MS_VIDEO_SIZE_CIF_H;
		d->fps=1;
		d->bitrate=d->bitrate*2;
	}
#if TARGET_OS_IPHONE
	if (d->bitrate>=512000){
        d->vsize.width=MS_VIDEO_SIZE_IOS2_W;
        d->vsize.height=MS_VIDEO_SIZE_IOS2_H;
		d->fps=10;
    }else if (d->bitrate>=64000){
        d->vsize.width=MS_VIDEO_SIZE_IOS1_W;
        d->vsize.height=MS_VIDEO_SIZE_IOS1_H;
		d->fps=10;
    }else if (d->bitrate>=20000){
        /* for static images */
        d->vsize.width=MS_VIDEO_SIZE_CIF_W;
        d->vsize.height=MS_VIDEO_SIZE_CIF_H;
		d->fps=10;
    }
#endif
    if (d->enc!=NULL)
    {
        d->reset=1;
    }
	return 0;
}

static int enc_set_fps(MSFilter *f, void *arg){
	NVidiaEncState *d=(NVidiaEncState*)f->data;
	d->fps=*(float*)arg;
	return 0;
}

static int enc_get_fps(MSFilter *f, void *arg){
	NVidiaEncState *d=(NVidiaEncState*)f->data;
	*(float*)arg=d->fps;
	return 0;
}

static int enc_get_vsize(MSFilter *f, void *arg){
	NVidiaEncState *d=(NVidiaEncState*)f->data;
	*(MSVideoSize*)arg=d->vsize;
	return 0;
}

static int enc_add_fmtp(MSFilter *f, void *arg){
	NVidiaEncState *d=(NVidiaEncState*)f->data;
	const char *fmtp=(const char *)arg;
	char value[12];
	if (fmtp_get_value(fmtp,"packetization-mode",value,sizeof(value))){
		d->mode=atoi(value);
		ms_message("msnvidiahw: packetization-mode set to %i",d->mode);
	}
	if (fmtp_get_value(fmtp,"profile-level-id",value,sizeof(value))){
		if (value[0]!='\0' && strlen(value)==6)
		{
			d->profile_idc=0;
			if (value[4] >= '0' && value[4] <= '9')
	            d->profile_idc = (d->profile_idc << 4) + (value[4] - '0');
			else if (value[4] >= 'A' && value[4] <= 'F')
	            d->profile_idc = (d->profile_idc << 4) + (value[4] - 'A' + 10);
			else if (value[4] >= 'a' && value[4] <= 'f')
	            d->profile_idc = (d->profile_idc << 4) + (value[4] - 'a' + 10);

			if (value[5] >= '0' && value[5] <= '9')
	            d->profile_idc = (d->profile_idc << 4) + (value[5] - '0');
			else if (value[5] >= 'A' && value[5] <= 'F')
	            d->profile_idc = (d->profile_idc << 4) + (value[5] - 'A' + 10);
			else if (value[5] >= 'a' && value[5] <= 'f')
	            d->profile_idc = (d->profile_idc << 4) + (value[5] - 'a' + 10);
		}
	}
	return 0;
}

static MSFilterMethod enc_methods[]={
	{	MS_FILTER_SET_FPS	,	enc_set_fps	},
	{	MS_FILTER_SET_BITRATE	,	enc_set_br	},
	{	MS_FILTER_GET_FPS	,	enc_get_fps	},
	{	MS_FILTER_GET_VIDEO_SIZE,	enc_get_vsize	},
	{	MS_FILTER_ADD_FMTP	,	enc_add_fmtp	},
	{	0	,			NULL		}
};

MSFilterDesc nvidiahw_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSH264NVIDIAEnc",
	"A H264 hardware encoder based on CUDA.",
	MS_FILTER_ENCODER,
	"H264",
	1,
	1,
	enc_init,
	enc_preprocess,
	enc_process,
	enc_postprocess,
	enc_uninit,
	enc_methods
};


typedef struct
{
	CUcontext cuContext;

    CUvideoparser cuParser;
    CUvideodecoder cuDecoder;
    CUstream cuStream;
    CUvideoctxlock cuCtxLock;
    CUVIDDECODECREATEINFO dci;
    CUVIDPARSERDISPINFO DisplayQueue[DISPLAY_DELAY];
    unsigned char *pRawNV12;
    int raw_nv12_size;
    int pic_cnt;
    int display_pos;
} DecodeSession;

typedef struct _NVidiaDecState{
	mblk_t *yuv_msg;
	mblk_t *sps,*pps;
	Rfc3984Context unpacker;
	MSPicture outbuf;
	struct MSScalerContext *sws_ctx;
	unsigned int packet_num;
	uint8_t *bitstream;
	int bitstream_size;

	DecodeSession state;
    CUVIDPARSERPARAMS parserInitParams;

}NVidiaDecState;



static int DisplayPicture(MSFilter *f, CUVIDPARSERDISPINFO *pPicParams);

/* Called when the decoder encounters a video format change (or initial sequence header) */
static int CUDAAPI HandleVideoSequence(void *pvUserData, CUVIDEOFORMAT *pFormat)
{
	MSFilter *f=(MSFilter*)pvUserData;
	NVidiaDecState *d=(NVidiaDecState*)f->data;
	DecodeSession *state = (DecodeSession *)&d->state;
    
    if ((pFormat->codec != state->dci.CodecType)
     || (pFormat->coded_width != state->dci.ulWidth)
     || (pFormat->coded_height != state->dci.ulHeight)
     || (pFormat->chroma_format != state->dci.ChromaFormat))
    {
        CAutoCtxLock lck(state->cuCtxLock);
        if (state->cuDecoder)
        {
            cuvidDestroyDecoder(state->cuDecoder);
            state->cuDecoder = NULL;
        }
        memset(&state->dci, 0, sizeof(CUVIDDECODECREATEINFO));
        state->dci.ulWidth = pFormat->coded_width;
        state->dci.ulHeight = pFormat->coded_height;
        state->dci.ulNumDecodeSurfaces = MAX_FRM_CNT;
        state->dci.CodecType = pFormat->codec;
        state->dci.ChromaFormat = pFormat->chroma_format;
        /* Output (pass through) */
        state->dci.OutputFormat = cudaVideoSurfaceFormat_NV12;
        state->dci.DeinterlaceMode = cudaVideoDeinterlaceMode_Weave; /* No deinterlacing */
        state->dci.ulTargetWidth = state->dci.ulWidth;
        state->dci.ulTargetHeight = state->dci.ulHeight;
        state->dci.ulNumOutputSurfaces = 1;
        /* Create the decoder */
        if (CUDA_SUCCESS != cuvidCreateDecoder(&state->cuDecoder, &state->dci))
        {
            ms_error("msnvidiahw: Failed to create video decoder");
            return 0;
        }
    }
    return 1;
}

/* Called by the video parser to decode a single picture
 Since the parser will deliver data as fast as it can, we need to make sure that the picture
 index we're attempting to use for decode is no longer used for display */
static int CUDAAPI HandlePictureDecode(void *pvUserData, CUVIDPICPARAMS *pPicParams)
{
	MSFilter *f=(MSFilter*)pvUserData;
	NVidiaDecState *d=(NVidiaDecState*)f->data;
	DecodeSession *state = (DecodeSession *)&d->state;
    CAutoCtxLock lck(state->cuCtxLock);
    CUresult result;
    int flush_pos;
    
    if (pPicParams->CurrPicIdx < 0) /* Should never happen */
    {
        ms_error("msnvidiahw: Invalid picture index");
        return 0;
    }
    /* Make sure that the new frame we're decoding into is not still in the display queue
     (this could happen if we do not have enough free frame buffers to handle the max delay) */
    flush_pos = state->display_pos; /* oldest frame */
    for (;;)
    {
        bool frame_in_use = false;
        for (int i=0; i<DISPLAY_DELAY; i++)
        {
            if (state->DisplayQueue[i].picture_index == pPicParams->CurrPicIdx)
            {
                frame_in_use = true;
                break;
            }
        }
        if (!frame_in_use)
        {
            /* No problem: we're safe to use this frame */
            break;
        }
        /* The target frame is still pending in the display queue:
         Flush the oldest entry from the display queue and repeat */
        if (state->DisplayQueue[flush_pos].picture_index >= 0)
        {
            DisplayPicture(f, &state->DisplayQueue[flush_pos]);
            state->DisplayQueue[flush_pos].picture_index = -1;
        }
        flush_pos = (flush_pos + 1) % DISPLAY_DELAY;
    }
    result = cuvidDecodePicture(state->cuDecoder, pPicParams);
    if (result != CUDA_SUCCESS)
    {
        ms_error("msnvidiahw: cuvidDecodePicture: %d", result);
    }
    return (result == CUDA_SUCCESS);
}

/* Called by the video parser to display a video frame (in the case of field pictures, there may be
 2 decode calls per 1 display call, since two fields make up one frame) */
static int CUDAAPI HandlePictureDisplay(void *pvUserData, CUVIDPARSERDISPINFO *pPicParams)
{
	MSFilter *f=(MSFilter*)pvUserData;
	NVidiaDecState *d=(NVidiaDecState*)f->data;
	DecodeSession *state = (DecodeSession *)&d->state;
    
    if (state->DisplayQueue[state->display_pos].picture_index >= 0)
    {
        DisplayPicture(f, &state->DisplayQueue[state->display_pos]);
        state->DisplayQueue[state->display_pos].picture_index = -1;
    }
    state->DisplayQueue[state->display_pos] = *pPicParams;
    state->display_pos = (state->display_pos + 1) % DISPLAY_DELAY;
    return TRUE;
}

static mblk_t *get_as_yuvmsg(MSFilter *f, NVidiaDecState *s, MSPicture *orig){

	if (s->outbuf.w!=orig->w || s->outbuf.h!=orig->h){
		if (s->sws_ctx!=NULL){
			ms_video_scalercontext_free(s->sws_ctx);
			s->sws_ctx=NULL;
			freemsg(s->yuv_msg);
			s->yuv_msg=NULL;
		}

		ms_message("msnvidiahw: Getting yuv picture of %ix%i",orig->w,orig->h);
		s->yuv_msg=yuv_buf_alloc(&s->outbuf,orig->w,orig->h);
		s->outbuf.w=orig->w;
		s->outbuf.h=orig->h;
		s->sws_ctx=ms_video_scalercontext_init(orig->w,orig->h,MS_NV12,
			orig->w,orig->h,MS_YUV420P,MS_YUVFAST,
                	NULL, NULL, NULL);
	}
	if (ms_video_scalercontext_convert(s->sws_ctx,orig->planes, orig->strides, 0,
					orig->h, s->outbuf.planes, s->outbuf.strides)<0){
		ms_error("msnvidiahw: error in sws_scale().");
	}
	return dupmsg(s->yuv_msg);
}

static int DisplayPicture(MSFilter *f, CUVIDPARSERDISPINFO *pPicParams)
{
	NVidiaDecState *d=(NVidiaDecState*)f->data;
	DecodeSession *state = (DecodeSession *)&d->state;

    CAutoCtxLock lck(state->cuCtxLock);
    CUVIDPROCPARAMS vpp;
    CUdeviceptr devPtr;
    CUresult result;
    unsigned int pitch = 0, w, h;
    int nv12_size;

    memset(&vpp, 0, sizeof(vpp));
    vpp.progressive_frame = pPicParams->progressive_frame;
    vpp.top_field_first = pPicParams->top_field_first;
    result = cuvidMapVideoFrame(state->cuDecoder, pPicParams->picture_index, &devPtr, &pitch, &vpp);
    if (result != CUDA_SUCCESS)
    {
        ms_error("msnvidiahw: cuvidMapVideoFrame: %d", result);
        return 0;
    }
    w = state->dci.ulTargetWidth;
    h = state->dci.ulTargetHeight;
    nv12_size = pitch * (h + h/2);  /* 12bpp */
    if ((!state->pRawNV12) || (nv12_size > state->raw_nv12_size))
    {
        state->raw_nv12_size = 0;
        if (state->pRawNV12)
        {
            cuMemFreeHost(state->pRawNV12);    /* Just to be safe (the pitch should be constant) */
            state->pRawNV12 = NULL;
        }
        result = cuMemAllocHost((void**)&state->pRawNV12, nv12_size);
        if (result != CUDA_SUCCESS)
            ms_error("msnvidiahw: cuMemAllocHost failed to allocate %d bytes (%d)", nv12_size, result);
        state->raw_nv12_size = nv12_size;
    }
    if (state->pRawNV12)
    {
    #if USE_ASYNC_COPY
        result = cuMemcpyDtoHAsync(state->pRawNV12, devPtr, nv12_size, state->cuStream);
        if (result != CUDA_SUCCESS)
            ms_error("msnvidiahw: cuMemcpyDtoHAsync: %d", result);
        /* Gracefully wait for async copy to complete */
        while (CUDA_ERROR_NOT_READY == cuStreamQuery(state->cuStream))
        {
            Sleep(1);
        }
    #else
        result = cuMemcpyDtoH(state->pRawNV12, devPtr, nv12_size);
    #endif
    }
    cuvidUnmapVideoFrame(state->cuDecoder, devPtr);

	if (state->pRawNV12)
    {
		MSPicture src_pic;
		src_pic.w=w;
		src_pic.h=h;
		yuv_buf_init_with_format(&src_pic, MS_NV12, pitch, h, state->pRawNV12);
		ms_queue_put(f->outputs[0],get_as_yuvmsg(f,d,&src_pic));
	}
    state->pic_cnt++;
    return 1;
}

static bool nvidia_init(DecodeSession *state){
    CUresult err;

	err = cuCtxCreate(&state->cuContext, 0, best_cuDevice);
    if (err != CUDA_SUCCESS)
        ms_error("msnvidiahw: cuCtxCreate: %d", err);

#if USE_FLOATING_CONTEXTS
    err = cuvidCtxLockCreate(&state->cuCtxLock, state->cuContext);
    if (err != CUDA_SUCCESS)
        ms_error("msnvidiahw: cuvidCtxLockCreate: %d (cuContext=%p)", err, state->cuContext);
#endif
	return true;
}

static bool nvidia_uninit(DecodeSession *state){
#if USE_FLOATING_CONTEXTS
    if (state->cuCtxLock)
    {
        cuvidCtxLockDestroy(state->cuCtxLock);
        state->cuCtxLock = NULL;
    }
#endif
    if (state->cuContext)
    {
        CUresult err = cuCtxDestroy(state->cuContext);
        if (err != CUDA_SUCCESS)
            ms_error("msnvidiahw: cuCtxDestroy failed (%d)", err);
        state->cuContext = NULL;
    }
    return true;
}

static void dec_close(NVidiaDecState *d){
	/* Delete all created objects */
	if (d->state.cuParser != NULL)
	{
		cuvidDestroyVideoParser(d->state.cuParser);
		d->state.cuParser = NULL;
	}
	if (d->state.cuDecoder != NULL)
	{
		CAutoCtxLock lck(d->state.cuCtxLock);
		cuvidDestroyDecoder(d->state.cuDecoder);
		d->state.cuDecoder = NULL;
	}
	if (d->state.cuStream != NULL)
	{
		CAutoCtxLock lck(d->state.cuCtxLock);
		cuStreamDestroy(d->state.cuStream);
		d->state.cuStream = NULL;
	}
	if (d->state.pRawNV12)
	{
		cuMemFreeHost(d->state.pRawNV12);
		d->state.pRawNV12 = NULL;
	}
}

static void dec_open(MSFilter *f, NVidiaDecState *d){
    CUresult result;
	int i;
    d->parserInitParams.CodecType = cudaVideoCodec_H264;
    d->parserInitParams.ulMaxNumDecodeSurfaces = MAX_FRM_CNT;
    d->parserInitParams.pUserData = f;
    d->parserInitParams.pfnSequenceCallback = HandleVideoSequence;
    d->parserInitParams.pfnDecodePicture = HandlePictureDecode;
    d->parserInitParams.pfnDisplayPicture = HandlePictureDisplay;
    result = cuvidCreateVideoParser(&d->state.cuParser, &d->parserInitParams);
    if (result != CUDA_SUCCESS)
    {
		ms_error("msnvidiahw: dec_open: Failed to create video parser (%d)", result);
		dec_close(d);
		return;
    }
    {
        CAutoCtxLock lck(d->state.cuCtxLock);
        result = cuStreamCreate(&d->state.cuStream, 0);
        if (result != CUDA_SUCCESS)
        {
            ms_error("msnvidiahw: dec_open: cuStreamCreate failed (%d)", result);
			dec_close(d);
			return;
        }
    }
    /* Init display queue */
    for (i=0; i<DISPLAY_DELAY; i++)
    {
        d->state.DisplayQueue[i].picture_index = -1;   /* invalid */
    }
}

static void dec_init(MSFilter *f){
	NVidiaDecState *d=(NVidiaDecState*)ms_new(NVidiaDecState,1);
    memset(&d->state, 0, sizeof(d->state));
    memset(&d->parserInitParams, 0, sizeof(d->parserInitParams));
	nvidia_init(&d->state);
	dec_open(f, d);

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

static void dec_reinit(MSFilter *f, NVidiaDecState *d){
	CUresult result;
	int i;

	if (d->state.cuParser != NULL)
	{
		cuvidDestroyVideoParser(d->state.cuParser);
		d->state.cuParser = NULL;
	}
	/* Create video parser */
	memset(&d->parserInitParams, 0, sizeof(d->parserInitParams));
	d->parserInitParams.CodecType = cudaVideoCodec_H264;
	d->parserInitParams.ulMaxNumDecodeSurfaces = MAX_FRM_CNT;
	d->parserInitParams.pUserData = f;
	d->parserInitParams.pfnSequenceCallback = HandleVideoSequence;
	d->parserInitParams.pfnDecodePicture = HandlePictureDecode;
	d->parserInitParams.pfnDisplayPicture = HandlePictureDisplay;
	result = cuvidCreateVideoParser(&d->state.cuParser, &d->parserInitParams);
	if (result != CUDA_SUCCESS)
	{
		ms_error("msnvidiahw: dec_reinit: Failed to create video parser (%d)", result);
		return;
	}

	/* Flush display queue */
	for (i = 0; i < DISPLAY_DELAY; i++)
		d->state.DisplayQueue[i].picture_index = -1;
	d->state.display_pos = 0;

	return;
}

static void dec_uninit(MSFilter *f){
	NVidiaDecState *d=(NVidiaDecState*)f->data;
	rfc3984_uninit(&d->unpacker);
	dec_close(d);
	nvidia_uninit(&d->state);
	if (d->sws_ctx!=NULL) ms_video_scalercontext_free(d->sws_ctx);
	if (d->yuv_msg) freemsg(d->yuv_msg);
	if (d->sps) freemsg(d->sps);
	if (d->pps) freemsg(d->pps);
	ms_free(d->bitstream);
	ms_free(d);
}

static void update_sps(NVidiaDecState *d, mblk_t *sps){
	if (d->sps)
		freemsg(d->sps);
	d->sps=NULL;
	if (sps)
		d->sps=dupb(sps);
}

static void update_pps(NVidiaDecState *d, mblk_t *pps){
	if (d->pps)
		freemsg(d->pps);
	d->pps=NULL;
	if (pps)
		d->pps=dupb(pps);
}

static bool_t check_sps_pps_change(NVidiaDecState *d, mblk_t *sps, mblk_t *pps){
	bool_t ret1=FALSE,ret2=FALSE;
	if (d->sps){
		if (sps){
			ret1=(msgdsize(sps)!=msgdsize(d->sps)) || (memcmp(d->sps->b_rptr,sps->b_rptr,msgdsize(sps))!=0);
			if (ret1) {
				update_sps(d,sps);
				ms_message("msnvidiahw: SPS changed !");
				update_pps(d,NULL);
			}
		}
	}else if (sps) {
		ms_message("msnvidiahw: Receiving first SPS");
		update_sps(d,sps);
	}
	if (d->pps){
		if (pps){
			ret2=(msgdsize(pps)!=msgdsize(d->pps)) || (memcmp(d->pps->b_rptr,pps->b_rptr,msgdsize(pps))!=0);
			if (ret2) {
				update_pps(d,pps);
				ms_message("msnvidiahw: PPS changed ! %i,%i",msgdsize(pps),msgdsize(d->pps));
			}
		}
	}else if (pps) {
		ms_message("msnvidiahw: Receiving first PPS");
		update_pps(d,pps);
	}
	return ret1 || ret2;
}

static void enlarge_bitstream(NVidiaDecState *d, int new_size){
	d->bitstream_size=new_size;
	d->bitstream=(uint8_t*)ms_realloc(d->bitstream,d->bitstream_size);
}

static int nalusToFrame(NVidiaDecState *d, MSQueue *naluq, bool_t *new_sps_pps){
	mblk_t *im;
	uint8_t *dst=d->bitstream,*src,*end;
	int nal_len;
	bool_t start_picture=TRUE;
	uint8_t nalu_type;
	*new_sps_pps=FALSE;
	end=d->bitstream+d->bitstream_size;
	while((im=ms_queue_get(naluq))!=NULL){
		src=im->b_rptr;
		nal_len=(int)(im->b_wptr-src);
		if (dst+nal_len+100>end){
			int pos=(int)(dst-d->bitstream);
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
	return (int)(dst-d->bitstream);
}

static void dec_process(MSFilter *f){
	NVidiaDecState *d=(NVidiaDecState*)f->data;
	mblk_t *im;
	MSQueue nalus;

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
				CUresult err;
				CUVIDSOURCEDATAPACKET pkt;
				pkt.flags = CUVID_PKT_ENDOFSTREAM;
				pkt.payload_size = 0;
				pkt.payload = NULL;
				pkt.timestamp = 0;  /* not using timestamps */
				err = cuvidParseVideoData(d->state.cuParser, &pkt);
				if (err<0) {
					ms_error("msnvidiahw: cuvidParseVideoData: error %i.",err);
				}
			}
			p=d->bitstream;
			end=d->bitstream+size;

			while (end-p>0) {
				CUresult err;
				CUVIDSOURCEDATAPACKET pkt;
				pkt.flags = CUVID_PKT_TIMESTAMP;
				pkt.payload_size = (unsigned long)(end-p);
				pkt.payload = p;
				pkt.timestamp = ts;  /* not using timestamps */
				err = cuvidParseVideoData(d->state.cuParser, &pkt);

				if (err<0) {
					ms_error("msnvidiahw: cuvidParseVideoData: error %i.",err);
					break;
				}
				p+=end-p;
			}
		}
		d->packet_num++;
	}
}


static int dec_add_fmtp(MSFilter *f, void *arg){
	NVidiaDecState *d=(NVidiaDecState*)f->data;
	const char *fmtp=(const char *)arg;
	char value[256];
	if (fmtp_get_value(fmtp,"sprop-parameter-sets",value,sizeof(value))){
		char * b64_sps=value;
		char * b64_pps=strchr(value,',');
		if (b64_pps){
			*b64_pps='\0';
			++b64_pps;
			ms_message("msnvidiahw: Got sprop-parameter-sets : sps=%s , pps=%s",b64_sps,b64_pps);
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

static MSFilterDesc nvidiahw_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSH264NVIDIADec",
	"A H264 hardware decoder based on CUDA.",
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


inline int _ConvertSMVer2Cores(int major, int minor)
{
    /* Defines for GPU Architecture types (using the SM version to determine the # of cores per SM */
    typedef struct {
       int SM; /* 0xMm (hexidecimal notation), M = SM Major version, and m = SM minor version */
       int Cores;
	   char cores_generation[1024];
    } sSMtoCores;

    sSMtoCores nGpuArchCoresPerSM[] = 
    { { 0x10,  8 }, /* Tesla Generation (SM 1.0) G80 class */
      { 0x11,  8 }, /* Tesla Generation (SM 1.1) G8x class */
      { 0x12,  8 }, /* Tesla Generation (SM 1.2) G9x class */
      { 0x13,  8 }, /* Tesla Generation (SM 1.3) GT200 class */
      { 0x20, 32 }, /* Fermi Generation (SM 2.0) GF100 class */
      { 0x21, 48 }, /* Fermi Generation (SM 2.1) GF10x class */
      {   -1, -1 }
    };

    int index = 0;
    while (nGpuArchCoresPerSM[index].SM != -1) {
       if (nGpuArchCoresPerSM[index].SM == ((major << 4) + minor) ) {
		   return nGpuArchCoresPerSM[index].Cores;
       }	
       index++;
    }
    ms_message("msnvidiahw: MapSMtoCores undefined SM %d.%d is undefined (please update to the latest SDK)!\n", major, minor);
    return -1;
}

/* This function returns the best GPU based on performance */
inline int getMaxGflopsDeviceId()
{
    CUresult err;
    CUdevice current_device = 0, max_perf_device = 0;
    int device_count     = 0, sm_per_multiproc = 0;
    int max_compute_perf = 0, best_SM_arch     = 0;
    int major = 0, minor = 0, multiProcessorCount, clockRate;

    err = cuDeviceGetCount(&device_count);
	if (err != CUDA_SUCCESS)
	{
		ms_error("msnvidiahw: cuDeviceGetCount err = %04d - %s", getCudaDrvErrorString(err));
		return -1;
	}

	/* Find the best major SM Architecture GPU device */
	while ( current_device < device_count ) {
		err = cuDeviceComputeCapability(&major, &minor, current_device );
		if (err != CUDA_SUCCESS)
		{
			ms_error("msnvidiahw: cuDeviceComputeCapability err = %04d - %s", getCudaDrvErrorString(err));
		} else {
			if (major > 0 && major < 9999) {
				best_SM_arch = MAX(best_SM_arch, major);
			}
		}
		current_device++;
	}

    /* Find the best CUDA capable GPU device */
	current_device = 0;
	while( current_device < device_count ) {
		char device_name[1024];
		err = cuDeviceGetAttribute( &multiProcessorCount, CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT, current_device );
		if (err != CUDA_SUCCESS)
		{
			ms_error("msnvidiahw: cuDeviceGetAttribute err = %04d - %s", getCudaDrvErrorString(err));
			++current_device;
			continue;
		}
        cuDeviceGetAttribute( &clockRate, CU_DEVICE_ATTRIBUTE_CLOCK_RATE, current_device );
		if (err != CUDA_SUCCESS)
		{
			ms_error("msnvidiahw: cuDeviceGetAttribute err = %04d - %s", getCudaDrvErrorString(err));
			++current_device;
			continue;
		}
		cuDeviceComputeCapability(&major, &minor, current_device );
		if (err != CUDA_SUCCESS)
		{
			ms_error("msnvidiahw: cuDeviceComputeCapability err = %04d - %s", getCudaDrvErrorString(err));
			++current_device;
			continue;
		}
		err = cuDeviceGetName(device_name, 1024, current_device);
		if (err != CUDA_SUCCESS)
		{
			ms_error("msnvidiahw: cuDeviceGetName err = %04d - %s", getCudaDrvErrorString(err));
			++current_device;
			continue;
		}

		if (major == 9999 && minor == 9999) {
		    sm_per_multiproc = 1;
		} else {
		    sm_per_multiproc = _ConvertSMVer2Cores(major, minor);
		}

		int compute_perf  = multiProcessorCount * sm_per_multiproc * clockRate;
		ms_message("msnvidiahw:      device name (%i:%i) #proc=%i perf=%i) %s ", major, minor, sm_per_multiproc, compute_perf, device_name);
		if( compute_perf  > max_compute_perf ) {
            /* If we find GPU with SM major > 2, search only these */
			if ( best_SM_arch > 2 ) {
				/* If our device==dest_SM_arch, choose this, or else pass */
				if (major == best_SM_arch) {	
                    max_compute_perf  = compute_perf;
                    max_perf_device   = current_device;
				}
			} else {
				max_compute_perf  = compute_perf;
				max_perf_device   = current_device;
			}
		}
		++current_device;
	}
	return max_perf_device;
}

#ifdef WIN32
#define GLOBAL_LINKAGE __declspec(dllexport)
#else
#define GLOBAL_LINKAGE
#endif

extern "C" GLOBAL_LINKAGE void libmsnvidiahw_init(void){
    CUresult err;
	int device_count = 0;
	char device_name[1024];
	int major=0;
	int minor=0;
	DecodeSession state;
	memset(&state, 0, sizeof(DecodeSession));

	ms_message("msnvidiahw: NVIDIA GPU Computing Toolkit // CUDA ");
	ms_message("msnvidiahw:   version:    %i", CUDA_VERSION);

	err = cuInit(0);
	if (err != CUDA_SUCCESS)
	{
		ms_warning("msnvidiahw: nvidia hardware acceleration for h264 not detected.");
		return;
	}

    err = cuDeviceGetCount(&device_count);
	if (err != CUDA_SUCCESS)
	{
		ms_error("msnvidiahw: cuDeviceGetCount err = %04d - %s", getCudaDrvErrorString(err));
		return;
	}

	if (device_count<=0)
	{
		ms_warning("msnvidiahw: nvidia hardware acceleration for h264 not detected (no device).");
		return;
	}

	best_cuDevice = getMaxGflopsDeviceId();

	err = cuDeviceGetName(device_name, 1024, best_cuDevice);
	if (err != CUDA_SUCCESS)
	{
		ms_error("msnvidiahw: cuDeviceGetName err = %04d - %s", getCudaDrvErrorString(err));
		return;
	}

	ms_message("msnvidiahw:      selected device name %s ", device_name);
		
	ms_filter_register(&nvidiahw_dec_desc);
	ms_filter_register(&nvidiahw_enc_desc);
}
