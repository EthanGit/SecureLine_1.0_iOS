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

#include "mediastreamer2/msvideo.h"

#ifdef VIDEO_ENABLED

#if !defined(NO_FFMPEG)
#include "ffmpeg-priv.h"
#endif

static void yuv_buf_init(MSPicture *buf, int w, int h, uint8_t *ptr){
	int ysize,usize;
	ysize=w*h;
	usize=(ysize>>2);
	buf->w=w;
	buf->h=h;
	buf->planes[0]=ptr;
	buf->planes[1]=buf->planes[0]+ysize;
	buf->planes[2]=buf->planes[1]+usize;
	buf->planes[3]=0;
	buf->strides[0]=w;
	buf->strides[1]=(w>>1);
	buf->strides[2]=buf->strides[1];
	buf->strides[3]=0;
}

struct ms_yuv_desc {
	int format;
	int planar_fmt;
	int bpp;
	int planes;
};

#define MODE_PACKED 0
#define MODE_SEMIPLANAR 1
#define MODE_420 2
#define MODE_422 3
#define MODE_444 4

struct ms_yuv_desc _yuv_desc[] = {
	{ MS_YUV420P, MODE_420, 12, 3 },
	{ MS_YUYV, MODE_PACKED, 16, 1 },
	{ MS_RGB24, MODE_PACKED, 24, 1 },
	{ MS_RGB24_REV, MODE_PACKED, 24, 1 },
	{ MS_UYVY, MODE_PACKED, 16, 1 },
	{ MS_YUY2, MODE_PACKED, 16, 1 },
	{ MS_RGBA32, MODE_PACKED, 32, 1 },
	{ MS_NV21, MODE_SEMIPLANAR, 12, 2 },
	{ MS_NV12, MODE_SEMIPLANAR, 12, 2 },
	{ MS_ABGR, MODE_PACKED, 32, 1 },
	{ MS_ARGB, MODE_PACKED, 32, 1 },
	{ MS_RGB565, MODE_PACKED, 16, 1 },
	{ -1, -1, -1, -1 },
};

int yuv_buf_init_with_format(MSPicture *buf, MSPixFmt fmt, int pitch, int height, uint8_t *ptr){
	struct ms_yuv_desc *yuv_desc=NULL;
	int total_size=0;
	int i;

	if (buf->w==0)
		buf->w=pitch;
	if (buf->h==0)
		buf->h=height;

	buf->strides[0]=0;
	buf->strides[1]=0;
	buf->strides[2]=0;
	buf->strides[3]=0;
	buf->planes[0]=NULL;
	buf->planes[1]=NULL;
	buf->planes[2]=NULL;
	buf->planes[3]=NULL;

	i=0;
	while (_yuv_desc[i].format!=-1) {
		if (_yuv_desc[i].format==fmt)
		{
			yuv_desc = &_yuv_desc[i];
			break;
		}
		i++;
	}
	if (_yuv_desc[i].format==-1)
		return -1;

	if (yuv_desc->planar_fmt==MODE_PACKED) {
		buf->strides[0]= (pitch*yuv_desc->bpp)>>3;
		buf->planes[0]=ptr;
		total_size = height * buf->strides[0];
	} else if (yuv_desc->planar_fmt==MODE_420) {
		int ysize=pitch*height;
		buf->strides[0]= pitch;
		buf->strides[1]= (pitch>>1);
		buf->strides[2]= (pitch>>1);
		buf->planes[0]=ptr;
		buf->planes[1]=ptr+ysize;
		buf->planes[2]=buf->planes[1]+(ysize>>2);
		total_size = ysize + (ysize>>1);
	} else if (yuv_desc->planar_fmt==MODE_SEMIPLANAR) {
		int ysize=pitch*height;
		buf->strides[0]= pitch; //buf->w;
		buf->strides[1]= pitch; //buf->w;
		buf->planes[0]=ptr;
		buf->planes[1]=ptr+ysize;
		total_size=ysize+(ysize>>1);
	} else {
		return -1;
	}

	return total_size;
}

