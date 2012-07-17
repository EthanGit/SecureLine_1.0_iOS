/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#ifndef __AM_OPTIONS_H__
#define __AM_OPTIONS_H__

#include <amsip/am_version.h>

#include <osipparser2/osip_port.h>
#include <eXosip2/eXosip.h>

#include <amsip/am_network.h>
#include <amsip/am_event.h>

#include <mediastreamer2/mediastream.h>
#include <mediastreamer2/msvideoout.h>
#include "mediastreamer2/msvideo.h"

#include <amsip/am_codec.h>
#include <amsip/am_video_codec.h>
#include <amsip/am_text_codec.h>
#include <amsip/am_udpftp_codec.h>
#include <amsip/am_register.h>
#include <amsip/am_call.h>
#include <amsip/am_publish.h>
#include <amsip/am_message.h>
#include <amsip/am_subscribe.h>
#include <amsip/am_player.h>
#include <amsip/am_filter.h>
#include <amsip/am_service.h>

#ifndef DISABLE_SRTP
//struct srtp_ctx_t;
//#include <ortp/srtp.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file am_options.h
 * @brief amsip init API
 *
 * This file provide the API needed to initialize amsip. You can
 * use it to:
 *
 * <ul>
 * <li>initialize amsip.</li>
 * <li>set amsip options.</li>
 * </ul>
 *
 */

/**
 * @defgroup amsip_init amsip init interface
 * @ingroup amsip_setup
 * @{
 */

#define AMSIP_SUCCESS              OSIP_SUCCESS
#define AMSIP_UNDEFINED_ERROR      OSIP_UNDEFINED_ERROR
#define AMSIP_BADPARAMETER         OSIP_BADPARAMETER
#define AMSIP_WRONG_STATE          OSIP_WRONG_STATE
#define AMSIP_NOMEM                OSIP_NOMEM
#define AMSIP_SYNTAXERROR          OSIP_SYNTAXERROR
#define AMSIP_NOTFOUND             OSIP_NOTFOUND
#define AMSIP_API_NOT_INITIALIZED  OSIP_API_NOT_INITIALIZED

#define AMSIP_NO_NETWORK           OSIP_NO_NETWORK
#define AMSIP_PORT_BUSY            OSIP_PORT_BUSY
#define AMSIP_UNKNOWN_HOST         OSIP_UNKNOWN_HOST

#define AMSIP_DISK_FULL            OSIP_DISK_FULL
#define AMSIP_NO_RIGHTS            OSIP_NO_RIGHTS
#define AMSIP_FILE_NOT_EXIST       OSIP_FILE_NOT_EXIST

#define AMSIP_TIMEOUT              OSIP_TIMEOUT
#define AMSIP_TOOMUCHCALL          OSIP_TOOMUCHCALL
#define AMSIP_WRONG_FORMAT         OSIP_WRONG_FORMAT
#define AMSIP_NOCOMMONCODEC        OSIP_NOCOMMONCODEC

#define AMSIP_NODEVICE             -100

	typedef struct am_dtmf_event {
		char dtmf;
		int duration;
	} am_dtmf_event_t;

	struct am_candidates {
		int local_port[2];
		int socket[2];
		char media[64];
		struct SdpCandidate local_candidates[10];
		struct SdpCandidate stun_candidates[10];
		struct SdpCandidate relay_candidates[10];
	};

	struct am_sndcard {
		int card;
		char name[256];
		unsigned int capabilities;
		char driver_type[256];
	};

#define	AMSIP_ALGO_AES_CM_128_HMAC_SHA1_80 1
#define	AMSIP_ALGO_AES_CM_128_HMAC_SHA1_32 2
#define	AMSIP_ALGO_AES_CM_128_NULL_AUTH 3
#define	AMSIP_ALGO_NULL_CIPHER_HMAC_SHA1_80 4
//#define	AMSIP_ALGO_AES_CM_192_HMAC_SHA1_80 5 /*  [RFC6188] */
//#define	AMSIP_ALGO_AES_CM_192_HMAC_SHA1_32 6 /*  [RFC6188] */
#define	AMSIP_ALGO_AES_CM_256_HMAC_SHA1_80 7 /*  [RFC6188] */
#define	AMSIP_ALGO_AES_CM_256_HMAC_SHA1_32 8 /*  [RFC6188] */

	/**
    * Structure for srtp crypto suite configuration.
    * @struct am_srtp_info
    */
	struct am_srtp_info {
		int pos;
		int enable;
		int srtp_algo;
	};

	/**
	* structure used to describe credentials for a client or server
	* consists of a certificate, a corresponding private key and its password
	* @struct am_tls_credentials_t
	*/
	typedef eXosip_tls_credentials_t am_tls_credentials_t;

	/**
	* structure to describe the whole TLS-context for eXosip
	* consists of a certificate, a corresponding private key and its password
	* @struct am_tls_ctx_t
	*/
	typedef eXosip_tls_ctx_t am_tls_ctx_t;

