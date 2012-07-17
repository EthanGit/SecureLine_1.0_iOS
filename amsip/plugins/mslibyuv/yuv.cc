/* mslibyuv - yuv rgb converter for amsip/mediastreamer2
 * Copyright (C) 2010-2012 Aymeric MOIZARD - <amoizard@gmail.com>
 */
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <math.h>

#include <mediastreamer2/msfilter.h>
#include <mediastreamer2/msvideo.h>
#include <mediastreamer2/msticker.h>

#include <libyuv.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif

#ifdef ANDROID
#ifndef KFILTER_QUALITY
#define KFILTER_QUALITY kFilterNone
#endif
#elif TARGET_OS_IPHONE
#ifndef KFILTER_QUALITY
#define KFILTER_QUALITY kFilterNone
#endif
#endif

#ifndef KFILTER_QUALITY
#define KFILTER_QUALITY kFilterBox
#endif

#ifdef PERF_TEST
#define COUNTER_MAX 1000

/*
r160: perf on windows // COUNTER_MAX 1000
mslibyuv: diff with sws_scale 352x288 -> 352x288 MS_YUV420P->MS_RGB24_REV: -- 130 128ms
mslibyuv: diff with sws_scale 352x288 -> 352x288 MS_YUYV->MS_YUV420P: -- 35 20ms
mslibyuv: diff with sws_scale 352x288 -> 880x720 MS_YUV420P->MS_YUV420P: -- 1249 2274ms
mslibyuv: diff with sws_scale 1280x720 -> 1280x720 MS_YUV420P->MS_RGB24_REV: -- 1225 1014ms

r165: perf on windows // COUNTER_MAX 1000
mslibyuv: diff with sws_scale 352x288 -> 352x288 MS_YUV420P->MS_RGB24_REV: -- 131 129ms
mslibyuv: diff with sws_scale 352x288 -> 352x288 MS_YUYV->MS_YUV420P: -- 34 20ms
mslibyuv: diff with sws_scale 352x288 -> 880x720 MS_YUV420P->MS_YUV420P: -- 1482 3047ms
mslibyuv: diff with sws_scale 1280x720 -> 1280x720 MS_YUV420P->MS_RGB24_REV: -- 1319 1082ms

r165: perf on ios // COUNTER_MAX 1000 (camera in QCIF, RTP in CIF)
mslibyuv: diff with sws_scale 352x288 -> 319x261 MS_YUV420P->MS_YUV420P: -- 7666 2552ms
mslibyuv: diff with sws_scale 320x366 -> 320x366 MS_YUV420P->unknown format: -- 2097 1244ms (MS_RGB32/MS_ARGB)

r165: perf on android // COUNTER_MAX 1000 (camera in QCIF, RTP in CIF)
mslibyuv: diff with sws_scale 176x144 -> 352x288 MS_NV21->MS_YUV420P: -- 5804 2452ms
mslibyuv: diff with sws_scale 352x288 -> 352x288 MS_YUV420P->RGB565: -- 1322 1679ms

r165: perf on android // COUNTER_MAX 1000 (Camera in QCIF, RTP in QCIF)
mslibyuv: diff with sws_scale 176x144 -> 176x144 MS_NV21->MS_YUV420P: -- 946 76ms
mslibyuv: diff with sws_scale 176x144 -> 352x288 MS_YUV420P->RGB565: -- 4567 4135ms

r165: perf on android // COUNTER_MAX 1000 (Camera in QCIF, RTP in QCIF, without neon)
mslibyuv: diff with sws_scale 176x144 -> 352x288 MS_YUV420P->RGB565: -- 4413 7946ms
mslibyuv: diff with sws_scale 352x288 -> 176x144 MS_NV21->MS_YUV420P: -- 2136 819ms (kFilterBox)
mslibyuv: diff with sws_scale 176x144 -> 352x288 MS_YUV420P->RGB565: -- 4405 6207ms (kFilterNone)
*/
uint64_t _my_gettime();

#ifdef IPP_PERF
uint64_t _my_gettime() {
	return (uint64_t)ippGetCpuClocks();
}
#elif 1 && defined(WIN32)
uint64_t _my_gettime() {
	static uint32_t rollover_count=0;
	static DWORD last_t=0;
	DWORD t = timeGetTime();
	if (last_t > t)
		++rollover_count;
	last_t = t;
	return ((uint64_t)rollover_count<<32) + t;
}
#elif defined(WIN32)

uint64_t _my_gettime() {
	LARGE_INTEGER m_t0;
	LARGE_INTEGER m_freq;
	QueryPerformanceFrequency(&m_freq);
	QueryPerformanceCounter(&m_t0);
	return (uint64_t)(m_t0.QuadPart) / m_freq.QuadPart;
}

#else

