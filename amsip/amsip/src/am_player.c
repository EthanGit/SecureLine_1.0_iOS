/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
  Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>
*/

#include "amsip/am_options.h"

#include "amsip/am_player.h"
#include "mediastreamer2/msfileplayer.h"

PPL_DECLARE (int) am_player_stop_all(void)
{
	am_player_t *player;
	int count = 0;
	int i;

	if (_antisipc.player_ticker==NULL)
	{
		return AMSIP_SUCCESS;
	}

	for (i = 0; i < AM_MAX_PLAYERS; i++) {
		if (_antisipc.players[i] != NULL) {
			player = _antisipc.players[i];
#ifdef WIN32
			if (player->player_soundread != NULL)
				ms_ticker_detach(_antisipc.player_ticker,
								 player->player_soundread);
#endif
			ms_ticker_detach(_antisipc.player_ticker,
							 player->player_fileplayer);
			ms_filter_unlink(player->player_fileplayer, 0,
							 player->player_resample_soundwrite, 0);
			ms_filter_unlink(player->player_resample_soundwrite, 0,
							 player->player_soundwrite, 0);
#ifdef WIN32
			if (player->player_soundread != NULL && player->player_void != NULL)
				ms_filter_unlink(player->player_soundread, 0,
								 player->player_void, 0);
#endif
			ms_filter_call_method_noarg(player->player_fileplayer,
										MS_FILE_PLAYER_CLOSE);

#ifdef WIN32
			if (player->player_soundread != NULL)
				ms_filter_destroy(player->player_soundread);
			if (player->player_void != NULL)
				ms_filter_destroy(player->player_void);
#endif
			if (player->player_fileplayer != NULL)
				ms_filter_destroy(player->player_fileplayer);
			if (player->player_resample_soundwrite != NULL)
				ms_filter_destroy(player->player_resample_soundwrite);
			if (player->player_soundwrite != NULL)
				ms_filter_destroy(player->player_soundwrite);
			if (player->player_card != NULL)
				ms_snd_card_destroy(player->player_card);

			osip_free(_antisipc.players[i]);
			_antisipc.players[i] = NULL;
			count++;
		}
	}

	return count;
}

PPL_DECLARE (int) am_player_stop_terminated(void)
{
	am_player_t *player;
	int count = 0;
	int i;

	if (_antisipc.player_ticker==NULL)
	{
		return AMSIP_SUCCESS;
	}
	
	for (i = 0; i < AM_MAX_PLAYERS; i++) {
		if (_antisipc.players[i] != NULL) {
			player = _antisipc.players[i];

			/* check loop property (-2 is no loop) */
			if (player->player_loop == -2) {
				int val = -1;

				/* check if player is playing in loop and if terminated */
				ms_mutex_lock(&_antisipc.player_ticker->lock);
				ms_filter_call_method(player->player_fileplayer,
									  MS_FILE_PLAYER_DONE, (void *) &val);
				ms_mutex_unlock(&_antisipc.player_ticker->lock);
				if (val == FALSE) {
					continue;
				}
#ifdef WIN32
				if (player->player_soundread != NULL)
					ms_ticker_detach(_antisipc.player_ticker,
									 player->player_soundread);
#endif
				ms_ticker_detach(_antisipc.player_ticker,
								 player->player_fileplayer);
				ms_filter_unlink(player->player_fileplayer, 0,
								 player->player_resample_soundwrite, 0);
				ms_filter_unlink(player->player_resample_soundwrite, 0,
								 player->player_soundwrite, 0);
#ifdef WIN32
				if (player->player_soundread != NULL && player->player_void != NULL)
					ms_filter_unlink(player->player_soundread, 0,
									 player->player_void, 0);
#endif
				ms_filter_call_method_noarg(player->player_fileplayer,
											MS_FILE_PLAYER_CLOSE);

#ifdef WIN32
				if (player->player_soundread != NULL)
					ms_filter_destroy(player->player_soundread);
				if (player->player_void != NULL)
					ms_filter_destroy(player->player_void);
#endif
				if (player->player_fileplayer != NULL)
					ms_filter_destroy(player->player_fileplayer);
				if (player->player_resample_soundwrite != NULL)
					ms_filter_destroy(player->player_resample_soundwrite);
				if (player->player_soundwrite != NULL)
					ms_filter_destroy(player->player_soundwrite);
				if (player->player_card != NULL)
					ms_snd_card_destroy(player->player_card);

				osip_free(_antisipc.players[i]);
				_antisipc.players[i] = NULL;
				count++;
			}
		}
	}

	return count;
}