#define AMSIP_OUTGOINGCALL 0
#define AMSIP_INCOMINGCALL 1

	struct am_call {
		int cid;
		int did;
		int tid;
		int call_direction;
		char remote_useragent[256];
		char p_am_sessiontype[256];
		char ice_pwd[256];
		char ice_ufrag[256];

		struct am_candidates audio_candidate;
		struct IceCheckList audio_checklist;
		//struct CandidatePair candidate_pair_table[10];

#ifdef ENABLE_VIDEO
		struct am_candidates video_candidate;
		struct IceCheckList video_checklist;
		//struct CandidatePair video_candidate_pair_table[10];
#endif

		struct am_candidates text_candidate;
		struct IceCheckList text_checklist;
		//struct CandidatePair text_candidate_pair_table[10];

		struct am_candidates udpftp_candidate;
		struct IceCheckList udpftp_checklist;
		//struct CandidatePair udpftp_candidate_pair_table[10];

		int enable_audio;		/* 0 started, -1 stopped */
		sdp_message_t *local_sdp;
		int versionid;
		int local_sendrecv;		/* _SENDRECV, _SENDONLY, _RECVONLY */
		char hold_wav_file[256];

		void *audio_ctx;
		int audio_rtp_in_direct_mode;
		char audio_rtp_remote_addr[256];

#ifdef ENABLE_VIDEO
		int enable_video;		/* 0 started, -1 stopped */
		int video_local_sendrecv;	/* _SENDRECV, _SENDONLY, _RECVONLY */

		void *video_ctx;

		int reject_video;		/* 0 allow video to be negotiated, 1 reject any video negotiation */
		int video_rtp_in_direct_mode;
		char video_rtp_remote_addr[256];

#endif

		int enable_text;		/* 0 started, -1 stopped */
		int text_local_sendrecv;	/* _SENDRECV, _SENDONLY, _RECVONLY */
		RtpSession *text_rtp_session;
		RtpProfile *text_rtp_profile;
		MSTicker *text_ticker;
		MSFilter *text_encoder;
		MSFilter *text_decoder;
		MSFilter *text_rtprecv;
		MSFilter *text_rtpsend;
		MSFilter *text_ice;
		int text_rtp_in_direct_mode;
		char text_rtp_remote_addr[256];

		int enable_udpftp;		/* 0 started, -1 stopped */
		int udpftp_local_sendrecv;	/* _SENDRECV, _SENDONLY, _RECVONLY */
		RtpSession *udpftp_rtp_session;
		RtpProfile *udpftp_rtp_profile;
		MSTicker *udpftp_ticker;
		MSFilter *udpftp_encoder;
		MSFilter *udpftp_decoder;
		MSFilter *udpftp_rtprecv;
		MSFilter *udpftp_rtpsend;
		MSFilter *udpftp_ice;
		int udpftp_rtp_in_direct_mode;
		char udpftp_rtp_remote_addr[256];


		char conf_name[256];

#define NOT_USED      0
		int state;

		int call_established;	/* 200 ok received for the initial INVITE */

		int conf_id;
	};

	typedef struct am_call am_call_t;

#ifndef MAX_NUMBER_OF_CALLS
#define MAX_NUMBER_OF_CALLS 63	/* CONF_MAX_PINS/2-2 = 63 */
#endif

#ifndef AMSIP_CONF_MAX
#define AMSIP_CONF_MAX 1
#endif

#define STITCHER_MAX_INPUTS 9

	struct audio_module {
		const char *name;		/* module name */
		const char *text;		/* descriptive text */
		const char *media_type;	/* media type */
		int (*audio_module_init) (const char *name, int debug_level);
		int (*audio_module_reset) (const char *name, int debug_level);
		int (*audio_module_quit) ();

		int (*audio_module_set_volume_out_sound_card) (int card, int master, int percent);
		int (*audio_module_get_volume_out_sound_card) (int card, int master);
		int (*audio_module_set_volume_in_sound_card) (int card, int percent);
		int (*audio_module_get_volume_in_sound_card) (int card);
		int (*audio_module_set_mute_out_sound_card) (int card, int master, int val);
		int (*audio_module_set_mute_in_sound_card) (int card, int val);

		int (*audio_module_find_out_sound_card) (struct am_sndcard *
												 sndcard);
		int (*audio_module_find_in_sound_card) (struct am_sndcard *
												sndcard);

		int (*audio_module_select_in_sound_card_conf) (int conf_id, int card);
		int (*audio_module_select_out_sound_card_conf) (int conf_id, int card);
		int (*audio_module_select_in_custom_sound_card_conf) (int conf_id, MSSndCard *
														 captcard);
		int (*audio_module_select_out_custom_sound_card_conf) (int conf_id, MSSndCard *
														  playcard);

		int (*audio_module_set_rate_conf) (int conf_id, int rate);
		int (*audio_module_enable_echo_canceller_conf) (int conf_id, int enable,
												   int frame_size,
												   int tail_length);
		int (*audio_module_enable_vad_conf) (int conf_id, int enable,
												int vad_prob_start,
												int vad_prob_continue);
		int (*audio_module_enable_agc_conf) (int conf_id, int enable, int agc_level,
										int max_gain);
		int (*audio_module_set_denoise_level_conf) (int conf_id, int enable);

		int (*audio_module_set_callback_conf) (int conf_id, unsigned int id,
										  MSFilterNotifyFunc
										  speex_pp_process,
										  void *userdata);
		int (*audio_module_set_volume_gain_conf) (int conf_id, float capture_gain, float playback_gain);
		int (*audio_module_set_echo_limitation_conf) (int conf_id, int enabled, float threshold, float speed, float force, int sustain);
		int (*audio_module_set_noise_gate_threshold_conf)(int conf_id, int enabled, float threshold);
		int (*audio_module_set_equalizer_state_conf) (int conf_id, int enable);
		int (*audio_module_set_equalizer_params_conf) (int conf_id, float frequency, float gain, float width);
		int (*audio_module_set_mic_equalizer_state_conf) (int conf_id, int enable);
		int (*audio_module_set_mic_equalizer_params_conf) (int conf_id, float frequency, float gain, float width);

		int (*audio_module_sound_init) (am_call_t * ca, int idx);
		int (*audio_module_sound_release) (am_call_t * ca);
		int (*audio_module_sound_start) (am_call_t * ca,
										 sdp_message_t * sdp_answer,
										 sdp_message_t * sdp_offer,
										 sdp_message_t * sdp_local,
										 sdp_message_t * sdp_remote,
										 int local_port, char *remote_ip,
										 int remote_port,
										 int setup_passive);
		int (*audio_module_sound_close) (am_call_t * ca);

		int (*audio_module_session_mute) (am_call_t * ca);
		int (*audio_module_session_unmute) (am_call_t * ca);
		int (*audio_module_session_get_bandwidth_statistics) (am_call_t *
															  ca,
															  struct
															  am_bandwidth_stats
															  *
															  band_stats);
		OrtpEvent *(*audio_module_session_get_audio_rtp_events) (am_call_t *ca);
		int (*audio_module_session_set_audio_zrtp_sas_verified) (am_call_t *ca);
		int (*audio_module_session_get_audio_statistics) (am_call_t * ca,
														  struct
														  am_audio_stats *
														  audio_stats);
		int (*audio_module_session_get_dtmf_event) (am_call_t * ca,
													struct am_dtmf_event *
													dtmf_event);
		int (*audio_module_session_send_inband_dtmf) (am_call_t * ca,
													  char dtmf_number);
		int (*audio_module_session_send_rtp_dtmf) (am_call_t * ca,
												   char dtmf_number);
		int (*audio_module_session_play_file) (am_call_t * ca,
											   const char *wav_file,
											   int repeat,
											   MSFilterNotifyFunc
											   cb_fileplayer_eof);
		int (*audio_module_session_record) (am_call_t * ca,
											const char *recfile);
		int (*audio_module_session_stop_record) (am_call_t * ca);
		int (*audio_module_session_add_external_rtpdata) (am_call_t * ca,
														  MSFilter *
														  external_rtpdata);

		int (*audio_module_session_detach_conf) (am_call_t * ca);
		int (*audio_module_session_attach_conf) (am_call_t * ca, int conf_id);
	};

