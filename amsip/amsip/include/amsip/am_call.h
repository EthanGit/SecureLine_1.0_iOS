/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#ifndef __AM_CALL_H__
#define __AM_CALL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <amsip/am_version.h>
#include <amsip/am_filter.h>

	struct am_audio_stats;
	struct am_bandwidth_stats;
	struct am_dtmf_event;

	struct am_fileinfo {
		char filename[1024];
		int file_size;
		int bytes_sent;
		int bytes_received;
	};

/**
 * @file am_call.h
 * @brief amsip session API
 *
 * This file provide the API needed to control session.
 *
 * <ul>
 * <li>start call</li>
 * <li>answer call</li>
 * <li>modify call</li>
 * <li>transfer call</li>
 * <li>redirect call</li>
 * <li>stop call</li>
 * </ul>
 *
 */

/**
 * @defgroup amsip_session amsip session interface
 * @ingroup amsip_message
 * @{
 */

/**
 * Configure amsip to start a SIP session.
 *
 * @param identity           Caller SIP identity
 * @param url                Callee SIP identity
 * @param proxy              Set the proxy server
 * @param outbound_proxy     OutBound Proxy
 */
	PPL_DECLARE (int) am_session_start(const char *identity,
									   const char *url, const char *proxy,
									   const char *outbound_proxy);

#ifdef ENABLE_VIDEO
/**
 * Configure amsip to start a SIP session with video.
 *
 * @param identity           Caller SIP identity
 * @param url                Callee SIP identity
 * @param proxy              Set the proxy server
 * @param outbound_proxy     OutBound Proxy
 */
	PPL_DECLARE (int) am_session_start_with_video(const char *identity,
												  const char *url,
												  const char *proxy,
												  const char
												  *outbound_proxy);
#endif

/**
 * Configure amsip to start a SIP session with text.
 *
 * @param identity           Caller SIP identity
 * @param url                Callee SIP identity
 * @param proxy              Set the proxy server
 * @param outbound_proxy     OutBound Proxy
 */
	PPL_DECLARE (int) am_session_start_text_conversation(const char
														 *identity,
														 const char *url,
														 const char *proxy,
														 const char
														 *outbound_proxy);

/**
 * Configure amsip to start a file transfer using UDP/RTP.
 *
 * @param identity           Caller SIP identity
 * @param url                Callee SIP identity
 * @param proxy              Set the proxy server
 * @param outbound_proxy     OutBound Proxy
 */
	PPL_DECLARE (int) am_session_start_file_transfer(const char *identity,
													 const char *url,
													 const char *proxy,
													 const char
													 *outbound_proxy);

/**
 * Get UDP/RTP data from remote party.
 *
 * @param did                Session identifier.
 * @param _blk               Block that contains text data.
 */
	PPL_DECLARE (int) am_session_get_udpftp(int did, MST140Block * _blk);

/**
 * Send UDP/RTP data from remote party.
 *
 * @param did                Session identifier.
 * @param _blk               Block that contains text data.
 */
	PPL_DECLARE (int) am_session_send_udpftp(int did, MST140Block * _blk);

/**
 * Get UDP/RTP text from remote party.
 *
 * @param did                Session identifier.
 * @param _blk               Block that contains text data.
 */
	PPL_DECLARE (int) am_session_get_text(int did, MST140Block * _blk);

/**
 * Send UDP/RTP text from remote party.
 *
 * @param did                Session identifier.
 * @param _blk               Block that contains text data.
 */
	PPL_DECLARE (int) am_session_send_text(int did, MST140Block * _blk);

/**
 * Release allocated string inside MST140Block.
 *
 * @param _blk               Block that contains text data.
 */
	PPL_DECLARE (void) am_free_t140block(MST140Block * _blk);

/**
 * Configure amsip to put on hold a SIP session. (send wav_file)
 *
 * @param did                Session identifier.
 * @param wav_file           Wav file to play.
 */
	PPL_DECLARE (int) am_session_hold(int did, const char *wav_file);

/**
 * Configure amsip to put off hold a SIP session.
 *
 * @param did                Session identifier.
 */
	PPL_DECLARE (int) am_session_off_hold(int did);

/**
 * Configure amsip to make streams inactive.
 *
 * @param did                Session identifier.
 */
	PPL_DECLARE (int) am_session_inactive(int did);

/**
 * Configure amsip to make streams inactive.
 * Note: put ip=0_0_0_0 and port=0 for the
 * audio SDP media block.
 * Note2: THIS IS NOT POSSIBLE TO RETREIVE BACK
 * THIS CALL. THIS METHOD MUST ONLY BE USED BEFORE
 * MAKING A CALL TRANSFER WITH SOME UNCOMPLIANT
 * SIP-PBX.
 *
 * @param did                Session identifier.
 */
	PPL_DECLARE (int) am_session_inactive_0_0_0_0(int did);