PPL_DECLARE (int)
am_player_start(int soundcard, const char *file, int loop)
{
	MSSndCard *playcard = NULL;
	const MSList *elem =
		ms_snd_card_manager_get_list(ms_snd_card_manager_get());
	int k = 0;

	am_player_t *player;
	int val;
	int index;

#ifdef WIN32
	int cap = 0;
#endif
	int i;

	if (soundcard > 20)
		return AMSIP_BADPARAMETER;
	if (soundcard <= 0)
		soundcard = 0;
	
	am_player_stop(soundcard);

	if (_antisipc.player_ticker==NULL)
	{
		_antisipc.player_ticker = ms_ticker_new_withname("amsip-player");
	}
		
	player = NULL;
	for (index = 0; index < AM_MAX_PLAYERS; index++) {
		if (_antisipc.players[index] == NULL) {
			_antisipc.players[index] =
				osip_malloc(sizeof(struct am_player));
			if (_antisipc.players[index] == NULL)
				return AMSIP_NOMEM;
			player = _antisipc.players[index];
			memset(player, 0, sizeof(struct am_player));
			ms_message("am_player.c: using %i for new player.", index);
			break;
		}
	}

	if (player == NULL) {
		ms_message("am_player.c: no player available.");
		return AMSIP_NOTFOUND;	/* no channel available */
	}

	for (; elem != NULL; elem = elem->next) {
		MSSndCard *acard = (MSSndCard *) elem->data;

		if (acard->capabilities == MS_SND_CARD_CAP_PLAYBACK
			|| acard->capabilities == 3) {
			if (k == soundcard) {
#ifdef WIN32
				if (acard->capabilities == 3)
					cap = 1;
#endif
				playcard = acard;
				ms_message("am_player.c: select default playcard = %s %s.",
						   acard->desc->driver_type, acard->name);
				break;
			}
			k++;
		}
	}

	if (playcard == NULL) {
		osip_free(_antisipc.players[index]);
		_antisipc.players[index] = NULL;
		return AMSIP_NOTFOUND;
	}

	player->player_soundcard = soundcard;
	if (loop != 0)
		player->player_loop = 0;
	else
		player->player_loop = -2;
	snprintf(player->player_file, sizeof(player->player_file), "%s", file);

	player->player_card = ms_snd_card_dup(playcard);
	if (player->player_card == NULL)
	{
		osip_free(_antisipc.players[index]);
		_antisipc.players[index] = NULL;
		return AMSIP_NOTFOUND;
	}
#ifdef WIN32
	if (cap == 1) {
		OSVERSIONINFO info;

		info.dwOSVersionInfoSize = sizeof(info);
		GetVersionEx(&info);
		if (info.dwMajorVersion >= 5 && info.dwMinorVersion > 0) {
			player->player_soundread =
				ms_snd_card_create_reader(player->player_card);
			if (player->player_soundread != NULL)
				player->player_void = ms_filter_new(MS_VOID_SINK_ID);
		}
	}
#endif
	player->player_soundwrite =
		ms_snd_card_create_writer(player->player_card);
	if (player->player_soundwrite==NULL)
	{
#ifdef WIN32
		if (player->player_soundread != NULL)
			ms_filter_destroy(player->player_soundread);
		if (player->player_void != NULL)
			ms_filter_destroy(player->player_void);
#endif
		if (player->player_fileplayer != NULL)
			ms_filter_destroy(player->player_fileplayer);
		if (player->player_resample_soundwrite != NULL)
			ms_filter_destroy(player->player_resample_soundwrite);
		if (player->player_soundwrite != NULL)
			ms_filter_destroy(player->player_soundwrite);
		if (player->player_card != NULL)
			ms_snd_card_destroy(player->player_card);

		osip_free(_antisipc.players[index]);
		_antisipc.players[index] = NULL;
		return AMSIP_NODEVICE;
	}

	player->player_fileplayer = ms_filter_new(MS_FILE_PLAYER_ID);
	player->player_resample_soundwrite = ms_filter_new(MS_RESAMPLE_ID);
	if (player->player_fileplayer == NULL || player->player_resample_soundwrite == NULL)
	{
#ifdef WIN32
		if (player->player_soundread != NULL)
			ms_filter_destroy(player->player_soundread);
		if (player->player_void != NULL)
			ms_filter_destroy(player->player_void);
#endif
		if (player->player_fileplayer != NULL)
			ms_filter_destroy(player->player_fileplayer);
		if (player->player_resample_soundwrite != NULL)
			ms_filter_destroy(player->player_resample_soundwrite);
		if (player->player_soundwrite != NULL)
			ms_filter_destroy(player->player_soundwrite);
		if (player->player_card != NULL)
			ms_snd_card_destroy(player->player_card);

		osip_free(_antisipc.players[index]);
		_antisipc.players[index] = NULL;
		return AMSIP_NODEVICE;
	}

	i = ms_filter_call_method(player->player_fileplayer,
							  MS_FILE_PLAYER_OPEN,
							  (void *) player->player_file);
	if (i < 0) {
#ifdef WIN32
		if (player->player_soundread != NULL)
			ms_filter_destroy(player->player_soundread);
		if (player->player_void != NULL)
			ms_filter_destroy(player->player_void);
#endif
		if (player->player_fileplayer != NULL)
			ms_filter_destroy(player->player_fileplayer);
		if (player->player_resample_soundwrite != NULL)
			ms_filter_destroy(player->player_resample_soundwrite);
		if (player->player_soundwrite != NULL)
			ms_filter_destroy(player->player_soundwrite);
		if (player->player_card != NULL)
			ms_snd_card_destroy(player->player_card);

		osip_free(_antisipc.players[index]);
		_antisipc.players[index] = NULL;
		return AMSIP_FILE_NOT_EXIST;
	}

	if (player->player_loop == -2) {
		val = -2;				/* special value to play file only once and close */
		ms_filter_call_method(player->player_fileplayer,
							  MS_FILE_PLAYER_LOOP, (void *) &val);
	}

	if (strcmp(player->player_card->desc->driver_type, "WINSND")==0)
	{
		val = 4; /* each 80 ms a buffer will be sent to sound card */
		ms_filter_call_method(player->player_fileplayer,
							  MS_FILE_PLAYER_BIG_BUFFER, (void *) &val);
	}

	ms_filter_call_method(player->player_fileplayer,
						  MS_FILTER_GET_NCHANNELS, (void *) &val);
	if (val != 1) {
		ms_warning
			("Wrong wav format: (need file with rate/channel -> %i:%i)",
			 8000, 1);
		ms_filter_call_method_noarg(player->player_fileplayer,
									MS_FILE_PLAYER_CLOSE);

#ifdef WIN32
		if (player->player_soundread != NULL)
			ms_filter_destroy(player->player_soundread);
		if (player->player_void != NULL)
			ms_filter_destroy(player->player_void);
#endif
		if (player->player_fileplayer != NULL)
			ms_filter_destroy(player->player_fileplayer);
		if (player->player_resample_soundwrite != NULL)
			ms_filter_destroy(player->player_resample_soundwrite);
		if (player->player_soundwrite != NULL)
			ms_filter_destroy(player->player_soundwrite);
		if (player->player_card != NULL)
			ms_snd_card_destroy(player->player_card);

		osip_free(_antisipc.players[index]);
		_antisipc.players[index] = NULL;
		return AMSIP_WRONG_FORMAT;
	}

	ms_filter_call_method(player->player_fileplayer,
						  MS_FILTER_GET_SAMPLE_RATE, (void *) &val);
	if (val != 8000 && val != 9600 && val != 11025 && val != 12000 && val != 16000
		&& val != 22050 && val != 24000 && val != 32000 && val != 44100
		&& val != 48000 && val != 88200 && val != 96000 && val != 192000) {
		ms_warning("Wrong wav format: unrecognized sample rate");
		ms_filter_call_method_noarg(player->player_fileplayer,
									MS_FILE_PLAYER_CLOSE);

#ifdef WIN32
		if (player->player_soundread != NULL)
			ms_filter_destroy(player->player_soundread);
		if (player->player_void != NULL)
			ms_filter_destroy(player->player_void);
#endif
		if (player->player_fileplayer != NULL)
			ms_filter_destroy(player->player_fileplayer);
		if (player->player_resample_soundwrite != NULL)
			ms_filter_destroy(player->player_resample_soundwrite);
		if (player->player_soundwrite != NULL)
			ms_filter_destroy(player->player_soundwrite);
		if (player->player_card != NULL)
			ms_snd_card_destroy(player->player_card);

		osip_free(_antisipc.players[index]);
		_antisipc.players[index] = NULL;
		return AMSIP_WRONG_FORMAT;
	}
	ms_filter_call_method(player->player_resample_soundwrite,
						  MS_FILTER_SET_SAMPLE_RATE, (void *) &val);
	ms_filter_call_method(player->player_soundwrite,
						  MS_FILTER_SET_SAMPLE_RATE, (void *) &val);
	i = ms_filter_call_method(player->player_soundwrite,
						  MS_FILTER_GET_SAMPLE_RATE, (void *) &val);
	if (i != 0) {
		/* device support any rate */
		ms_filter_call_method(player->player_fileplayer,
							  MS_FILTER_GET_SAMPLE_RATE, (void *) &val);
	}
	ms_filter_call_method(player->player_resample_soundwrite,
						  MS_FILTER_SET_OUTPUT_SAMPLE_RATE, (void *) &val);

	ms_filter_call_method_noarg(player->player_fileplayer,
								MS_FILE_PLAYER_START);
	ms_filter_link(player->player_fileplayer, 0, player->player_resample_soundwrite,
				   0);
	ms_filter_link(player->player_resample_soundwrite, 0, player->player_soundwrite,
				   0);
#ifdef WIN32
	if (player->player_soundread != NULL && player->player_void != NULL)
		ms_filter_link(player->player_soundread, 0, player->player_void,
					   0);
#endif
	ms_ticker_attach(_antisipc.player_ticker, player->player_fileplayer);
#ifdef WIN32
	if (player->player_soundread != NULL)
		ms_ticker_attach(_antisipc.player_ticker,
						 player->player_soundread);
#endif
	return AMSIP_SUCCESS;
}