uint64_t _my_gettime() {
#ifdef CLOCK_PROCESS_CPUTIME_ID
    struct timespec ts;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&ts);
    return (ts.tv_sec*1000LL) + (ts.tv_nsec/1000000LL);
#elif defined(CLOCK_MONOTONIC)
    struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC,&ts);
    return (ts.tv_sec*1000LL) + (ts.tv_nsec/1000000LL);
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec*1000LL) + (tv.tv_usec/1000LL);
#endif
}

#endif

#endif

static int _ms_pix_fmt_to_ffmpeg(MSPixFmt fmt){
	switch(fmt){
		case MS_YUV420P:
			return PIX_FMT_YUV420P;
		case MS_YUYV:
			return PIX_FMT_YUYV422;
		case MS_RGB24:
			return PIX_FMT_RGB24;
		case MS_RGB24_REV:
			return PIX_FMT_BGR24;
		case MS_UYVY:
			return PIX_FMT_UYVY422;
		case MS_YUY2:
			return PIX_FMT_YUYV422;   /* <- same as MS_YUYV */
		case MS_RGBA:
			return PIX_FMT_RGBA;
		case MS_NV21:
			return PIX_FMT_NV21;
		case MS_NV12:
			return PIX_FMT_NV12;
		case MS_ABGR:
			return PIX_FMT_ABGR;
		case MS_ARGB:
			return PIX_FMT_ARGB;
		case MS_RGB565:
			return PIX_FMT_RGB565;
		default:
			ms_fatal("format not supported.");
			return PIX_FMT_NONE;
	}
	return PIX_FMT_NONE;
}


typedef int (*libyuv_ms_video_scalercontext_convertFunc)(struct libyuv_SwsContext *context, uint8_t* srcSlice[], int srcStride[],
              int srcSliceY, int srcSliceH, uint8_t* dst[], int dstStride[]);

typedef struct libyuv_SwsContext {
	MSPixFmt in_fmt;
	MSPixFmt out_fmt;
	int srcW;
	int srcH;
	int dstW;
	int dstH;
	mblk_t *pic_buf;
	MSPicture pic;
	libyuv_ms_video_scalercontext_convertFunc conv;
	struct SwsContext *sws_ctx;
}libyuv_SwsContext;

static int simple_yuv_copy(struct libyuv_SwsContext *ctx, uint8_t* src[], int srcStride[], int srcSliceY, 
             int srcSliceH, uint8_t* dst[], int dstStride[]){
  return libyuv::I420Copy(src[0], srcStride[0],
			  src[1], srcStride[1],
			  src[2], srcStride[2],
			  dst[0], dstStride[0],
			  dst[1], dstStride[1],
			  dst[2], dstStride[2],
			  ctx->dstW, ctx->dstH);
}

//TODO: NOT YET TESTED!
static int libyuv_convYUV420ToRGB24(struct libyuv_SwsContext *ctx, uint8_t* src[], int srcStride[], int srcSliceY, 
	int srcSliceH, uint8_t* dst[], int dstStride[]){

		const uint8_t* yplane = src[0];
		const uint8_t* uplane = src[1];
		const uint8_t* vplane = src[2];
		if (dstStride[0]<0) {
			dst[0]-=+(ctx->srcW*3*(ctx->srcH-1));
			dstStride[0]=-dstStride[0];

			if (ctx->pic_buf==NULL)
			{
				ctx->pic_buf = yuv_buf_alloc(&ctx->pic, ctx->dstW, ctx->dstH);
			}
			if (ctx->pic_buf==NULL)
				return -1;
			//we have to use a temporary buffer?...
			uint8_t* dst_yplane = ctx->pic.planes[0];
			uint8_t* dst_uplane = ctx->pic.planes[1];
			uint8_t* dst_vplane = ctx->pic.planes[2];

			// Inserting negative height flips the frame.
			libyuv::I420Copy(yplane, ctx->dstW,
				uplane, ctx->dstW / 2,
				vplane, ctx->dstW / 2,
				dst_yplane, ctx->dstW,
				dst_uplane, ctx->dstW / 2,
				dst_vplane, ctx->dstW / 2,
				ctx->dstW, -ctx->dstH);
			return libyuv::I420ToRGB24(dst_yplane, ctx->dstW,
				dst_uplane, ctx->dstW / 2,
				dst_vplane, ctx->dstW / 2,
				dst[0], dstStride[0],
				ctx->dstW, ctx->dstH);
		}
		return libyuv::I420ToRGB24(yplane, ctx->dstW,
			uplane, ctx->dstW / 2,
			vplane, ctx->dstW / 2,
			dst[0], dstStride[0],
			ctx->dstW, ctx->dstH);
}