#if 0
int yuv_buf_init_with_format(MSPicture *buf, MSPixFmt fmt, int width, int height, uint8_t *ptr){
	int i;
	int total_size=0;

	/* max pixel step for each plane */
	int max_step[4] = { 0, 0, 0, 0 };
	/* the component for each plane which has the max pixel step */
	int max_step_comp[4] = { 0, 0, 0, 0 };;
	uint8_t num_shift_to_get_chroma_width = 0;
	uint8_t num_shift_to_get_chroma_height = 0;
	int size[4] = { 0, 0, 0, 0 };
	int has_plane[4] = { 1, 0, 0, 0 };

	buf->strides[0]=0;
	buf->strides[1]=0;
	buf->strides[2]=0;
	buf->strides[3]=0;
	buf->planes[0]=NULL;
	buf->planes[1]=NULL;
	buf->planes[2]=NULL;
	buf->planes[3]=NULL;

	if (fmt==MS_YUV420P) {
		max_step[0] = 1;
		max_step[1] = 1;
		max_step[2] = 1;
		max_step_comp[1] = 1;
		max_step_comp[2] = 2;
		num_shift_to_get_chroma_width=1;
		num_shift_to_get_chroma_height=1;
		has_plane[1] = 1;
		has_plane[2] = 1;
	} else if (fmt==MS_YUY2 || fmt==MS_YUYV) {
		max_step[0] = 4;
		max_step_comp[0] = 1;
		num_shift_to_get_chroma_width=1;
	} else if (fmt==MS_RGB24 || fmt==MS_RGB24_REV) {
		max_step[0] = 3;
	} else if (fmt==MS_UYVY) {
		max_step[0] = 4;
		max_step_comp[0] = 1;
		num_shift_to_get_chroma_width=1;
	} else if (fmt==MS_NV12 || fmt==MS_NV21) {
		max_step[0] = 1;
		max_step[1] = 2;
		max_step_comp[1] = 1;
		num_shift_to_get_chroma_width=1;
		num_shift_to_get_chroma_height=1;
		has_plane[1] = 1;
	} else if (fmt==MS_RGBA || fmt==MS_ABGR || fmt==MS_ARGB) {
		max_step[0] = 4;
	} else if (fmt==MS_RGB565) {
		max_step[0] = 2;
	} else {
		return 0;
	}

	for (i = 0; i < 4; i++) {
		int s = (max_step_comp[i] == 1 || max_step_comp[i] == 2) ? num_shift_to_get_chroma_width : 0;
		int shifted_w = ((width + (1 << s) - 1)) >> s;
		buf->strides[i] = max_step[i] * shifted_w;
	}

	buf->planes[0] = ptr;
	size[0] = buf->strides[0] * height;

	total_size = size[0];
	for (i = 1; has_plane[i] && i < 4; i++) {
		int h, s = (i == 1 || i == 2) ? num_shift_to_get_chroma_height : 0;
		buf->planes[i] = buf->planes[i-1] + size[i-1];
		h = (height + (1 << s) - 1) >> s;
		size[i] = h * buf->strides[i];
		total_size += size[i];
	}

	return total_size;
}
#endif

