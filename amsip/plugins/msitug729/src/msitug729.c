/* msitug279 - g729 codec support from ITU for amsip/mediastreamer2
 * Copyright (C) 2010 Aymeric Moizard <amoizard@gmail.com>
 */

#include "typedef.h"
#include "basic_op.h"
#include "ld8k.h"
#include "dtx.h"
#include "octet.h"

#include "mediastreamer2/msfilter.h"

#define  L_FRAME_COMPRESSED 10

typedef struct EncState{
	int nsamples;
	int nbytes;
	int ms_per_frame;
	int ptime;
	uint32_t ts;
	MSBufferizer bufferizer;

	uint16_t frame;
	uint16_t vad_enable;
	uint16_t prm[PRM_SIZE+1];        /* Analysis parameters.                  */
	uint16_t serial[SERIAL_SIZE];    /* Output bitstream buffer               */
	uint16_t syn[L_FRAME];           /* Buffer for synthesis speech           */

}EncState;

extern Word16 *new_speech;

static void enc_init(MSFilter *f){
	int i;
	EncState *s=ms_new(EncState,1);
	s->nsamples=L_FRAME;
	s->nbytes=10;
	s->ms_per_frame=L_FRAME_COMPRESSED;
	s->ptime=30;
	s->ts=0;
	ms_bufferizer_init(&s->bufferizer);

	s->frame=0;
	s->vad_enable=0;
	for(i=0; i<PRM_SIZE; i++) s->prm[i] = (Word16)0;

	f->data=s;
}

static void enc_uninit(MSFilter *f){
	EncState *s=(EncState*)f->data;
	ms_bufferizer_uninit(&s->bufferizer);
	ms_free(f->data);
}

static void enc_preprocess(MSFilter *f){
	EncState *s=(EncState*)f->data;
	int i;

	s->frame=0;
	s->vad_enable=0;

	Init_Pre_Process();
	Init_Coder_ld8k();
	for(i=0; i<PRM_SIZE; i++) s->prm[i] = (Word16)0;

	/* for G.729B */
	Init_Cod_cng();

}

static int enc_add_fmtp(MSFilter *f, void *arg){
	char buf[64];
	const char *fmtp=(const char *)arg;
	EncState *s=(EncState*)f->data;

	memset(buf, '\0', sizeof(buf));
#if 0
	fmtp_get_value(fmtp, "mode", buf, sizeof(buf));
#endif
	if (buf[0]=='\0'){
		ms_warning("unsupported fmtp parameter (%s)!", fmtp);
	}
	return 0;
}

static int enc_add_attr(MSFilter *f, void *arg){
	const char *fmtp=(const char *)arg;
	EncState *s=(EncState*)f->data;
	if (strstr(fmtp,"ptime:10")!=NULL){
		s->ptime=10;
	}else if (strstr(fmtp,"ptime:20")!=NULL){
		s->ptime=20;
	}else if (strstr(fmtp,"ptime:30")!=NULL){
		s->ptime=30;
	}else if (strstr(fmtp,"ptime:40")!=NULL){
		s->ptime=40;
	}else if (strstr(fmtp,"ptime:50")!=NULL){
		s->ptime=50;
	}else if (strstr(fmtp,"ptime:60")!=NULL){
		s->ptime=60;
	}else if (strstr(fmtp,"ptime:70")!=NULL){
		s->ptime=70;
	}else if (strstr(fmtp,"ptime:80")!=NULL){
		s->ptime=80;
	}else if (strstr(fmtp,"ptime:90")!=NULL){
		s->ptime=90;
	}else if (strstr(fmtp,"ptime:100")!=NULL){
		s->ptime=100;
	}else if (strstr(fmtp,"ptime:110")!=NULL){
		s->ptime=110;
	}else if (strstr(fmtp,"ptime:120")!=NULL){
		s->ptime=120;
	}else if (strstr(fmtp,"ptime:130")!=NULL){
		s->ptime=130;
	}else if (strstr(fmtp,"ptime:140")!=NULL){
		s->ptime=140;
	}
	return 0;
}