static int libyuv_convYUV420ToBGR24(struct libyuv_SwsContext *ctx, uint8_t* src[], int srcStride[], int srcSliceY, 
	int srcSliceH, uint8_t* dst[], int dstStride[]){

		const uint8_t* yplane = src[0];
		const uint8_t* uplane = src[1];
		const uint8_t* vplane = src[2];
#if 0
		if (dstStride[0]<0) {
			dst[0]-=+(ctx->srcW*3*(ctx->srcH-1));
			dstStride[0]=-dstStride[0];

			if (ctx->pic_buf==NULL)
			{
				ctx->pic_buf = yuv_buf_alloc(&ctx->pic, ctx->dstW, ctx->dstH);
			}
			if (ctx->pic_buf==NULL)
				return -1;
			//we have to use a temporary buffer?...
			uint8_t* dst_yplane = ctx->pic.planes[0];
			uint8_t* dst_uplane = ctx->pic.planes[1];
			uint8_t* dst_vplane = ctx->pic.planes[2];

			// Inserting negative height flips the frame.
			libyuv::I420Copy(yplane, ctx->dstW,
				uplane, ctx->dstW / 2,
				vplane, ctx->dstW / 2,
				dst_yplane, ctx->dstW,
				dst_uplane, ctx->dstW / 2,
				dst_vplane, ctx->dstW / 2,
				ctx->dstW, -ctx->dstH);
			return libyuv::I420ToRGB24(dst_yplane, ctx->dstW,
				dst_uplane, ctx->dstW / 2,
				dst_vplane, ctx->dstW / 2,
				dst[0], dstStride[0],
				ctx->dstW, ctx->dstH);
		}
#endif
		return libyuv::I420ToRGB24(yplane, ctx->dstW,
			uplane, ctx->dstW / 2,
			vplane, ctx->dstW / 2,
			dst[0], dstStride[0],
			ctx->dstW, ctx->dstH);
}

static int libyuv_convYUV420ToRGB565(struct libyuv_SwsContext *ctx, uint8_t* src[], int srcStride[], int srcSliceY, 
                                   int srcSliceH, uint8_t* dst[], int dstStride[]){
  if (ctx->srcW!=ctx->dstW || ctx->srcH!=ctx->dstH) {
    
    if (ctx->pic_buf==NULL)
      {
	ctx->pic_buf = yuv_buf_alloc(&ctx->pic, ctx->dstW, ctx->dstH);
      }
    if (ctx->pic_buf==NULL)
      return -1;
    
    //we have to use a temporary buffer?...
    uint8_t* pic_yplane = ctx->pic.planes[0];
    uint8_t* pic_uplane = ctx->pic.planes[1];
    uint8_t* pic_vplane = ctx->pic.planes[2];
    libyuv::I420Scale(src[0], srcStride[0],
		      src[1], srcStride[1],
		      src[2], srcStride[2],
		      ctx->srcW, ctx->srcH,
		      pic_yplane, ctx->dstW,
		      pic_uplane, ctx->dstW / 2,
		      pic_vplane, ctx->dstW / 2,
		      ctx->dstW, ctx->dstH, libyuv::KFILTER_QUALITY);
    return libyuv::I420ToRGB565(pic_yplane, ctx->dstW,
				pic_uplane, ctx->dstW / 2,
				pic_vplane, ctx->dstW / 2,
				dst[0], dstStride[0],
				ctx->dstW, ctx->dstH);
  }
  return libyuv::I420ToRGB565(src[0], srcStride[0],
                              src[1], srcStride[1],
                              src[2], srcStride[2],
                              dst[0], dstStride[0],
                              ctx->dstW, ctx->dstH);
}

static int libyuv_convYUV420ToARGB(struct libyuv_SwsContext *ctx, uint8_t* src[], int srcStride[], int srcSliceY, 
                                   int srcSliceH, uint8_t* dst[], int dstStride[]){
    if (ctx->srcW!=ctx->dstW || ctx->srcH!=ctx->dstH) {
        
        if (ctx->pic_buf==NULL)
        {
            ctx->pic_buf = yuv_buf_alloc(&ctx->pic, ctx->dstW, ctx->dstH);
        }
        if (ctx->pic_buf==NULL)
            return -1;
        
        //we have to use a temporary buffer?...
        uint8_t* pic_yplane = ctx->pic.planes[0];
        uint8_t* pic_uplane = ctx->pic.planes[1];
        uint8_t* pic_vplane = ctx->pic.planes[2];
        libyuv::I420Scale(src[0], srcStride[0],
                          src[1], srcStride[1],
                          src[2], srcStride[2],
                          ctx->srcW, ctx->srcH,
                          pic_yplane, ctx->dstW,
                          pic_uplane, ctx->dstW / 2,
                          pic_vplane, ctx->dstW / 2,
                          ctx->dstW, ctx->dstH, libyuv::KFILTER_QUALITY);
        return libyuv::I420ToBGRA(pic_yplane, ctx->dstW,
                                  pic_uplane, ctx->dstW / 2,
                                  pic_vplane, ctx->dstW / 2,
                                  dst[0], dstStride[0],
                                  ctx->dstW, ctx->dstH);
    }
    return libyuv::I420ToBGRA(src[0], srcStride[0], //RGBA
                              src[1], srcStride[1],
                              src[2], srcStride[2],
                              dst[0], dstStride[0],
                              ctx->dstW, ctx->dstH);
}

