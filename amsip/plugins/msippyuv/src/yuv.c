/* msippyuv - yuv rgb converter for amsip/mediastreamer2
 * Copyright (C) 2010-2012 Aymeric MOIZARD - <amoizard@gmail.com>
 */
#include <inttypes.h>
#include <math.h>

#include <mediastreamer2/msfilter.h>
#include <mediastreamer2/msvideo.h>
#include <mediastreamer2/msticker.h>

#include <ippi.h>
#include <ipp.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif

#ifdef PERF_TEST
#define COUNTER_MAX 1000

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
    timespec tS;
    tS.tv_sec = 0;
    tS.tv_nsec = 0;
    clock_settime(CLOCK_PROCESS_CPUTIME_ID, &tS);
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

struct ipp_SwsContext;

typedef int (*ipp_ms_video_scalercontext_convertFunc)(struct ipp_SwsContext *context, const uint8_t* srcSlice[], int srcStride[],
              int srcSliceY, int srcSliceH, uint8_t* dst[], int dstStride[]);

struct ipp_SwsContext {
	MSPixFmt in_fmt;
	MSPixFmt out_fmt;
	int srcW;
	int srcH;
	int dstW;
	int dstH;
	ipp_ms_video_scalercontext_convertFunc conv;
	struct SwsContext *sws_ctx;
};


static int ipp_convNV12ToYUV420(struct ipp_SwsContext *ctx, const uint8_t* src[], int srcStride[], int srcSliceY, 
             int srcSliceH, uint8_t* dst[], int dstStride[]){
	IppiSize Sz = {ctx->dstW, srcSliceH};
	
	return ippiYCbCr420_8u_P2P3R(src[0], srcStride[0], src[1], srcStride[1],
				 dst,
				 dstStride, Sz);
}

static int ipp_convNV21ToYUV420(struct ipp_SwsContext *ctx, const uint8_t* src[], int srcStride[], int srcSliceY, 
             int srcSliceH, uint8_t* dst[], int dstStride[]){
	IppiSize Sz = {ctx->dstW, srcSliceH};

	uint8_t *dst_yvu[3];
	int dstStride_yvu[3];

	dst_yvu[0]=dst[0];
	dst_yvu[2]=dst[1]; //V
	dst_yvu[1]=dst[2]; //U

	dstStride_yvu[0]=dstStride[0];
	dstStride_yvu[2]=dstStride[1]; //V
	dstStride_yvu[1]=dstStride[2]; //U

	return ippiYCbCr420_8u_P2P3R(src[0], srcStride[0], src[1], srcStride[1],
				 dst_yvu,
				 dstStride_yvu, Sz);
}

static int ipp_ippiYUV420ToRGB_8u_P3C3R(struct ipp_SwsContext *ctx, const uint8_t* src[], int srcStride[], int srcSliceY, 
             int srcSliceH, uint8_t* dst[], int dstStride[]){
	IppiSize Sz = {ctx->dstW, srcSliceH};
	
	return ippiYUV420ToRGB_8u_P3C3R(src, srcStride, (uint8_t *)
				 (dst[0]+srcSliceY*dstStride[0]),
				 dstStride[0], Sz);
}

static int ipp_ippiYUV420ToBGR_8u_P3C3R(struct ipp_SwsContext *ctx, const uint8_t* src[], int srcStride[], int srcSliceY, 
             int srcSliceH, uint8_t* dst[], int dstStride[]){
	IppiSize Sz = {ctx->dstW, srcSliceH};
	
	return ippiYUV420ToBGR_8u_P3C3R(src, srcStride, (uint8_t *)
				 (dst[0]+srcSliceY*dstStride[0]),
				 dstStride[0], Sz);
}

static int ipp_ippiYUV420ToRGB_8u_P3AC4R(struct ipp_SwsContext *ctx, const uint8_t* src[], int srcStride[], int srcSliceY, 
             int srcSliceH, uint8_t* dst[], int dstStride[]){
	IppiSize Sz = {ctx->dstW, srcSliceH};
	
	return ippiYUV420ToRGB_8u_P3AC4R(src, srcStride, (uint8_t *)
				 (dst[0]+srcSliceY*dstStride[0]),
				 dstStride[0], Sz);
}