static int toRealBits(uint16_t *fakebits, uint8_t *real){
	int q;
	//byte[] real = new byte[10];
	memset(real, 0, 10);
	for(q=0; q<80; q++) {
		if(fakebits[q+2] == BIT_1) {
			int tmp = real[q/8];
			int onebit = 1<<(7-(q%8));
			tmp|=onebit;
			real[q/8] = (uint8_t)(0xFF&tmp);
		}

	}
	return 0;
}

static int fromRealBits(uint8_t *real, uint16_t *fake){
	int q;
	//short[] fake = new short[82];
	memset(fake, 0, 82*sizeof(short));
	fake[0] = SYNC_WORD;
	fake[1] = SIZE_WORD;
	for(q=0; q<80; q++) {
		if((real[q/8]&(1<<(7-(q%8)))) != 0)
			fake[q+2] = BIT_1;
		else
			fake[q+2] = BIT_0;
	}
	return 0;
}

static void enc_process(MSFilter *f){
	EncState *s=(EncState*)f->data;
	mblk_t *im,*om;
	int size=s->nsamples*2;
	int16_t samples[2240]; /* 160 * 14 is the largest size for ptime == 140 */
	int frame_per_packet=1;

	if (s->ptime>=10 && s->ms_per_frame>0 && s->ptime%s->ms_per_frame==0)
	{
		frame_per_packet = s->ptime/s->ms_per_frame;
	}

	if (frame_per_packet<=0)
		frame_per_packet=1;
	if (frame_per_packet>14) /* 14*10 == 140 ms max */
		frame_per_packet=14;

	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		ms_bufferizer_put(&s->bufferizer,im);
	}
	while(ms_bufferizer_read(&s->bufferizer,(uint8_t*)samples,size*frame_per_packet)==(size*frame_per_packet)){
		int k;
		om=allocb(L_FRAME_COMPRESSED*frame_per_packet,0);
		for (k=0;k<frame_per_packet;k++)
		{
			uint16_t nb_words;
			//?
			if (s->frame == 32767) s->frame = 256;
			else s->frame++;


			memcpy(new_speech, (short *) samples+(L_FRAME*k), L_FRAME*2);
			Pre_Process(new_speech, L_FRAME);

			Coder_ld8k(s->prm, s->syn, s->frame, s->vad_enable);

			prm2bits_ld8k( s->prm, s->serial);

			nb_words = add((Word16)s->serial[1], 2);
			toRealBits(s->serial, (uint8_t*)om->b_wptr);
			om->b_wptr+=L_FRAME_COMPRESSED;
			s->ts+=s->nsamples;
		}
		mblk_set_timestamp_info(om,s->ts-s->nsamples);
		ms_queue_put(f->outputs[0],om);
	}
}

static MSFilterMethod enc_methods[]={
	{	MS_FILTER_ADD_FMTP,		enc_add_fmtp },
	{	MS_FILTER_ADD_ATTR,		enc_add_attr},
	{	0								,		NULL			}
};

#ifdef _MSC_VER

MSFilterDesc ms_itug729_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSITUG729Enc",
	"G729b ITU encoder",
	MS_FILTER_ENCODER,
	"G729",
	1,
	1,
	enc_init,
    enc_preprocess,
	enc_process,
    NULL,
	enc_uninit,
	enc_methods
};

#else

MSFilterDesc ms_itug729_enc_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSITUG729Enc",
	.text="G729b ITU encoder",
	.category=MS_FILTER_ENCODER,
	.enc_fmt="G729",
	.ninputs=1,
	.noutputs=1,
	.init=enc_init,
	.preprocess=enc_preprocess,
	.process=enc_process,
	.uninit=enc_uninit,
	.methods=enc_methods
};

#endif

typedef struct DecState{
	Word16 synth_buf[L_FRAME+M];
	Word16 *synth;
	Word16 voicing;              /* voicing from previous frame */
	Word16  parm[PRM_SIZE+2];     /* Synthesis parameters        */
	Word16  Az_dec[MP1*2]; /* Decoded Az for post-filter  */
	Word16  T0_first;             /* Pitch lag in 1st subframe   */
	int nodata;
}DecState;


static void dec_init(MSFilter *f){
	DecState *s=ms_new(DecState,1);
	int i;
	s->nodata=0;
	for (i=0; i<M; i++) s->synth_buf[i] = 0;
	s->synth = s->synth_buf + M;
	f->data=s;
}