static int libyuv_convNV12ToYUV420(struct libyuv_SwsContext *ctx, uint8_t* src[], int srcStride[], int srcSliceY, 
                                   int srcSliceH, uint8_t* dst[], int dstStride[]){
    if (ctx->srcW!=ctx->dstW || ctx->srcH!=ctx->dstH) {
        
        if (ctx->pic_buf==NULL)
        {
            ctx->pic_buf = yuv_buf_alloc(&ctx->pic, ctx->srcW, ctx->srcH);
        }
        if (ctx->pic_buf==NULL)
            return -1;
        
        //we have to use a temporary buffer?...
        uint8_t* pic_yplane = ctx->pic.planes[0];
        uint8_t* pic_uplane = ctx->pic.planes[1];
        uint8_t* pic_vplane = ctx->pic.planes[2];
	libyuv::NV12ToI420(src[0], srcStride[0],
			   src[1], srcStride[1],
			   pic_yplane, ctx->srcW,
			   pic_uplane, ctx->srcW / 2,
			   pic_vplane, ctx->srcW / 2,
			   ctx->srcW, ctx->srcH);
        return libyuv::I420Scale(pic_yplane, ctx->srcW,
				 pic_uplane, ctx->srcW / 2,
				 pic_vplane, ctx->srcW / 2,
				 ctx->srcW, ctx->srcH,
				 dst[0], dstStride[0],
				 dst[1], dstStride[1],
				 dst[2], dstStride[2],
				 ctx->dstW, ctx->dstH, libyuv::KFILTER_QUALITY);
    }
    return libyuv::NV12ToI420(src[0], srcStride[0],
                              src[1], srcStride[1],
                              dst[0], dstStride[0],
                              dst[1], dstStride[1],
                              dst[2], dstStride[2],
                              ctx->dstW, ctx->dstH);
}

static int libyuv_convNV21ToYUV420(struct libyuv_SwsContext *ctx, uint8_t* src[], int srcStride[], int srcSliceY, 
                                   int srcSliceH, uint8_t* dst[], int dstStride[]){
    if (ctx->srcW!=ctx->dstW || ctx->srcH!=ctx->dstH) {
        
        if (ctx->pic_buf==NULL)
        {
            ctx->pic_buf = yuv_buf_alloc(&ctx->pic, ctx->srcW, ctx->srcH);
        }
        if (ctx->pic_buf==NULL)
            return -1;
        
        //we have to use a temporary buffer?...
        uint8_t* pic_yplane = ctx->pic.planes[0];
        uint8_t* pic_uplane = ctx->pic.planes[1];
        uint8_t* pic_vplane = ctx->pic.planes[2];
	libyuv::NV12ToI420(src[0], srcStride[0],
			   src[1], srcStride[1],
			   pic_yplane, ctx->srcW,
			   pic_vplane, ctx->srcW / 2,
			   pic_uplane, ctx->srcW / 2,
			   ctx->srcW, ctx->srcH);
#if 0 //def ANDROID
	if (ctx->srcW>ctx->dstW && ctx->srcH>ctx->dstH) {
	  MSPicture center_img;
	  int X=ctx->srcW/2-ctx->dstW/2;
	  int Y=ctx->srcH/2-ctx->dstH/2;
	  MSVideoSize roi;
	  roi.width=ctx->dstW;
	  roi.height=ctx->dstH;
	  
	  center_img.strides[0]=ctx->srcW;
	  center_img.strides[1]=ctx->srcW / 2;
	  center_img.strides[2]=ctx->srcW / 2;
	  center_img.strides[3]=0;
	  center_img.planes[0]=pic_yplane+(X+(Y*center_img.strides[0]));
	  center_img.planes[1]=pic_uplane+((X/2)+((Y/2)*center_img.strides[1]));
	  center_img.planes[2]=pic_vplane+((X/2)+((Y/2)*center_img.strides[2]));
	  center_img.planes[3]=0;
	  
	  MSPicture dst_pic;
	  dst_pic.planes[0]=dst[0];	
	  dst_pic.planes[1]=dst[1];
	  dst_pic.planes[2]=dst[2];
	  dst_pic.planes[3]=dst[3];
	  dst_pic.strides[0]=dstStride[0];
	  dst_pic.strides[1]=dstStride[1];
	  dst_pic.strides[2]=dstStride[2];
	  dst_pic.strides[3]=0;
	  
	  ms_yuv_buf_copy(ctx->pic.planes,ctx->pic.strides, dst_pic.planes,dst_pic.strides,roi);
	  return 0;
	} else {
	  return libyuv::I420Scale(pic_yplane, ctx->srcW,
				   pic_uplane, ctx->srcW / 2,
				   pic_vplane, ctx->srcW / 2,
				   ctx->srcW, ctx->srcH,
				   dst[0], dstStride[0],
				   dst[1], dstStride[1],
				   dst[2], dstStride[2],
				   ctx->dstW, ctx->dstH, libyuv::KFILTER_QUALITY);
	  
	}
#else
        return libyuv::I420Scale(pic_yplane, ctx->srcW,
				 pic_uplane, ctx->srcW / 2,
				 pic_vplane, ctx->srcW / 2,
				 ctx->srcW, ctx->srcH,
				 dst[0], dstStride[0],
				 dst[1], dstStride[1],
				 dst[2], dstStride[2],
				 ctx->dstW, ctx->dstH, libyuv::KFILTER_QUALITY);
#endif
    }
    return libyuv::NV12ToI420(src[0], srcStride[0],
                              src[1], srcStride[1],
                              dst[0], dstStride[0],
                              dst[2], dstStride[2],
                              dst[1], dstStride[1],
                              ctx->dstW, ctx->dstH);
}


