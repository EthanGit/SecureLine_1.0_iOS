/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#ifndef __AM_CALLS_H__
#define __AM_CALLS_H__

#define _SENDRECV 0
#define _SENDONLY 1
#define _RECVONLY 2
#define _INACTIVE 3

#include <mediastreamer2/mediastream.h>

#include <amsip/am_codec.h>
#include <amsip/am_video_codec.h>

#include <amsip/am_options.h>

#include <eXosip2/eXosip.h>

int am_hexa_generate_random(char *val, int val_size, char *str1, char *str2, char *str3);
int am_base64_encode(char *b64_text, const char *target_key, size_t target_key_size);

int os_sound_init(void);

int os_video_init(void);
int os_video_start(am_call_t * ca, sdp_message_t * sdp_answer,
				   sdp_message_t * sdp_offer, sdp_message_t * sdp_local,
				   sdp_message_t * sdp_remote, int local_port,
				   char *remote_ip, int remote_port, int setup_passive);
void os_video_close(am_call_t * ca);
int os_text_init(void);
int os_text_start(am_call_t * ca, sdp_message_t * sdp_answer,
				  sdp_message_t * sdp_offer, sdp_message_t * sdp_local,
				  sdp_message_t * sdp_remote, int local_port,
				  char *remote_ip, int remote_port, int setup_passive);
void os_text_close(am_call_t * ca);
int os_udpftp_start(am_call_t * ca, sdp_message_t * sdp_answer,
					sdp_message_t * sdp_offer, sdp_message_t * sdp_local,
					sdp_message_t * sdp_remote, int local_port,
					char *remote_ip, int remote_port, int setup_passive);
void os_udpftp_close(am_call_t * ca);

int calls_options_outside_calls(eXosip_event_t * je);

void am_call_release(am_call_t * ca);
int call_new(eXosip_event_t * je, int *answered_code);
int call_ack(eXosip_event_t * je);
int call_answered(eXosip_event_t * je);
int call_proceeding(eXosip_event_t * je);
int call_ringing(eXosip_event_t * je);
int call_redirected(eXosip_event_t * je);
int call_requestfailure(eXosip_event_t * je);
int call_serverfailure(eXosip_event_t * je);
int call_globalfailure(eXosip_event_t * je);

int call_stop_sound(int cid, int did);
int call_closed(eXosip_event_t * je);
int call_modified(eXosip_event_t * je, int *answer_code);
int call_update(eXosip_event_t * je, int *answer_code);

int _calls_start_audio_with_id(int tid_for_offer, int did,
							   osip_message_t * answer);
int _calls_start_video_with_id(int tid_for_offer, int did,
							   osip_message_t * answer);
int _calls_start_text_with_id(int tid_for_offer, int did,
							  osip_message_t * answer);
int _calls_start_udpftp_with_id(int tid_for_offer, int did,
								osip_message_t * answer);


am_call_t *_am_calls_find_audio_connection(int tid, int did);
am_call_t *_am_calls_new_audio_connection(void);
int _am_calls_get_remote_candidate(am_call_t * ca,
								   osip_message_t * message, char *media);

int _am_calls_get_remote_user_agent(am_call_t * ca,
									osip_message_t * message);
int _am_calls_get_p_am_sessiontype(am_call_t * ca, osip_message_t * message);

int _am_calls_get_conf_name(am_call_t * ca, osip_message_t * message);
int _am_calls_add_audio_for_conference(am_call_t * orig, char *pcm_data,
									   int pcm_size);

int _calls_get_ip_port(sdp_message_t * sdp, const char *media, char *ip,
					   int *port);

void am_call_release(am_call_t * ca);

int _am_service_automatic_pickup(eXosip_event_t * evt_notify);

int _am_udpftp_start_thread(void);
int _am_udpftp_stop_thread(void);

#endif
