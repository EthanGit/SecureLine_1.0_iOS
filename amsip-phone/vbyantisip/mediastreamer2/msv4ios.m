//
//  msv4ios.m
//  mediastreamer2
//
//  Created by Aymeric MOIZARD on 9/29/11.
//  Copyright 2011 antisip. All rights reserved.
//

#import <AVFoundation/AVFoundation.h>

#include "mediastreamer-config.h"
#include "mediastreamer2/msvideo.h"
#include "mediastreamer2/msticker.h"
#include "mediastreamer2/msv4l.h"
#include "mediastreamer2/mswebcam.h"
#include "nowebcam.h"

typedef struct v4mState{	
	ms_mutex_t mutex;
	queue_t rq;
	
	MSVideoSize size;
	int pix_fmt;
	
	mblk_t *mire;
	int frame_ind;
	float fps;
    AVCaptureVideoOrientation mCurrentOrientation;
	float start_time;
	int frame_count;
	bool_t usemire;
}v4mState;

static void v4m_init(MSFilter *f){
	v4mState *s=ms_new0(v4mState,1);
    
	qinit(&s->rq);
	ms_mutex_init(&s->mutex,NULL);
	s->size.width = MS_VIDEO_SIZE_QCIF_W;
	s->size.height = MS_VIDEO_SIZE_QCIF_H;
	s->pix_fmt=MS_NV12;
	
	s->mire=NULL;	
	s->start_time=0;
	s->frame_count=-1;
	s->fps=15;
    s->mCurrentOrientation = AVCaptureVideoOrientationPortrait;
    
	//s->pix_fmt=MS_YUV420P;
	s->usemire=false;
	
	f->data=s;
}

static int v4m_start(MSFilter *f, void *arg) {
	ms_message("v4m video device opened.");
	return 0;
}

static int v4m_stop(MSFilter *f, void *arg){
	ms_message("v4m video device closed.");
	return 0;
}

static void v4m_uninit(MSFilter *f){
	v4mState *s=(v4mState*)f->data;
	v4m_stop(f,NULL);
	
	flushq(&s->rq,0);
	ms_mutex_destroy(&s->mutex);
	
	freemsg(s->mire);
	ms_free(s);
}

static mblk_t * v4m_make_mire(v4mState *s){
	unsigned char *data;
	int i,j,line,pos;
	int patternw=s->size.width/6; 
	int patternh=s->size.height/6;
	int red,green=0,blue=0;
	if (s->mire==NULL){
		s->mire=allocb(s->size.width*s->size.height*3,0);
		s->mire->b_wptr=s->mire->b_datap->db_lim;
	}
	data=s->mire->b_rptr;
	for (i=0;i<s->size.height;++i){
		line=i*s->size.width*3;
		if ( ((i+s->frame_ind)/patternh) & 0x1) red=255;
		else red= 0;
		for (j=0;j<s->size.width;++j){
			pos=line+(j*3);
			
			if ( ((j+s->frame_ind)/patternw) & 0x1) blue=255;
			else blue= 0;
			
			data[pos]=red;
			data[pos+1]=green;
			data[pos+2]=blue;
		}
	}
	s->frame_ind++;
	return s->mire;
}

static void v4m_process(MSFilter * obj){
	v4mState *s=(v4mState*)obj->data;
	uint32_t timestamp;
	int cur_frame;
	if (s->frame_count==-1){
		s->start_time=obj->ticker->time;
		s->frame_count=0;
	}
	
	ms_mutex_lock(&s->mutex);
	
	cur_frame=((obj->ticker->time-s->start_time)*s->fps/1000.0);
	if (cur_frame>=s->frame_count)
	{
		mblk_t *om=NULL;
		
		if (s->usemire==true)
		{
			if (s->pix_fmt==MS_YUV420P)
			{
				if (s->mire==NULL)
					s->mire=ms_load_nowebcam(&s->size, -1);
				//om=dupmsg(v4m_make_mire(s));
				om=dupmsg(s->mire);
			}
		}
		else {
			/*keep the most recent frame if several frames have been captured */
			om=getq(&s->rq);
		}
		
		if (om!=NULL)
		{
			timestamp=obj->ticker->time*90;/* rtp uses a 90000 Hz clockrate for video*/
			mblk_set_timestamp_info(om,timestamp);
			mblk_set_marker_info(om,TRUE);
            //if (s->mCurrentOrientation==AVCaptureVideoOrientationPortrait
            //    ||s->mCurrentOrientation==AVCaptureVideoOrientationPortraitUpsideDown)
            //    mblk_set_payload_type(om, 90); //ask for rotation
			ms_queue_put(obj->outputs[0],om);
			s->frame_count++;
		}
	}
	else
		flushq(&s->rq,0);
	
	ms_mutex_unlock(&s->mutex);
}