static int libyuv_convYUV420ToRGBA(struct libyuv_SwsContext *ctx, uint8_t* src[], int srcStride[], int srcSliceY, 
             int srcSliceH, uint8_t* dst[], int dstStride[]){
		return libyuv::I420ToABGR(src[0], srcStride[0], //RGBA
			src[1], srcStride[1],
			src[2], srcStride[2],
			dst[0], dstStride[0],
			ctx->dstW, ctx->dstH);
}

static int libyuv_convUYVY422ToYUV420(struct libyuv_SwsContext *ctx, uint8_t* src[], int srcStride[], int srcSliceY, 
	int srcSliceH, uint8_t* dst[], int dstStride[]){
    return libyuv::UYVYToI420(src[0], srcStride[0],
                              dst[0], dstStride[0],
                              dst[1], dstStride[1],
                              dst[2], dstStride[2],
                              ctx->srcW, ctx->srcH);
}


static int libyuv_convYUV420ToUYVY422(struct libyuv_SwsContext *ctx, uint8_t* src[], int srcStride[], int srcSliceY, 
                                      int srcSliceH, uint8_t* dst[], int dstStride[]){
    return libyuv::I420ToUYVY(src[0], srcStride[0],
                              src[1], srcStride[1],
                              src[2], srcStride[2],
                              dst[0], dstStride[0],
                              ctx->srcW, ctx->srcH);
}

static int libyuv_convYUY2ToYUV420(struct libyuv_SwsContext *ctx, uint8_t* src[], int srcStride[], int srcSliceY, 
	int srcSliceH, uint8_t* dst[], int dstStride[]){
		return libyuv::YUY2ToI420(src[0], srcStride[0],
			dst[0], dstStride[0],
			dst[1], dstStride[1],
			dst[2], dstStride[2],
			ctx->srcW, ctx->srcH);
}

//TODO: NOT YET TESTED!
static int libyuv_convRGB24ToYUV420(struct libyuv_SwsContext *ctx, uint8_t* src[], int srcStride[], int srcSliceY, 
	int srcSliceH, uint8_t* dst[], int dstStride[]){
		//should it be BGR24ToI420 here????
		return libyuv::RGB24ToI420(src[0], srcStride[0],
			dst[0], dstStride[0],
			dst[1], dstStride[1],
			dst[2], dstStride[2],
			ctx->srcW, ctx->srcH);
}

static int libyuv_convBGR24ToYUV420(struct libyuv_SwsContext *ctx, uint8_t* src[], int srcStride[], int srcSliceY, 
	int srcSliceH, uint8_t* dst[], int dstStride[]){
		return libyuv::RGB24ToI420(src[0], srcStride[0],
			dst[0], dstStride[0],
			dst[1], dstStride[1],
			dst[2], dstStride[2],
			ctx->srcW, ctx->srcH);
}

static int libyuv_convResizeYUV420(struct libyuv_SwsContext *ctx, uint8_t* src[], int srcStride[], int srcSliceY, 
	int srcSliceH, uint8_t* dst[], int dstStride[]){
		return libyuv::I420Scale(src[0], srcStride[0],
			src[1], srcStride[1],
			src[2], srcStride[2],
			ctx->srcW, ctx->srcH,
			dst[0], dstStride[0],
			dst[1], dstStride[1],
			dst[2], dstStride[2],
			ctx->dstW, ctx->dstH, libyuv::KFILTER_QUALITY);
}