PPL_DECLARE (int) am_player_stop(int soundcard)
{
	am_player_t *player;
	int i;

	if (soundcard > 20)
		return AMSIP_BADPARAMETER;
	if (soundcard <= 0)
		soundcard = 0;

	if (_antisipc.player_ticker==NULL)
	{
		return AMSIP_SUCCESS;
	}
	
	for (i = 0; i < AM_MAX_PLAYERS; i++) {
		if (_antisipc.players[i] != NULL) {
			player = _antisipc.players[i];
			if (player->player_soundcard == soundcard) {
#ifdef WIN32
				if (player->player_soundread != NULL)
					ms_ticker_detach(_antisipc.player_ticker,
									 player->player_soundread);
#endif
				ms_ticker_detach(_antisipc.player_ticker,
								 player->player_fileplayer);
				ms_filter_unlink(player->player_fileplayer, 0,
								 player->player_resample_soundwrite, 0);
				ms_filter_unlink(player->player_resample_soundwrite, 0,
								 player->player_soundwrite, 0);
#ifdef WIN32
				if (player->player_soundread != NULL && player->player_void != NULL)
					ms_filter_unlink(player->player_soundread, 0,
									 player->player_void, 0);
#endif
				ms_filter_call_method_noarg(player->player_fileplayer,
											MS_FILE_PLAYER_CLOSE);

#ifdef WIN32
				if (player->player_soundread != NULL)
					ms_filter_destroy(player->player_soundread);
				if (player->player_void != NULL)
					ms_filter_destroy(player->player_void);
#endif
				if (player->player_soundwrite != NULL)
					ms_filter_destroy(player->player_soundwrite);
				if (player->player_resample_soundwrite != NULL)
					ms_filter_destroy(player->player_resample_soundwrite);
				if (player->player_fileplayer != NULL)
					ms_filter_destroy(player->player_fileplayer);
				if (player->player_card != NULL)
					ms_snd_card_destroy(player->player_card);

				osip_free(_antisipc.players[i]);
				_antisipc.players[i] = NULL;
				return AMSIP_SUCCESS;
			}
		}
	}

	return AMSIP_NOTFOUND;
}
