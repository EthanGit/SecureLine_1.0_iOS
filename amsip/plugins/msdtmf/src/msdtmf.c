/*
*/

#include "mediastreamer2/dtmfgen.h"

#define __inline__ __inline

#include "tone_detect.h"
#include "dtmf.h"

#include <math.h>

struct DtmfDetectState{
	int rate;
  dtmf_rx_state_t dtmf_state;
};

typedef struct DtmfDetectState DtmfDetectState;

static void dtmfdetector_init(MSFilter *f){
	DtmfDetectState *s=(DtmfDetectState *)ms_new(DtmfDetectState,1);
	s->rate=8000;
  dtmf_rx_init(&s->dtmf_state, NULL, NULL);
	f->data=s;
}

static void dtmfdetector_uninit(MSFilter *f){
	ms_free(f->data);
}

static int dtmfdetector_set_rate(MSFilter *f, void *arg){
	DtmfDetectState *s=(DtmfDetectState*)f->data;
	s->rate=*((int*)arg);
	return 0;
}

static void dtmfdetector_process(MSFilter *f){
	mblk_t *m;
	DtmfDetectState *s=(DtmfDetectState*)f->data;

	while((m=ms_queue_get(f->inputs[0]))!=NULL){
    char buf[128 + 1];

    int nsamples=(m->b_wptr-m->b_rptr)/2;
		int16_t *sample=(int16_t*)m->b_rptr;

    memset(buf, 0, 128);
    dtmf_rx(&s->dtmf_state, sample, nsamples);

    ms_queue_put(f->outputs[0],m);
	}
}

typedef struct am_dtmf_event
{
    char dtmf;
    int duration;
} am_dtmf_event_t;

static int dtmfdetector_getdtmf(MSFilter *f, void *data){
	DtmfDetectState *s=(DtmfDetectState*)f->data;
  int actual;
  char buf[2];
	struct am_dtmf_event *evt=(struct am_dtmf_event *)data;
  memset(evt, 0, sizeof(struct am_dtmf_event));
  actual = dtmf_rx_get(&s->dtmf_state, buf, 1);
  if (actual)
    {
      /* printf("DTMF detected: %s\n", buf); */
      evt->duration = 250;
      evt->dtmf = buf[0];
      return 0;
    }
	return -1;
}

#define MS_FILTER_GET_DTMF  MS_FILTER_METHOD(MS_FILTER_PLUGIN_ID,270,struct am_dtmf_event*)

MSFilterMethod dtmfdetector_methods[]={
	{	MS_FILTER_SET_SAMPLE_RATE	,	dtmfdetector_set_rate	},
	{	MS_FILTER_GET_DTMF	,	dtmfdetector_getdtmf	},
	{	0				,	NULL			}
};

#ifdef _MSC_VER

MSFilterDesc ms_dtmf_detect_desc={
	MS_FILTER_PLUGIN_ID,
	"MSDtmfDetect",
	"DTMF detector",
	MS_FILTER_OTHER,
	NULL,
    1,
	1,
	dtmfdetector_init,
	NULL,
    dtmfdetector_process,
	NULL,
    dtmfdetector_uninit,
	dtmfdetector_methods
};

#else

MSFilterDesc ms_dtmf_detect_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSDtmfDetect",
	.text="DTMF detector",
	.category=MS_FILTER_OTHER,
	.ninputs=1,
	.noutputs=1,
	.init=dtmfdetector_init,
	.process=dtmfdetector_process,
	.uninit=dtmfdetector_uninit,
	.methods=dtmfdetector_methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_dtmf_detect_desc)


#ifdef WIN32
#define GLOBAL_LINKAGE __declspec(dllexport)
#else
#define GLOBAL_LINKAGE
#endif

GLOBAL_LINKAGE void libmsdtmf_init(void){
	ms_filter_register(&ms_dtmf_detect_desc);
}