static int ipp_ippiYUV422ToRGB_8u_P3AC4R(struct ipp_SwsContext *ctx, const uint8_t* src[], int srcStride[], int srcSliceY, 
             int srcSliceH, uint8_t* dst[], int dstStride[]){
	IppiSize Sz = {ctx->dstW, srcSliceH};
	
	return ippiYUV422ToRGB_8u_P3AC4R(src, srcStride, (uint8_t *)
				 (dst[0]+srcSliceY*dstStride[0]),
				 dstStride[0], Sz);
}

static int ipp_ippiYUV422ToRGB_8u_P3C3R(struct ipp_SwsContext *ctx, const uint8_t* src[], int srcStride[], int srcSliceY, 
             int srcSliceH, uint8_t* dst[], int dstStride[]){
	IppiSize Sz = {ctx->dstW, srcSliceH};
	
	return ippiYUV422ToRGB_8u_P3C3R(src, srcStride, (uint8_t *)
				 (dst[0]+srcSliceY*dstStride[0]),
				 dstStride[0], Sz);
}

static int ipp_ippiYCbCr422ToYCrCb420_8u_C2P3R(struct ipp_SwsContext *ctx, uint8_t* src[], int srcStride[], int srcSliceY, 
             int srcSliceH, uint8_t* dst[], int dstStride[]){
	IppiSize Sz = {ctx->dstW, srcSliceH};
	
	return ippiYCbCr422ToYCrCb420_8u_C2P3R(src[0], srcStride[0],
				 dst,
				 dstStride, Sz);
}

static int ipp_ippiCbYCr422ToYCbCr420_8u_C2P3R(struct ipp_SwsContext *ctx, const uint8_t* src[], int srcStride[], int srcSliceY, 
             int srcSliceH, uint8_t* dst[], int dstStride[]){
	IppiSize Sz = {ctx->dstW, srcSliceH};
	
	return ippiCbYCr422ToYCbCr420_8u_C2P3R(src[0], srcStride[0],
				 dst,
				 dstStride, Sz);
}

static int ipp_ippiYCbCr422ToYCbCr420_8u_C2P3R(struct ipp_SwsContext *ctx, const uint8_t* src[], int srcStride[], int srcSliceY, 
             int srcSliceH, uint8_t* dst[], int dstStride[]){
	IppiSize Sz = {ctx->dstW, srcSliceH};

	return ippiYCbCr422ToYCbCr420_8u_C2P3R(src[0], srcStride[0],
				 dst,
				 dstStride, Sz);
}

static int ipp_ippiRGBToYCbCr420_8u_C3P3R(struct ipp_SwsContext *ctx, const uint8_t* src[], int srcStride[], int srcSliceY, 
             int srcSliceH, uint8_t* dst[], int dstStride[]){
	IppiSize Sz = {ctx->dstW, srcSliceH};
	
	return ippiRGBToYCbCr420_8u_C3P3R(src[0], srcStride[0],
				 dst,
				 dstStride, Sz);
}

static int ipp_ippiBGRToYCbCr420_8u_C3P3R(struct ipp_SwsContext *ctx, const uint8_t* src[], int srcStride[], int srcSliceY, 
             int srcSliceH, uint8_t* dst[], int dstStride[]){
	IppiSize Sz = {ctx->dstW, srcSliceH};
	
	return ippiBGRToYCbCr420_8u_C3P3R(src[0], srcStride[0],
				 dst,
				 dstStride, Sz);
}

static int ipp_ippiResizeSqrPixel_8u_C1R(struct ipp_SwsContext *ctx, const uint8_t* src[], int srcStride[], int srcSliceY, 
             int srcSliceH, uint8_t* dst[], int dstStride[]){
	int i;
	int k;
	IppiSize srcZ = {ctx->srcW, ctx->srcH};
	IppiRect srcROI = {0, 0, ctx->srcW, ctx->srcH};
	IppiRect dstROI = {0, 0, ctx->dstW, ctx->dstH};
	double xFactor = (double)ctx->dstW / ctx->srcW;
	double yFactor = (double)ctx->dstH / ctx->srcH;
	double xShift = 0.0;
	double yShift = 0.0;
	int interpolation = IPPI_INTER_NN; /* IPPI_INTER_LANCZOS; */

	Ipp8u * pBuffer;
	int bufSize = 0;
	int nChannel=3;
	ippiResizeGetBufSize(srcROI, dstROI, nChannel, interpolation, &bufSize );
    pBuffer= ippsMalloc_8u(bufSize );

	for (i = 0; i < 3; i ++) {
		if (i == 1) {
			srcROI.width >>= 1;
			srcROI.height >>= 1;
			dstROI.width >>= 1;
			dstROI.height >>= 1;
		}
		k = ippiResizeSqrPixel_8u_C1R(src[i], srcZ, srcStride[i],
			srcROI,
			dst[i],
			dstStride[i],
			dstROI,
			xFactor,
			yFactor,
			xShift,
			yShift,
			interpolation,
			pBuffer);
	}

	if( NULL != pBuffer )	ippiFree( pBuffer);
	return k;
}

