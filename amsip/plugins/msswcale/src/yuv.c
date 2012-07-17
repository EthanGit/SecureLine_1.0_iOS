/* msippyuv - yuv rgb converter for amsip/mediastreamer2
 * Copyright (C) 2010-2012 Aymeric MOIZARD - <amoizard@gmail.com>
 */
#include <inttypes.h>
#include <math.h>

#include <mediastreamer2/msfilter.h>
#include <mediastreamer2/msvideo.h>
#include <mediastreamer2/msticker.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
//#include <libavutil/opt.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
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

struct swscale_SwsContext;

typedef int (*swscale_ms_video_scalercontext_convertFunc)(struct swscale_SwsContext *context, uint8_t* srcSlice[], int srcStride[],
              int srcSliceY, int srcSliceH, uint8_t* dst[], int dstStride[]);

typedef struct swscale_SwsContext {
	MSPixFmt in_fmt;
	MSPixFmt out_fmt;
	int srcW;
	int srcH;
	int dstW;
	int dstH;
	struct SwsContext *sws_ctx;
}swscale_SwsContext;

static int swscale_video_scalercontext_convert(struct swscale_SwsContext *ctx, uint8_t* src[], int srcStride[], int srcSliceY, 
	int srcSliceH, uint8_t* dst[], int dstStride[]){
		if (ctx==NULL)
			return -1;
		if (ctx->sws_ctx==NULL)
			return -1;
		return sws_scale(ctx->sws_ctx, (const uint8_t* const*)src, srcStride, srcSliceY, srcSliceH, dst, dstStride);
}

static struct swscale_SwsContext *swscale_video_scalercontext_init(int srcW, int srcH, MSPixFmt srcFormat,
                                  int dstW, int dstH, MSPixFmt dstFormat,
                                  int flags, void *unused,
                                  void *unused2, double *param)
{
	struct swscale_SwsContext *ctx;
	int swscale_flags;
	ctx = (struct swscale_SwsContext *)ms_malloc0(sizeof(swscale_SwsContext));
	ctx->srcW = srcW;
	ctx->srcH = srcH;
	ctx->dstW = dstW;
	ctx->dstH = dstH;
	ctx->in_fmt = srcFormat;
	ctx->out_fmt = dstFormat;
	
	ms_message("msswscale: conversion %ix%i -> %ix%i %s->%s", ctx->srcW, ctx->srcH, ctx->dstW, ctx->dstH, ms_video_display_format(ctx->in_fmt), ms_video_display_format(ctx->out_fmt));
	swscale_flags=SWS_FAST_BILINEAR;
	if (flags==MS_YUVFAST)
		swscale_flags = SWS_FAST_BILINEAR;
	else if (flags==MS_YUVNORMAL)
		swscale_flags = SWS_BILINEAR;
	else if (flags==MS_YUVSLOW)
		swscale_flags = SWS_BICUBIC;
	
#if 0
	ctx->sws_ctx = sws_alloc_context();
	if (ctx->sws_ctx==NULL)
	{
		ms_error("msswscale: error allocating context");
		return ctx;
	}
	
	av_set_int(ctx->sws_ctx, "sws_flags", swscale_flags|SWS_PRINT_INFO);
	
	av_set_int(ctx->sws_ctx, "srcw", srcW);
	av_set_int(ctx->sws_ctx, "srch", srcH);
	
	av_set_int(ctx->sws_ctx, "dstw", dstW);
	av_set_int(ctx->sws_ctx, "dsth", dstH);
	
	av_set_int(ctx->sws_ctx, "src_range", 0); 
	av_set_int(ctx->sws_ctx, "dst_range", 0); 

	av_set_int(ctx->sws_ctx, "src_format", _ms_pix_fmt_to_ffmpeg(srcFormat));
	av_set_int(ctx->sws_ctx, "dst_format", _ms_pix_fmt_to_ffmpeg(dstFormat));

	if (sws_init_context(ctx->sws_ctx, NULL, NULL) < 0)
	{
		ms_error("msswscale: error initializing context");
		return ctx;
	}
#else
	ctx->sws_ctx = sws_getContext(srcW, srcH, (enum PixelFormat)_ms_pix_fmt_to_ffmpeg(srcFormat),
		dstW, dstH, (enum PixelFormat)_ms_pix_fmt_to_ffmpeg(dstFormat),
		swscale_flags, NULL, NULL, param);
#endif
	return ctx;
}

static void swscale_video_scalercontext_free(struct swscale_SwsContext *swsContext)
{
	if (swsContext==NULL)
		return;
	if (swsContext->sws_ctx!=NULL)
		sws_freeContext(swsContext->sws_ctx);
	ms_free(swsContext);
}

static enum PixelFormat jpeg_convert_format(enum PixelFormat format)
{
	switch (format) {
    case PIX_FMT_YUVJ420P: return PIX_FMT_YUV420P;
    case PIX_FMT_YUVJ422P: return PIX_FMT_YUV422P;
    case PIX_FMT_YUVJ444P: return PIX_FMT_YUV444P;
    case PIX_FMT_YUVJ440P: return PIX_FMT_YUV440P;
    default: return format;
	}
}