static void dec_uninit(MSFilter *f){
	ms_free(f->data);
}

static void dec_process(MSFilter *f){
	DecState *s=(DecState*)f->data;
	mblk_t *im,*om;
	int nbytes;

	while ((im=ms_queue_get(f->inputs[0]))!=NULL){
		int frame_per_packet = 1;
		int k;

		nbytes=msgdsize(im);
		if (nbytes<=0)
			return;
		if (nbytes%L_FRAME_COMPRESSED!=0)
			return;
		if (s->nodata==0)
		{
			/* not yet configured, or misconfigured */
			int i;
			Init_Decod_ld8k();
			Init_Post_Filter();
			Init_Post_Process();
			s->nodata=1;
			s->voicing=60;
			for (i=0; i<M; i++) s->synth_buf[i] = 0;
			s->synth = s->synth_buf + M;
		}

		frame_per_packet = nbytes/L_FRAME_COMPRESSED;

		for (k=0;k<frame_per_packet;k++)
		{
			Word16  *ptr_Az;
			//Word16  pst_out[L_FRAME];     /* Postfilter output           */
			Word16  Vad;
			Word16  i;
			Word16  sf_voic;              /* voicing for subframe        */
			Word16  serial[82];

			om=allocb(L_FRAME*2,0);

			//copy bitstream to parm
			fromRealBits((unsigned char*)im->b_rptr+(k*L_FRAME_COMPRESSED), serial);
			bits2prm_ld8k(&serial[1], s->parm);
			s->parm[0] = 0;           /* No frame erasure */
			if(serial[1] != 0) {
				for (i=0; i < serial[1]; i++)
					if (serial[i+2] == 0 ) s->parm[0] = 1;  /* frame erased     */
			}
			else if(serial[0] != SYNC_WORD) s->parm[0] = 1;

			if(s->parm[1] == 1) {
				/* check parity and put 1 in parm[5] if parity error */
				s->parm[5] = Check_Parity_Pitch(s->parm[4], s->parm[5]);
			}

			Decod_ld8k(s->parm, s->voicing, s->synth, s->Az_dec, &s->T0_first, &Vad);

			/* Postfilter */
			s->voicing = 0;
			ptr_Az = s->Az_dec;
			for(i=0; i<L_FRAME; i+=L_SUBFR) {
				//Post(T0_first, &synth[i], ptr_Az, &pst_out[i], &sf_voic, Vad);
				Post(s->T0_first, &s->synth[i], ptr_Az, &((short *)om->b_wptr)[i], &sf_voic, Vad);
				
				if (sf_voic != 0) { s->voicing = sf_voic;}
				ptr_Az += MP1;
			}
			Copy(&s->synth_buf[L_FRAME], &s->synth_buf[0], M);

			Post_Process((short *)om->b_wptr, L_FRAME);

	        //va_g729a_decoder ((unsigned char*)im->b_rptr+(k*L_FRAME_COMPRESSED), (short *)om->b_wptr, 0);

			om->b_wptr+=L_FRAME*2;
			ms_queue_put(f->outputs[0],om);
		}
		freemsg(im);
	}
}

#ifdef _MSC_VER

MSFilterDesc ms_itug729_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSITUG729Dec",
	"G729b ITU decoder",
	MS_FILTER_DECODER,
	"G729",
	1,
	1,
	dec_init,
    NULL,
	dec_process,
    NULL,
	dec_uninit,
    NULL
};

#else

MSFilterDesc ms_itug729_dec_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSITUG729Dec",
	.text="G729b ITU decoder",
	.category=MS_FILTER_DECODER,
	.enc_fmt="G729",
	.ninputs=1,
	.noutputs=1,
	.init=dec_init,
	.process=dec_process,
	.uninit=dec_uninit
};

#endif

#ifdef _MSC_VER
#define MS_PLUGIN_DECLARE(type) __declspec(dllexport) type
#else
#define MS_PLUGIN_DECLARE(type) type
#endif

MS_PLUGIN_DECLARE(void) libmsitug729_init(){
	ms_filter_register(&ms_itug729_enc_desc);
	ms_filter_register(&ms_itug729_dec_desc);
}