#ifdef WIN32
	typedef void (__stdcall *on_video_module_new_image_cb)(int pin, int width, int height, int format, int size, void *pixel);
#else
	typedef void (*on_video_module_new_image_cb)(int pin, int width, int height, int format, int size, void *pixel);
#endif
	
#ifdef ENABLE_VIDEO

	/* TODO */
	struct am_camera {
		int card;
		char name[256];
	};

	struct video_module {
		const char *name;		/* module name */
		const char *text;		/* descriptive text */
		const char *media_type;	/* media type */
		int (*video_module_init) (const char *name, int debug_level);
		int (*video_module_reset) (const char *name, int debug_level);
		int (*video_module_quit) ();

		int (*video_module_find_camera) (struct am_camera *
												 camera);
		int (*video_module_select_camera) (int card);
		int (*video_module_set_input_video_size) (int width, int height);
		int (*video_module_set_window_display) (MSDisplayDesc *desc, long handle, int width, int height);
		int (*video_module_set_window_handle) (long handle, int width, int height);
		int (*video_module_set_window_preview_handle) (long handle, int width, int height);
		int (*video_module_set_nowebcam) (const char *nowebcam_image);
		int (*video_module_enable_preview) (int enable);
		int (*video_module_set_image_callback)(on_video_module_new_image_cb func);

		int (*video_module_set_selfview_mode)(int mode);
		int (*video_module_set_selfview_position)(const float posx, const float posy, const float size);
		int (*video_module_get_selfview_position)(float *posx, float *posy, float *size);
		int (*video_module_set_selfview_scalefactor)(const float scalefactor);
		int (*video_module_set_background_color)(int red, int green, int blue);
		int (*video_module_set_video_option)(int opt, void *arg);

		int (*video_module_set_callback) (unsigned int id,
										  MSFilterNotifyFunc
										  speex_pp_process,
										  void *userdata);
		int (*video_module_session_init) (am_call_t * ca, int idx);
		int (*video_module_session_release) (am_call_t * ca);
		int (*video_module_session_start) (am_call_t * ca,
										 sdp_message_t * sdp_answer,
										 sdp_message_t * sdp_offer,
										 sdp_message_t * sdp_local,
										 sdp_message_t * sdp_remote,
										 int local_port, char *remote_ip,
										 int remote_port,
										 int setup_passive);
		int (*video_module_session_close) (am_call_t * ca);

		int (*video_module_session_get_bandwidth_statistics) (am_call_t *
															  ca,
															  struct
															  am_bandwidth_stats
															  *
															  band_stats);
		OrtpEvent *(*video_module_session_get_video_rtp_events) (am_call_t * ca);
		int (*video_module_session_set_video_zrtp_sas_verified)(am_call_t * ca);
		int (*video_module_session_send_vfu)(am_call_t * ca);
		int (*video_module_session_adapt_video_bitrate)(am_call_t * ca, float lost);
	};
#endif

	struct antisipc {
		char syslog_name[256];
		char user_agent[256];
		char supported_extensions[256];
		char allowed_methods[256];
		char accepted_types[1024];
		char allowed_events[256];
		am_codec_info_t codecs[5];
		am_codec_attr_t codec_attr;
		am_codec_info_t video_codecs[5];

#ifdef ENABLE_VIDEO
		am_video_codec_attr_t video_codec_attr;
#endif
		am_codec_info_t text_codecs[5];
		am_text_codec_attr_t text_codec_attr;
		am_codec_info_t udpftp_codecs[5];
		am_udpftp_codec_attr_t udpftp_codec_attr;

		am_call_t calls[MAX_NUMBER_OF_CALLS];

		int supported_path;
		char supported_gruu[64];
		char gruu_contact[256];
		int session_timers;

		int use_stun_server;
		int use_turn_server;
		char stun_server[256];
		struct stun_test stuntest;

		int use_relay_server;
		char relay_server[256];

		char stun_firewall[256];
		int stun_port;

		char outbound_proxy[256];

		int do_symmetric_rtp;
		int do_sdp_in_ack;
		int use_udpkeepalive;
		int use_101;
		int use_rport;
		int dns_capabilities;
		int enable_p_am_sessiontype;

		int audio_dscp;
		int video_dscp;
		int text_dscp;
		int udpftp_dscp;

		int audio_jitter;
		int enable_zrtp;
		char audio_profile[32];
		char video_profile[32];
		char text_profile[32];
		char udpftp_profile[32];
		struct am_srtp_info srtp_info[10];
		int optionnal_encryption;

		int add_nortpproxy;
		int automatic_rfc5168;
		int enable_sdpsetupparameter;

		char ipv4_for_gateway[256];

		int port_range_min;

		MSFilterDesc *rtprecv_desc;
		MSFilterDesc *rtpsend_desc;
		MSSndCardDesc *snd_driver_desc;

		struct audio_module *audio_media;
		struct video_module *video_media;

		MSTicker *player_ticker;
#define AM_MAX_PLAYERS 2
		struct am_player *players[AM_MAX_PLAYERS];
	};

#define STAT_ENOUGH_BANDWIDTH 0
#define STAT_NOT_ENOUGH_BANDWIDTH 1

	/**
    * Structure for amsip statistics.
    * @struct am_audio_stats
    */
	struct am_audio_stats {
		int proposed_action;
		int pk_loss;

		int incoming_received;
		int incoming_expected;
		int incoming_packetloss;
		int incoming_outoftime;
		int incoming_notplayed;
		int incoming_discarded;

		int outgoing_sent;

		int sndcard_recorded;
		int sndcard_played;
		int sndcard_discarded;
		int msconf_processed;
		int msconf_missed;
		int msconf_discarded;
	};

	struct am_bandwidth_stats {
		float upload_rate;
		float download_rate;

		int incoming_received;
		int incoming_expected;
		int incoming_packetloss;
		int incoming_outoftime;
		int incoming_notplayed;
		int incoming_discarded;

		int outgoing_sent;

		int reserved1;
		int reserved2;
		int reserved3;
		int reserved4;
	};

	extern struct antisipc _antisipc;

	extern struct audio_module ms2_audio_module;
