/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006  Simon MORLAT (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef msvideo_h
#define msvideo_h

#include "msfilter.h"

/* some global constants for video MSFilter(s) */
#define MS_VIDEO_SIZE_SQCIF_W 128
#define MS_VIDEO_SIZE_SQCIF_H 96
#define MS_VIDEO_SIZE_WQCIF_W 256
#define MS_VIDEO_SIZE_WQCIF_H 144
#define MS_VIDEO_SIZE_QCIF_W 176
#define MS_VIDEO_SIZE_QCIF_H 144
#define MS_VIDEO_SIZE_CIF_W 352
#define MS_VIDEO_SIZE_CIF_H 288
#define MS_VIDEO_SIZE_ICIF_W 352
#define MS_VIDEO_SIZE_ICIF_H 576
#define MS_VIDEO_SIZE_4CIF_W 704
#define MS_VIDEO_SIZE_4CIF_H 576
#define MS_VIDEO_SIZE_W4CIF_W 1024
#define MS_VIDEO_SIZE_W4CIF_H 576
#define MS_VIDEO_SIZE_16CIF_W 1408
#define MS_VIDEO_SIZE_16CIF_H 1152

#define MS_VIDEO_SIZE_QQVGA_W 160
#define MS_VIDEO_SIZE_QQVGA_H 120
#define MS_VIDEO_SIZE_QVGA_W 320
#define MS_VIDEO_SIZE_QVGA_H 240
#define MS_VIDEO_SIZE_HVGA_W 480
#define MS_VIDEO_SIZE_HVGA_H 320
#define MS_VIDEO_SIZE_VGA_W 640
#define MS_VIDEO_SIZE_VGA_H 480
#define MS_VIDEO_SIZE_SVGA_W 800
#define MS_VIDEO_SIZE_SVGA_H 600

#define MS_VIDEO_SIZE_WVGA_W 800
#define MS_VIDEO_SIZE_WVGA_H 480
#define MS_VIDEO_SIZE_FWVGA_W 854
#define MS_VIDEO_SIZE_FWVGA_H 480
#define MS_VIDEO_SIZE_WQVGA_W 432
#define MS_VIDEO_SIZE_WQVGA_H 240
#define MS_VIDEO_SIZE_HQVGA_W 240
#define MS_VIDEO_SIZE_HQVGA_H 160
#define MS_VIDEO_SIZE_WSVGA_W 1024 /* 576P */
#define MS_VIDEO_SIZE_WSVGA_H 576 /* 576P */

#define MS_VIDEO_SIZE_NS1_W 324
#define MS_VIDEO_SIZE_NS1_H 248

#define MS_VIDEO_SIZE_QSIF_W 176
#define MS_VIDEO_SIZE_QSIF_H 120
#define MS_VIDEO_SIZE_SIF_W 352
#define MS_VIDEO_SIZE_SIF_H 240
#define MS_VIDEO_SIZE_ISIF_W 352
#define MS_VIDEO_SIZE_ISIF_H 480
#define MS_VIDEO_SIZE_4SIF_W 704
#define MS_VIDEO_SIZE_4SIF_H 480

#define MS_VIDEO_SIZE_288P_W 512
#define MS_VIDEO_SIZE_288P_H 288
#define MS_VIDEO_SIZE_432P_W 768
#define MS_VIDEO_SIZE_432P_H 432
#define MS_VIDEO_SIZE_448P_W 768
#define MS_VIDEO_SIZE_448P_H 448
#define MS_VIDEO_SIZE_480P_W 848
#define MS_VIDEO_SIZE_480P_H 480
#define MS_VIDEO_SIZE_576P_W 1024
#define MS_VIDEO_SIZE_576P_H 576
#define MS_VIDEO_SIZE_720P_W 1280
#define MS_VIDEO_SIZE_720P_H 720
#define MS_VIDEO_SIZE_1080P_W 1920
#define MS_VIDEO_SIZE_1080P_H 1080

#define MS_VIDEO_SIZE_SDTV_W 768
#define MS_VIDEO_SIZE_SDTV_H 576
#define MS_VIDEO_SIZE_HDTVP_W 1920
#define MS_VIDEO_SIZE_HDTVP_H 1200

#define MS_VIDEO_SIZE_XGA_W 1024
#define MS_VIDEO_SIZE_XGA_H 768
#define MS_VIDEO_SIZE_WXGA_W 1280
#define MS_VIDEO_SIZE_WXGA_H 768
#define MS_VIDEO_SIZE_XGAP_W 1152
#define MS_VIDEO_SIZE_XGAP_H 864
#define MS_VIDEO_SIZE_WXGAP_W 1440
#define MS_VIDEO_SIZE_WXGAP_H 900
#define MS_VIDEO_SIZE_SXGA_W 1280
#define MS_VIDEO_SIZE_SXGA_H 1024
#define MS_VIDEO_SIZE_SXGAP_W 1400
#define MS_VIDEO_SIZE_SXGAP_H 1050
#define MS_VIDEO_SIZE_WSXGAP_W 1680
#define MS_VIDEO_SIZE_WSXGAP_H 1050
#define MS_VIDEO_SIZE_UXGA_W 1600
#define MS_VIDEO_SIZE_UXGA_H 1200
#define MS_VIDEO_SIZE_WUXGA_W 1920
#define MS_VIDEO_SIZE_WUXGA_H 1200

/* previous definition for WXGA? */
#define MS_VIDEO_SIZE_WXGA_OTHER1_W 1080
#define MS_VIDEO_SIZE_WXGA_OTHER1_H 768

/* definition for some format used */
#define MS_VIDEO_SIZE_IOS1_W 192
#define MS_VIDEO_SIZE_IOS1_H 144

#define MS_VIDEO_SIZE_IOS2_W 480
#define MS_VIDEO_SIZE_IOS2_H 360

#define MS_VIDEO_SIZE_MAX_W MS_VIDEO_SIZE_1024_W
#define MS_VIDEO_SIZE_MAX_H MS_VIDEO_SIZE_1024_H


/* those structs are part of the ABI: don't change their size otherwise binary plugins will be broken*/

typedef struct MSVideoSize{
	int width,height;
} MSVideoSize;

typedef struct MSRect{
	int x,y,w,h;
} MSRect;

#define MS_VIDEO_SIZE_CIF (MSVideoSize){MS_VIDEO_SIZE_CIF_W,MS_VIDEO_SIZE_CIF_H}
#define MS_VIDEO_SIZE_QCIF (MSVideoSize){MS_VIDEO_SIZE_QCIF_W,MS_VIDEO_SIZE_QCIF_H}
#define MS_VIDEO_SIZE_4CIF (MSVideoSize){MS_VIDEO_SIZE_4CIF_W,MS_VIDEO_SIZE_4CIF_H}

#define MS_VIDEO_SIZE_QQVGA (MSVideoSize){MS_VIDEO_SIZE_QQVGA_W,MS_VIDEO_SIZE_QQVGA_H}
#define MS_VIDEO_SIZE_QVGA (MSVideoSize){MS_VIDEO_SIZE_QVGA_W,MS_VIDEO_SIZE_QVGA_H}
#define MS_VIDEO_SIZE_VGA (MSVideoSize){MS_VIDEO_SIZE_VGA_W,MS_VIDEO_SIZE_VGA_H}

#define MS_VIDEO_SIZE_720P (MSVideoSize){MS_VIDEO_SIZE_720P_W, MS_VIDEO_SIZE_720P_H}

#define MS_VIDEO_SIZE_NS1 (MSVideoSize){MS_VIDEO_SIZE_NS1_W,MS_VIDEO_SIZE_NS1_H}

#define MS_VIDEO_SIZE_XGA (MSVideoSize){MS_VIDEO_SIZE_XGA_W, MS_VIDEO_SIZE_XGA_H}

#define MS_VIDEO_SIZE_SVGA (MSVideoSize){MS_VIDEO_SIZE_SVGA_W, MS_VIDEO_SIZE_SVGA_H}

/*deprecated: use MS_VIDEO_SIZE_SVGA*/
#define MS_VIDEO_SIZE_800X600_W MS_VIDEO_SIZE_SVGA_W
#define MS_VIDEO_SIZE_800X600_H MS_VIDEO_SIZE_SVGA_H
#define MS_VIDEO_SIZE_800X600 MS_VIDEO_SIZE_SVGA
/*deprecated use MS_VIDEO_SIZE_XGA*/
#define MS_VIDEO_SIZE_1024_W 1024
#define MS_VIDEO_SIZE_1024_H 768
#define MS_VIDEO_SIZE_1024 MS_VIDEO_SIZE_XGA

typedef enum{
	MS_YUV420P,
	MS_YUYV,
	MS_RGB24,
	MS_RGB24_REV, /*->microsoft down-top bitmaps */
	MS_MJPEG,
	MS_UYVY,
	MS_YUY2,   /* -> same as MS_YUYV */
	MS_RGBA32,
	MS_NV21,
    MS_NV12,
	MS_ABGR,
	MS_ARGB,
	MS_RGB565,
	MS_PIX_FMT_UNKNOWN
}MSPixFmt;

#define MS_RGBA MS_RGBA32

typedef struct _MSPicture{
	int w,h;
	uint8_t *planes[4]; /*we usually use 3 planes, 4th is for compatibility */
	int strides[4];	/*with ffmpeg's swscale.h */
}MSPicture;

typedef struct _MSPicture YuvBuf; /*for backward compatibility*/

#ifdef __cplusplus
extern "C"{
#endif


MSPixFmt ms_fourcc_to_pix_fmt(uint32_t fourcc);
void ms_ffmpeg_check_init(void);
int yuv_buf_init_with_format(MSPicture *buf, MSPixFmt fmt, int w, int h, uint8_t *ptr);
int yuv_buf_init_from_mblk(MSPicture *buf, mblk_t *m);
void yuv_buf_init_from_mblk_with_size(MSPicture *buf, mblk_t *m, int w, int h);
mblk_t * yuv_buf_alloc(MSPicture *buf, int w, int h);
void yuv_buf_background(MSPicture *buf, int r, int g, int b);

void ms_yuv_buf_copy(uint8_t *src_planes[], const int src_strides[], 
		uint8_t *dst_planes[], const int dst_strides[3], MSVideoSize roi);
void ms_yuv_buf_mirror(MSPicture *buf);
void rgb24_mirror(uint8_t *buf, int w, int h, int linesize);
void rgb24_revert(uint8_t *buf, int w, int h, int linesize);
void rgb24_copy_revert(uint8_t *dstbuf, int dstlsz,
				const uint8_t *srcbuf, int srclsz, MSVideoSize roi);

static inline bool_t ms_video_size_greater_than(MSVideoSize vs1, MSVideoSize vs2){
	return (vs1.width>=vs2.width) && (vs1.height>=vs2.height);
}

static inline MSVideoSize ms_video_size_max(MSVideoSize vs1, MSVideoSize vs2){
	return ms_video_size_greater_than(vs1,vs2) ? vs1 : vs2;
}

static inline MSVideoSize ms_video_size_min(MSVideoSize vs1, MSVideoSize vs2){
	return ms_video_size_greater_than(vs1,vs2) ? vs2 : vs1;
}

static inline bool_t ms_video_size_equal(MSVideoSize vs1, MSVideoSize vs2){
	return vs1.width==vs2.width && vs1.height==vs2.height;
}

MSVideoSize ms_video_size_get_just_lower_than(MSVideoSize vs);

char *ms_video_display_format(MSPixFmt fmt);

struct MSScalerContext;

#define MS_YUVFAST    1
#define MS_YUVNORMAL  2
#define MS_YUVSLOW    4

struct MSScalerContext *ms_video_scalercontext_init(int srcW, int srcH, MSPixFmt srcFormat,
                                  int dstW, int dstH, MSPixFmt dstFormat,
                                  int flags, void *unused,
                                  void *unused2, double *param);
void ms_video_scalercontext_free(struct MSScalerContext *swsContext);
int ms_video_scalercontext_convert(struct MSScalerContext *context, uint8_t* srcSlice[], int srcStride[],
              int srcSliceY, int srcSliceH, uint8_t* dst[], int dstStride[]);

mblk_t *ms_yuv_load_mjpeg(uint8_t *jpgbuf, int bufsize, MSVideoSize *reqsize);

typedef struct MSScalerContext *(*ms_video_scalercontext_initFunc)(int srcW, int srcH, MSPixFmt srcFormat,
                                  int dstW, int dstH, MSPixFmt dstFormat,
                                  int flags, void *unused,
                                  void *unused2, double *param);
typedef void (*ms_video_scalercontext_freeFunc)(struct MSScalerContext *swsContext);
typedef int (*ms_video_scalercontext_convertFunc)(struct MSScalerContext *context, uint8_t* srcSlice[], int srcStride[],
              int srcSliceY, int srcSliceH, uint8_t* dst[], int dstStride[]);
typedef void (*yuv_buf_mirrorFunc)(MSPicture *buf);
typedef void (*yuv_buf_copyFunc)(uint8_t *src_planes[], const int src_strides[], 
		uint8_t *dst_planes[], const int dst_strides[3], MSVideoSize roi);
typedef mblk_t* (*yuv_load_mjpegFunc)(uint8_t *jpgbuf, int bufsize, MSVideoSize *reqsize);

struct MSVideoDesc {
	int quality_priority;
	ms_video_scalercontext_initFunc video_scalercontext_init;
	ms_video_scalercontext_freeFunc video_scalercontext_free;
	ms_video_scalercontext_convertFunc video_scalercontext_convert;
	yuv_buf_mirrorFunc yuv_buf_mirror;
	yuv_buf_copyFunc yuv_buf_copy;
};

void ms_video_set_video_func(struct MSVideoDesc *_ms_desc);

struct MSVideoJpegDesc {
	int quality_priority;
	yuv_load_mjpegFunc yuv_load_mjpeg;
};

void ms_video_set_videojpeg_func(struct MSVideoJpegDesc *_ms_desc);

#ifdef __cplusplus
}
#endif

#define MS_FILTER_SET_VIDEO_SIZE	MS_FILTER_BASE_METHOD(100,MSVideoSize)
#define MS_FILTER_GET_VIDEO_SIZE	MS_FILTER_BASE_METHOD(101,MSVideoSize)

#define MS_FILTER_SET_PIX_FMT		MS_FILTER_BASE_METHOD(102,MSPixFmt)
#define MS_FILTER_GET_PIX_FMT		MS_FILTER_BASE_METHOD(103,MSPixFmt)

#define MS_FILTER_SET_FPS		MS_FILTER_BASE_METHOD(104,float)
#define MS_FILTER_GET_FPS		MS_FILTER_BASE_METHOD(105,float)

/* request a video-fast-update (=I frame for H263,MP4V-ES) to a video encoder*/
#define MS_FILTER_REQ_VFU		MS_FILTER_BASE_METHOD_NO_ARG(106)

#define	MS_FILTER_SET_IMAGE	MS_FILTER_BASE_METHOD(107,char)

#endif