/**
 * Configure amsip to mute a SIP session. (send silence)
 *
 * @param did                Session identifier.
 */
	PPL_DECLARE (int) am_session_mute(int did);

/**
 * Configure amsip to unmute a SIP session.
 *
 * @param did                Session identifier.
 */
	PPL_DECLARE (int) am_session_unmute(int did);

/**
 * OBSOLETE.
 *
 * @param identity           Caller SIP identity
 * @param url                Callee SIP identity
 * @param proxy              Set the proxy server
 * @param outbound_proxy     OutBound Proxy
 * @param conf_name          Conference name
 */
	PPL_DECLARE (int) am_session_add_in_conference(const char *identity,
												   const char *url,
												   const char *proxy,
												   const char
												   *outbound_proxy,
												   const char *conf_name);

/**
 * Configure amsip to establish a SIP session.
 *
 * @param tid                Transaction identifier.
 * @param did                Session identifier.
 * @param code               Code to use.
 * @param enable_audio       start audio if available.
 */
	PPL_DECLARE (int) am_session_answer(int tid, int did, int code, int enable_audio);

/**
 * Configure amsip to establish a SIP session.
 *
 * @param conf_id            Conference room number (below #define AMSIP_CONF_MAX)
 * @param tid                Transaction identifier.
 * @param did                Session identifier.
 * @param code               Code to use.
 * @param enable_audio       start audio if available.
 */
	PPL_DECLARE (int) am_session_conference_answer(int conf_id, int tid, int did, int code, int enable_audio);

/**
 * Configure amsip to establish a SIP session.
 *
 * @param tid                Transaction identifier.
 * @param did                Session identifier.
 * @param code               Code to use.
 * @param enable_audio       start audio if available.
 * @param enable_video       start video if available.
 */
	PPL_DECLARE (int) am_session_answer_with_video(int tid, int did, int code, int enable_audio, int enable_video);

/**
 * Configure amsip to establish a SIP session.
 *
 * @param conf_id            Conference room number (below #define AMSIP_CONF_MAX)
 * @param tid                Transaction identifier.
 * @param did                Session identifier.
 * @param code               Code to use.
 * @param enable_audio       start audio if available.
 * @param enable_video       start video if available.
 */
	PPL_DECLARE (int) am_session_conference_answer_with_video (int conf_id, int tid, int did, int code, int enable_audio,
										int enable_video);

/**
 * Connect a call to a conference.
 *
 * @param cid                dialog identifier.
 * @param did                Session identifier.
 */
	PPL_DECLARE (int) am_session_conference_detach(int cid, int did);

/**
 * Disconnect a call from a conference.
 *
 * @param conf_id            Conference room number (below #define AMSIP_CONF_MAX)
 * @param cid                dialog identifier.
 * @param did                Session identifier.
 */
	PPL_DECLARE (int) am_session_conference_attach(int conf_id, int cid, int did);

/**
 * Configure amsip to redirect a SIP session.
 *
 * @param tid                Transaction identifier.
 * @param did                Session identifier.
 * @param code               Code to use.
 * @param url                SIP (or other url) address for redirection.
 */
	PPL_DECLARE (int) am_session_redirect(int tid, int did, int code,
										  const char *url);

/**
 * Configure amsip to establish a SIP session.
 *
 * @param cid                dialog identifier.
 * @param did                Session identifier.
 * @param code               Code to use (if answer needed).
 */
	PPL_DECLARE (int) am_session_stop(int cid, int did, int code);

/**
 * Configure amsip to transfer a SIP session.
 *
 * @param did                Session identifier.
 * @param url                SIP (or other url) address for redirection.
 */
	PPL_DECLARE (int) am_session_refer(int did, const char *url,
									   const char *referred_by);

/**
 * Add Replaces parameter to refer-to url.
 *
 * @param did                Session identifier.
 * @param refer_to           String to carry refer_to.
 * @param refer_to_len       Size of refer_to string.
 */
	PPL_DECLARE (int) am_session_get_referto(int did, char *refer_to,
											 size_t refer_to_len);

/**
 * Find the call that relates to the Replaces header.
 *
 * @param request       requet containging a replace header.
 */
	PPL_DECLARE (int) am_session_find_by_replaces(osip_message_t *
												  request);

/**
 * Configure amsip to establish a SIP session.
 *
 * @param tid                Transaction identifier.
 * @param did                Session identifier.
 * @param code               Code to use.
 */
	PPL_DECLARE (int) am_session_answer_request(int tid, int did,
												int code);

/**
 * Send a request during a session
 *
 * @param did                Session identifier.
 * @param sub_state			 Subscription-State parameter.
 * @param content_type		 Content-type of the body
 * @param body				 Attached content
 * @param size				 Size of body
 */
	PPL_DECLARE (int) am_session_send_notify(int did, const char *sub_state,
		const char *content_type, const char *body,int size);

