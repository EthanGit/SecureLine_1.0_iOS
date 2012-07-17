/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#include "amsip/am_options.h"
#include "amsip/am_filter.h"

typedef struct MSTextConversationDec {
	ms_mutex_t mutex;
	queue_t incoming_text;
} MSTextConversationDec;

static void textconversationdec_init(MSFilter * f)
{
	MSTextConversationDec *d =
		(MSTextConversationDec *) ms_new0(MSTextConversationDec, 1);

	ms_mutex_init(&d->mutex, NULL);
	qinit(&d->incoming_text);
	f->data = d;
}

static void textconversationdec_uninit(MSFilter * f)
{
	MSTextConversationDec *d = (MSTextConversationDec *) f->data;

	flushq(&d->incoming_text, 0);
	ms_mutex_destroy(&d->mutex);
	ms_free(f->data);
	f->data = NULL;
}

static void textconversationdec_process(MSFilter * f)
{
	MSTextConversationDec *d = (MSTextConversationDec *) f->data;
	mblk_t *im;

	while ((im = ms_queue_get(f->inputs[0])) != NULL) {
		unsigned short val = *((unsigned short *) im->b_rptr);

		if (val == 0xFFFE && msgdsize(im) == 2) {
			freemsg(im);
		} else if (val == 0xFEFF && msgdsize(im) == 2) {
			freemsg(im);
		} else
			putq(&d->incoming_text, im);
	}

}

static int textconversationdec_get(MSFilter * f, void *arg)
{
	MSTextConversationDec *d = (MSTextConversationDec *) f->data;
	MST140Block *t140b = (MST140Block *) arg;
	mblk_t *im;

	ms_mutex_lock(&d->mutex);

	t140b->utf8_string = NULL;
	t140b->size = 0;
	if ((im = getq(&d->incoming_text)) != NULL) {
		t140b->utf8_string = (char *) osip_malloc(msgdsize(im) + 1);
		if (t140b->utf8_string != NULL) {
			memset(t140b->utf8_string, 0, msgdsize(im) + 1);
			memcpy(t140b->utf8_string, im->b_rptr, msgdsize(im));
			t140b->size = msgdsize(im);
		}
		freemsg(im);
	}

	ms_mutex_unlock(&d->mutex);
	return 0;
}

static MSFilterMethod textconversationdec_methods[] = {
	{MS_FILTER_TEXT_GET, textconversationdec_get},
	{0, NULL}
};

#ifdef _MSC_VER

MSFilterDesc ms_textconversationdec_desc = {
	MS_FILTER_PLUGIN_ID,
	"MSDecTextConversation",
	"A filter for decoding t140.",
	MS_FILTER_DECODER,
	"t140",
	1,
	0,
	textconversationdec_init,
	NULL,
	textconversationdec_process,
	NULL,
	textconversationdec_uninit,
	textconversationdec_methods
};

#else

MSFilterDesc ms_textconversationdec_desc = {
	.id = MS_FILTER_PLUGIN_ID,
	.name = "MSDecTextConversation",
	.text = "A filter for decoding t140.",
	.category = MS_FILTER_DECODER,
	.enc_fmt = "t140",
	.ninputs = 1,
	.noutputs = 0,
	.init = textconversationdec_init,
	.process = textconversationdec_process,
	.uninit = textconversationdec_uninit,
	.methods = textconversationdec_methods
};

#endif

#ifdef _MSC_VER

MSFilterDesc ms_udpftpdec_desc = {
	MS_FILTER_PLUGIN_ID,
	"MSDecUDPFileTranfer",
	"A filter for UDP file transfer.",
	MS_FILTER_DECODER,
	"x-udpftp",
	1,
	0,
	textconversationdec_init,
	NULL,
	textconversationdec_process,
	NULL,
	textconversationdec_uninit,
	textconversationdec_methods
};

#else