#ifdef ENABLE_VIDEO
	extern struct video_module ms2_video_module;
#endif

#if defined(WIN32)
#define vsnprintf _vsnprintf
#define snprintf _snprintf

	typedef void (CALLBACK * FPTR_CALL) (int cid, int did, int tid,
										 int type, const char *method,
										 int status, const char *reason,
										 const char *remote_url,
										 const char *local_url,
										 const char *message);

	PPL_DECLARE (void) amsip_set_call_callback(FPTR_CALL pf);

	void amsip_send_call_info(int cid, int did, int tid, int type,
							  const char *method, int status,
							  const char *reason, const char *remote_url,
							  const char *local_url, const char *message);

#else
#define amsip_send_call_info(A, B, C, D, E, F, G, H, I, J)
#endif

/**
 * Initialize amsip library
 *
 * @param name        Text information for logging. (like vendor id)
 * @param debug_level Debug level for application.
 */
	PPL_DECLARE (int) am_init(const char *name, int debug_level);

/**
 * Reset amsip library
 *
 * @param name        Text information for logging. (like vendor id)
 * @param debug_level Debug level for application.
 */
	PPL_DECLARE (int) am_reset(const char *name, int debug_level);

/**
 * Initialize log facility of library
 * This method MUST be called only ONCE.
 *
 * @param log_file    File name for debugging
 * @param debug_level Debug level for application.
 */
	PPL_DECLARE (int) am_option_debug(const char *log_file,
									  int debug_level);

/**
 * Close amsip library & release ressource.
 *
 */
	PPL_DECLARE (int) am_quit(void);

/**
 * Get amsip version.
 *
 */
	PPL_DECLARE (char *) am_option_get_version(void);

/* int: <=0 to accept expired and self signed certificate */
#define AMSIP_OPTION_TLS_CHECK_CERTIFICATE 0
/* int: <=0 to disable >0 to enable inserting a P-AM-ST header */
#define AMSIP_OPTION_ENABLE_P_SESSION_TYPE 1
/* int: <=0 to disable >0 to enable automatic INFO to request PPS/SPS */
#define AMSIP_OPTION_ENABLE_AUTOMATIC_RFC5168 2
/* int: <=0 to disable >0 to enable a=setup:? parameter */
#define AMSIP_OPTION_ENABLE_SDPSETUPPARAMETER 3
/* struct am_srtp_info with "enabled" (0 or 1), "pos" (between 0 and 5 included) and SRTP_ALGO_XXX */
#define AMSIP_OPTION_SET_SRTP_CRYPTO_SUITE 4
/* struct eXosip_tls_ctx_t: load certificates */
#define AMSIP_OPTION_TLS_SET_CERTIFICATES 5
/* int: <=0 to disable >1 to enable ZRTP */
#define AMSIP_OPTION_ENABLE_ZRTP 6
/* int: <0 to use RTP adaptative jitter, 0 to use default adaptative jitter  >1 to use static jitter buffer */
#define AMSIP_OPTION_SET_AUDIO_JITTER 7

/**
 * Configure amsip options.
 *
 * @param opt       option.
 * @param arg       pointer to option value.
 */
	PPL_DECLARE (int) am_option_set_option(int opt, void *arg);

/**
 * Configure amsip to use User-Agent header value.
 *
 * @param user_agent       User-Agent header.
 */
	PPL_DECLARE (int) am_option_set_user_agent(const char *user_agent);

/**
 * Configure amsip to use a specific audio port.
 *
 * @param initial_port     initial audio port.
 */
	PPL_DECLARE (int) am_option_set_initial_audio_port(int initial_port);

/**
 * Configure amsip to use stun server.
 *
 * @param stun_server      IP of stun server.
 * @param use_stun_server  enable use of stun server
 */
	PPL_DECLARE (int) am_option_enable_stun_server(const char *stun_server,
												   int use_stun_server);

/**
 * Configure amsip to use stun server.
 *   INCOMPLETE: the server MUST be actually a STUN server, this options
 *   currently ask for enabling STUN connectivity checks and candidates
 *   handling in SDP. 
 *
 *   This is currently used to use ICE without TURN/relay server:
 *   Only STUN and LOCAL candidates.
 *
 * @param turn_server      IP of stun server.
 * @param use_turn_server  enable use of stun server
 */
	PPL_DECLARE (int) am_option_enable_turn_server(const char *turn_server,
												   int use_turn_server);

/**
 * Configure amsip to use turn server for relay.
 *
 * EXPERIMENTAL: support for a real TURN server.
 * 
 * @param relay_server      IP of relay (turn) server.
 * @param use_relay_server  enable use of relay server
 */
	PPL_DECLARE (int) am_option_enable_relay_server(const char
													*relay_server,
													int use_relay_server);

/**
 * Configure amsip to detect interface with a specific IP to reach.
 * This method will have an impact on the IP put in the Via header,
 * Contact header, and in the SDP connection address.
 *
 * @param ipv4_for_gateway specific IP to reach.
 */
	PPL_DECLARE (int) am_option_set_ipv4_for_gateway(const char
													 *ipv4_for_gateway);

/**
 * Configure amsip to add "rport" parameter.
 *
 * @param enable      0: disable 1: enable
 */
	PPL_DECLARE (int) am_option_enable_rport(int enable);

/**
 * Configure amsip and DNS capabilities:
 *
 *  use '2' for NAPTR+SRV support.
 *  use '1' for SRV support.
 *  use '0' for neither NAPTR nor SRV.
 *
 * @param dns_capabilities      0, 1 or 2
 */
	PPL_DECLARE (int) am_option_set_dns_capabilities(int dns_capabilities);

/**
 * Configure amsip to use an outbound proxy (PLEASE DO NOT USE)
 *
 * @param outbound     proxy parameter.
 */
	PPL_DECLARE (int) am_option_enable_outbound_proxy(const char *outbound);

/**
 * Configure amsip to send empty UDP packets to keep alive binding behind NATs
 *
 * @param interval      0: disable   X: interval in ms between each UDP packets (default: 25000)
 */
	PPL_DECLARE (int) am_option_enable_keepalive(int interval);

/**
 * Configure amsip to enable SRTP profile.
 *
 * @param profile      profile string (RTP/AVP or RTP/SAVP)
 */
	PPL_DECLARE (int) am_option_set_audio_profile(const char *profile);

