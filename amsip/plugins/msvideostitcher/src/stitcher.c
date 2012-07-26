/** Copyright 2007 Simon Morlat, all rights reserved **/
/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2007-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#include <mediastreamer2/msfilter.h>
#include <mediastreamer2/msvideo.h>
#include <mediastreamer2/msticker.h>

#define STITCHER_MAX_INPUTS 9
#define STITCHER_INPUT_TIMEOUT 5000

#define AMD_MODE
//#define VIDEOSTITCHER_DUPLICATESELFVIEW

#include "stitcher.h"

extern Layout msstitcher_default_layout[];


typedef struct _InputInfo{
	uint64_t last_frame_time;
	struct MSScalerContext *sws_ctx;
	int input_w;
	int input_h;
	bool_t active;
	int counter;
} InputInfo;

static void input_info_reset_sws(InputInfo *info){
	if (info->sws_ctx!=NULL){
		ms_video_scalercontext_free(info->sws_ctx);
		info->sws_ctx=NULL;
	}
}

static void input_info_update(InputInfo *info, uint64_t curtime, bool_t has_data){
	if (has_data){
		info->active=TRUE;
		info->last_frame_time=curtime;
	}else if (curtime-info->last_frame_time>STITCHER_INPUT_TIMEOUT){	
		input_info_reset_sws(info);
		info->active=FALSE;
	}
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

static void input_info_process(InputInfo *info, mblk_t *im, MSPicture *dstframe, Region *pos, int background_color[3]){
	MSPicture inbuf,dest;
	int x,y,w,h;
	//AMD float ratio;
	int ratiow, ratioh;
	int wtmp,htmp;
	if (yuv_buf_init_from_mblk(&inbuf,im)!=0) return;

	//screen size:
	w=((int)((float)dstframe->w*pos->w)) & ~0x1;
	h=((int)((float)dstframe->h*pos->w)) & ~0x1;

	if (info->sws_ctx==NULL || (inbuf.w != info->input_w || inbuf.h != info->input_h)) {

		x=((int)(((float)dstframe->w*pos->x)-((float)w/2))) & ~0x1;
		y=((int)(((float)dstframe->h*pos->y)-((float)h/2))) & ~0x1;

		dest.w=w;
		dest.h=h;
		dest.planes[0]=dstframe->planes[0]+(y*dstframe->strides[0])+x;
		dest.planes[1]=dstframe->planes[1]+((y/2)*dstframe->strides[1])+(x/2);
		dest.planes[2]=dstframe->planes[2]+((y/2)*dstframe->strides[2])+(x/2);
		dest.planes[3]=NULL;
		dest.strides[0]=w;
		dest.strides[1]=w/2;
		dest.strides[2]=w/2;
		dest.strides[3]=0;
		yuv_buf_background(&dest, background_color[0], background_color[1], background_color[2]);
	}

	ratiow=inbuf.w;
	ratioh=inbuf.h;
	reduce(&ratiow, &ratioh);
	wtmp = w/ratiow*ratiow;
	htmp = h/ratioh*ratioh;

	if (htmp*ratiow>wtmp*ratioh)
	{
		wtmp = wtmp;
		htmp = wtmp*ratioh/ratiow;
	}
	else
	{
		htmp = htmp;
		wtmp = htmp*ratiow/ratioh;
	}
	
	w=wtmp;
	h=htmp;

	x=((int)(((float)dstframe->w*pos->x)-((float)w/2))) & ~0x1;

	//AMDratio=(float)w/(float)inbuf.w;
	//AMDh=(float)inbuf.h*ratio;
	//AMD y=((float)dstframe->h*pos->y)-((float)h/2);
	y=((int)(((float)dstframe->h*pos->y)-((float)h/2))) & ~0x1;

	dest.w=w;
	dest.h=h;
	dest.planes[0]=dstframe->planes[0]+(y*dstframe->strides[0])+x;
	dest.planes[1]=dstframe->planes[1]+((y/2)*dstframe->strides[1])+(x/2);
	dest.planes[2]=dstframe->planes[2]+((y/2)*dstframe->strides[2])+(x/2);
	dest.planes[3]=NULL;
	dest.strides[0]=w;
	dest.strides[1]=w/2;
	dest.strides[2]=w/2;
	dest.strides[3]=0;
	if (info->sws_ctx==NULL){

		info->input_w = inbuf.w;
		info->input_h = inbuf.h;
		info->sws_ctx=ms_video_scalercontext_init(inbuf.w,inbuf.h,MS_YUV420P,
			dest.w,dest.h,MS_YUV420P,MS_YUVFAST,
			NULL, NULL, NULL);
	}
	else if (inbuf.w != info->input_w || inbuf.h != info->input_h) {
		input_info_reset_sws(info);
		info->sws_ctx=ms_video_scalercontext_init(inbuf.w,inbuf.h,MS_YUV420P,
			dest.w,dest.h,MS_YUV420P,MS_YUVFAST,
			NULL, NULL, NULL);
		info->input_w = inbuf.w;
		info->input_h = inbuf.h;
	}

		if (ms_video_scalercontext_convert(info->sws_ctx,(uint8_t**)inbuf.planes,inbuf.strides, 0,
		inbuf.h, dest.planes, dstframe->strides)<0){
			ms_error("Error in ms_video_scalercontext_convert().");
	}
}

typedef struct _StitcherData{
	Layout *layout_table;
	Region *regions;
	InputInfo inputinfo[STITCHER_MAX_INPUTS];
	int counter;
	int nregions;
	MSVideoSize vsize;
	int background_color[3];
	mblk_t *frame_msg;
	MSPicture frame;
#ifdef AMD_MODE
	mblk_t *frame_msg_amd_mode;
	MSPicture frame_amd_mode;
#endif
	int pin_controller;
}StitcherData;

static void stitcher_init(MSFilter *f){
	StitcherData *s=ms_new0(StitcherData,1);
	s->vsize.width=MS_VIDEO_SIZE_CIF_W;
	s->vsize.height=MS_VIDEO_SIZE_CIF_H;
	s->background_color[0]=s->background_color[1]=s->background_color[2]=0;
	s->layout_table=msstitcher_default_layout;
	f->data=s;
}

static void stitcher_reset_all(MSFilter *f, StitcherData *d){
	int i;
	for (i=0;i<f->desc->ninputs;++i){
		input_info_reset_sws(&d->inputinfo[i]);
		/* ms_message("nb of incoming image on pin %i: %i", i, d->inputinfo[i].counter); */
		d->inputinfo[i].counter=0;
	}
	d->pin_controller=0;
	ms_message("nb of outgoing images %i", d->counter);
	d->counter=0;
	/* put frame black */
	if (d->frame_msg!=NULL){
		int ysize=d->frame.strides[0]*d->frame.h;
		memset(d->frame.planes[0],16,ysize);
		memset(d->frame.planes[1],128,ysize/4);
		memset(d->frame.planes[2],128,ysize/4);
		d->frame.planes[3]=NULL;
	}
#ifdef AMD_MODE
	/* put frame black */
	if (d->frame_msg_amd_mode!=NULL){
		int ysize=d->frame_amd_mode.strides[0]*d->frame_amd_mode.h;
		memset(d->frame_amd_mode.planes[0],16,ysize);
		memset(d->frame_amd_mode.planes[1],128,ysize/4);
		memset(d->frame_amd_mode.planes[2],128,ysize/4);
		d->frame_amd_mode.planes[3]=NULL;
	}
#endif
}

static void stitcher_uninit(MSFilter *f){
	StitcherData *d=(StitcherData*)f->data;
	stitcher_reset_all(f,d);
	ms_free(f->data);
}

static int check_inputs(MSFilter *f, StitcherData *d){
	int i;
	int active=0;
	uint64_t curtime=f->ticker->time;
	for (i=0;i<f->desc->ninputs;++i){
		if (f->inputs[i]!=NULL){
			input_info_update(&d->inputinfo[i],
				curtime,
				!ms_queue_empty(f->inputs[i]));
			if (d->inputinfo[i].active)
				++active;
		}
	}
	return active;
}

static void stitcher_update_layout(StitcherData *d, int nregions){
	Layout *it;
	for(it=d->layout_table;it->regions!=NULL;++it){
		if (it->nregions==nregions){
			/*found a layout for this number of regions*/
			d->nregions=nregions;
			d->regions=it->regions;
			return;	
		}
	}
	ms_error("No layout defined for %i regions",nregions);
}

static void stitcher_preprocess(MSFilter *f){
	StitcherData *d=(StitcherData*)f->data;
	d->frame_msg=yuv_buf_alloc(&d->frame,d->vsize.width,d->vsize.height);
#ifdef AMD_MODE
	d->frame_msg_amd_mode=yuv_buf_alloc(&d->frame_amd_mode,d->vsize.width,d->vsize.height);	
#endif
}

#ifdef DISABLE_STITCHER

static void stitcher_process(MSFilter *f){
	StitcherData *d=(StitcherData*)f->data;
	mblk_t *im;
    int i;

	check_inputs(f,d);
    /* find out which "first pin" is active */
    for (i=0;i<f->desc->ninputs;++i){
        if (f->inputs[i]!=NULL && d->inputinfo[i].active==TRUE ){
            break;
        }
    }

    if (i<f->desc->ninputs)
    {
#ifdef AMD_MODE
        /* copy image to output pins */
        im=ms_queue_get(f->inputs[i]);
        if (im!=NULL)
        {
            /* in this mode ONLY PIN 1 IS CONNECTED */
            if (f->outputs[0]!=NULL){
                ms_queue_put(f->outputs[0],im);
            }
        }
#else
        /* copy image to output pins */
        im=ms_queue_peek_last(f->inputs[i]);
        if (im!=NULL)
        {
            for(i=0;i<f->desc->noutputs;++i){
                if (f->outputs[i]!=NULL){
                    ms_queue_put(f->outputs[i],dupb(im));
                }
            }
        }
#endif
    }

    for (i=0;i<f->desc->ninputs;++i){
        if (f->inputs[i]!=NULL){
			ms_queue_flush(f->inputs[i]);
        }
    }
    return;
}

#else

static void stitcher_process(MSFilter *f){
#ifdef AMD_MODE
	int update_pin0=-1;
	int update_pin1=-1;
#endif
	int i,found;
	mblk_t *im;
	StitcherData *d=(StitcherData*)f->data;
	int nregions=check_inputs(f,d);	
    if (d->nregions!=nregions){
		stitcher_update_layout(d,nregions);
		stitcher_reset_all(f,d);
		d->pin_controller=0;
	}
    
#ifdef AMD_MODE
	if (d->nregions==2)
	{
		/* special mode to enable video conference with 1 guy:
		DO NOT send him his own picture. */
		/* First INPUT has local SOURCE connected */
		if (f->inputs[0]!=NULL && (im=ms_queue_peek_last(f->inputs[0]))!=NULL ){
			Region *pos=&msstitcher_default_layout->regions[0];
			input_info_process(&d->inputinfo[0],im,&d->frame,pos,d->background_color);
			ms_queue_flush(f->inputs[0]);
			//ms_message("WINDS: new image on pin0");
			update_pin0=1;
		}

		/* second INPUT is some RTP connected */
		for (i=1,found=0;i<f->desc->ninputs;++i){
			if (f->inputs[i]!=NULL && (im=ms_queue_peek_last(f->inputs[i]))!=NULL ){
				Region *pos=&msstitcher_default_layout->regions[0];
				input_info_process(&d->inputinfo[i],im,&d->frame_amd_mode,pos,d->background_color);
				ms_queue_flush(f->inputs[i]);
				update_pin1=1;
				break; /* only one in this case */
			}
		}
	}
	else
	{
		int max=0;
		update_pin0=1;
		update_pin1=1;

		/* select the fastest pin */
		d->pin_controller=0;
		for (i=0,found=0;i<f->desc->ninputs;++i){
			if (f->inputs[i]!=NULL && (im=ms_queue_peek_last(f->inputs[i]))!=NULL ){
				Region *pos=&d->regions[found];
				input_info_process(&d->inputinfo[i],im,&d->frame,pos,d->background_color);
				ms_queue_flush(f->inputs[i]);
				d->inputinfo[i].counter++;
			}
			if (d->inputinfo[i].counter>max){
				max=d->inputinfo[i].counter;
				d->pin_controller=i;
			}
			if (f->inputs[i]!=NULL && d->inputinfo[i].active)
				++found;
		}

		if (max>20) /* reset */
		{
			for (i=0,found=0;i<f->desc->ninputs;++i){
				/* ms_message("nb of incoming image on pin %i: %i", i, d->inputinfo[i].counter); */
				if (i==d->pin_controller)
					d->inputinfo[i].counter=3; /* keep this one advanced to avoid changing too often */
				else
					d->inputinfo[i].counter=0;
			}
			/* ms_message("nb of outgoing images %i", d->counter); */
			d->counter=0;
		}
	}
#else
	for (i=0,found=0;i<f->desc->ninputs;++i){
		if (f->inputs[i]!=NULL && (im=ms_queue_peek_last(f->inputs[i]))!=NULL ){
			Region *pos=&d->regions[found];
			input_info_process(&d->inputinfo[i],im,&d->frame,pos,d->background_color);
			ms_queue_flush(f->inputs[i]);
		}
		if (f->inputs[i]!=NULL && d->inputinfo[i].active)
			++found;
	}
#endif
#ifdef AMD_MODE
	if (d->nregions==2)
	{
		if (update_pin1==1) /* new frame_msg_amd_mode */
		{
			if (f->outputs[0]!=NULL){
				ms_queue_put(f->outputs[0],dupb(d->frame_msg_amd_mode));
			}
		}
		if (update_pin0==1) /* new frame_msg */
		{
			for(i=1;i<f->desc->noutputs;++i){
				if (f->outputs[i]!=NULL){
					ms_queue_put(f->outputs[i],dupb(d->frame_msg));
				}
			}
		}
	}
	else
	{
		if (d->inputinfo[d->pin_controller].last_frame_time==f->ticker->time) /* the pin controller was updated */
		{
			d->counter++;
			for(i=0;i<f->desc->noutputs;++i){
				if (f->outputs[i]!=NULL){
#ifdef VIDEOSTITCHER_DUPLICATESELFVIEW
					if (d->nregions==1 && d->inputinfo[0].active==0 && i>0) {
						//special case: we don't want remote image back to same user
					} else {
	                    ms_queue_put(f->outputs[i],dupb(d->frame_msg));
					}
#else
					if (d->nregions==1 && d->inputinfo[0].active && i==0)
					{
						//special case: we don't want this image in mainview
					} else if (d->nregions==1 && d->inputinfo[0].active==0 && i>0) {
						//special case: we don't want remote image back to same user
					} else
						ms_queue_put(f->outputs[i],dupb(d->frame_msg));
#endif
				}
			}
		}
	}
#else
	for(i=0;i<f->desc->noutputs;++i){
		if (f->outputs[i]!=NULL){
			ms_queue_put(f->outputs[i],dupb(d->frame_msg));
		}
	}
#endif
}

#endif

static void stitcher_postprocess(MSFilter *f){
	StitcherData *d=(StitcherData*)f->data;
	int i;

	stitcher_reset_all(f,d);
	freemsg(d->frame_msg);
	d->frame_msg=NULL;
#ifdef AMD_MODE
	freemsg(d->frame_msg_amd_mode);
	d->frame_msg_amd_mode=NULL;
#endif
	for (i=0;i<f->desc->ninputs;++i){
		d->inputinfo[i].active=FALSE;
	}
}

static int stitcher_set_vsize(MSFilter *f, void *data){
	StitcherData *d=(StitcherData*)f->data;
	MSVideoSize *sz=(MSVideoSize*)data;
	d->vsize=*sz;
	return 0;
}

static int stitcher_set_background_color(MSFilter *f,void *arg){
	StitcherData *d=(StitcherData*)f->data;
	d->background_color[0]=((int*)arg)[0];
	d->background_color[1]=((int*)arg)[1];
	d->background_color[2]=((int*)arg)[2];
	return 0;
}

static MSFilterMethod methods[]={
	{MS_FILTER_SET_VIDEO_SIZE,	stitcher_set_vsize},
	{MS_VIDEO_DISPLAY_SET_BACKGROUND_COLOR,	stitcher_set_background_color},
	{0,NULL}
};

#ifdef _MSC_VER

MSFilterDesc ms_video_stitcher_desc={
	MS_FILTER_PLUGIN_ID,
	"MSVideoStitcher",
	"A video stitching (mixing) filter",
	MS_FILTER_OTHER,
	NULL,
	STITCHER_MAX_INPUTS,
	STITCHER_MAX_INPUTS,
	stitcher_init,
	stitcher_preprocess,
	stitcher_process,
	stitcher_postprocess,
	stitcher_uninit,
	methods
};

#else

MSFilterDesc ms_video_stitcher_desc={
	MS_FILTER_PLUGIN_ID,
	"MSVideoStitcher",
	"A video stitching (mixing) filter",
	MS_FILTER_OTHER,
	NULL,
	STITCHER_MAX_INPUTS,
	STITCHER_MAX_INPUTS,
	stitcher_init,
	stitcher_preprocess,
	stitcher_process,
	stitcher_postprocess,
	stitcher_uninit,
	methods
};

#endif

#ifdef WIN32
#define GLOBAL_LINKAGE __declspec(dllexport)
#else
#define GLOBAL_LINKAGE
#endif

GLOBAL_LINKAGE void libmsvideostitcher_init(void);

GLOBAL_LINKAGE void libmsvideostitcher_init(void){
	ms_filter_register(&ms_video_stitcher_desc);
}
