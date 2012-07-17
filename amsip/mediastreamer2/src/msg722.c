/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2011-2012 Aymeric MOIZARD amoizard_at_osip.org

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

#include <mediastreamer2/msfilter.h>

#include "g722.h"

typedef struct _G722EncState {
	MSBufferizer *bz;
	int   ptime;
	uint32_t ts;
	g722_encode_state_t *state;
} G722EncState;

static G722EncState * g722_enc_data_new(){
	G722EncState *obj=(G722EncState *)ms_new(G722EncState,1);
	obj->state = g722_encode_init(NULL, 64000, 0);
	obj->bz=ms_bufferizer_new();
	obj->ptime=0;
	obj->ts=0;
	return obj;
}

static void g722_enc_data_destroy(G722EncState *obj){
	g722_encode_release(obj->state);
	ms_bufferizer_destroy(obj->bz);
	ms_free(obj);
}

static void g722_enc_init(MSFilter *obj){
	obj->data=g722_enc_data_new();
}

static void g722_enc_uninit(MSFilter *obj){
	g722_enc_data_destroy((G722EncState*)obj->data);
}

static void g722_enc_process(MSFilter *obj)
{
	G722EncState *dt=(G722EncState*)obj->data;
	MSBufferizer *bz=dt->bz;
	uint8_t buffer[4480];
	int frame_per_packet=2;
	int size_of_pcm;
	
	mblk_t *m;

	if (dt->ptime>=10)
	{
		frame_per_packet = dt->ptime/10;
	}

	if (frame_per_packet<=0)
		frame_per_packet=1;
	if (frame_per_packet>14) /* 7*20 == 140 ms max */
		frame_per_packet=14;

	if(frame_per_packet<=0)
		frame_per_packet=1;

	size_of_pcm = 160*2*frame_per_packet; /* ex: for 10ms -> 160*2==320 */

	while((m=ms_queue_get(obj->inputs[0]))!=NULL){
		ms_bufferizer_put(bz,m);
	}
	while (ms_bufferizer_read(bz,buffer,size_of_pcm)==size_of_pcm){
		mblk_t *o=allocb(size_of_pcm/4,0);
		int k;
		
		k = g722_encode(dt->state, o->b_wptr, (int16_t *)buffer, size_of_pcm/2);		
		o->b_wptr += k;
		mblk_set_timestamp_info(o,dt->ts);		
		ms_queue_put(obj->outputs[0],o);
		dt->ts += size_of_pcm/4;  /* G722 is 16Khz but has ts as if it was 8kHZ... */
	}
}

static int enc_add_fmtp(MSFilter *f, void *arg){
	const char *fmtp=(const char *)arg;
	G722EncState *dt=(G722EncState*)f->data;
	char tmp[30];
	if (fmtp_get_value(fmtp,"ptime",tmp,sizeof(tmp))){
		dt->ptime=atoi(tmp);
		ms_message("G722EncState: got ptime=%i",dt->ptime);
	}
	return 0;
}

static int enc_add_attr(MSFilter *f, void *arg){
	const char *fmtp=(const char *)arg;
	G722EncState *dt=(G722EncState*)f->data;
	if (strstr(fmtp,"ptime:10")!=NULL){
		dt->ptime=10;
	}else if (strstr(fmtp,"ptime:20")!=NULL){
		dt->ptime=20;
	}else if (strstr(fmtp,"ptime:30")!=NULL){
		dt->ptime=30;
	}else if (strstr(fmtp,"ptime:40")!=NULL){
		dt->ptime=40;
	}else if (strstr(fmtp,"ptime:50")!=NULL){
		dt->ptime=50;
	}else if (strstr(fmtp,"ptime:60")!=NULL){
		dt->ptime=60;
	}else if (strstr(fmtp,"ptime:70")!=NULL){
		dt->ptime=70;
	}else if (strstr(fmtp,"ptime:80")!=NULL){
		dt->ptime=80;
	}else if (strstr(fmtp,"ptime:90")!=NULL){
		dt->ptime=90;
	}else if (strstr(fmtp,"ptime:100")!=NULL){
		dt->ptime=100;
	}else if (strstr(fmtp,"ptime:110")!=NULL){
		dt->ptime=110;
	}else if (strstr(fmtp,"ptime:120")!=NULL){
		dt->ptime=120;
	}else if (strstr(fmtp,"ptime:130")!=NULL){
		dt->ptime=130;
	}else if (strstr(fmtp,"ptime:140")!=NULL){
		dt->ptime=140;
	}
	return 0;
}