/**
 * Configure amsip to enable SRTP profile.
 *
 * @param profile      profile string (RTP/AVP or RTP/SAVP)
 */
	PPL_DECLARE (int) am_option_set_video_profile(const char *profile);

/**
 * Configure amsip to enable SRTP profile.
 *
 * @param profile      profile string (RTP/AVP or RTP/SAVP)
 */
	PPL_DECLARE (int) am_option_set_text_profile(const char *profile);

/**
 * Configure amsip to enable SRTP profile.
 *
 * @param profile      profile string (RTP/AVP or RTP/SAVP)
 */
	PPL_DECLARE (int) am_option_set_udpftp_profile(const char *profile);

/**
 * Configure amsip to send 101
 *
 * @param enable      0: disable 1: enable
 */
	PPL_DECLARE (int) am_option_enable_101(int enable);

/**
 * Configure amsip to enable session timer and set expiration interval
 *
 * @param session_expires      0: disable >0: set expiration interval
 */
	PPL_DECLARE (int) am_option_enable_session_timers(int session_expires);

/**
 * Configure amsip to put SDP in ACK
 *
 * @param enable      0: disable 1: enable
 */
	PPL_DECLARE (int) am_option_enable_sdp_in_ack(int enable);

/**
 * Configure amsip to use symmetric RTP for streaming
 *
 * @param enable      0: disable 1: enable
 */
	PPL_DECLARE (int) am_option_enable_symmetric_rtp(int enable);

/**
 * Retreive card name from id number:
 *    sndcard.card = 0;
 *    am_option_find_out_sound_card(&sndcard);
 *
 * @param card        -1: default 0 to N: for selecting audio input
 */
	PPL_DECLARE (int) am_option_find_out_sound_card(struct am_sndcard
													*sndcard);

/**
 * Retreive card name from id number:
 *    sndcard.card = 0;
 *    am_option_find_in_sound_card(&sndcard);
 *
 * @param card        -1: default 0 to N: for selecting audio input
 */
	PPL_DECLARE (int) am_option_find_in_sound_card(struct am_sndcard
												   *sndcard);

/**
 * Configure amsip to use a specific audio card for recording audio
 *
 * @param card        -1: default 0 to N: for selecting audio input
 */
	PPL_DECLARE (int) am_option_select_in_sound_card(int card);

/**
 * Configure amsip to use a specific audio card for playing audio
 *
 * @param card        -1: default 0 to N: for selecting audio output
 */
	PPL_DECLARE (int) am_option_select_out_sound_card(int card);

/**
 * Configure amsip to use a specific audio card for recording audio
 *
 * @param captcard        MSSndCard object pointer
 */
	PPL_DECLARE (int) am_option_select_in_custom_sound_card(MSSndCard *captcard);

/**
 * Configure amsip to use a specific audio card for playing audio
 *
 * @param playcard        MSSndCard object pointer
 */
	PPL_DECLARE (int) am_option_select_out_custom_sound_card(MSSndCard *playcard);

/**
 * Configure amsip to use a specific audio card for recording audio
 *
 * @param conf_id       Conference room number (below #define AMSIP_CONF_MAX)
 * @param card          -1: default 0 to N: for selecting audio input
 */
	PPL_DECLARE (int) am_option_conference_select_in_sound_card(int conf_id, int card);

/**
 * Configure amsip to use a specific audio card for playing audio
 *
 * @param conf_id       Conference room number (below #define AMSIP_CONF_MAX)
 * @param card          -1: default 0 to N: for selecting audio output
 */
	PPL_DECLARE (int) am_option_conference_select_out_sound_card(int conf_id, int card);

/**
 * Configure amsip to use a specific audio card for recording audio
 *
 * @param conf_id       Conference room number (below #define AMSIP_CONF_MAX)
 * @param captcard      MSSndCard object pointer
 */
	PPL_DECLARE (int) am_option_conference_select_in_custom_sound_card(int conf_id, MSSndCard *captcard);

/**
 * Configure amsip to use a specific audio card for playing audio
 *
 * @param conf_id       Conference room number (below #define AMSIP_CONF_MAX)
 * @param playcard      MSSndCard object pointer
 */
	PPL_DECLARE (int) am_option_conference_select_out_custom_sound_card(int conf_id, MSSndCard *playcard);

/**
 * Configure amsip to set volume of playback card
 *
 * @param card        -1: default 0 to N: for selecting audio output
 * @param mixer        0: master, 1: playback
 * @param percent      0: no volume, 100: maximum volume
 */
	PPL_DECLARE (int) am_option_set_volume_out_sound_card(int card, int mixer, int percent);

/**
 * Configure amsip to get volume of playback card
 *
 * @param card        -1: default 0 to N: for selecting audio output
 * @param mixer        0: master, 1: playback
 */
	PPL_DECLARE (int) am_option_get_volume_out_sound_card(int card, int mixer);


/**
 * Configure amsip to set volume of capture card
 *
 * @param card        -1: default 0 to N: for selecting audio input
 * @param percent      0: no volume, 100: maximum volume
 */
	PPL_DECLARE (int) am_option_set_volume_in_sound_card(int card, int percent);

/**
 * Configure amsip to get volume of capture card
 *
 * @param card        -1: default 0 to N: for selecting audio input
 */
	PPL_DECLARE (int) am_option_get_volume_in_sound_card(int card);

/**
 * Configure amsip to mute/unmute playback card
 *
 * @param card        -1: default 0 to N: for selecting audio output
 * @param mixer        0: mixer, 1: playback
 * @param val          0: unmute, 1: mute
 */
	PPL_DECLARE (int) am_option_set_mute_out_sound_card(int card, int mixer, int val);

/**
 * Configure amsip to mute/unmute capture card
 *
 * @param card        -1: default 0 to N: for selecting audio output
 * @param val          0: unmute, 1: mute
 */
	PPL_DECLARE (int) am_option_set_mute_in_sound_card(int card, int val);

#ifdef ENABLE_VIDEO

/**
 * Retreive camera name from id number:
 *    camera.card = 0;
 *    am_option_find_camera(&camera);
 *
 * @param camera        -1: "Static Image" and 0 to N: for selecting camera device
 */
	PPL_DECLARE (int) am_option_find_camera(struct am_camera *camera);