int yuv_buf_init_from_mblk(MSPicture *buf, mblk_t *m){
	int size=(int)(m->b_wptr-m->b_rptr);
	int w,h;

	w = mblk_get_video_width(m);
	h = mblk_get_video_height(m);
	if (w>0 && h>0){
		/* ms_warning("non-standard image size: %ix%i size=%i", w, h, size); */
	}else if (size==(MS_VIDEO_SIZE_IOS1_W*MS_VIDEO_SIZE_IOS1_H*3)/2){
		w=MS_VIDEO_SIZE_IOS1_W;
		h=MS_VIDEO_SIZE_IOS1_H;
	}else if (size==(MS_VIDEO_SIZE_IOS2_W*MS_VIDEO_SIZE_IOS2_H*3)/2){
		w=MS_VIDEO_SIZE_IOS2_W;
		h=MS_VIDEO_SIZE_IOS2_H;
	}else if (size==(MS_VIDEO_SIZE_QCIF_W*MS_VIDEO_SIZE_QCIF_H*3)/2){
		w=MS_VIDEO_SIZE_QCIF_W;
		h=MS_VIDEO_SIZE_QCIF_H;
	}else if (size==(MS_VIDEO_SIZE_CIF_W*MS_VIDEO_SIZE_CIF_H*3)/2){
		w=MS_VIDEO_SIZE_CIF_W;
		h=MS_VIDEO_SIZE_CIF_H;
	}else if (size==(MS_VIDEO_SIZE_QVGA_W*MS_VIDEO_SIZE_QVGA_H*3)/2){
		w=MS_VIDEO_SIZE_QVGA_W;
		h=MS_VIDEO_SIZE_QVGA_H;
	}else if (size==(MS_VIDEO_SIZE_VGA_W*MS_VIDEO_SIZE_VGA_H*3)/2){
		w=MS_VIDEO_SIZE_VGA_W;
		h=MS_VIDEO_SIZE_VGA_H;
	}else if (size==(MS_VIDEO_SIZE_4CIF_W*MS_VIDEO_SIZE_4CIF_H*3)/2){
		w=MS_VIDEO_SIZE_4CIF_W;
		h=MS_VIDEO_SIZE_4CIF_H;
	}else if (size==(MS_VIDEO_SIZE_W4CIF_W*MS_VIDEO_SIZE_W4CIF_H*3)/2){
		w=MS_VIDEO_SIZE_W4CIF_W;
		h=MS_VIDEO_SIZE_W4CIF_H;
	}else if (size==(MS_VIDEO_SIZE_16CIF_W*MS_VIDEO_SIZE_16CIF_H*3)/2){
		w=MS_VIDEO_SIZE_16CIF_W;
		h=MS_VIDEO_SIZE_16CIF_H;
	}else if (size==(MS_VIDEO_SIZE_SVGA_W*MS_VIDEO_SIZE_SVGA_H*3)/2){
		w=MS_VIDEO_SIZE_SVGA_W;
		h=MS_VIDEO_SIZE_SVGA_H;
	}else if (size==(MS_VIDEO_SIZE_SQCIF_W*MS_VIDEO_SIZE_SQCIF_H*3)/2){
		w=MS_VIDEO_SIZE_SQCIF_W;
		h=MS_VIDEO_SIZE_SQCIF_H;
	}else if (size==(MS_VIDEO_SIZE_QQVGA_W*MS_VIDEO_SIZE_QQVGA_H*3)/2){
		w=MS_VIDEO_SIZE_QQVGA_W;
		h=MS_VIDEO_SIZE_QQVGA_H;
	}else if (size==(MS_VIDEO_SIZE_NS1_W*MS_VIDEO_SIZE_NS1_H*3)/2){
		w=MS_VIDEO_SIZE_NS1_W;
		h=MS_VIDEO_SIZE_NS1_H;
	}else if (size==(MS_VIDEO_SIZE_QSIF_W*MS_VIDEO_SIZE_QSIF_H*3)/2){
		w=MS_VIDEO_SIZE_QSIF_W;
		h=MS_VIDEO_SIZE_QSIF_H;
	}else if (size==(MS_VIDEO_SIZE_SIF_W*MS_VIDEO_SIZE_SIF_H*3)/2){
		w=MS_VIDEO_SIZE_SIF_W;
		h=MS_VIDEO_SIZE_SIF_H;
	}else if (size==(MS_VIDEO_SIZE_4SIF_W*MS_VIDEO_SIZE_4SIF_H*3)/2){
		w=MS_VIDEO_SIZE_4SIF_W;
		h=MS_VIDEO_SIZE_4SIF_H;
	}else if (size==(MS_VIDEO_SIZE_288P_W*MS_VIDEO_SIZE_288P_H*3)/2){
		w=MS_VIDEO_SIZE_288P_W;
		h=MS_VIDEO_SIZE_288P_H;
	}else if (size==(MS_VIDEO_SIZE_432P_W*MS_VIDEO_SIZE_432P_H*3)/2){
		w=MS_VIDEO_SIZE_432P_W;
		h=MS_VIDEO_SIZE_432P_H;
	}else if (size==(MS_VIDEO_SIZE_448P_W*MS_VIDEO_SIZE_448P_H*3)/2){
		w=MS_VIDEO_SIZE_448P_W;
		h=MS_VIDEO_SIZE_448P_H;
	}else if (size==(MS_VIDEO_SIZE_480P_W*MS_VIDEO_SIZE_480P_H*3)/2){
		w=MS_VIDEO_SIZE_480P_W;
		h=MS_VIDEO_SIZE_480P_H;
	}else if (size==(MS_VIDEO_SIZE_576P_W*MS_VIDEO_SIZE_576P_H*3)/2){
		w=MS_VIDEO_SIZE_576P_W;
		h=MS_VIDEO_SIZE_576P_H;
	}else if (size==(MS_VIDEO_SIZE_720P_W*MS_VIDEO_SIZE_720P_H*3)/2){
		w=MS_VIDEO_SIZE_720P_W;
		h=MS_VIDEO_SIZE_720P_H;
	}else if (size==(MS_VIDEO_SIZE_1080P_W*MS_VIDEO_SIZE_1080P_H*3)/2){
		w=MS_VIDEO_SIZE_1080P_W;
		h=MS_VIDEO_SIZE_1080P_H;
	}else if (size==(MS_VIDEO_SIZE_SDTV_W*MS_VIDEO_SIZE_SDTV_H*3)/2){
		w=MS_VIDEO_SIZE_SDTV_W;
		h=MS_VIDEO_SIZE_SDTV_H;
	}else if (size==(MS_VIDEO_SIZE_HDTVP_W*MS_VIDEO_SIZE_HDTVP_H*3)/2){
		w=MS_VIDEO_SIZE_HDTVP_W;
		h=MS_VIDEO_SIZE_HDTVP_H;
	}else if (size==(MS_VIDEO_SIZE_XGA_W*MS_VIDEO_SIZE_XGA_H*3)/2){
		w=MS_VIDEO_SIZE_XGA_W;
		h=MS_VIDEO_SIZE_XGA_H;
	}else if (size==(MS_VIDEO_SIZE_WXGA_W*MS_VIDEO_SIZE_WXGA_H*3)/2){
		w=MS_VIDEO_SIZE_WXGA_W;
		h=MS_VIDEO_SIZE_WXGA_H;
	}else if (size==(MS_VIDEO_SIZE_WQCIF_W*MS_VIDEO_SIZE_WQCIF_H*3)/2){
		w=MS_VIDEO_SIZE_WQCIF_W;
		h=MS_VIDEO_SIZE_WQCIF_H;
	}else if (size==(MS_VIDEO_SIZE_HVGA_W*MS_VIDEO_SIZE_HVGA_H*3)/2){
		w=MS_VIDEO_SIZE_HVGA_W;
		h=MS_VIDEO_SIZE_HVGA_H;
	}else if (size==(MS_VIDEO_SIZE_WVGA_W*MS_VIDEO_SIZE_WVGA_H*3)/2){
		w=MS_VIDEO_SIZE_WVGA_W;
		h=MS_VIDEO_SIZE_WVGA_H;
	}else if (size==(MS_VIDEO_SIZE_FWVGA_W*MS_VIDEO_SIZE_FWVGA_H*3)/2){
		w=MS_VIDEO_SIZE_FWVGA_W;
		h=MS_VIDEO_SIZE_FWVGA_H;
	}else if (size==(MS_VIDEO_SIZE_FWVGA_W*MS_VIDEO_SIZE_WQVGA_H*3)/2){
		w=MS_VIDEO_SIZE_WQVGA_W;
		h=MS_VIDEO_SIZE_WQVGA_H;
	}else if (size==(MS_VIDEO_SIZE_HQVGA_W*MS_VIDEO_SIZE_HQVGA_H*3)/2){
		w=MS_VIDEO_SIZE_HQVGA_W;
		h=MS_VIDEO_SIZE_HQVGA_H;
	}else if (size==(MS_VIDEO_SIZE_WSVGA_W*MS_VIDEO_SIZE_WSVGA_H*3)/2){
		w=MS_VIDEO_SIZE_WSVGA_W;
		h=MS_VIDEO_SIZE_WSVGA_H;
	}else if (size==(MS_VIDEO_SIZE_XGAP_W*MS_VIDEO_SIZE_XGAP_H*3)/2){
		w=MS_VIDEO_SIZE_XGAP_W;
		h=MS_VIDEO_SIZE_XGAP_H;
	}else if (size==(MS_VIDEO_SIZE_WXGAP_W*MS_VIDEO_SIZE_WXGAP_H*3)/2){
		w=MS_VIDEO_SIZE_WXGAP_W;
		h=MS_VIDEO_SIZE_WXGAP_H;
	}else if (size==(MS_VIDEO_SIZE_SXGA_W*MS_VIDEO_SIZE_SXGA_H*3)/2){
		w=MS_VIDEO_SIZE_SXGA_W;
		h=MS_VIDEO_SIZE_SXGA_H;
	}else if (size==(MS_VIDEO_SIZE_SXGAP_W*MS_VIDEO_SIZE_SXGAP_H*3)/2){
		w=MS_VIDEO_SIZE_SXGAP_W;
		h=MS_VIDEO_SIZE_SXGAP_H;
	}else if (size==(MS_VIDEO_SIZE_WSXGAP_W*MS_VIDEO_SIZE_WSXGAP_H*3)/2){
		w=MS_VIDEO_SIZE_WSXGAP_W;
		h=MS_VIDEO_SIZE_WSXGAP_H;
	}else if (size==(MS_VIDEO_SIZE_UXGA_W*MS_VIDEO_SIZE_UXGA_H*3)/2){
		w=MS_VIDEO_SIZE_UXGA_W;
		h=MS_VIDEO_SIZE_UXGA_H;
	}else if (size==(MS_VIDEO_SIZE_WUXGA_W*MS_VIDEO_SIZE_WUXGA_H*3)/2){
		w=MS_VIDEO_SIZE_WUXGA_W;
		h=MS_VIDEO_SIZE_WUXGA_H;
	}else if (size==(MS_VIDEO_SIZE_WXGA_OTHER1_W*MS_VIDEO_SIZE_WXGA_OTHER1_H*3)/2){
		w=MS_VIDEO_SIZE_WXGA_OTHER1_W;
		h=MS_VIDEO_SIZE_WXGA_OTHER1_H;
	}else if (size==(656*368*3)/2){/*format used by lifesize*/
		w=656;
		h=368;
	}else if (size==(160*112*3)/2){/*format used by econf*/
		w=160;
		h=112;
	}else if (size==(320*200*3)/2){/*format used by gTalk */
		w=320;
		h=200;
	}else {
		ms_error("Unsupported image size: size=%i (bug somewhere !)",size);
		return -1;
	}
	yuv_buf_init(buf,w,h,m->b_rptr);
	mblk_set_video_width(m, w);
	mblk_set_video_height(m, h);
	mblk_set_video_format(m, MS_YUV420P);
	return 0;
}