static int ipp_ippiResizeSqrPixel_8u_C3R(struct ipp_SwsContext *ctx, uint8_t* src[], int srcStride[], int srcSliceY, 
             int srcSliceH, uint8_t* dst[], int dstStride[]){
	int k;
	IppiSize srcZ = {ctx->srcW, ctx->srcH};
	IppiRect srcROI = {0, 0, ctx->srcW, ctx->srcH};
	IppiRect dstROI = {0, 0, ctx->dstW, ctx->dstH};
	double xFactor = (double)ctx->dstW / ctx->srcW;
	double yFactor = (double)ctx->dstH / ctx->srcH;
	double xShift = 0.0;
	double yShift = 0.0;
	int interpolation = IPPI_INTER_LANCZOS;

	Ipp8u * pBuffer;
	int bufSize = 0;
	int nChannel=3;
	ippiResizeGetBufSize(srcROI, dstROI, nChannel, interpolation, &bufSize );
	/* ippiResizeSqrPixelGetBufSize ( dstZ, 3, IPPI_INTER_CUBIC, &bufSize); */
    pBuffer= ippsMalloc_8u(bufSize );

	k = ippiResizeSqrPixel_8u_C3R(src[0], srcZ, srcStride[0],
		srcROI,
		dst[0],
		dstStride[0],
		dstROI,
		xFactor,
		yFactor,
		xShift,
		yShift,
		interpolation,
		pBuffer);

	if( NULL != pBuffer )	ippiFree( pBuffer);
	return k;
}

static int simple_yuv_copy(struct ipp_SwsContext *ctx, uint8_t* src[], int srcStride[], int srcSliceY, 
             int srcSliceH, uint8_t* dst[], int dstStride[]){
	int ysize,usize;
	ysize=ctx->dstW*ctx->dstH;
	usize=ysize/4;
	memcpy(dst[0], src[0], ysize);
	memcpy(dst[1], src[1], usize);
	memcpy(dst[2], src[2], usize);
	return 0;
}