/**
 * Configure amsip to use a specific camera device for grabbing video
 *
 * @param card        -1: "Static Image" and 0 to N: for selecting camera device
 */
	PPL_DECLARE (int) am_option_select_camera(int card);

/**
 * Configure amsip to use different view mode for selfview
 * -1: disabled
 * 0: bottom right corner
 * 1: top left corner
 * 2: top right corner
 * 3: bottom left corner
 * 4: bottom right corner OR automatic on bottom or right if there is enaough space
 * 5: top left corner OR automatic on bottom or right if there is enaough space
 * 6: top right corner OR automatic on bottom or right if there is enaough space
 * 7: bottom left corner OR automatic on bottom or right if there is enaough space
 *
 * @param mode        display mode for selfview
 */
	PPL_DECLARE (int) am_option_set_selfview_mode(int mode);

/**
 * Configure amsip to display selfview at specific position
 * Position and size are provided in percentage of the full window.
 *
 * @param posx        X
 * @param posy        Y
 * @param size        size
 */
	PPL_DECLARE (int) am_option_set_selfview_position(float posx, float posy, float size);

/**
 * Configure amsip to retreive current display position of selfview
 * Position and size are provided in percentage of the full window.
 *
 * @param posx        X
 * @param posy        Y
 * @param size        size
 */
	PPL_DECLARE (int) am_option_get_selfview_position(float *posx, float *posy, float *size);

/**
 * Configure amsip to use specific scale factor.
 * Selfview size will equal to imagesize/scalefactor
 *
 * @param scalefactor        Scale Factor
 */
	PPL_DECLARE (int) am_option_set_selfview_scalefactor(float scalefactor);

/**
 * Configure amsip to set background color for display border.
 *
 * @param red        red
 * @param green      green
 * @param blue       blue
 */
	PPL_DECLARE (int) am_option_set_background_color(int red, int green, int blue);

#define AMSIP_OPTION_WEBCAM_FPS 0 /* float */
#define AMSIP_OPTION_FORCE_ENCODER_FPS 1 /* float */

/**
 * Configure video options for amsip.
 * Current options are:
 *    AM_OPTION_WEBCAM_FPS
 *    AM_OPTION_FORCE_ENCODER_FPS
 *
 * @param opt        opt identifier
 * @param arg        pointer to structure holding parameter value
 */
	PPL_DECLARE (int) am_option_set_video_option(int opt, void *arg);
#endif

/**
 * Configure amsip to enable or disable echo canceller.
 *
 * @param enable        0 to disable, 1 to enable
 * @param frame_size    frame size for echo canceller (should be 128 or 160)
 * @param tail_length   tail_length for echo canceller (should be 2048 or 4096)
 */
	PPL_DECLARE (int) am_option_enable_echo_canceller(int enable, int frame_size, int tail_length);

/**
 * Configure amsip to enable or disable internal VAD -used to discard silence packets upon buffer overflow-.
 * Please refer to speex documentation for values.
 *
 * @param enable            0 to disable, 1 to enable
 * @param vad_prob_start    value for speex VAD_PROB_START (between 0 and 100)
 * @param vad_prob_continue value for speex VAD_PROB_CONTINUE (between 0 and 100)
 */
	PPL_DECLARE (int) am_option_enable_vad(int enable, int vad_prob_start, int vad_prob_continue);

/**
 * Configure amsip to use AGC on MIC input.
 * Please refer to speex documentation for values.
 *
 * @param enable            0 to disable, 1 to enable
 * @param agc_level         value for speex AGC_LEVEL
 * @param max_gain          value for speex MAX_GAIN
 */
	PPL_DECLARE (int) am_option_enable_agc(int enable, int agc_level, int max_gain);

/**
 * Configure amsip to set DENOISER LEVEL on MIC input.
 * Please refer to speex documentation for values.
 * default value is "-30"
 *
 * @param denoise_level     0 to denoise_level, negative value to enable
 */
	PPL_DECLARE (int) am_option_set_denoise_level(int denoise_level);

/**
 * Configure amsip to enable or disable echo canceller.
 *
 * @param conf_id       Conference room number (below #define AMSIP_CONF_MAX)
 * @param enable        0 to disable, 1 to enable
 * @param frame_size    frame size for echo canceller (should be 128 or 160)
 * @param tail_length   tail_length for echo canceller (should be 2048 or 4096)
 */
	PPL_DECLARE (int) am_option_conference_enable_echo_canceller(int conf_id, int enable, int frame_size, int tail_length);

/**
 * Configure amsip to enable or disable internal VAD -used to discard silence packets upon buffer overflow-.
 * Please refer to speex documentation for values.
 *
 * @param conf_id           Conference room number (below #define AMSIP_CONF_MAX)
 * @param enable            0 to disable, 1 to enable
 * @param vad_prob_start    value for speex VAD_PROB_START (between 0 and 100)
 * @param vad_prob_continue value for speex VAD_PROB_CONTINUE (between 0 and 100)
 */
	PPL_DECLARE (int) am_option_conference_enable_vad(int conf_id, int enable, int vad_prob_start, int vad_prob_continue);

/**
 * Configure amsip to use AGC on MIC input.
 * Please refer to speex documentation for values.
 *
 * @param conf_id           Conference room number (below #define AMSIP_CONF_MAX)
 * @param enable            0 to disable, 1 to enable
 * @param agc_level         value for speex AGC_LEVEL
 * @param max_gain          value for speex MAX_GAIN
 */
	PPL_DECLARE (int) am_option_conference_enable_agc(int conf_id, int enable, int agc_level, int max_gain);

/**
 * Configure amsip to set DENOISER LEVEL on MIC input.
 * Please refer to speex documentation for values.
 * default value is "-30"
 *
 * @param conf_id           Conference room number (below #define AMSIP_CONF_MAX)
 * @param denoise_level     0 to denoise_level, negative value to enable
 */
	PPL_DECLARE (int) am_option_conference_set_denoise_level(int conf_id, int denoise_level);

/**
 * Configure amsip to add new SIP realm/login/password entry.
 *
 * @param realm         SIP realm on server.
 * @param login         SIP login on server.
 * @param passwd        SIP password on server.
 */
	PPL_DECLARE (int) am_option_set_password(const char *realm,
											 const char *login,
											 const char *passwd);

/**
 * Configure amsip to remove a SIP realm/login/password entry.
 *
 * @param realm         SIP realm on server.
 * @param login         SIP login on server.
 */
	PPL_DECLARE (int) am_option_delete_password(const char *realm,
											 const char *login);