static struct libyuv_SwsContext *libyuv_video_scalercontext_init(int srcW, int srcH, MSPixFmt srcFormat,
                                  int dstW, int dstH, MSPixFmt dstFormat,
                                  int flags, void *unused,
                                  void *unused2, double *param)
{
	struct libyuv_SwsContext *ctx;
	ctx = (struct libyuv_SwsContext *)ms_malloc0(sizeof(libyuv_SwsContext));
	ctx->srcW = srcW;
	ctx->srcH = srcH;
	ctx->dstW = dstW;
	ctx->dstH = dstH;
	ctx->in_fmt = srcFormat;
	ctx->out_fmt = dstFormat;
	ctx->pic_buf=NULL;
	memset(&ctx->pic, 0, sizeof(MSPicture));

	ms_message("mslibyuv: conversion %ix%i -> %ix%i %s->%s", ctx->srcW, ctx->srcH, ctx->dstW, ctx->dstH, ms_video_display_format(ctx->in_fmt), ms_video_display_format(ctx->out_fmt));
	if (ctx->in_fmt == MS_YUV420P
	    && ctx->out_fmt == MS_RGB565)
	{
#ifdef HAVE_NEON
                ms_message("mslibyuv: conv MS_YUV420P to MS_RGB565 (bad perf without neon)");
		ctx->conv = libyuv_convYUV420ToRGB565;
#else
                ms_message("mslibyuv: conv MS_YUV420P to MS_RGB565 (revert to swscale)");
		ctx->sws_ctx = sws_getContext(srcW, srcH, (enum PixelFormat)_ms_pix_fmt_to_ffmpeg(srcFormat), dstW, dstH, (enum PixelFormat)_ms_pix_fmt_to_ffmpeg(dstFormat),
			SWS_FAST_BILINEAR, NULL, NULL, param);
#endif
	}
	else if (ctx->in_fmt == MS_YUV420P
             && ctx->out_fmt == MS_ARGB) //was PIX_FMT_ARGB
	{
		ms_message("mslibyuv: conv MS_YUV420P to MS_ARGB");
		ctx->conv = libyuv_convYUV420ToARGB;
	}
	else if (ctx->in_fmt == MS_NV12
             && ctx->out_fmt == MS_YUV420P)
	{
		ms_message("mslibyuv: conv MS_NV21 to MS_YUV420P");
		ctx->conv = libyuv_convNV12ToYUV420;
	}
	else if (ctx->in_fmt == MS_NV21
             && ctx->out_fmt == MS_YUV420P)
	{
		ms_message("mslibyuv: conv MS_NV21 to MS_YUV420P");
		ctx->conv = libyuv_convNV21ToYUV420;
	}
	else if (ctx->in_fmt != ctx->out_fmt && ((srcW != dstW) || (srcH!=dstH)))
	{
		ms_message("mslibyuv: resize + conversion: swscale");
		ctx->sws_ctx = sws_getContext(srcW, srcH, (enum PixelFormat)_ms_pix_fmt_to_ffmpeg(srcFormat), dstW, dstH, (enum PixelFormat)_ms_pix_fmt_to_ffmpeg(dstFormat),
			SWS_FAST_BILINEAR, NULL, NULL, param);
	}
	else if (ctx->in_fmt == MS_YUV420P
		&& ctx->out_fmt == MS_RGB24)
	{
		ms_message("mslibyuv: TODO: conv MS_YUV420P to MS_RGB24");
		//ctx->conv = libyuv_convYUV420ToRGB24;
		ctx->sws_ctx = sws_getContext(srcW, srcH, (enum PixelFormat)_ms_pix_fmt_to_ffmpeg(srcFormat), dstW, dstH, (enum PixelFormat)_ms_pix_fmt_to_ffmpeg(dstFormat),
			SWS_FAST_BILINEAR, NULL, NULL, param);
	}
	else if (ctx->in_fmt == MS_YUV420P
		&& ctx->out_fmt == MS_RGB24_REV)
	{
		ms_message("mslibyuv: conv MS_YUV420P to MS_RGB24_REV");
		ctx->conv = libyuv_convYUV420ToBGR24;
	}
	else if (ctx->in_fmt == MS_YUV420P
		&& ctx->out_fmt == MS_RGBA) //was PIX_FMT_RGBA OR PIX_FMT_RGB32
	{
		ms_message("mslibyuv: conv MS_YUV420P to MS_RGBA");
		ctx->conv = libyuv_convYUV420ToRGBA;
	}
	else if (ctx->in_fmt == MS_YUV420P
		&& ctx->out_fmt == MS_YUV420P
		&& ctx->srcW==ctx->dstW
		&& ctx->srcH==ctx->dstH)
	{
		ms_message("mslibyuv: conv MS_YUV420P to MS_YUV420P");
		ctx->conv = simple_yuv_copy;;
	}
	else if (ctx->in_fmt == MS_YUV420P
		&& ctx->out_fmt == MS_YUV420P)
	{
		ms_message("mslibyuv: resizing MS_YUV420P to MS_YUV420P");
		ctx->conv = libyuv_convResizeYUV420;
	}
	else if (ctx->in_fmt == MS_UYVY
		&& ctx->out_fmt == MS_YUV420P)
	{
		ms_message("mslibyuv: conv MS_UYVY to MS_YUV420P");
		ctx->conv = libyuv_convUYVY422ToYUV420;
	}
	else if (ctx->in_fmt == MS_YUV420P
             && ctx->out_fmt == MS_UYVY)
	{
		ms_message("mslibyuv: conv MS_YUV420P to MS_UYVY");
		ctx->conv = libyuv_convYUV420ToUYVY422;
	}
	else if ((ctx->in_fmt == MS_YUYV || ctx->in_fmt == MS_YUY2)
		&& ctx->out_fmt == MS_YUV420P)
	{
		ms_message("mslibyuv: conv MS_YUYV to MS_YUV420P");
		ctx->conv = libyuv_convYUY2ToYUV420; 
	}
	else if (ctx->in_fmt == MS_RGB24
		&& ctx->out_fmt == MS_YUV420P)
	{
		ms_message("mslibyuv: TODO: conv MS_RGB24 to MS_YUV420P");
		//ctx->conv = libyuv_convRGB24ToYUV420;
		ctx->sws_ctx = sws_getContext(srcW, srcH, (enum PixelFormat)_ms_pix_fmt_to_ffmpeg(srcFormat), dstW, dstH, (enum PixelFormat)_ms_pix_fmt_to_ffmpeg(dstFormat),
			SWS_FAST_BILINEAR, NULL, NULL, param);
	}
	else if (ctx->in_fmt == MS_RGB24_REV
		&& ctx->out_fmt == MS_YUV420P)
	{
		ms_message("mslibyuv: conv MS_RGB24_REV to MS_YUV420P");
		ctx->conv = libyuv_convBGR24ToYUV420;
	}
	else
	{
		ms_message("mslibyuv: fallback to swscale conversion");
		ctx->sws_ctx = sws_getContext(srcW, srcH, (enum PixelFormat)_ms_pix_fmt_to_ffmpeg(srcFormat), dstW, dstH, (enum PixelFormat)_ms_pix_fmt_to_ffmpeg(dstFormat),
			SWS_FAST_BILINEAR, NULL, NULL, param);
	}
#ifdef PERF_TEST
	if (ctx->sws_ctx==NULL)
	{
	  ctx->sws_ctx = sws_getContext(srcW, srcH, (enum PixelFormat)_ms_pix_fmt_to_ffmpeg(srcFormat), dstW, dstH, (enum PixelFormat)_ms_pix_fmt_to_ffmpeg(dstFormat),
			SWS_FAST_BILINEAR, NULL, NULL, param);
	}
#endif
	return ctx;
}