static void v4m_preprocess(MSFilter *f){
	v4m_start(f,NULL);
	
}

static void v4m_postprocess(MSFilter *f){
	v4m_stop(f,NULL);
}

static int v4m_set_fps(MSFilter *f, void *arg){
	v4mState *s=(v4mState*)f->data;
	s->fps=*((float*)arg);
	s->frame_count=-1;
	return 0;
}

static int v4m_get_pix_fmt(MSFilter *f,void *arg){
	v4mState *s=(v4mState*)f->data;
	*((MSPixFmt*)arg) = s->pix_fmt; //TODO: must be MS_NV12
	return 0;
}

static int v4m_set_vsize(MSFilter *f, void *arg){
	v4mState *s=(v4mState*)f->data;
	s->size=*(MSVideoSize*)arg;
	return 0;
}

static int v4m_get_vsize(MSFilter *f, void *arg){
	v4mState *s=(v4mState*)f->data;
	*(MSVideoSize*)arg = s->size;
	return 0;
}

static int v4m_set_data(MSFilter *f, void *arg){
	v4mState *s=(v4mState*)f->data;
	ms_mutex_lock(&s->mutex);
    flushq(&s->rq,0);
	putq(&s->rq, arg);
	ms_mutex_unlock(&s->mutex);
	return 0;
}

static int v4m_set_orientation(MSFilter *f, void *arg){
	v4mState *s=(v4mState*)f->data;
	ms_mutex_lock(&s->mutex);	
	s->mCurrentOrientation = *(AVCaptureVideoOrientation*)arg;
	ms_mutex_unlock(&s->mutex);
	return 0;
}


static MSFilterMethod methods[]={
	{	MS_FILTER_SET_FPS		,	v4m_set_fps		},
	{	MS_FILTER_GET_PIX_FMT	,	v4m_get_pix_fmt	},
	{	MS_FILTER_SET_VIDEO_SIZE, 	v4m_set_vsize	},
	{	MS_V4L_START			,	v4m_start		},
	{	MS_V4L_STOP				,	v4m_stop		},
	{	MS_FILTER_GET_VIDEO_SIZE,	v4m_get_vsize	},
	{	MS_FILTER_SET_DATA		,	v4m_set_data	},
	{	MS_FILTER_SET_ORIENTATION,	v4m_set_orientation	},
	{	0						,	NULL			}
};

MSFilterDesc ms_v4m_desc={
	.id=MS_V4L_ID,
	.name="MSV4m",
	.text="A video for macosx compatible source filter to stream pictures.",
	.ninputs=0,
	.noutputs=1,
	.category=MS_FILTER_OTHER,
	.init=v4m_init,
	.preprocess=v4m_preprocess,
	.process=v4m_process,
	.postprocess=v4m_postprocess,
	.uninit=v4m_uninit,
	.methods=methods
};

MS_FILTER_DESC_EXPORT(ms_v4m_desc)

static void ms_v4m_detect(MSWebCamManager *obj);

static void ms_v4m_cam_init(MSWebCam *cam)
{
}

static MSFilter *ms_v4m_create_reader(MSWebCam *obj)
{	
	MSFilter *f= ms_filter_new_from_desc(&ms_v4m_desc); 
    
	return f;
}

MSWebCamDesc ms_v4m_cam_desc={
	"VideoForIphone grabber",
	&ms_v4m_detect,
    NULL,
	&ms_v4m_cam_init,
	&ms_v4m_create_reader,
	NULL
};


static void ms_v4m_detect(MSWebCamManager *obj){
	
	MSWebCam *cam=ms_web_cam_new(&ms_v4m_cam_desc);
	
	cam->name= ms_strdup("iphone video grabber");
	cam->id = ms_strdup("iphone id");
	cam->data = NULL;
	ms_web_cam_manager_add_cam(obj,cam);
}
