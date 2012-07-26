/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#ifndef __AM_PLAYER_H__
#define __AM_PLAYER_H__

#ifdef __cplusplus
extern "C" {
#endif
	
#include <amsip/am_version.h>

/**
 * @file am_player.h
 * @brief amsip player API
 *
 * This file provide the API needed to play file on sound cards.
 *
 * <ul>
 * <li>play file</li>
 * <li>stop playing file</li>
 * </ul>
 *
 */

/**
 * @defgroup amsip_player amsip player interface
 * @ingroup amsip_message
 * @{
 */

	struct am_player {
		int player_soundcard;
		int player_loop;
		char player_file[256];
#ifdef WIN32
		MSFilter *player_soundread;
		MSFilter *player_void;
#endif
		MSFilter *player_resample_soundwrite;
		MSFilter *player_soundwrite;
		MSFilter *player_fileplayer;
		MSSndCard *player_card;
	};

	typedef struct am_player am_player_t;

/**
 * Configure amsip to play a file.
 *
 * @param soundcard          Device to use
 * @param file               File to play
 * @param loop               Play the file in a loop
 */
	PPL_DECLARE (int) am_player_start(int soundcard, const char *file,
									  int loop);

/**
 * Configure amsip to stop playing a file.
 *
 * @param soundcard          id of soundcard to stop.
 */
	PPL_DECLARE (int) am_player_stop(int soundcard);

/**
 * Configure amsip to stop release ressource in terminated players.
 *
 */
	PPL_DECLARE (int) am_player_stop_all(void);

/**
 * Configure amsip to stop release ressource in terminated players.
 *
 */
	PPL_DECLARE (int) am_player_stop_terminated(void);

/** @} */

#ifdef __cplusplus
}
#endif
#endif