/**
 * Send a request during a session
 *
 * @param did                Session identifier.
 * @param method			 SIP method (e.g. INVITE)
 * @param content_type		 Content-type of the body
 * @param body				 Attached content
 * @param size				 Size of body
 */
	PPL_DECLARE (int) am_session_send_request(int did, const char *method,
											  const char *content_type,
											  const char *body, int size);

/**
 * Configure amsip to play a file in an outgoing RTP session.
 *
 * @param did                Session identifier.
 * @param wav_file           File to play.
 */
	PPL_DECLARE (int) am_session_play_file(int did, const char *wav_file);

/**
 * Configure amsip to play a file in an outgoing RTP session.
 *
 * @param did                Session identifier.
 * @param wav_file           File to play.
 * @param repeat             repeat: -2 to play once, >0 delai between replay
 * @param cb_fileplayer_eof  callback to indicate endoffile
 */
	PPL_DECLARE (int) am_session_playfile(int did, const char *wav_file,
										  int repeat,
										  MSFilterNotifyFunc
										  cb_fileplayer_eof);

/**
 * Configure amsip to send an in-band DTMF in an outgoing RTP session.
 *
 * @param did                Session identifier.
 * @param dtmf_number        DTMF to send.
 */
	PPL_DECLARE (int) am_session_send_inband_dtmf(int did,
												  char dtmf_number);

/**
 * Configure amsip to send a RTP telephone-event in an outgoing RTP session.
 *
 * @param did                Session identifier.
 * @param dtmf_number        DTMF to send.
 */
	PPL_DECLARE (int) am_session_send_rtp_dtmf(int did, char dtmf_number);

/**
 * Configure amsip to send an INFO with dtmf attachement.
 *
 * @param did                Session identifier.
 * @param dtmf_number        DTMF to send.
 */
	PPL_DECLARE (int) am_session_send_dtmf(int did, char dtmf_number);

/**
 * Configure amsip to send an INFO with dtmf attachement.
 *
 * @param did                Session identifier.
 * @param dtmf_number        DTMF to send.
 * @param duration           DTMF duration.
 */
	PPL_DECLARE (int) am_session_send_dtmf_with_duration(int did,
														 char dtmf_number,
														 int duration);

/**
 * Get dtmf from RTP session.
 *
 * @param did                Session identifier.
 * @param dtmf_event         dtmf_event structure.
 */
	PPL_DECLARE (int) am_session_get_dtmf_event(int did,
												struct am_dtmf_event
												*dtmf_event);

/**
 * Find out the destination IP/PORT for audio stream.
 * return >0  remote SDP ip/port used
 * return ==0 remote SDP candidate ip/port used (probably for direct comunication)
 * return <0  undefined error.
 *
 * @param did                Session identifier.
 * @param remote_info        string of size 256 to get the remote IP/PORT for audio stream.
 */
	PPL_DECLARE (int) am_session_get_audio_remote(int did,
												  char *remote_info);

/**
 * Get statistics for RTP udpftp stream.
 *
 * @param did                Session identifier.
 * @param band_stats         struct to hold information.
 */
	PPL_DECLARE (int) am_session_get_udpftp_bandwidth(int did,
													  struct
													  am_bandwidth_stats
													  *band_stats);

/**
 * Get statistics for RTP text stream.
 *
 * @param did                Session identifier.
 * @param band_stats         struct to hold information.
 */
	PPL_DECLARE (int) am_session_get_text_bandwidth(int did,
													struct
													am_bandwidth_stats
													*band_stats);

/**
 * Get statistics for RTP audio stream.
 *
 * @param did                Session identifier.
 * @param band_stats         struct to hold information.
 */
	PPL_DECLARE (int) am_session_get_audio_bandwidth(int did,
													 struct
													 am_bandwidth_stats
													 *band_stats);

/**
 * Get statistics for RTP audio stream.
 *
 * @param did                Session identifier.
 * @param audio_stats        struct to hold statistics.
 */
	PPL_DECLARE (int) am_session_get_audio_statistics(int did, struct am_audio_stats
													  *audio_stats);

/**
 * Get statistics for RTP audio stream.
 *
 * @param did                Session identifier.
 * @param preferred_codec    Preferred codec to use.
 * @param compress_more      compress_more. (0 means you want to use more bandwidth)
 */
	PPL_DECLARE (int) am_session_modify_bitrate(int did,
												const char
												*preferred_codec,
												int compress_more);

/**
 * Record both audio stream (incoming+outgoing).
 *
 * @param did                Session identifier.
 * @param recfile            File Name for recording.
 */
	PPL_DECLARE (int) am_session_record(int did, const char *recfile);

