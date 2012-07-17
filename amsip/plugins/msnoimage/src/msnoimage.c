/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#include "mediastreamer2/mscommon.h"
#include "mediastreamer2/msvideo.h"
#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msticker.h"
#include "mediastreamer2/mswebcam.h"

#ifdef WIN32
#include <fcntl.h>
#include <sys/types.h>
#include <io.h>
#include <stdio.h>
#include <malloc.h>
#endif


typedef struct _NoImageData{
	MSVideoSize vsize;
	uint64_t lasttime;
	mblk_t *pic;
	float fps;
}NoImageData;

void ms_noimage_init(MSFilter *f){
	NoImageData *d=(NoImageData*)ms_new(NoImageData,1);
	d->vsize.width=MS_VIDEO_SIZE_CIF_W;
	d->vsize.height=MS_VIDEO_SIZE_CIF_H;

	d->lasttime=0;
	d->fps=15;
	d->pic=NULL;
	f->data=d;
}

void ms_noimage_uninit(MSFilter *f){
	ms_free(f->data);
}

void ms_noimage_process(MSFilter *f){
	NoImageData *d=(NoImageData*)f->data;
}

void ms_noimage_postprocess(MSFilter *f){
	NoImageData *d=(NoImageData*)f->data;
	if (d->pic) {
		freemsg(d->pic);
		d->pic=NULL;
	}
}

int ms_noimage_set_vsize(MSFilter *f, void* data){
	NoImageData *d=(NoImageData*)f->data;
	d->vsize=*(MSVideoSize*)data;
	return 0;
}

int ms_noimage_get_vsize(MSFilter *f, void* data){
	NoImageData *d=(NoImageData*)f->data;
	*(MSVideoSize*)data=d->vsize;
	return 0;
}

int ms_noimage_get_pix_fmt(MSFilter *f, void *data){
	*(MSPixFmt*)data=MS_YUV420P;
	return 0;
}

static int ms_noimage_set_fps(MSFilter *f, void *arg){
	NoImageData *d=(NoImageData*)f->data;
	d->fps=*((float*)arg);
	if (d->fps<1.0f)
		d->fps=1.0;
	if (d->fps>30.0f)
		d->fps=15.0f;
	return 0;
}

MSFilterMethod ms_noimage_methods[]={
	{	MS_FILTER_SET_VIDEO_SIZE, ms_noimage_set_vsize },
	{	MS_FILTER_GET_VIDEO_SIZE, ms_noimage_get_vsize },
	{	MS_FILTER_GET_PIX_FMT, ms_noimage_get_pix_fmt },
	{	MS_FILTER_SET_FPS, ms_noimage_set_fps },
	{	0,0 }
};

MSFilterDesc ms_noimage_desc={
	MS_FILTER_PLUGIN_ID,
	"MSNoImage",
	N_("A filter for privacy."),
	MS_FILTER_OTHER,
	NULL,
	0,
	1,
	ms_noimage_init,
	NULL,
	ms_noimage_process,
	ms_noimage_postprocess,
	ms_noimage_uninit,
	ms_noimage_methods
};

static void ms_noimage_detect(MSWebCamManager *obj);

static void ms_noimage_cam_init(MSWebCam *cam){
	cam->name=ms_strdup("No Image");
}


static MSFilter *ms_noimage_create_reader(MSWebCam *obj){
	return ms_filter_new_from_desc(&ms_noimage_desc);
}

MSWebCamDesc ms_webcam_noimage_desc={
	"NoImage",
	&ms_noimage_detect,
	NULL,
	&ms_noimage_cam_init,
	&ms_noimage_create_reader,
	NULL
};

static void ms_noimage_detect(MSWebCamManager *obj){
	MSWebCam *cam=ms_web_cam_new(&ms_webcam_noimage_desc);
	ms_web_cam_manager_add_cam(obj,cam);
}

#ifdef WIN32
#define GLOBAL_LINKAGE __declspec(dllexport)
#else
#define GLOBAL_LINKAGE
#endif

GLOBAL_LINKAGE void libmsnoimage_init(void);

GLOBAL_LINKAGE void libmsnoimage_init(void){
	MSWebCamManager *wm;
	ms_filter_register(&ms_noimage_desc);

	wm=ms_web_cam_manager_get();
	ms_web_cam_manager_register_desc(wm,&ms_webcam_noimage_desc);
}