static struct ipp_SwsContext *ipp_video_scalercontext_init(int srcW, int srcH, MSPixFmt srcFormat,
                                  int dstW, int dstH, MSPixFmt dstFormat,
                                  int flags, void *unused,
                                  void *unused2, double *param)
{
	struct ipp_SwsContext *ctx;
	ctx = (struct ipp_SwsContext *)ms_malloc0(sizeof(struct ipp_SwsContext));
	ctx->srcW = srcW;
	ctx->srcH = srcH;
	ctx->dstW = dstW;
	ctx->dstH = dstH;
	ctx->in_fmt = srcFormat;
	ctx->out_fmt = dstFormat;

	ms_message("msippyuv: conversion %ix%i -> %ix%i %s->%s", ctx->srcW, ctx->srcH, ctx->dstW, ctx->dstH, ms_video_display_format(ctx->in_fmt), ms_video_display_format(ctx->out_fmt));
	if (ctx->in_fmt != ctx->out_fmt && ((srcW != dstW) || (srcH!=dstH)))
	{
		ms_message("msippyuv: resize + conversion: swscale");
		ctx->sws_ctx = sws_getContext(srcW, srcH, (enum PixelFormat)_ms_pix_fmt_to_ffmpeg(srcFormat), dstW, dstH, (enum PixelFormat)_ms_pix_fmt_to_ffmpeg(dstFormat),
			SWS_FAST_BILINEAR, NULL, NULL, param);
	}
	else if (ctx->in_fmt == MS_NV12
             && ctx->out_fmt == MS_YUV420P)
	{
		ms_message("msippyuv: conv MS_NV12 to MS_YUV420P");
		ctx->conv = ipp_convNV12ToYUV420;
	}
	else if (ctx->in_fmt == MS_NV21
             && ctx->out_fmt == MS_YUV420P)
	{
		ms_message("msippyuv: conv MS_NV21 to MS_YUV420P");
		ctx->conv = ipp_convNV21ToYUV420;
	}
	else if (ctx->in_fmt == MS_YUV420P
		&& ctx->out_fmt == MS_RGB24)
	{
		ms_message("msippyuv: conv MS_YUV420P to MS_RGB24");
		ctx->conv = ipp_ippiYUV420ToRGB_8u_P3C3R;
	}
	else if (ctx->in_fmt == MS_YUV420P
		&& ctx->out_fmt == MS_RGB24_REV)
	{
		ms_message("msippyuv: conv MS_YUV420P to MS_RGB24_REV");
		ctx->conv = ipp_ippiYUV420ToBGR_8u_P3C3R;
	}
	else if (ctx->in_fmt == MS_YUV420P
		&& ctx->out_fmt == MS_RGBA) //and? ctx->out_fmt == PIX_FMT_RGB32
	{
		ms_message("msippyuv: conv MS_YUV420P to MS_RGBA");
		ctx->conv = ipp_ippiYUV420ToRGB_8u_P3AC4R;
	}
	else if (ctx->in_fmt == MS_UYVY
		&& ctx->out_fmt == MS_RGBA) //and? ctx->out_fmt == PIX_FMT_BGR32
	{
		ms_message("msippyuv: conv MS_UYVY to MS_RGBA");
		ctx->conv = ipp_ippiYUV422ToRGB_8u_P3AC4R;
	}
	else if (ctx->in_fmt == MS_UYVY
		&& ctx->out_fmt == MS_RGB24)
	{
		ms_message("msippyuv: conv MS_UYVY to MS_RGB24");
		ctx->conv = ipp_ippiYUV422ToRGB_8u_P3C3R;
	}
	else if (ctx->in_fmt == MS_YUV420P
		&& ctx->out_fmt == MS_YUV420P
		&& ctx->srcW==ctx->dstW
		&& ctx->srcH==ctx->dstH)
	{
		ms_message("msippyuv: conv MS_YUV420P to MS_YUV420P");
		/* seems to be better that ctx->conv = simple_yuv_copy; */
		ctx->sws_ctx = sws_getContext(srcW, srcH, (enum PixelFormat)_ms_pix_fmt_to_ffmpeg(srcFormat), dstW, dstH, (enum PixelFormat)_ms_pix_fmt_to_ffmpeg(dstFormat),
			SWS_FAST_BILINEAR, NULL, NULL, param);
	}
	else if (ctx->in_fmt == MS_YUV420P
		&& ctx->out_fmt == MS_YUV420P)
	{
		ms_message("msippyuv: resizing MS_YUV420P to MS_YUV420P");
		ctx->conv = ipp_ippiResizeSqrPixel_8u_C1R;
	}
	else if (ctx->in_fmt == MS_UYVY
		&& ctx->out_fmt == MS_YUV420P)
	{
		ms_message("msippyuv: conv MS_UYVY to MS_YUV420P");
		ctx->conv = ipp_ippiCbYCr422ToYCbCr420_8u_C2P3R;
	}
	else if ((ctx->in_fmt == MS_YUYV || ctx->in_fmt == MS_YUY2)
		&& ctx->out_fmt == MS_YUV420P)
	{
		ms_message("msippyuv: conv MS_YUYV to MS_YUV420P");
		ctx->conv = ipp_ippiYCbCr422ToYCbCr420_8u_C2P3R;
	}
	else if (ctx->in_fmt == MS_RGB24
		&& ctx->out_fmt == MS_YUV420P)
	{
		ms_message("msippyuv: conv MS_RGB24 to MS_YUV420P");
		ctx->conv = ipp_ippiRGBToYCbCr420_8u_C3P3R;
	}
	else if (ctx->in_fmt == MS_RGB24_REV
		&& ctx->out_fmt == MS_YUV420P)
	{
		ms_message("msippyuv: conv MS_RGB24_REV to MS_YUV420P");
		ctx->conv = ipp_ippiBGRToYCbCr420_8u_C3P3R;
	}
	
	else
	{
		ms_message("msippyuv: fallback to swscale conversion");
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

void ipp_video_scalercontext_free(struct ipp_SwsContext *swsContext)
{
	if (swsContext==NULL)
		return;
	if (swsContext->sws_ctx!=NULL)
		sws_freeContext(swsContext->sws_ctx);
	ms_free(swsContext);
}

#ifdef PERF_TEST
static void perf_test_video_scalercontext_convert(struct ipp_SwsContext *ctx, uint8_t* srcSlice[], int srcStride[],
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

	ms_message("msippyuv: diff with sws_scale %ix%i -> %ix%i %s->%s: -- %"PRIu64" %"PRIu64"ms", ctx->srcW, ctx->srcH, ctx->dstW, ctx->dstH, ms_video_display_format(ctx->in_fmt), ms_video_display_format(ctx->out_fmt), perfswscale, perfipp);
}
#endif

static int ipp_video_scalercontext_convert(struct ipp_SwsContext *ctx, const uint8_t* srcSlice[], int srcStride[],
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

static void plane_copy(const uint8_t *src_plane, int src_stride,
	uint8_t *dst_plane, int dst_stride, MSVideoSize roi){
	int i;
	for(i=0;i<roi.height;++i){
		memcpy(dst_plane,src_plane,roi.width);
		src_plane+=src_stride;
		dst_plane+=dst_stride;
	}
}

static void yuv_buf_copy(uint8_t *src_planes[], const int src_strides[], 
		uint8_t *dst_planes[], const int dst_strides[3], MSVideoSize roi){
	plane_copy(src_planes[0],src_strides[0],dst_planes[0],dst_strides[0],roi);
	roi.width=roi.width/2;
	roi.height=roi.height/2;
	plane_copy(src_planes[1],src_strides[1],dst_planes[1],dst_strides[1],roi);
	plane_copy(src_planes[2],src_strides[2],dst_planes[2],dst_strides[2],roi);
}

#ifdef PERF_TEST
static void perf_test_yuv_buf_copy(uint8_t *src_planes[], const int src_strides[], 
		uint8_t *dst_planes[], const int dst_strides[3], MSVideoSize roi)
{
	uint64_t start, stop;
	uint64_t perfswscale, perfipp;
	int i;
	int counter;
	start = _my_gettime();
	for (counter=0;counter<COUNTER_MAX;counter++)
	{
		yuv_buf_copy(src_planes,src_strides, dst_planes,dst_strides,roi);
	}
	stop = _my_gettime();
	perfswscale = stop-start;
	for (counter=0;counter<COUNTER_MAX;counter++)
	{
		IppiSize sZ = {roi.width, roi.height};
		for (i=0;i<3;i++)
		{
			if (i == 1) {
				sZ.width >>= 1;
				sZ.height >>= 1;
			}
			ippiCopy_8u_C1R(src_planes[i],src_strides[i],
				dst_planes[i],dst_strides[i],sZ);
		}
	}
	perfipp =  stop-start;

	ms_message("msippyuv: yuv_buf_copy diff with C copy %ix%i: -- %"PRIu64" %"PRIu64"ms", roi.width, roi.height, perfswscale, perfipp);
}
#endif

void ipp_yuv_buf_copy(uint8_t *src_planes[], const int src_strides[], 
		uint8_t *dst_planes[], const int dst_strides[3], MSVideoSize roi)
{
	IppiSize sZ = {roi.width, roi.height};
	int i;

#undef PERF_TEST
#ifdef PERF_TEST
	perf_test_yuv_buf_copy(src_planes, src_strides, dst_planes, dst_strides, roi);
#endif

	for (i=0;i<3;i++)
	{
		if (i == 1) {
			sZ.width >>= 1;
			sZ.height >>= 1;
		}
		ippiCopy_8u_C1R(src_planes[i],src_strides[i],
			dst_planes[i],dst_strides[i],sZ);
	}

}

static void plane_mirror(uint8_t *p, int linesize, int w, int h){
	int i,j;
	uint8_t tmp;
	for(j=0;j<h;++j){
		for(i=0;i<w/2;++i){
			tmp=p[i];
			p[i]=p[w-1-i];
			p[w-1-i]=tmp;
		}
		p+=linesize;
	}
}

/*in place mirroring*/
static void yuv_buf_mirror(MSPicture *buf){
	plane_mirror(buf->planes[0],buf->strides[0],buf->w,buf->h);
	plane_mirror(buf->planes[1],buf->strides[1],buf->w/2,buf->h/2);
	plane_mirror(buf->planes[2],buf->strides[2],buf->w/2,buf->h/2);
}

#ifdef PERF_TEST
static void test_perf_yuv_buf_mirror(MSPicture *buf)
{
	uint64_t start, stop;
	uint64_t perfswscale, perfipp;
	int counter;
	int i;
	start = _my_gettime();
	for (counter=0;counter<COUNTER_MAX;counter++)
	{
		yuv_buf_mirror(buf);
	}
	stop = _my_gettime();
	perfswscale = stop-start;
	for (counter=0;counter<COUNTER_MAX;counter++)
	{
		MSPicture tmpbuf;
		mblk_t *b;
	
		IppiSize sZ = {buf->w, buf->h};
		MSVideoSize roi;
		roi.width=buf->w;
		roi.height=buf->h;

		b = yuv_buf_alloc(&tmpbuf, buf->w, buf->h);
		ipp_yuv_buf_copy(buf->planes,buf->strides,
				tmpbuf.planes,tmpbuf.strides,roi);

		sZ.width = buf->w;
		sZ.height = buf->h;

		for (i=0;i<3;i++)
		{
			if (i == 1) {
				sZ.width >>= 1;
				sZ.height >>= 1;
			}
			ippiMirror_8u_C1R(tmpbuf.planes[i],tmpbuf.strides[i],buf->planes[i], buf->strides[i], sZ, ippAxsVertical);
		}
		freemsg(b);
	}
	perfipp =  stop-start;

	ms_message("msippyuv: yuv_buf_mirror diff with C copy %ix%i: -- %"PRIu64" %"PRIu64"ms", buf->w, buf->h, perfswscale, perfipp);
}
#endif

void ipp_yuv_buf_mirror(MSPicture *buf)
{
	int i;
	MSPicture tmpbuf;
	mblk_t *b;
	
	IppiSize sZ = {buf->w, buf->h};
	MSVideoSize roi;

#ifdef PERF_TEST
	test_perf_yuv_buf_mirror(buf);
#endif

	roi.width=buf->w;
	roi.height=buf->h;

	b = yuv_buf_alloc(&tmpbuf, buf->w, buf->h);
	ipp_yuv_buf_copy(buf->planes,buf->strides,
			tmpbuf.planes,tmpbuf.strides,roi);

	sZ.width = buf->w;
	sZ.height = buf->h;

	for (i=0;i<3;i++)
	{
		if (i == 1) {
			sZ.width >>= 1;
			sZ.height >>= 1;
		}
		ippiMirror_8u_C1R(tmpbuf.planes[i],tmpbuf.strides[i],buf->planes[i], buf->strides[i], sZ, ippAxsVertical);
	}
	freemsg(b);
}

#ifdef WIN32
#define GLOBAL_LINKAGE __declspec(dllexport)
#else
#define GLOBAL_LINKAGE
#endif

GLOBAL_LINKAGE void libmsippyuv_init(void);

GLOBAL_LINKAGE void libmsippyuv_init(void){
	struct MSVideoDesc ms_video_desc;
	int plugin_quality=20;
	const IppLibraryVersion* ippj;
	ippStaticInit();

	ippj = ippiGetLibVersion();

	ms_message("msippyuv: Intel(R) Integrated Performance Primitives ");
	ms_message("  version: %s, [%d.%d.%d.%d] ",
		ippj->Version, ippj->major, ippj->minor, ippj->build, ippj->majorBuild);
	ms_message("  name:    %s ", ippj->Name);
	ms_message("  date:    %s ", ippj->BuildDate);

	memset(&ms_video_desc, 0, sizeof(ms_video_desc));
	ms_video_desc.quality_priority = plugin_quality;
	ms_video_desc.video_scalercontext_init=(ms_video_scalercontext_initFunc)ipp_video_scalercontext_init;
	ms_video_desc.video_scalercontext_free=(ms_video_scalercontext_freeFunc)ipp_video_scalercontext_free;
	ms_video_desc.video_scalercontext_convert=(ms_video_scalercontext_convertFunc)ipp_video_scalercontext_convert;
	ms_video_desc.yuv_buf_mirror=ipp_yuv_buf_mirror;
	ms_video_desc.yuv_buf_copy=ipp_yuv_buf_copy;
	ms_video_set_video_func(&ms_video_desc);
}