static void libyuv_video_scalercontext_free(struct libyuv_SwsContext *swsContext)
{
	if (swsContext==NULL)
		return;
	if (swsContext->pic_buf!=NULL)
		freeb(swsContext->pic_buf);
	if (swsContext->sws_ctx!=NULL)
		sws_freeContext(swsContext->sws_ctx);
	ms_free(swsContext);
}

#ifdef PERF_TEST
static void perf_test_video_scalercontext_convert(struct libyuv_SwsContext *ctx, uint8_t* srcSlice[], int srcStride[],
	int srcSliceY, int srcSliceH, uint8_t* dst[], int dstStride[])
{
	uint64_t start, stop;
	uint64_t perfswscale, perfipp;
	int counter;
	if (ctx->conv==NULL)
		return;

	start = _my_gettime();
	for (counter=0;counter<COUNTER_MAX;counter++)
	{
		sws_scale(ctx->sws_ctx, srcSlice, srcStride, srcSliceY, srcSliceH, dst, dstStride);
	}
	stop = _my_gettime();
	perfswscale = stop-start;
	start = _my_gettime();
	for (counter=0;counter<COUNTER_MAX;counter++)
	{
		ctx->conv(ctx, srcSlice, srcStride, srcSliceY, srcSliceH, dst, dstStride);
	}
	stop = _my_gettime();
	perfipp =  stop-start;

	ms_message("mslibyuv: diff with sws_scale %ix%i -> %ix%i %s->%s: -- %" PRIu64 " %"PRIu64"ms", ctx->srcW, ctx->srcH, ctx->dstW, ctx->dstH, ms_video_display_format(ctx->in_fmt), ms_video_display_format(ctx->out_fmt), perfswscale, perfipp);
}
#endif