/**
 * Stop recording the call.
 *
 * @param did                Session identifier.
 */
	PPL_DECLARE (int) am_session_stop_record(int did);

#ifdef ENABLE_VIDEO

/**
 * Reject any incoming video negotiation by setting port to 0 in SDP.
 *
 * @param did                Session identifier.
 * @param _reject_video      0 to not reject, >0 to reject.
 */
	PPL_DECLARE (int) am_session_reject_video(int did, int _reject_video);

/**
 * Add video in the call.
 *
 * @param did                Session identifier.
 */
	PPL_DECLARE (int) am_session_add_video(int did);

/**
 * Disable video in the call.
 *
 * @param did                Session identifier.
 */
	PPL_DECLARE (int) am_session_stop_video(int did);

/**
 * Adapt video bitrate in the call.
 *
 * @param did                Session identifier.
 * @param lost               packet loss from RTCP report in percentage.
 */
	PPL_DECLARE (int) am_session_adapt_video_bitrate(int did, float lost);

/**
 * Get statistics for RTP video stream.
 *
 * @param did                Session identifier.
 * @param band_stats         struct to hold information.
 */
	PPL_DECLARE (int) am_session_get_video_bandwidth(int did,
													 struct
													 am_bandwidth_stats
													 *band_stats);

/**
 * Get RTP events (DTMF, RTCP...) on audio stream
 *
 * @param did                Session identifier.
 */
	PPL_DECLARE (OrtpEvent*) am_session_get_audio_rtp_events(int did);

/**
 * Confirm ZRTP SAS for audio stream has been verified.
 *
 * @param did                Session identifier.
 */
	PPL_DECLARE (int) am_session_set_audio_zrtp_sas_verified(int did);

/**
 * Get RTP events (DTMF, RTCP...) on video stream
 *
 * @param did                Session identifier.
 */
	PPL_DECLARE (OrtpEvent*) am_session_get_video_rtp_events(int did);

/**
 * Confirm ZRTP SAS for video stream has been verified.
 *
 * @param did                Session identifier.
 */
	PPL_DECLARE (int) am_session_set_video_zrtp_sas_verified(int did);

/**
 * Release allocated RTP events.
 *
 * @param evt                Event to release.
 */
	PPL_DECLARE (int) am_session_release_rtp_events(OrtpEvent *evt);

/**
 * Send VFU in the video call.
 *
 * @param did                Session identifier.
 */
	PPL_DECLARE (int) am_session_send_vfu(int did);

#endif

/**
 * Add t140 chat session in the call.
 *
 * @param did                Session identifier.
 */
	PPL_DECLARE (int) am_session_add_t140(int did);


/**
 * Receive file within a text conversation (t140) established call.
 *
 * @param cid                Session identifier.
 * @param did                Session identifier.
 * @param filename           filename of the file to receive.
 */
	PPL_DECLARE (int) am_session_receive_file(int cid, int did,
											  const char *filename);

/**
 * Receive file within a text conversation (t140) established call.
 *
 * @param cid                Session identifier.
 * @param did                Session identifier.
 * @param fileinfo           information about file transfer.
 */
	PPL_DECLARE (int) am_session_file_info(int cid, int did,
		struct am_fileinfo *fileinfo);

/**
 * Send file within a text conversation (t140) established call.
 *
 * @param cid                Session identifier.
 * @param did                Session identifier.
 * @param filename           filename of the file to receive.
 */
	PPL_DECLARE (int) am_session_send_file(int cid, int did,
										   const char *filename,
										   const char *filename_short);

/**
 * Release udpftp transfer resources.
 *
 * @param cid                Session identifier.
 * @param did                Session identifier.
 */
	PPL_DECLARE (int) am_session_stop_transfer(int cid, int did);

/**
 * Add new source for sending RTP data.
 *
 * @param did                Session identifier.
 * @param external_rtpdata   External filter.
 */
	PPL_DECLARE (int)
	 am_session_add_external_rtpdata(int did, MSFilter * external_rtpdata);

/**
 * Send a request during a session (NOTE: NOT FOR INVITE, ACK, BYE, CANCEL)
 *
 * @param did                Session identifier.
 * @param method			 SIP method (e.g. MESSAGE, REFER, NOTIFY, INFO...)
 */
	PPL_DECLARE (int) am_session_build_req(int did, const char *method, osip_message_t **
												  request);

/**
 * Send a request during a session (NOTE: NOT FOR INVITE, ACK, BYE, CANCEL)
 *
 * @param did                Session identifier.
 * @param method			 SIP method (e.g. MESSAGE, REFER, NOTIFY, INFO...)
 */
	PPL_DECLARE (int) am_session_send_req(int did, const char *method, osip_message_t *
												  request);


/** @} */

#ifdef __cplusplus
}
#endif
#endif