/**
 * Configure amsip to add new SIP realm/login/password entry.
 *
 * @param from_username SIP username which appears in From headers.
 * @param realm         SIP realm on server.
 * @param login         SIP login on server.
 * @param passwd        SIP password on server.
 */
	PPL_DECLARE (int) am_option_set_password_for_username(const char *from_username,
		const char *realm,
		const char *login,
		const char *passwd);

/**
 * Configure amsip to remove all previous SIP realm/login/password entries.
 *
 */
	PPL_DECLARE (int) am_option_remove_password(void);

/**
 * Configure amsip to read mediastreamer2 plugins from a specific directory.
 *
 * @param directory        directory on host.
 */
	PPL_DECLARE (int) am_option_load_plugins(const char *directory);

/**
 * Configure amsip to suppport optionnal encryption (using RTP/AVP).
 *
 * @param optionnal_encryption  0 to disable, 1 to enable.
  */
	PPL_DECLARE (int) am_option_enable_optionnal_encryption(int
															optionnal_encryption);

/**
 * Configure amsip to add a=nortpproxy attribute in SDP.
 *
 * @param add_nortpproxy  0 to not add, 1 to add the attribute.
  */
	PPL_DECLARE (int) am_option_add_nortpproxy(int add_nortpproxy);

/**
 * Configure amsip to use a specific DSCP value in RTP streams.
 * DEPRECATED: use instead:
 *    am_option_set_audio_dscp
 *    am_option_set_video_dscp
 *    am_option_set_text_dscp
 *    am_option_set_udpftp_dscp
 * 
 * @param dscp_value  DSCP value to use in RTP streams.
  */
	PPL_DECLARE (int) am_option_set_dscp_value(int dscp_value);

/**
 * Configure amsip to use a specific DSCP value in SIP streams.
 *  Note: may not work on windows platform.
 * 
 * @param dscp_value  DSCP value to use in SIP streams.
  */
	PPL_DECLARE (int) am_option_set_sip_dscp(int dscp_value);

/**
 * Configure amsip to use a specific DSCP value in audio RTP streams (default is 0x38).
 *
 * @param dscp_value  DSCP value to use in audio RTP streams.
  */
	PPL_DECLARE (int) am_option_set_audio_dscp(int dscp_value);

/**
 * Configure amsip to use a specific DSCP value in video RTP streams (default is 0x28).
 *
 * @param dscp_value  DSCP value to use in video RTP streams.
  */
	PPL_DECLARE (int) am_option_set_video_dscp(int dscp_value);

/**
 * Configure amsip to use a specific DSCP value in text RTP streams (default is 0x38).
 *
 * @param dscp_value  DSCP value to use in text RTP streams.
  */
	PPL_DECLARE (int) am_option_set_text_dscp(int dscp_value);

/**
 * Configure amsip to use a specific DSCP value in udpftp RTP streams (default is 0x28).
 *
 * @param dscp_value  DSCP value to use in udpftp RTP streams.
  */
	PPL_DECLARE (int) am_option_set_udpftp_dscp(int dscp_value);

/**
 * Configure amsip to resolv host argument to this IP.
 *
 * @param host        host that will be resolved to ip.
 * @param ip          ip for resolution of host.
 */
	PPL_DECLARE (int) am_option_add_dns_cache(const char *host,
											  const char *ip);

/**
 * Configure amsip to support those extensions.
 * Supported header will be added in OPTIONS
 * and request/response initiating dialogs
 * and REGISTER.
 *
 * @param supported_extensions        list of supported extensions.
 */
	PPL_DECLARE (int) am_option_set_supported_extensions(const char
														 *supported_extensions);

/**
 * Configure amsip to accept those content-types.
 * Accepted header will be added in OPTIONS
 *
 * @param accepted_types        list of accepted types.
 */
	PPL_DECLARE (int) am_option_set_accepted_types(const char
												   *accepted_types);

/**
 * Configure amsip to support those METHODS.
 * Allow header will be added in OPTIONS
 * and request/response initiating dialogs.
 *
 * @param allowed_methods        list of supported METHODS.
 */
	PPL_DECLARE (int) am_option_set_allowed_methods(const char
													*allowed_methods);

/**
 * Configure amsip to support those Events.
 * Allow-events header will be added in OPTIONS
 * and request/response initiating dialogs and
 * in REGISTER.
 *
 * @param allowed_methods        list of supported Events.
 */
	PPL_DECLARE (int) am_option_set_allowed_events(const char
													*allowed_events);

/**
 * Configure amsip to TRY to open webcam using the specified size.
 * WINDOWS PLATFORM ONLY
 *
 * @param handle        handle of a window.
 * @param width         width size of window.
 * @param height        height size of window.
 */
	PPL_DECLARE (int) am_option_set_input_video_size(int width,
													 int height);
/**
 * Configure amsip to output video on this windows.
 *
 * @param desc          Display plugin description
 * @param handle        handle of a window. (native windows only)
 * @param width         width size of window.
 * @param height        height size of window.
 */

	PPL_DECLARE (int) am_option_set_window_display(MSDisplayDesc *desc, long handle, int width, int height);

/**
 * Configure amsip to output video on this windows.
 * handle is set on WINDOWS PLATFORM ONLY
 *
 * @param handle        handle of a window.
 * @param width         width size of window.
 * @param height        height size of window.
 */
	PPL_DECLARE (int) am_option_set_window_handle(long handle, int width,
												  int height);

/**
 * Configure amsip to output video preview on this windows.
 * ** NOT IMPLEMENTED IN AMSIP, video preview is inserted inside main view**
 *
 * @param handle        handle of a window.
 * @param width         width size of window.
 * @param height        height size of window.
 */
	PPL_DECLARE (int) am_option_set_window_preview_handle(long handle, int width,
												  int height);

/**
 * Configure amsip to use this image when no webcam is used.
 *
 * @param enable        open or close preview.
 */
	PPL_DECLARE (int) am_option_set_nowebcam(const char *nowebcam_image);

/**
 * Configure amsip to start/stop video preview.
 *
 * @param enable        open or close preview.
 */
	PPL_DECLARE (int) am_option_enable_preview(int enable);

/**
 * Configure amsip to provide images to application via a callback.
 *
 * @param func        the callback
 */
	PPL_DECLARE (int) am_option_set_image_callback(on_video_module_new_image_cb func);