MSFilterDesc ms_udpftpdec_desc = {
	.id = MS_FILTER_PLUGIN_ID,
	.name = "MSDecUDPFileTranfer",
	.text = "A filter for UDP file transfer.",
	.category = MS_FILTER_DECODER,
	.enc_fmt = "x-udpftp",
	.ninputs = 1,
	.noutputs = 0,
	.init = textconversationdec_init,
	.process = textconversationdec_process,
	.uninit = textconversationdec_uninit,
	.methods = textconversationdec_methods
};

#endif

typedef struct MSTextConversationEnc {
	ms_mutex_t mutex;
	queue_t outgoing_text;
	int outgoing_counter;
} MSTextConversationEnc;

static void textconversationenc_init(MSFilter * f)
{
	MSTextConversationEnc *d =
		(MSTextConversationEnc *) ms_new0(MSTextConversationEnc, 1);

	ms_mutex_init(&d->mutex, NULL);
	qinit(&d->outgoing_text);
	d->outgoing_counter = 0;
	f->data = d;
}

static void textconversationenc_uninit(MSFilter * f)
{
	MSTextConversationEnc *d = (MSTextConversationEnc *) f->data;

	flushq(&d->outgoing_text, 0);
	ms_mutex_destroy(&d->mutex);
	ms_free(f->data);
	f->data = NULL;
}

static int textconversationenc_send(MSFilter * f, void *arg)
{
	MSTextConversationEnc *d = (MSTextConversationEnc *) f->data;
	MST140Block *t140b = (MST140Block *) arg;
	mblk_t *om;

	om = allocb(t140b->size, 0);
	memcpy(om->b_wptr, t140b->utf8_string, t140b->size);
	om->b_wptr += t140b->size;

	putq(&d->outgoing_text, om);
	return 0;
}

static void textconversationenc_process(MSFilter * f)
{
	MSTextConversationEnc *d = (MSTextConversationEnc *) f->data;
	mblk_t *im;
	uint32_t timestamp = (uint32_t) f->ticker->time * 1LL;

	if (d->outgoing_counter == 0) {
		MST140Block t140b;
		unsigned short val = 0xFEFF;

		/* Start conversation with:
		   "ZERO WIDTH NON-BREAKING SPACE" which is also known
		   informally as "BYTE ORDER MARK"/"BOM") */
		t140b.utf8_string = ortp_malloc(4);
		memcpy(t140b.utf8_string, &val, sizeof(val));
		t140b.size = sizeof(val);

		textconversationenc_send(f, &t140b);
	}
	d->outgoing_counter++;
	if (d->outgoing_counter % 1000 == 0) {
		MST140Block t140b;
		unsigned short val = 0xFEFF;

		/* Start conversation with:
		   "ZERO WIDTH NON-BREAKING SPACE" which is also known
		   informally as "BYTE ORDER MARK"/"BOM") */
		t140b.utf8_string = ortp_malloc(4);
		memcpy(t140b.utf8_string, &val, sizeof(val));
		t140b.size = sizeof(val);

		textconversationenc_send(f, &t140b);
	}

	while ((im = getq(&d->outgoing_text)) != NULL) {
		mblk_set_timestamp_info(im, timestamp);
		ms_queue_put(f->outputs[0], im);
	}
}

static MSFilterMethod textconversationenc_methods[] = {
	{MS_FILTER_TEXT_SEND, textconversationenc_send},
	{0, NULL}
};

#ifdef _MSC_VER

MSFilterDesc ms_textconversationenc_desc = {
	MS_FILTER_PLUGIN_ID,
	"MSEncTextConversation",
	"A filter for encoding t140.",
	MS_FILTER_ENCODER,
	"t140",
	0,
	1,
	textconversationenc_init,
	NULL,
	textconversationenc_process,
	NULL,
	textconversationenc_uninit,
	textconversationenc_methods
};

#else

MSFilterDesc ms_textconversationenc_desc = {
	.id = MS_FILTER_PLUGIN_ID,
	.name = "MSEncTextConversation",
	.text = "A filter for encoding t140.",
	.category = MS_FILTER_ENCODER,
	.enc_fmt = "t140",
	.ninputs = 0,
	.noutputs = 1,
	.init = textconversationenc_init,
	.process = textconversationenc_process,
	.uninit = textconversationenc_uninit,
	.methods = textconversationenc_methods
};