static int libyuv_video_scalercontext_convert(struct libyuv_SwsContext *ctx, uint8_t* srcSlice[], int srcStride[],
              int srcSliceY, int srcSliceH, uint8_t* dst[], int dstStride[])
{
	int i=-1;

#ifdef PERF_TEST
	perf_test_video_scalercontext_convert(ctx, srcSlice, srcStride, srcSliceY, srcSliceH, dst, dstStride);
#endif

	if (ctx->conv==NULL)
		i = sws_scale(ctx->sws_ctx, srcSlice, srcStride, srcSliceY, srcSliceH, dst, dstStride);
	else
		i = ctx->conv(ctx, srcSlice, srcStride, srcSliceY, srcSliceH, dst, dstStride);

	return i;
}

static void yuv_buf_copy(uint8_t *src_planes[], const int src_strides[], 
		uint8_t *dst_planes[], const int dst_strides[3], MSVideoSize roi){
  libyuv::I420Copy(src_planes[0], src_strides[0],
			  src_planes[1], src_strides[1],
			  src_planes[2], src_strides[2],
			  dst_planes[0], dst_strides[0],
			  dst_planes[1], dst_strides[1],
			  dst_planes[2], dst_strides[2],
			  roi.width, roi.height);
}

#if defined(WIN32) || defined(__APPLE__)
#include <libyuv/mjpeg_decoder.h>

static mblk_t *yuv_load_mjpeg(uint8_t *jpgbuf, int bufsize, MSVideoSize *dst_size){

	int r = 0;
	MSPicture pic_src;
	MSPicture pic_dst;
	mblk_t *pic_buf_src;
	mblk_t *pic_buf_dst;
	MSVideoSize src_size;
	libyuv::MJpegDecoder mjpeg_decoder;
	bool ret = mjpeg_decoder.LoadFrame(jpgbuf, bufsize);
	if (ret==false)
	{
		return NULL;
	}
	src_size.width = mjpeg_decoder.GetWidth();
	src_size.height = mjpeg_decoder.GetHeight();
	mjpeg_decoder.UnloadFrame();

	/* temporary buffer */
	pic_buf_src = yuv_buf_alloc(&pic_src, src_size.width, src_size.height);
	uint8_t* src_yplane = pic_src.planes[0];
	uint8_t* src_uplane = pic_src.planes[1];
	uint8_t* src_vplane = pic_src.planes[2];

    r = libyuv::MJPGToI420(jpgbuf, bufsize,
		src_yplane, src_size.width,
		src_uplane, src_size.width / 2,
		src_vplane, src_size.width / 2,
		src_size.width, src_size.height, src_size.width, src_size.height);
	if (r==0) {
		pic_buf_dst = yuv_buf_alloc(&pic_dst, dst_size->width, dst_size->height);
		uint8_t* dst_yplane = pic_dst.planes[0];
		uint8_t* dst_uplane = pic_dst.planes[1];
		uint8_t* dst_vplane = pic_dst.planes[2];
		libyuv::I420Scale(src_yplane, src_size.width,
					src_uplane, src_size.width / 2,
					src_vplane, src_size.width / 2,
					src_size.width, src_size.height,
					dst_yplane, dst_size->width,
					dst_uplane, dst_size->width / 2,
					dst_vplane, dst_size->width / 2,
					dst_size->width, dst_size->height, libyuv::KFILTER_QUALITY);
		freeb(pic_buf_src);
		return pic_buf_dst;
	}
	return NULL;
}
#endif

#ifdef WIN32
#define GLOBAL_LINKAGE __declspec(dllexport)
#else
#define GLOBAL_LINKAGE
#endif

extern "C" GLOBAL_LINKAGE void libmslibyuv_init(void);

extern "C" GLOBAL_LINKAGE void libmslibyuv_init(void){
	struct MSVideoDesc ms_video_desc;
	struct MSVideoJpegDesc ms_video_jpeg_desc;
	int plugin_quality=40;

	ms_message("mslibyuv: YUV/RGB conversion from libyuv");
	memset(&ms_video_desc, 0, sizeof(ms_video_desc));
	ms_video_desc.quality_priority = plugin_quality;
	ms_video_desc.video_scalercontext_init=(ms_video_scalercontext_initFunc)libyuv_video_scalercontext_init;
	ms_video_desc.video_scalercontext_free=(ms_video_scalercontext_freeFunc)libyuv_video_scalercontext_free;
	ms_video_desc.video_scalercontext_convert=(ms_video_scalercontext_convertFunc)libyuv_video_scalercontext_convert;
	ms_video_desc.yuv_buf_mirror=NULL;
	ms_video_desc.yuv_buf_copy=yuv_buf_copy;
	ms_video_set_video_func(&ms_video_desc);
	
#if defined(WIN32) || defined(__APPLE__)
	memset(&ms_video_jpeg_desc, 0, sizeof(ms_video_jpeg_desc));
	ms_video_jpeg_desc.quality_priority = plugin_quality;
	ms_video_jpeg_desc.yuv_load_mjpeg=yuv_load_mjpeg;
	ms_video_set_videojpeg_func(&ms_video_jpeg_desc);
#endif
	
	
}