/**
 * Configure amsip to use 16kHz internally and allow wideband codecs.
 *
 * @param rate        internal rate to use.
 */
	PPL_DECLARE (int) am_option_set_rate(int rate);

/**
 * Set callback to analyse mic/speaker signal.
 *
 * @param speex_pp_process   Callback.
 */
	PPL_DECLARE (int) am_option_set_callback(unsigned int id, MSFilterNotifyFunc speex_pp_process, void *userdata);

/**
 * Set capture and playback amplification.
 *
 * 1  -> no change
 * >1 -> amplify volume
 * <1 -> decrease volume
 *
 * @param capture_gain    Linear amplification to recorded signal.
 * @param playback_gain   Linear amplification to plback signal.
 */
	PPL_DECLARE (int) am_option_set_volume_gain(float capture_gain, float playback_gain);

/**
 * Activate and Set Echo Limiter with parameters.
 *
 * @param enabled      0 disabled / 1 enabled.
 * @param threshold    threshold parameter for capture.
 * @param speed        speed parameter for capture.
 * @param force        force parameter for capture.
 * @param sustain      delay (ms) for which e.l. remains active after resuming from speech to silence.
 */
	PPL_DECLARE (int) am_option_set_echo_limitation(int enabled, float threshold, float speed, float force, int sustain);

/**
 * Activate and set noise gate parameter for Echo Limiter.
 *
 * @param enabled      0 disabled / 1 enabled.
 * @param threshold    threshold parameter.
 */
	PPL_DECLARE (int) am_option_set_noise_gate_threshold(int enabled, float threshold);

/**
 * Enable equalizer to control gain on some specific frequency.
 *
 * @param enable         0: Disabled, 1: Enabled
 */
	PPL_DECLARE (int) am_option_set_equalizer_state(int enable);

/**
 * Set equalizer parameter for a specific frequency.
 *
 * @param freq        frequency.
 * @param gain        gain to apply to frequency.
 * @param width       width.
 */
	PPL_DECLARE (int) am_option_set_equalizer_params(float freq, float gain, float width);

/**
 * Enable equalizer to control gain on some specific frequency (on microphone side).
 *
 * @param enable         0: Disabled, 1: Enabled
 */
	PPL_DECLARE (int) am_option_set_mic_equalizer_state(int enable);

/**
 * Set equalizer parameter for a specific frequency (on microphone side).
 *
 * @param freq        frequency.
 * @param gain        gain to apply to frequency.
 * @param width       width.
 */
	PPL_DECLARE (int) am_option_set_mic_equalizer_params(float freq, float gain, float width);

/**
 * Configure amsip to use 16kHz internally and allow wideband codecs.
 *
 * @param conf_id           Conference room number (below #define AMSIP_CONF_MAX)
 * @param rate              internal rate to use.
 */
	PPL_DECLARE (int) am_option_conference_set_rate(int conf_id, int rate);

/**
 * Set callback to analyse mic/speaker signal.
 *
 * @param conf_id           Conference room number (below #define AMSIP_CONF_MAX)
 * @param speex_pp_process  Callback.
 */
	PPL_DECLARE (int) am_option_conference_set_callback(int conf_id, unsigned int id, MSFilterNotifyFunc speex_pp_process, void *userdata);

/**
 * Set capture and playback amplification.
 *
 * 1  -> no change
 * >1 -> amplify volume
 * <1 -> decrease volume
 *
 * @param conf_id           Conference room number (below #define AMSIP_CONF_MAX)
 * @param capture_gain      Linear amplification to recorded signal.
 * @param playback_gain     Linear amplification to plback signal.
 */
	PPL_DECLARE (int) am_option_conference_set_volume_gain(int conf_id, float capture_gain, float playback_gain);

/**
 * Activate and Set Echo Limiter with parameters.
 *
 * @param conf_id           Conference room number (below #define AMSIP_CONF_MAX)
 * @param enabled           0 disabled / 1 enabled.
 * @param threshold         threshold parameter for capture.
 * @param speed             speed parameter for capture.
 * @param force             force parameter for capture.
 * @param sustain           delay (ms) for which e.l. remains active after resuming from speech to silence.
 */
	PPL_DECLARE (int) am_option_conference_set_echo_limitation(int conf_id, int enabled, float threshold, float speed, float force, int sustain);

/**
 * Activate and set noise gate parameter for Echo Limiter.
 *
 * @param conf_id           Conference room number (below #define AMSIP_CONF_MAX)
 * @param enabled           0 disabled / 1 enabled.
 * @param threshold         threshold parameter.
 */
	PPL_DECLARE (int) am_option_conference_set_noise_gate_threshold(int conf_id, int enabled, float threshold);

/**
 * Enable equalizer to control gain on some specific frequency.
 *
 * @param conf_id           Conference room number (below #define AMSIP_CONF_MAX)
 * @param enable            0: Disabled, 1: Enabled
 */
	PPL_DECLARE (int) am_option_conference_set_equalizer_state(int conf_id, int enable);

/**
 * Set equalizer parameter for a specific frequency.
 *
 * @param conf_id           Conference room number (below #define AMSIP_CONF_MAX)
 * @param freq              frequency.
 * @param gain              gain to apply to frequency.
 * @param width             width.
 */
	PPL_DECLARE (int) am_option_conference_set_equalizer_params(int conf_id, float freq, float gain, float width);

/**
 * Enable equalizer to control gain on some specific frequency (on microphone side).
 *
 * @param conf_id           Conference room number (below #define AMSIP_CONF_MAX)
 * @param enable            0: Disabled, 1: Enabled
 */
	PPL_DECLARE (int) am_option_conference_set_mic_equalizer_state(int conf_id, int enable);

/**
 * Set equalizer parameter for a specific frequency (on microphone side).
 *
 * @param conf_id           Conference room number (below #define AMSIP_CONF_MAX)
 * @param freq              frequency.
 * @param gain              gain to apply to frequency.
 * @param width             width.
 */
	PPL_DECLARE (int) am_option_conference_set_mic_equalizer_params(int conf_id, float freq, float gain, float width);

/**
 * Retreive a socket where data is written when a eXosip_event
 * is available in amsip/eXosip2 fifo.
 *
 */
	PPL_DECLARE (int) am_option_geteventsocket(void);

	int _amsip_get_stun_socket(const char *stun_server, int srcport,
							   char *firewall, int *port);


/** @} */


#ifdef __cplusplus
}
#endif
#endif
