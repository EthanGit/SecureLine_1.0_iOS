/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#ifndef __AM_UDPFTP_START_H__
#define __AM_UDPFTP_START_H__

int _calls_start_udpftp_with_sipanswer(am_call_t * ca, int tid_for_offer,
									   osip_message_t * answer,
									   int local_is_offer);
int _calls_start_udpftp_from_sdpmessage(am_call_t * ca,
										sdp_message_t * offer,
										sdp_message_t * answer,
										int local_offer);
int _calls_start_udpftp_from_sipmessage(am_call_t * ca,
										osip_message_t * offer,
										osip_message_t * answer,
										int local_offer);

#endif
