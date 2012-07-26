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

#ifdef HAVE_CONFIG_H
#include "mediastreamer-config.h"
#endif

#ifdef VIDEO_ENABLED

#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msvideo.h"

typedef struct PixConvState{
	MSPicture outbuf;
	mblk_t *yuv_msg;
	struct MSScalerContext *sws_ctx;
	MSVideoSize size;
	MSPixFmt in_fmt;
	MSPixFmt out_fmt;
	int reset;
}PixConvState;

static void pixconv_init(MSFilter *f){
	PixConvState *s=(PixConvState *)ms_new(PixConvState,1);
	s->yuv_msg=NULL;
	s->size.width = MS_VIDEO_SIZE_CIF_W;
	s->size.height = MS_VIDEO_SIZE_CIF_H;
	s->in_fmt=MS_YUV420P;
	s->out_fmt=MS_YUV420P;
	s->sws_ctx=NULL;
	s->reset=0;
	f->data=s;
}

static void pixconv_uninit(MSFilter *f){
	PixConvState *s=(PixConvState*)f->data;
	if (s->sws_ctx!=NULL){
		ms_video_scalercontext_free(s->sws_ctx);
		s->sws_ctx=NULL;
	}
	if (s->yuv_msg!=NULL) freemsg(s->yuv_msg);
	ms_free(s);
}

static mblk_t * pixconv_alloc_mblk(PixConvState *s){
	if (s->yuv_msg!=NULL){
		int ref=s->yuv_msg->b_datap->db_ref;
		if (ref==1){
			return dupmsg(s->yuv_msg);
		}else{
			/*the last msg is still referenced by somebody else*/
			ms_message("Somebody still retaining yuv buffer (ref=%i)",ref);
			freemsg(s->yuv_msg);
			s->yuv_msg=NULL;
		}
	}
	s->yuv_msg=yuv_buf_alloc(&s->outbuf,s->size.width,s->size.height);
	return dupmsg(s->yuv_msg);
}

#define rot(x, y, p1, p2) \
{ register int i,j; register int k = 0; \
for (i = 0; i < (int)x; i ++ ){ \
for( j = y-1; j >=0; j -- ){ \
p2[k++] = p1[j * x + i]; } } }

static void pixconv_process(MSFilter *f){
	mblk_t *im,*om;
	PixConvState *s=(PixConvState*)f->data;

	ms_filter_lock(f);

	if (s->reset==1)
	{
		/* live change happen on incoming data */
		if (s->sws_ctx!=NULL){
			ms_video_scalercontext_free(s->sws_ctx);
			s->sws_ctx=NULL;
		}
		if (s->yuv_msg!=NULL) freemsg(s->yuv_msg);
		s->yuv_msg=NULL;
		ms_queue_flush(f->inputs[0]);
		s->reset=0;
    }
    
	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		if (s->in_fmt==s->out_fmt && (((im)->reserved2>>3)&0x7F)==0){
			om=im;
            if (om!=NULL) ms_queue_put(f->outputs[0],om);
		}else{
			MSPicture inbuf;
			int rotation=0;
			inbuf.w=0;
			inbuf.h=0;
			yuv_buf_init_with_format(&inbuf,s->in_fmt,s->size.width,s->size.height, im->b_rptr);
            
            om=pixconv_alloc_mblk(s);
            if (s->sws_ctx==NULL){
                s->sws_ctx=ms_video_scalercontext_init(s->size.width,s->size.height,
                                             s->in_fmt,s->size.width,s->size.height,
                                             s->out_fmt,MS_YUVFAST,
                                             NULL, NULL, NULL);
                
                ms_message("MSPixConv: conversion from %i:%ix%i -> %i:%ix%i. (size=%i)",
                           s->in_fmt,
                           s->size.width,s->size.height,
                           s->out_fmt,
                           s->size.width,s->size.height,
                           msgdsize(im));
                //freemsg(om);
                //freemsg(im);
                //continue;
            }
            if (s->in_fmt==MS_RGB24_REV){
				inbuf.planes[0]+=inbuf.strides[0]*(s->size.height-1);
                inbuf.strides[0]=-inbuf.strides[0];
            }
            if (ms_video_scalercontext_convert(s->sws_ctx,inbuf.planes,inbuf.strides, 0,
                             s->size.height, s->outbuf.planes, s->outbuf.strides)<0){
                ms_error("MSPixConv: Error in ms_video_scalercontext_convert().");
                freemsg(om);
                freemsg(im);
                continue;
            }
            rotation = (((im)->reserved2>>3)&0x7F);
            freemsg(im);
            
            if (s->out_fmt==MS_YUV420P && rotation==90)
            {
                MSPicture rotyuvbuf;
                int srcW=s->size.height;
                int srcH=s->size.width;
                mblk_t *out=yuv_buf_alloc(&rotyuvbuf,srcW,srcH);
				inbuf.w=0;
				inbuf.h=0;
                yuv_buf_init_with_format(&inbuf, s->out_fmt,s->size.width,s->size.height, om->b_rptr);
                //rotate 90 degree
                rot(s->size.width, s->size.height, inbuf.planes[0], rotyuvbuf.planes[0]);
                rot(s->size.width/2, s->size.height/2, inbuf.planes[1], rotyuvbuf.planes[1]);
                rot(s->size.width/2, s->size.height/2, inbuf.planes[2], rotyuvbuf.planes[2]);

                //ms_message("MSPixConv: rotation from %i:%ix%i -> %i:%ix%i.",
                //           s->in_fmt,
                //           s->size.width,s->size.height,
                //           s->out_fmt,
                //           srcW,srcH);
                freemsg(om);
                if (out!=NULL) ms_queue_put(f->outputs[0],out);
            } else {
                if (om!=NULL) ms_queue_put(f->outputs[0],om);
            }
            
		}
	}
	ms_filter_unlock(f);
}