#endif

#ifdef _MSC_VER

MSFilterDesc ms_udpftpenc_desc = {
	MS_FILTER_PLUGIN_ID,
	"MSEncUDPFileTranfer",
	"A filter for UDP file transfer.",
	MS_FILTER_ENCODER,
	"x-udpftp",
	0,
	1,
	textconversationenc_init,
	NULL,
	textconversationenc_process,
	NULL,
	textconversationenc_uninit,
	textconversationenc_methods
};

#else

MSFilterDesc ms_udpftpenc_desc = {
	.id = MS_FILTER_PLUGIN_ID,
	.name = "MSEncUDPFileTranfer",
	.text = "A filter for UDP file transfer.",
	.category = MS_FILTER_ENCODER,
	.enc_fmt = "x-udpftp",
	.ninputs = 0,
	.noutputs = 1,
	.init = textconversationenc_init,
	.process = textconversationenc_process,
	.uninit = textconversationenc_uninit,
	.methods = textconversationenc_methods
};

#endif

static void dispatcher_process(MSFilter * f)
{
	mblk_t *im;

	while ((im = ms_queue_get(f->inputs[0])) != NULL) {
		int payload;

		payload = mblk_get_payload_type(im);

		if (payload == 123 && f->outputs[1] != NULL)
			ms_queue_put(f->outputs[1], im);
		else if (f->outputs[0] != NULL)
			ms_queue_put(f->outputs[0], im);
		else
			freemsg(im);
	}
}

#ifdef _MSC_VER

MSFilterDesc ms_dispatcher_desc = {
	MS_FILTER_PLUGIN_ID,
	"MSDispatcher",
	"A filter that dispatch input depending on payload.",
	MS_FILTER_OTHER,
	NULL,
	1,
	2,
	NULL,
	NULL,
	dispatcher_process,
	NULL,
	NULL,
	NULL
};

#else

MSFilterDesc ms_dispatcher_desc = {
	.id = MS_FILTER_PLUGIN_ID,
	.name = "MSDispatcher",
	.text = "A filter that dispatch input depending on payload.",
	.category = MS_FILTER_OTHER,
	.ninputs = 1,
	.noutputs = 2,
	.process = dispatcher_process
};

#endif

MS_FILTER_DESC_EXPORT(ms_dispatcher_desc)
void am_filter_register(void)
{
	ms_filter_register(&ms_textconversationenc_desc);
	ms_filter_register(&ms_textconversationdec_desc);
	ms_filter_register(&ms_udpftpenc_desc);
	ms_filter_register(&ms_udpftpdec_desc);
	ms_filter_register(&ms_dispatcher_desc);
}

MSFilter *am_filter_new_dispatcher(void)
{
	return ms_filter_new_from_desc(&ms_dispatcher_desc);
}

MSFilter *am_filter_new_rtprecv(void)
{
	if (_antisipc.rtprecv_desc != NULL)
		return ms_filter_new_from_desc(_antisipc.rtprecv_desc);
	return ms_filter_new(MS_RTP_RECV_ID);
}

MSFilter *am_filter_new_rtpsend(void)
{
	if (_antisipc.rtpsend_desc != NULL)
		return ms_filter_new_from_desc(_antisipc.rtpsend_desc);
	return ms_filter_new(MS_RTP_SEND_ID);
}

PPL_DECLARE (void)
am_filter_set_rtpfilter(MSFilterDesc * rtprecv, MSFilterDesc * rtpsend)
{
	_antisipc.rtprecv_desc = NULL;
	_antisipc.rtpsend_desc = NULL;
	if (rtprecv != NULL)
		_antisipc.rtprecv_desc = rtprecv;
	if (rtpsend != NULL)
		_antisipc.rtpsend_desc = rtpsend;
}

PPL_DECLARE (void) am_filter_set_sounddriver(MSSndCardDesc * snd_driver)
{
}