static MSFilterMethod enc_methods[]={
	{	MS_FILTER_ADD_ATTR		,	enc_add_attr},
	{	MS_FILTER_ADD_FMTP		,	enc_add_fmtp},
	{	0				,	NULL		}
};

#ifdef _MSC_VER

MSFilterDesc ms_g722_enc_desc={
	MS_G722_ENC_ID,
	"MSG722Enc",
	"G.722 wideband encoder",
	MS_FILTER_ENCODER,
	"g722",
	1,
	1,
	g722_enc_init,
	NULL,
	g722_enc_process,
	NULL,
	g722_enc_uninit,
	enc_methods
};

#else

MSFilterDesc ms_g722_enc_desc={
	.id			= MS_G722_ENC_ID,
	.name		= "MSG722Enc",
	.text		= "G.722 wideband decoder",
	.category	= MS_FILTER_ENCODER,
	.enc_fmt	= "g722",
	.ninputs	= 1,
	.noutputs	= 1,
	.init		= g722_enc_init,
	.process	= g722_enc_process,
	.uninit		= g722_enc_uninit,
	.methods	= enc_methods
};
#endif

typedef struct _G722DecState {
	g722_decode_state_t *state;
} G722DecState;

static void g722_dec_init(MSFilter *obj){
	G722DecState *dt=(G722DecState*)ms_new(G722DecState,1);
	dt->state = g722_decode_init(NULL, 64000, 0);
	obj->data=dt;
}

static void g722_dec_uninit(MSFilter *obj)
{
	G722DecState *dt=(G722DecState*)obj->data;
	g722_decode_release(dt->state);
	ms_free(obj->data);
}

static void g722_dec_process(MSFilter *obj)
{
	G722DecState *dt=(G722DecState*)obj->data;
	mblk_t *m;
	while((m=ms_queue_get(obj->inputs[0]))!=NULL){
		mblk_t *o;
		int len;
		msgpullup(m,-1);
		o=allocb((int)((m->b_wptr-m->b_rptr)*4),0);
		len = g722_decode(dt->state,(int16_t *)o->b_wptr, m->b_rptr, (int)(m->b_wptr-m->b_rptr));
		freemsg(m);
		if (len<0)
		{
			freemsg(o);
			continue;
		}
		o->b_wptr+=len*2;
		ms_queue_put(obj->outputs[0],o);
	}
}

#ifdef _MSC_VER

MSFilterDesc ms_g722_dec_desc={
	MS_G722_DEC_ID,
	"MSG722Dec",
	"G.722 wideband decoder",
	MS_FILTER_DECODER,
	"g722",
	1,
	1,
	g722_dec_init,
	NULL,
	g722_dec_process,
	NULL,
	g722_dec_uninit,
	NULL
};

#else

MSFilterDesc ms_g722_dec_desc={
	.id			= MS_G722_DEC_ID,
	.name		= "MSG722Dec",
	.text		= "G.722 wideband decoder",
	.category	= MS_FILTER_DECODER,
	.enc_fmt	= "g722",
	.ninputs	= 1,
	.noutputs	= 1,
	.init		= g722_dec_init,
	.process	= g722_dec_process,
	.uninit		= g722_dec_uninit,
};

#endif


MS_FILTER_DESC_EXPORT(ms_g722_dec_desc)
MS_FILTER_DESC_EXPORT(ms_g722_enc_desc)