static mblk_t *yuv_load_mjpeg(uint8_t *jpgbuf, int bufsize, MSVideoSize *reqsize){
	AVCodecContext av_context;
	int got_picture=0;
	AVFrame orig;
	AVPicture dest;
	mblk_t *ret;
	struct SwsContext *sws_ctx;
	AVPacket pkt;

	avcodec_get_context_defaults(&av_context);
	if (avcodec_open(&av_context,avcodec_find_decoder(CODEC_ID_MJPEG))<0){
		ms_error("yuv_load_mjpeg: avcodec_open failed");
		return NULL;
	}
	av_init_packet(&pkt);
	pkt.data=jpgbuf;
	pkt.size=bufsize;
	if (avcodec_decode_video2(&av_context,&orig,&got_picture,&pkt)<0){
		ms_error("yuv_load_mjpeg: avcodec_decode_video2 failed");
		avcodec_close(&av_context);
		return NULL;
	}
	ret=allocb(avpicture_get_size(PIX_FMT_YUV420P,reqsize->width,reqsize->height),0);
	ret->b_wptr=ret->b_datap->db_lim;
	avpicture_fill(&dest,ret->b_rptr,PIX_FMT_YUV420P,reqsize->width,reqsize->height);
	
#if 0
	sws_ctx = sws_alloc_context();
	if (sws_ctx==NULL)
	{
		ms_error("yuv_load_mjpeg: sws_alloc_context() failed.");
		avcodec_close(&av_context);
		freemsg(ret);
		return NULL;
	}
	
	av_set_int(sws_ctx, "sws_flags", SWS_FAST_BILINEAR|SWS_PRINT_INFO);
	
	av_set_int(sws_ctx, "srcw", av_context.width);
	av_set_int(sws_ctx, "srch", av_context.height);
	
	av_set_int(sws_ctx, "dstw", reqsize->width);
	av_set_int(sws_ctx, "dsth", reqsize->height);
	
	av_set_int(sws_ctx, "src_range", 1); 
	av_set_int(sws_ctx, "dst_range", 0); 
	
	av_set_int(sws_ctx, "src_format", jpeg_convert_format(av_context.pix_fmt));
	av_set_int(sws_ctx, "dst_format", PIX_FMT_YUV420P);
	
	if (sws_init_context(sws_ctx, NULL, NULL) < 0)
	{
		ms_error("yuv_load_mjpeg: sws_init_context() failed.");
		avcodec_close(&av_context);
		freemsg(ret);
		return NULL;
	}
#else
	sws_ctx=sws_getContext(av_context.width,av_context.height,av_context.pix_fmt,
		reqsize->width,reqsize->height,PIX_FMT_YUV420P,SWS_FAST_BILINEAR,
                NULL, NULL, NULL);
	if (sws_ctx==NULL) {
		ms_error("yuv_load_mjpeg: sws_getContext() failed.");
		avcodec_close(&av_context);
		freemsg(ret);
		return NULL;
	}
#endif
	if (sws_scale(sws_ctx,(const uint8_t* const*)orig.data,orig.linesize,0,av_context.height,dest.data,dest.linesize)<0){
		ms_error("yuv_load_mjpeg: sws_scale() failed.");
		sws_freeContext(sws_ctx);
		avcodec_close(&av_context);
		freemsg(ret);
		return NULL;
	}
	sws_freeContext(sws_ctx);
	avcodec_close(&av_context);
	return ret;
}

#ifdef WIN32
#define GLOBAL_LINKAGE __declspec(dllexport)
#else
#define GLOBAL_LINKAGE
#endif

GLOBAL_LINKAGE void libmsswscale_init(void);

GLOBAL_LINKAGE void libmsswscale_init(void){
	struct MSVideoDesc ms_video_desc;
	struct MSVideoJpegDesc ms_video_jpeg_desc;
	int plugin_quality=49;

	ms_message("msswscale: ffmpeg rescaler plugin");
	ms_message("  version: [%d.%d.%d] ",
		LIBSWSCALE_VERSION_MAJOR, LIBSWSCALE_VERSION_MINOR, LIBSWSCALE_VERSION_MICRO );
	ms_message("  license: %s ",
		swscale_license() );
	
	memset(&ms_video_desc, 0, sizeof(ms_video_desc));
	ms_video_desc.quality_priority = plugin_quality;
	ms_video_desc.video_scalercontext_init=(ms_video_scalercontext_initFunc)swscale_video_scalercontext_init;
	ms_video_desc.video_scalercontext_free=(ms_video_scalercontext_freeFunc)swscale_video_scalercontext_free;
	ms_video_desc.video_scalercontext_convert=(ms_video_scalercontext_convertFunc)swscale_video_scalercontext_convert;
	ms_video_desc.yuv_buf_mirror=NULL;
	ms_video_desc.yuv_buf_copy=NULL;
	ms_video_set_video_func(&ms_video_desc);

	memset(&ms_video_jpeg_desc, 0, sizeof(ms_video_jpeg_desc));
	ms_video_jpeg_desc.quality_priority = plugin_quality;
	ms_video_jpeg_desc.yuv_load_mjpeg=yuv_load_mjpeg;
	ms_video_set_videojpeg_func(&ms_video_jpeg_desc);
}