void yuv_buf_init_from_mblk_with_size(MSPicture *buf, mblk_t *m, int w, int h){
	yuv_buf_init(buf,w,h,m->b_rptr);
	mblk_set_video_width(m, w);
	mblk_set_video_height(m, h);
	mblk_set_video_format(m, MS_YUV420P);
}

mblk_t * yuv_buf_alloc(MSPicture *buf, int w, int h){
	int size=(w*h*3)/2;
	mblk_t *msg=allocb(size,0);
	yuv_buf_init(buf,w,h,msg->b_wptr);
	mblk_set_video_width(msg, w);
	mblk_set_video_height(msg, h);
	mblk_set_video_format(msg, MS_YUV420P);
	msg->b_wptr+=size;
	return msg;
}

static int init_rgb_tables=0;
static int y_r[256], y_g[256], y_b[256];
static int u_r[256], u_g[256], u_b[256];
static int v_r[256], v_g[256], v_b[256];

void yuv_buf_background(MSPicture *buf, int r, int g, int b){
    unsigned int c_y, c_u, c_v, c_ysize;
    int i;
    if (init_rgb_tables==0)
    {
        /* initialize the RGB -> YUV tables */
        for (i = 0; i < 256; i++) {
            y_r[i] = (int)(0.257 * i * 65536); y_g[i] = (int)(0.504 * i * 65536); y_b[i] = (int)(0.098 * i * 65536);
            u_r[i] = (int)(-0.148 * i * 65536); u_g[i] = (int)(-0.291 * i * 65536); u_b[i] = (int)(0.439 * i * 65536);
            v_r[i] = (int)(0.439 * i * 65536); v_g[i] = (int)(-0.368 * i * 65536); v_b[i] = (int)(-0.071 * i * 65536);
        }
        init_rgb_tables=1;
    }
    c_y = (unsigned char)((y_r[r] + y_g[g] + y_b[b])/65536 + 16);
    c_u = (unsigned char)((u_r[r] + u_g[g] + u_b[b])/65536 + 128);
    c_v = (unsigned char)((v_r[r] + v_g[g] + v_b[b])/65536 + 128);
    c_ysize=buf->strides[0]*buf->h;
    memset(buf->planes[0],c_y,c_ysize);
    memset(buf->planes[1],c_u,c_ysize/4);
    memset(buf->planes[2],c_v,c_ysize/4);
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

#ifndef MAKEFOURCC
#define MAKEFOURCC(a,b,c,d) ((d)<<24 | (c)<<16 | (b)<<8 | (a))
#endif

MSPixFmt ms_fourcc_to_pix_fmt(uint32_t fourcc){
	MSPixFmt ret;
	switch (fourcc){
		case MAKEFOURCC('I','4','2','0'):
			ret=MS_YUV420P;
		break;
		case MAKEFOURCC('Y','U','Y','2'):
			ret=MS_YUY2;
		break;
		case MAKEFOURCC('Y','U','Y','V'):
			ret=MS_YUYV;
		break;
		case MAKEFOURCC('U','Y','V','Y'):
			ret=MS_UYVY;
		break;
		case 0: /*BI_RGB on windows*/
			ret=MS_RGB24;
		break;
		default:
			ret=MS_PIX_FMT_UNKNOWN;
	}
	return ret;
}

void rgb24_mirror(uint8_t *buf, int w, int h, int linesize){
	int i,j;
	int r,g,b;
	int end=w*3;
	for(i=0;i<h;++i){
		for(j=0;j<end/2;j+=3){
			r=buf[j];
			g=buf[j+1];
			b=buf[j+2];
			buf[j]=buf[end-j-3];
			buf[j+1]=buf[end-j-2];
			buf[j+2]=buf[end-j-1];
			buf[end-j-3]=r;
			buf[end-j-2]=g;
			buf[end-j-1]=b;
		}
		buf+=linesize;
	}
}

void rgb24_revert(uint8_t *buf, int w, int h, int linesize){
	uint8_t *p,*pe;
	int i,j;
	uint8_t *end=buf+((h-1)*linesize);
	uint8_t exch;
	p=buf;
	pe=end-1;
	for(i=0;i<h/2;++i){
		for(j=0;j<w*3;++j){
			exch=p[i];
			p[i]=pe[-i];
			pe[-i]=exch;
		}
		p+=linesize;
		pe-=linesize;
	}	
}

void rgb24_copy_revert(uint8_t *dstbuf, int dstlsz,
				const uint8_t *srcbuf, int srclsz, MSVideoSize roi){
	int i,j;
	const uint8_t *psrc;
	uint8_t *pdst;
	psrc=srcbuf;
	pdst=dstbuf+(dstlsz*(roi.height-1));
	for(i=0;i<roi.height;++i){
		for(j=0;j<roi.width*3;++j){
			pdst[(roi.width*3)-1-j]=psrc[j];
		}
		pdst-=dstlsz;
		psrc+=srclsz;
	}
}

static MSVideoSize _ordered_vsizes[]={
	{MS_VIDEO_SIZE_QCIF_W,MS_VIDEO_SIZE_QCIF_H},
	{MS_VIDEO_SIZE_QVGA_W,MS_VIDEO_SIZE_QVGA_H},
	{MS_VIDEO_SIZE_CIF_W,MS_VIDEO_SIZE_CIF_H},
	{MS_VIDEO_SIZE_VGA_W,MS_VIDEO_SIZE_VGA_H},
	{MS_VIDEO_SIZE_4CIF_W,MS_VIDEO_SIZE_4CIF_H},
	{MS_VIDEO_SIZE_720P_W,MS_VIDEO_SIZE_720P_H},
	{0,0}
};

MSVideoSize ms_video_size_get_just_lower_than(MSVideoSize vs){
	MSVideoSize *p;
	MSVideoSize ret;
	ret.width=0;
	ret.height=0;
	for(p=_ordered_vsizes;p->width!=0;++p){
		if (ms_video_size_greater_than(vs,*p) && !ms_video_size_equal(vs,*p)){
			ret=*p;
		}else return ret;
	}
	return ret;
}

char *ms_video_display_format(MSPixFmt fmt) {
	switch(fmt){
		case MS_YUV420P:
			return "MS_YUV420P";
		case MS_YUYV:
			return "MS_YUYV";     /* same as MS_YUY2 */
		case MS_RGB24:
			return "MS_RGB24";
		case MS_RGB24_REV:
			return "MS_RGB24_REV";
		case MS_UYVY:
			return "MS_UYVY";
		case MS_YUY2:
			return "MS_YUY2";
		case MS_RGBA:
			return "MS_RGBA";
		case MS_NV21:
			return "MS_NV21";
		case MS_NV12:
			return "MS_NV12";
		case MS_ABGR:
			return "MS_ABGR";
		case MS_ARGB:
			return "MS_ARGB";
		case MS_RGB565:
			return "MS_RGB565";
		default:
			return "unknown format";
	}
	return "unknown format";
}

#if !defined(NO_FFMPEG)

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

static int ms_pix_fmt_to_ffmpeg(MSPixFmt fmt){
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

	av_set_int(ctx->sws_ctx, "src_format", ms_pix_fmt_to_ffmpeg(srcFormat));
	av_set_int(ctx->sws_ctx, "dst_format", ms_pix_fmt_to_ffmpeg(dstFormat));
	
	if (sws_init_context(ctx->sws_ctx, NULL, NULL) < 0)
	{
		ms_error("msswscale: error initializing context");
		return ctx;
	}
#else
	ctx->sws_ctx = sws_getContext(srcW, srcH, (enum PixelFormat)ms_pix_fmt_to_ffmpeg(srcFormat),
																dstW, dstH, (enum PixelFormat)ms_pix_fmt_to_ffmpeg(dstFormat),
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
		ms_error("jpeg2yuv: avcodec_open failed");
		return NULL;
	}
	av_init_packet(&pkt);
	pkt.data=jpgbuf;
	pkt.size=bufsize;
	if (avcodec_decode_video2(&av_context,&orig,&got_picture,&pkt)<0){
		ms_error("jpeg2yuv: avcodec_decode_video failed");
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
		ms_error("jpeg2yuv: sws_alloc_context() failed.");
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
		ms_error("jpeg2yuv: sws_init_context() failed.");
		avcodec_close(&av_context);
		freemsg(ret);
		return NULL;
	}
#else
	sws_ctx=sws_getContext(av_context.width,av_context.height,av_context.pix_fmt,
		reqsize->width,reqsize->height,PIX_FMT_YUV420P,SWS_FAST_BILINEAR,
                NULL, NULL, NULL);
	if (sws_ctx==NULL) {
		ms_error("jpeg2yuv: ms_video_scalercontext_init() failed.");
		avcodec_close(&av_context);
		freemsg(ret);
		return NULL;
	}
#endif
	if (sws_scale(sws_ctx,(const uint8_t* const*)orig.data,orig.linesize,0,av_context.height,dest.data,dest.linesize)<0){
		ms_error("jpeg2yuv: ms_video_scalercontext_convert() failed.");
		sws_freeContext(sws_ctx);
		avcodec_close(&av_context);
		freemsg(ret);
		return NULL;
	}
	sws_freeContext(sws_ctx);
	avcodec_close(&av_context);
	return ret;
}

struct MSVideoDesc ms_video_desc = {
	50,
	(ms_video_scalercontext_initFunc)swscale_video_scalercontext_init,
	(ms_video_scalercontext_freeFunc)swscale_video_scalercontext_free,
	(ms_video_scalercontext_convertFunc)swscale_video_scalercontext_convert,
	yuv_buf_mirror,
	yuv_buf_copy
};

struct MSVideoJpegDesc ms_video_jpeg_desc = {
	50,
	yuv_load_mjpeg
};

#else

struct MSVideoDesc ms_video_desc = {
	1000,
	NULL,
	NULL,
	NULL,
	yuv_buf_mirror,
	yuv_buf_copy
};


struct MSVideoJpegDesc ms_video_jpeg_desc = {
	1000,
	NULL
};

#endif


struct MSScalerContext *ms_video_scalercontext_init(int srcW, int srcH, MSPixFmt srcFormat,
                                  int dstW, int dstH, MSPixFmt dstFormat,
                                  int flags, void *unused,
                                  void *unused2, double *param)
{
	if (ms_video_desc.video_scalercontext_init==NULL)
		return NULL;
	return (struct MSScalerContext *)ms_video_desc.video_scalercontext_init(srcW, srcH, srcFormat, dstW, dstH, dstFormat,
		flags, unused, unused2, param);
}

void ms_video_scalercontext_free(struct MSScalerContext *swsContext)
{
	if (ms_video_desc.video_scalercontext_init==NULL)
		return;
	ms_video_desc.video_scalercontext_free(swsContext);
}

int ms_video_scalercontext_convert(struct MSScalerContext *context, uint8_t* srcSlice[], int srcStride[],
              int srcSliceY, int srcSliceH, uint8_t* dst[], int dstStride[])
{
	if (ms_video_desc.video_scalercontext_init==NULL)
		return -1;
	return ms_video_desc.video_scalercontext_convert(context, srcSlice, srcStride, srcSliceY, srcSliceH, dst, dstStride);
}

mblk_t *ms_yuv_load_mjpeg(uint8_t *jpgbuf, int bufsize, MSVideoSize *reqsize)
{
	if (ms_video_jpeg_desc.yuv_load_mjpeg==NULL)
		return NULL;
	return ms_video_jpeg_desc.yuv_load_mjpeg(jpgbuf, bufsize, reqsize);
}

void ms_yuv_buf_mirror(MSPicture *buf)
{
	if (ms_video_desc.yuv_buf_mirror==NULL)
		return;
	return ms_video_desc.yuv_buf_mirror(buf);
}

void ms_yuv_buf_copy(uint8_t *src_planes[], const int src_strides[], 
		uint8_t *dst_planes[], const int dst_strides[3], MSVideoSize roi)
{
	if (ms_video_desc.yuv_buf_copy==NULL)
		return;
	return ms_video_desc.yuv_buf_copy(src_planes, src_strides, dst_planes, dst_strides, roi);
}

void ms_video_set_video_func(struct MSVideoDesc *_ms_video_desc)
{
	if (_ms_video_desc->quality_priority<=ms_video_desc.quality_priority)
	{
		ms_video_desc.quality_priority = _ms_video_desc->quality_priority;
		/* faster plugin detected: replace all method */
		if (_ms_video_desc->video_scalercontext_init!=NULL)
			ms_video_desc.video_scalercontext_init=_ms_video_desc->video_scalercontext_init;
		if (_ms_video_desc->video_scalercontext_free!=NULL)
			ms_video_desc.video_scalercontext_free=_ms_video_desc->video_scalercontext_free;
		if (_ms_video_desc->video_scalercontext_convert!=NULL)
			ms_video_desc.video_scalercontext_convert=_ms_video_desc->video_scalercontext_convert;
		if (_ms_video_desc->yuv_buf_mirror!=NULL)
			ms_video_desc.yuv_buf_mirror=_ms_video_desc->yuv_buf_mirror;
		if (_ms_video_desc->yuv_buf_copy!=NULL)
			ms_video_desc.yuv_buf_copy=_ms_video_desc->yuv_buf_copy;
		return;
	} else {
		/* slower plugin detected: replace all method only if they are not yet implemented */
		if (_ms_video_desc->video_scalercontext_init!=NULL && ms_video_desc.video_scalercontext_init==NULL)
			ms_video_desc.video_scalercontext_init=_ms_video_desc->video_scalercontext_init;
		if (_ms_video_desc->video_scalercontext_free!=NULL && ms_video_desc.video_scalercontext_free==NULL)
			ms_video_desc.video_scalercontext_free=_ms_video_desc->video_scalercontext_free;
		if (_ms_video_desc->video_scalercontext_convert!=NULL && ms_video_desc.video_scalercontext_convert==NULL)
			ms_video_desc.video_scalercontext_convert=_ms_video_desc->video_scalercontext_convert;
		if (_ms_video_desc->yuv_buf_mirror!=NULL && ms_video_desc.yuv_buf_mirror==NULL)
			ms_video_desc.yuv_buf_mirror=_ms_video_desc->yuv_buf_mirror;
		if (_ms_video_desc->yuv_buf_copy!=NULL && ms_video_desc.yuv_buf_copy==NULL)
			ms_video_desc.yuv_buf_copy=_ms_video_desc->yuv_buf_copy;
	}
}

void ms_video_set_videojpeg_func(struct MSVideoJpegDesc *_ms_video_jpeg_desc)
{
	if (_ms_video_jpeg_desc->quality_priority<=ms_video_jpeg_desc.quality_priority)
	{
		ms_video_jpeg_desc.quality_priority = _ms_video_jpeg_desc->quality_priority;
		/* faster plugin detected: replace all method */
		if (_ms_video_jpeg_desc->yuv_load_mjpeg!=NULL)
			ms_video_jpeg_desc.yuv_load_mjpeg=_ms_video_jpeg_desc->yuv_load_mjpeg;
	} else {
		/* slower plugin detected: replace all method only if they are not yet implemented */
		if (_ms_video_jpeg_desc->yuv_load_mjpeg!=NULL && ms_video_jpeg_desc.yuv_load_mjpeg==NULL)
			ms_video_jpeg_desc.yuv_load_mjpeg=_ms_video_jpeg_desc->yuv_load_mjpeg;
	}
}

#endif

#ifndef VIDEO_ENABLED

char *ms_video_display_format(MSPixFmt fmt) {
	return "unknown format";
}

void yuv_buf_background(MSPicture *buf, int r, int g, int b){
	//not implemented
}

struct MSScalerContext *ms_video_scalercontext_init(int srcW, int srcH, MSPixFmt srcFormat,
                                  int dstW, int dstH, MSPixFmt dstFormat,
                                  int flags, void *unused,
                                  void *unused2, double *param)
{
	return NULL; //not implemented
}

void ms_video_scalercontext_free(struct MSScalerContext *swsContext)
{
	 //not implemented
}

int ms_video_scalercontext_convert(struct MSScalerContext *context, uint8_t* srcSlice[], int srcStride[],
              int srcSliceY, int srcSliceH, uint8_t* dst[], int dstStride[])
{
	return -1; //not implemented
}

mblk_t *ms_yuv_load_mjpeg(uint8_t *jpgbuf, int bufsize, MSVideoSize *reqsize)
{
	return NULL; //not implemented
}

void ms_yuv_buf_mirror(MSPicture *buf)
{
	//not implemented
}

void ms_yuv_buf_copy(uint8_t *src_planes[], const int src_strides[], 
		uint8_t *dst_planes[], const int dst_strides[3], MSVideoSize roi)
{
	//not implemented
}

void ms_video_set_video_func(struct MSVideoDesc *_ms_video_desc){
	//not implemented
}

void ms_video_set_videojpeg_func(struct MSVideoJpegDesc *_ms_desc)
{
	//not implemented
}

int yuv_buf_init_with_format(MSPicture *buf, MSPixFmt fmt, int width, int height, uint8_t *ptr){
	return -1; //not implemented
}

int yuv_buf_init_from_mblk(MSPicture *buf, mblk_t *m){
	return -1; //not implemented
}

void yuv_buf_init_from_mblk_with_size(MSPicture *buf, mblk_t *m, int w, int h){
	//not implemented
}

mblk_t * yuv_buf_alloc(MSPicture *buf, int w, int h){
	return NULL; //not implemented
}
#endif