static int pixconv_set_vsize(MSFilter *f, void*arg){
	PixConvState *s=(PixConvState*)f->data;
	MSVideoSize vsize=*(MSVideoSize*)arg;
	ms_filter_lock(f);
	if (vsize.width!=s->size.width || vsize.height!=s->size.height)
	{
		s->reset=1;
	}
	s->size=*(MSVideoSize*)arg;
	ms_filter_unlock(f);
	return 0;
}

static int pixconv_set_pixfmt(MSFilter *f, void *arg){
	MSPixFmt fmt=*(MSPixFmt*)arg;
	PixConvState *s=(PixConvState*)f->data;
	ms_filter_lock(f);
	if (s->in_fmt!=fmt)
	{
		s->reset=1;
	}
	s->in_fmt=fmt;
	ms_filter_unlock(f);
	return 0;
}

static int get_statistics(MSFilter *f, void *arg){
	int i;
	ms_warning("filter: %s[%i->%i]", f->desc->name,
		f->desc->ninputs, f->desc->noutputs);
	for (i=0;i<f->desc->ninputs;i++) {
		if (f->inputs[i]!=NULL) ms_warning("filter: %s in[%i]=%i", f->desc->name,
			i, f->inputs[i]->q.q_mcount);
	}
	for (i=0;i<f->desc->noutputs;i++) {
		if (f->outputs[i]!=NULL) ms_warning("filter: %s out[%i]=%i", f->desc->name,
			i, f->outputs[i]->q.q_mcount);
	}
	return 0;
}

static MSFilterMethod methods[]={
	{	MS_FILTER_SET_VIDEO_SIZE, pixconv_set_vsize	},
	{	MS_FILTER_SET_PIX_FMT,	pixconv_set_pixfmt	},
	{	MS_FILTER_GET_STATISTICS, get_statistics },
	{	0	,	NULL }
};

#ifdef _MSC_VER

MSFilterDesc ms_pix_conv_desc={
	MS_PIX_CONV_ID,
	"MSPixConv",
	N_("A pixel format converter"),
	MS_FILTER_OTHER,
	NULL,
	1,
	1,
	pixconv_init,
	NULL,
	pixconv_process,
	NULL,
	pixconv_uninit,
	methods
};

#else

MSFilterDesc ms_pix_conv_desc={
	.id=MS_PIX_CONV_ID,
	.name="MSPixConv",
	.text=N_("A pixel format converter"),
	.category=MS_FILTER_OTHER,
	.ninputs=1,
	.noutputs=1,
	.init=pixconv_init,
	.process=pixconv_process,
	.uninit=pixconv_uninit,
	.methods=methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_pix_conv_desc)

#endif
