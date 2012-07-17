/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#include "amsip/am_options.h"
#include <osipparser2/osip_port.h>
#include <osip2/osip_mt.h>
#include <osip2/osip_time.h>

#include "mediastreamer2/msrtp.h"
#include "mediastreamer2/msticker.h"

#include "am_calls.h"

#ifndef WIN32
#define DWORD int
#include <sys/stat.h>
#endif

#define NUMBER_OF_PACKETS 20

#define UDPFTP_GETVERSION 1
#define UDPFTP_GETVERSION_ANSWER 2
#define UDPFTP_ERROR 3
#define UDPFTP_ERROR_ANSWER 4
#define UDPFTP_FILEINFO 5
#define UDPFTP_FILEINFO_ANSWER 6
#define UDPFTP_DOWNLOADFILE 7
#define UDPFTP_DOWNLOADFILE_ANSWER 8
#define UDPFTP_FILESIZE 9
#define UDPFTP_FILESIZE_ANSWER 10

struct udp_connection {
	int cid;
	int did;
	int remove;
#ifndef WIN32
	int fd;
#else
	HANDLE fd;
#endif
	char filename[1024];
	char filename_short[1024];
	int file_size;

	struct timeval timeout;

	int out_command_current;
	int out_command_acked;
	char arg[521];

	int in_command_current;

	/* context information for the file sender */
	int sent;
	int byte_sent;
	int bottom_seq;
	osip_list_t sender_packets;
	MST140Block blk_sent[NUMBER_OF_PACKETS];
	int restart;

	/* context information for the file receiver */
	int received;
	int byte_received;
};

#define MAX_UDPFTP_CONTEXT 15

struct udp_connection ops[MAX_UDPFTP_CONTEXT];

struct osip_thread *udpftp_thread = NULL;

int run_thread = 0;

struct osip_mutex *udp_connection_mutex = NULL;

int _am_session_stop_transfer(int cid, int did);

static int _udpftp_send_command(struct udp_connection *data)
{
	int i = AMSIP_UNDEFINED_ERROR;

	MST140Block blk_sent;

	memset(&blk_sent, 0, sizeof(MST140Block));

	if (data->out_command_current == UDPFTP_GETVERSION) {
		blk_sent.utf8_string =
			(char *) osip_malloc(11 + strlen(data->arg) + 1);
		snprintf(blk_sent.utf8_string, 11 + strlen(data->arg) + 1,
				 "GETVERSION %s", data->arg);
		blk_sent.size = 11 + (int) strlen(data->arg) + 1;
		i = am_session_send_udpftp(data->did, &blk_sent);
	} else if (data->out_command_current == UDPFTP_ERROR) {
		blk_sent.utf8_string =
			(char *) osip_malloc(6 + strlen(data->arg) + 1);
		snprintf(blk_sent.utf8_string, 6 + strlen(data->arg) + 1,
				 "ERROR %s", data->arg);
		blk_sent.size = 6 + (int) strlen(data->arg) + 1;
		i = am_session_send_udpftp(data->did, &blk_sent);
	} else if (data->out_command_current == UDPFTP_FILEINFO) {
		blk_sent.utf8_string =
			(char *) osip_malloc(9 + strlen(data->filename_short) + 1);
		snprintf(blk_sent.utf8_string,
				 9 + strlen(data->filename_short) + 1, "FILEINFO %s",
				 data->filename_short);
		blk_sent.size = 9 + (int) strlen(data->filename_short) + 1;
		i = am_session_send_udpftp(data->did, &blk_sent);

		/* send size AT the same time (TODO: retransmit...) */
		am_free_t140block(&blk_sent);
		memset(&blk_sent, 0, sizeof(MST140Block));
		blk_sent.utf8_string = (char *) osip_malloc(9 + 10 + 1);
		snprintf(blk_sent.utf8_string, 9 + 10 + 1, "FILESIZE %i",
				 data->file_size);
		blk_sent.size = 9 + 10 + 1;
		i = am_session_send_udpftp(data->did, &blk_sent);
	} else if (data->out_command_current == UDPFTP_FILESIZE) {
		blk_sent.utf8_string = (char *) osip_malloc(9 + 10 + 1);
		snprintf(blk_sent.utf8_string, 9 + 10 + 1, "FILESIZE %i",
				 data->file_size);
		blk_sent.size = 9 + 10 + 1;
		i = am_session_send_udpftp(data->did, &blk_sent);
	} else if (data->out_command_current == UDPFTP_DOWNLOADFILE) {
		blk_sent.utf8_string = (char *) osip_malloc(8 + 1);
		snprintf(blk_sent.utf8_string, 8 + 1, "DOWNLOAD");
		blk_sent.size = 8 + 1;
		i = am_session_send_udpftp(data->did, &blk_sent);
	} else {
		/* bad command? */
		am_free_t140block(&blk_sent);
		return i;
	}

	if (i < 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "udpftp: _udpftp_send_command (%s)\n",
							  osip_strerror(i)));
	}

	am_free_t140block(&blk_sent);
	return i;
}


static int
_udpftp_checkfor_reply(struct udp_connection *data,
					   MST140Block * blk_received)
{
	int i = AMSIP_UNDEFINED_ERROR;

	if (blk_received->size == 18
		&& osip_strncasecmp(blk_received->utf8_string,
							"GETVERSION_ANSWER ", 18) == 0) {
		if (data->out_command_current == UDPFTP_GETVERSION)
			data->out_command_acked = UDPFTP_GETVERSION;
		i = AMSIP_SUCCESS;
	} else if (blk_received->size == 13
			   && osip_strncasecmp(blk_received->utf8_string,
								   "ERROR_ANSWER", 13) == 0) {
		if (data->out_command_current == UDPFTP_ERROR)
			data->out_command_acked = UDPFTP_ERROR;
		i = AMSIP_SUCCESS;
	} else if (blk_received->size == 16
			   && osip_strncasecmp(blk_received->utf8_string,
								   "FILEINFO_ANSWER", 16) == 0) {
		if (data->out_command_current == UDPFTP_FILEINFO)
			data->out_command_acked = UDPFTP_FILEINFO;
		i = AMSIP_SUCCESS;
	} else if (blk_received->size == 16
			   && osip_strncasecmp(blk_received->utf8_string,
								   "FILESIZE_ANSWER", 16) == 0) {
		if (data->out_command_current == UDPFTP_FILESIZE)
			data->out_command_acked = UDPFTP_FILESIZE;
		i = AMSIP_SUCCESS;
	} else if (blk_received->size == 16
			   && osip_strncasecmp(blk_received->utf8_string,
								   "DOWNLOAD_ANSWER", 16) == 0) {
		if (data->out_command_current == UDPFTP_DOWNLOADFILE)
			data->out_command_acked = UDPFTP_DOWNLOADFILE;
		i = AMSIP_SUCCESS;
	}

	return i;
}

static int
_udpftp_checkfor_command(struct udp_connection *data,
						 MST140Block * blk_received)
{
	int i = AMSIP_SUCCESS;

	if (blk_received->size > 11
		&& osip_strncasecmp(blk_received->utf8_string, "GETVERSION ",
							11) == 0) {
		MST140Block blk_sent;

		memset(&blk_sent, 0, sizeof(MST140Block));
		blk_sent.utf8_string = (char *) osip_malloc(24 + 1);
		snprintf(blk_sent.utf8_string, 24 + 1, "GETVERSION_ANSWER v0.0.1");
		blk_sent.size = 24 + 1;
		i = am_session_send_udpftp(data->did, &blk_sent);
		if (i >= 0)
			i = UDPFTP_GETVERSION;
		am_free_t140block(&blk_sent);
	} else if (blk_received->size > 6
			   && osip_strncasecmp(blk_received->utf8_string, "ERROR ",
								   6) == 0) {
		MST140Block blk_sent;

		memset(&blk_sent, 0, sizeof(MST140Block));
		blk_sent.utf8_string = (char *) osip_malloc(12 + 1);
		snprintf(blk_sent.utf8_string, 12 + 1, "ERROR_ANSWER");
		blk_sent.size = 12 + 1;
		i = am_session_send_udpftp(data->did, &blk_sent);
		if (i >= 0)
			i = UDPFTP_ERROR;
		am_free_t140block(&blk_sent);
	} else if (blk_received->size > 9
			   && osip_strncasecmp(blk_received->utf8_string, "FILEINFO ",
								   9) == 0) {
		MST140Block blk_sent;

		memset(&blk_sent, 0, sizeof(MST140Block));
		blk_sent.utf8_string = (char *) osip_malloc(15 + 1);
		snprintf(blk_sent.utf8_string, 15 + 1, "FILEINFO_ANSWER");
		blk_sent.size = 15 + 1;
		i = am_session_send_udpftp(data->did, &blk_sent);
		if (i >= 0)
			i = UDPFTP_FILEINFO;
		am_free_t140block(&blk_sent);

		snprintf(data->filename_short, sizeof(data->filename_short), "%s",
				 blk_received->utf8_string + 9);

	} else if (blk_received->size > 9
			   && osip_strncasecmp(blk_received->utf8_string, "FILESIZE ",
								   9) == 0) {
		MST140Block blk_sent;

		memset(&blk_sent, 0, sizeof(MST140Block));
		blk_sent.utf8_string = (char *) osip_malloc(15 + 1);
		snprintf(blk_sent.utf8_string, 15 + 1, "FILESIZE_ANSWER");
		blk_sent.size = 15 + 1;
		i = am_session_send_udpftp(data->did, &blk_sent);
		if (i >= 0)
			i = UDPFTP_FILESIZE;
		am_free_t140block(&blk_sent);

		data->file_size = atoi(blk_received->utf8_string + 9);
	} else if (blk_received->size == 9
			   && osip_strncasecmp(blk_received->utf8_string, "DOWNLOAD",
								   9) == 0) {
		MST140Block blk_sent;

		memset(&blk_sent, 0, sizeof(MST140Block));
		blk_sent.utf8_string = (char *) osip_malloc(15 + 1);
		snprintf(blk_sent.utf8_string, 15 + 1, "DOWNLOAD_ANSWER");
		blk_sent.size = 15 + 1;
		i = am_session_send_udpftp(data->did, &blk_sent);
		if (i >= 0)
			i = UDPFTP_DOWNLOADFILE;
		am_free_t140block(&blk_sent);
	}

	return i;
}

static int
_udpftp_write_file(struct udp_connection *data, MST140Block * blk_received)
{
	int i;

	int ack_value = -1;

	if (blk_received->size >= sizeof(int))
		memcpy(&ack_value, blk_received->utf8_string, sizeof(int));

	if (osip_strncasecmp(blk_received->utf8_string, "EOF", 3) == 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "udpftp: _udpftp_write_file: receiving EOF\n"));
		//fix: handle packet loss at the end of data.
		if (data->byte_received==data->file_size)
		{
#ifndef WIN32
			close(data->fd);
			data->fd = -1;
#else
			CloseHandle(data->fd);
			data->fd = INVALID_HANDLE_VALUE;
#endif
		}
		else
		{
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
								  "udpftp: _udpftp_write_file: receiving EOF but file transfer is not complete yet\n"));
		}

		return AMSIP_SUCCESS;
	} else if (ack_value == data->received) {
		int err;

		DWORD byte_written = 0;

		MST140Block blk_sent;

		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "udpftp: _udpftp_write_file: get expected packet=%i\n",
							  ack_value));

#ifndef WIN32
		byte_written = (int)write(data->fd, blk_received->utf8_string + sizeof(int),
				  blk_received->size - sizeof(int));
		err = TRUE;
		if (byte_written <= 0)
			err = FALSE;
#else
		err =
			WriteFile(data->fd, blk_received->utf8_string + sizeof(int),
					  blk_received->size - sizeof(int), &byte_written,
					  NULL);
#endif
		if (err == FALSE
			|| byte_written != blk_received->size - sizeof(int)) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"udpftp: _udpftp_write_file: can't write to file\n"));
			return AMSIP_UNDEFINED_ERROR;
		}

		data->received++;
		data->byte_received =
			data->byte_received + blk_received->size - sizeof(int);

		/* send packet to acknowledge */
		memset(&blk_sent, 0, sizeof(blk_sent));
		blk_sent.utf8_string = (char *) osip_malloc(sizeof(int) + 1);
		memcpy(blk_sent.utf8_string, &ack_value, sizeof(int));
		blk_sent.size = sizeof(int) + 1;
		i = am_session_send_udpftp(data->did, &blk_sent);
		am_free_t140block(&blk_sent);
		if (i < 0) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
								  "udpftp: _udpftp_write_file: %s\n",
								  osip_strerror(i)));
			return AMSIP_UNDEFINED_ERROR;
		}
	} else if (ack_value >= 0 && ack_value < data->received) {
		MST140Block blk_sent;

		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "udpftp: _udpftp_write_file: get old packet=%i (waiting for %i)\n",
							  ack_value, data->received));

		/* send packet to acknowledge */
		memset(&blk_sent, 0, sizeof(blk_sent));
		blk_sent.utf8_string = (char *) osip_malloc(sizeof(int) + 1);
		memcpy(blk_sent.utf8_string, &ack_value, sizeof(int));
		blk_sent.size = sizeof(int) + 1;
		i = am_session_send_udpftp(data->did, &blk_sent);
		am_free_t140block(&blk_sent);
		if (i < 0) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
								  "udpftp: _udpftp_write_file: %s\n",
								  osip_strerror(i)));
			return AMSIP_UNDEFINED_ERROR;
		}
	} else {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "udpftp: _udpftp_write_file: get future packet=%i (waiting for %i)\n",
							  ack_value, data->received));
	}

	return AMSIP_SUCCESS;
}

static int
_udpftp_update_bottom_seq(struct udp_connection *data,
						  MST140Block * blk_received)
{
	int ack_value = -1;

	int i = AMSIP_SUCCESS;

	memcpy(&ack_value, blk_received->utf8_string, sizeof(int));
	if (ack_value < data->bottom_seq) {	/* already received */
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "udpftp: _udpftp_update_bottom_seq: already received %i\n",
							  ack_value));
	} else if (ack_value == data->bottom_seq) {	/* bottom packet received */
		int bytes = 1300;

		int res;

		int err;

		/* send a new one */
		MST140Block *tmp;

		MST140Block *blk_sent = osip_list_get(&data->sender_packets, 0);
		if (blk_sent==NULL)
		{
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
				"udpftp: _udpftp_update_bottom_seq: empty sent packet list (%i:%i:%i:%i:%i:%i)\n",
				data->file_size,
				data->bottom_seq,
				data->byte_received,
				data->byte_sent,
				data->received,
				data->sent));
			return -1;
		}

		osip_list_remove(&data->sender_packets, 0);
		data->bottom_seq++;

		data->restart = 0;		/* reset */

		/* set new bottom_seq in case the current packet was previously a lost packet */
		tmp = osip_list_get(&data->sender_packets, 0);
		if (tmp != NULL)
			memcpy(&data->bottom_seq, tmp->utf8_string, sizeof(int));

		memcpy(blk_sent->utf8_string, &data->sent, sizeof(int));
#ifndef WIN32
		err = (int)read(data->fd, blk_sent->utf8_string + sizeof(int), bytes);
		res = TRUE;
		if (err < 0)
			res = FALSE;
#else
		res =
			ReadFile(data->fd, blk_sent->utf8_string + sizeof(int), bytes,
					 &err, NULL);
#endif
		if (err > 0) {
			int i;

			blk_sent->size = err + sizeof(int);
			data->byte_sent = data->byte_sent + err;
			i = am_session_send_udpftp(data->did, blk_sent);
			if (i != 0) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
									  "udpftp: _udpftp_update_bottom_seq: am_session_send_udpftp (%s)\n",
									  osip_strerror(i)));
				return i;
			}
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
								  "udpftp: _udpftp_update_bottom_seq: data->sent=%i\n",
								  data->sent));
			data->sent++;

			osip_list_add(&data->sender_packets, blk_sent, -1);
		}

		if (res == TRUE && err < bytes) {
			/* stop reading */
			MST140Block blk_eof;

#ifndef WIN32
			close(data->fd);
			data->fd = -1;
#else
			CloseHandle(data->fd);
			data->fd = INVALID_HANDLE_VALUE;
#endif
			/* SEND 3 EOF to be sure */
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
								  "udpftp: _udpftp_update_bottom_seq: sending EOF\n"));
			memset(&blk_eof, 0, sizeof(blk_eof));
			blk_eof.utf8_string = (char *) osip_malloc(4);
			snprintf(blk_eof.utf8_string, 3, "%s", "EOF");
			blk_eof.size = 4;
			am_session_send_udpftp(data->did, &blk_eof);
			am_session_send_udpftp(data->did, &blk_eof);
			am_session_send_udpftp(data->did, &blk_eof);
			osip_free(blk_eof.utf8_string);
		}
	} else {					/* possible packet lost? */

		MST140Block *blk_sent = NULL;

		int count = 0;

		/* remove this packet from the list */
		while (!osip_list_eol(&data->sender_packets, count)) {
			int val;

			blk_sent = osip_list_get(&data->sender_packets, count);
			memcpy(&val, blk_sent->utf8_string, sizeof(int));
			if (ack_value == val) {	/* found it! */
				int bytes = 1300;

				int res;

				int err;

				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
									  "udpftp: _udpftp_update_bottom_seq: remove element %i\n",
									  ack_value));
				osip_list_remove(&data->sender_packets, count);


				memcpy(blk_sent->utf8_string, &data->sent, sizeof(int));
#ifndef WIN32
				err = (int)read(data->fd, blk_sent->utf8_string + sizeof(int), bytes);
				res = TRUE;
				if (err < 0)
					res = FALSE;
#else
				res =
					ReadFile(data->fd, blk_sent->utf8_string + sizeof(int),
							 bytes, &err, NULL);
#endif
				if (err > 0) {
					int i;

					blk_sent->size = err + sizeof(int);
					data->byte_sent = data->byte_sent + err;
					i = am_session_send_udpftp(data->did, blk_sent);
					if (i != 0) {
						OSIP_TRACE(osip_trace
								   (__FILE__, __LINE__, OSIP_ERROR, NULL,
									"udpftp: _udpftp_update_bottom_seq: am_session_send_udpftp (%s)\n",
									osip_strerror(i)));
						return i;
					}
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_INFO2, NULL,
								"udpftp: _udpftp_update_bottom_seq: data->sent=%i\n",
								data->sent));
					data->sent++;

					osip_list_add(&data->sender_packets, blk_sent, -1);
				}

				if (res == TRUE && err < bytes) {
					/* stop reading */
					MST140Block blk_eof;

#ifndef WIN32
					close(data->fd);
					data->fd = -1;
#else
					CloseHandle(data->fd);
					data->fd = INVALID_HANDLE_VALUE;
#endif
					/* SEND 3 EOF to be sure */
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_INFO2, NULL,
								"udpftp: _udpftp_update_bottom_seq: sending EOF\n"));
					memset(&blk_eof, 0, sizeof(blk_eof));
					blk_eof.utf8_string = (char *) osip_malloc(4);
					snprintf(blk_eof.utf8_string, 3, "%s", "EOF");
					blk_eof.size = 4;
					am_session_send_udpftp(data->did, &blk_eof);
					am_session_send_udpftp(data->did, &blk_eof);
					am_session_send_udpftp(data->did, &blk_eof);
					osip_free(blk_eof.utf8_string);
				}

				break;
			}
			count++;
		}

		/* restransmit */
		blk_sent = osip_list_get(&data->sender_packets, 0);
		i = am_session_send_udpftp(data->did, blk_sent);
		if (i != 0) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
								  "udpftp: _udpftp_update_bottom_seq: %s\n",
								  osip_strerror(i)));
			return AMSIP_UNDEFINED_ERROR;
		}
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "udpftp: _udpftp_update_bottom_seq: future packet %i\n",
							  ack_value));
	}
	return AMSIP_SUCCESS;
}

static int _udpftp_send_initial_data(struct udp_connection *data)
{
	int i;

	if (data->in_command_current != UDPFTP_DOWNLOADFILE)
		return AMSIP_UNDEFINED_ERROR;

	data->sent = 0; /* move from -1 to 0: transfer started */
	data->byte_sent = 0; /* move from -1 to 0: transfer started */

	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
						  "udpftp: _udpftp_send_initial_data: start sending data\n"));
	if (osip_list_size(&data->sender_packets) != 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "udpftp: _udpftp_send_initial_data: cleanup data?\n"));
		while (osip_list_size(&data->sender_packets) != 0) {
			MST140Block *blk_sent =
				osip_list_get(&data->sender_packets, 0);

			osip_list_remove(&data->sender_packets, 0);
			am_free_t140block(blk_sent);
			memset(blk_sent, 0, sizeof(MST140Block));
		}
	}

	for (i = 0; i < NUMBER_OF_PACKETS; i++) {
		/* prepare & send 10 initial blocks */
		int bytes = 1300;

		int res;

		int err;

		MST140Block *blk_sent = &data->blk_sent[i];

		memset(blk_sent, 0, sizeof(MST140Block));
		blk_sent->utf8_string = (char *) osip_malloc(bytes + sizeof(int));
		memcpy(blk_sent->utf8_string, &data->sent, sizeof(int));

#ifndef WIN32
		err = (int)read(data->fd, blk_sent->utf8_string + sizeof(int), bytes);
		res = TRUE;
		if (err < 0)
			res = FALSE;
#else
		res =
			ReadFile(data->fd, blk_sent->utf8_string + sizeof(int), bytes,
					 &err, NULL);
#endif

		if (err > 0) {
			int k;

			blk_sent->size = err + sizeof(int);
			data->byte_sent = data->byte_sent + err;
			k = am_session_send_udpftp(data->did, blk_sent);
			if (k != 0) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
									  "udpftp: _udpftp_send_initial_data: am_session_send_udpftp (%s)\n",
									  osip_strerror(i)));
				return k;
			}
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
								  "udpftp: _udpftp_send_initial_data: data->sent=%i\n",
								  data->sent));
			data->sent++;
			osip_list_add(&data->sender_packets, blk_sent, -1);
		}
		if (res == TRUE && err < bytes) {
			/* stop reading */
			MST140Block blk_eof;

#ifndef WIN32
			close(data->fd);
			data->fd = -1;
#else
			CloseHandle(data->fd);
			data->fd = INVALID_HANDLE_VALUE;
#endif
			/* SEND 3 EOF to be sure */
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
								  "udpftp: _udpftp_send_initial_data: sending EOF\n"));
			memset(&blk_eof, 0, sizeof(blk_eof));
			blk_eof.utf8_string = (char *) osip_malloc(4);
			snprintf(blk_eof.utf8_string, 3, "%s", "EOF");
			blk_eof.size = 4;
			am_session_send_udpftp(data->did, &blk_eof);
			am_session_send_udpftp(data->did, &blk_eof);
			am_session_send_udpftp(data->did, &blk_eof);
			osip_free(blk_eof.utf8_string);
		}
	}

	return i;
}

static int _udpftp_execute(struct udp_connection *data)
{
	MST140Block blk_received;

	int i;

	if (data->did > 0
		&& data->out_command_current > 0 && data->out_command_acked == 0) {
		i = _udpftp_send_command(data);
	}

	while (1) {

		memset(&blk_received, 0, sizeof(MST140Block));
		i = am_session_get_udpftp(data->did, &blk_received);
		if (i < 0) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
								  "udpftp: _udpftp_execute: am_session_get_udpftp (%s)\n",
								  osip_strerror(i)));
			return AMSIP_UNDEFINED_ERROR;
		}

		if (i == 0 && blk_received.size > 0) {
			int ack_value = -1;

			osip_gettimeofday(&data->timeout, NULL);
			add_gettimeofday(&data->timeout, 1);

			if (blk_received.size >= sizeof(int)) {
				memcpy(&ack_value, blk_received.utf8_string, sizeof(int));
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
									  "udpftp: _udpftp_execute: testing packet (%i)\n",
									  ack_value));
			}

			i = _udpftp_checkfor_reply(data, &blk_received);
			if (i == AMSIP_SUCCESS) {
				/* previous command ack-ed */
				am_free_t140block(&blk_received);
				return AMSIP_SUCCESS;
			}

			i = _udpftp_checkfor_command(data, &blk_received);
			if (i < 0) {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
									  "udpftp: _udpftp_execute: _udpftp_checkfor_command (%s)\n",
									  osip_strerror(i)));
				am_free_t140block(&blk_received);
				return AMSIP_UNDEFINED_ERROR;
			}

			if (i > 0) {
				/* new incoming UDPFTP_command */
				if (i == UDPFTP_DOWNLOADFILE
					&& data->out_command_current == UDPFTP_FILEINFO) {
					if (data->in_command_current != UDPFTP_DOWNLOADFILE) {
						data->in_command_current = UDPFTP_DOWNLOADFILE;
						_udpftp_send_initial_data(data);
					}

				}
				/* command was not usefull to us? */
				am_free_t140block(&blk_received);
				return AMSIP_SUCCESS;
			}

			if (i == 0) {
				/* not a command/reply -> data? */
				if (data->in_command_current == UDPFTP_DOWNLOADFILE) {
					/* we are the sender/we receive acknowledge packets */
					_udpftp_update_bottom_seq(data, &blk_received);
				} else if (data->out_command_current ==
						   UDPFTP_DOWNLOADFILE) {
					/* we are the receiver/we receive filedata packets */
					_udpftp_write_file(data, &blk_received);
				}
			}

			am_free_t140block(&blk_received);
		} else {
			if (data->out_command_current == UDPFTP_DOWNLOADFILE
				&& data->out_command_acked == UDPFTP_DOWNLOADFILE) {
				osip_gettimeofday(&data->timeout, NULL);
				add_gettimeofday(&data->timeout, 1);
			} else if (data->in_command_current == UDPFTP_DOWNLOADFILE) {
				osip_gettimeofday(&data->timeout, NULL);
				add_gettimeofday(&data->timeout, 1);
			}

			/* we are not receiving any acknowledge packet any more... */
			if (data->in_command_current == UDPFTP_DOWNLOADFILE) {
				data->restart++;
				/* restransmit all packets... */
				if (data->restart > 200) {
					MST140Block *blk_sent = NULL;

					int count = 0;

					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_ERROR, NULL,
								"udpftp: _udpftp_execute: restart sending all\n"));
					while (!osip_list_eol(&data->sender_packets, count)) {
						blk_sent =
							osip_list_get(&data->sender_packets, count);
						i = am_session_send_udpftp(data->did, blk_sent);
						if (i != 0) {
							OSIP_TRACE(osip_trace
									   (__FILE__, __LINE__, OSIP_ERROR,
										NULL,
										"udpftp: _udpftp_execute: am_session_send_udpftp (%s)\n",
										osip_strerror(i)));
							break;
						}
						count++;
					}

					data->restart = 0;
				}
			}
			break;
		}

	}

	return AMSIP_SUCCESS;
}

static void *udpftp_thread_func(void *arg)
{
	struct udp_connection *data;

	int i;

	int sleep_duration;

	while (run_thread == 1) {
		struct timeval now;

		osip_gettimeofday(&now, NULL);

		sleep_duration = 1000000;

		osip_mutex_lock(udp_connection_mutex);

		for (i = 0; i < MAX_UDPFTP_CONTEXT; i++) {
			data = &ops[i];

			if (data->did > 0 && osip_timercmp(&now, &data->timeout, >)) {
				/* at least one context needs to be executed! */
				sleep_duration = 0;
				break;
			} else if (data->did > 0)
				sleep_duration = 1000;
		}
		osip_mutex_unlock(udp_connection_mutex);

		if (sleep_duration > 0)
			osip_usleep(sleep_duration);


		osip_mutex_lock(udp_connection_mutex);
		for (i = 0; i < MAX_UDPFTP_CONTEXT; i++) {
			data = &ops[i];

			if (data->did > 0 && osip_timercmp(&now, &data->timeout, >)) {
				osip_gettimeofday(&data->timeout, NULL);
				add_gettimeofday(&data->timeout, 1000);

				_udpftp_execute(data);
				if (data->remove>0)
					data->remove++;
				if (data->remove>5)
				{
					//delete context
#ifndef WIN32
					if (data->fd > 0)
						close(data->fd);
					data->fd = -1;
#else
					if (data->fd != INVALID_HANDLE_VALUE)
						CloseHandle(data->fd);
					data->fd = INVALID_HANDLE_VALUE;
#endif

					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_ERROR, NULL,
							   "udpftp: old context remove (cid=%i:%i) size=%i down=%i sent=%i\n",
							   data->cid,
							   data->did,
							   data->file_size,
							   data->byte_received,
							   data->byte_sent));
					/* release all blocks */
					while (osip_list_size(&data->sender_packets) != 0) {
						MST140Block *blk_sent = osip_list_get(&data->sender_packets, 0);

						osip_list_remove(&data->sender_packets, 0);
						am_free_t140block(blk_sent);
						memset(blk_sent, 0, sizeof(MST140Block));
					}

					memset(data, 0, sizeof(struct udp_connection));
				}
			}
		}
		osip_mutex_unlock(udp_connection_mutex);
	}

	return NULL;
}

int _am_session_stop_transfer(int cid, int did)
{
	struct udp_connection *data = NULL;

	int i;

	if (udp_connection_mutex==NULL)
		return AMSIP_UNDEFINED_ERROR;
	
	for (i = 0; i < MAX_UDPFTP_CONTEXT; i++) {
		data = &ops[i];

		if (data->did == did) {
			break;
		}
		data = NULL;
	}

	if (data == NULL) {
		for (i = 0; i < MAX_UDPFTP_CONTEXT; i++) {
			data = &ops[i];

			if (data->cid == cid) {
				break;
			}
			data = NULL;
		}
	}

	if (data == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "udpftp: _am_session_stop_transfer. context not found!\n"));
		return AMSIP_NOTFOUND;
	}
	data->remove = 1;
	return 0;
}

PPL_DECLARE (int) am_session_stop_transfer(int cid, int did)
{
	struct udp_connection *data = NULL;

	int i;

	if (udp_connection_mutex==NULL)
		return AMSIP_UNDEFINED_ERROR;
	
	osip_mutex_lock(udp_connection_mutex);
	for (i = 0; i < MAX_UDPFTP_CONTEXT; i++) {
		data = &ops[i];

		if (data->did == did) {
			break;
		}
		data = NULL;
	}

	if (data == NULL) {
		for (i = 0; i < MAX_UDPFTP_CONTEXT; i++) {
			data = &ops[i];

			if (data->cid == cid) {
				break;
			}
			data = NULL;
		}
	}

	if (data == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "udpftp: am_session_stop_transfer. context not found!\n"));
		osip_mutex_unlock(udp_connection_mutex);
		return AMSIP_NOTFOUND;
	}
#ifndef WIN32
	if (data->fd > 0)
		close(data->fd);
	data->fd = -1;
#else
	if (data->fd != INVALID_HANDLE_VALUE)
		CloseHandle(data->fd);
	data->fd = INVALID_HANDLE_VALUE;
#endif

	/* release all blocks */
	while (osip_list_size(&data->sender_packets) != 0) {
		MST140Block *blk_sent = osip_list_get(&data->sender_packets, 0);

		osip_list_remove(&data->sender_packets, 0);
		am_free_t140block(blk_sent);
		memset(blk_sent, 0, sizeof(MST140Block));
	}

	memset(data, 0, sizeof(struct udp_connection));
	osip_mutex_unlock(udp_connection_mutex);
	return AMSIP_SUCCESS;
}

PPL_DECLARE (int)
am_session_send_file(int cid, int did, const char *filename,
					 const char *filename_short)
{
	struct udp_connection *data = NULL;

	int i;

	if (udp_connection_mutex==NULL)
		return AMSIP_UNDEFINED_ERROR;
	
	osip_mutex_lock(udp_connection_mutex);
	for (i = 0; i < MAX_UDPFTP_CONTEXT; i++) {
		data = &ops[i];

		if (data->did == did) {
			break;
		}
		data = NULL;
	}

	if (data == NULL) {
		for (i = 0; i < MAX_UDPFTP_CONTEXT; i++) {
			data = &ops[i];

			if (data->did <= 0) {
				break;
			}
			data = NULL;
		}
	}

	if (data == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "udpftp: am_session_send_file. maximum number of transfer reached\n"));
		osip_mutex_unlock(udp_connection_mutex);
		return AMSIP_SUCCESS;
	}
#ifndef WIN32
	data->fd = open(filename, O_RDONLY);
	if (data->fd == -1) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "udpftp: am_session_send_file (%s)\n",
							  osip_strerror(AMSIP_FILE_NOT_EXIST)));
		osip_mutex_unlock(udp_connection_mutex);
		return AMSIP_FILE_NOT_EXIST;
	}
	{
		struct stat buf;

		i = fstat(data->fd, &buf);
		if (i < 0) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
								  "udpftp: am_session_send_file can't get file size\n"));
			osip_mutex_unlock(udp_connection_mutex);
			return AMSIP_FILE_NOT_EXIST;
		}
		data->file_size = (int)buf.st_size;
	}
#else
	{
		WCHAR wUnicode[1024];

		MultiByteToWideChar(CP_UTF8, 0, filename, -1, wUnicode, 1024);
		data->fd =
			CreateFileW(wUnicode, GENERIC_READ, FILE_SHARE_READ, NULL,
						OPEN_EXISTING, 0, NULL);
	}
	if (data->fd == INVALID_HANDLE_VALUE) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "udpftp: am_session_send_file (%s)\n",
							  osip_strerror(AMSIP_FILE_NOT_EXIST)));
		osip_mutex_unlock(udp_connection_mutex);
		return AMSIP_FILE_NOT_EXIST;
	}
	data->file_size = GetFileSize(data->fd, NULL);
#endif



	data->out_command_current = UDPFTP_FILEINFO;
	data->out_command_acked = 0;
	snprintf(data->filename, sizeof(data->filename), "%s", filename);
	snprintf(data->filename_short, sizeof(data->filename_short), "%s",
			 filename_short);

	data->bottom_seq = 0;
	data->sent = -1;
	data->byte_sent = -1;
	data->received = -1;
	data->byte_received = -1;
	data->restart = 0;

	data->did = did;
	data->cid = cid;
	osip_mutex_unlock(udp_connection_mutex);

	return AMSIP_SUCCESS;
}

PPL_DECLARE (int)
am_session_receive_file(int cid, int did, const char *filename)
{
	struct udp_connection *data = NULL;

	int i;

	if (udp_connection_mutex==NULL)
		return AMSIP_UNDEFINED_ERROR;
	
	osip_mutex_lock(udp_connection_mutex);
	for (i = 0; i < MAX_UDPFTP_CONTEXT; i++) {
		data = &ops[i];

		if (data->cid == cid) {
			break;
		}
		data = NULL;
	}

	if (data == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "udpftp: am_session_receive_file. context not found!\n"));
		osip_mutex_unlock(udp_connection_mutex);
		return AMSIP_NOTFOUND;
	}

	snprintf(data->filename, sizeof(data->filename), "%s", filename);

#ifndef WIN32
	data->fd =
		open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (data->fd == -1) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "udpftp: am_session_receive_file (%s)\n",
							  osip_strerror(AMSIP_FILE_NOT_EXIST)));
		osip_mutex_unlock(udp_connection_mutex);
		return AMSIP_FILE_NOT_EXIST;
	}
#else
	{
		WCHAR wUnicode[1024];

		MultiByteToWideChar(CP_UTF8, 0, data->filename, -1, wUnicode,
							1024);
		data->fd =
			CreateFileW(wUnicode, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0,
						NULL);
	}

	if (data->fd == INVALID_HANDLE_VALUE) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "udpftp: am_session_receive_file (%s)\n",
							  osip_strerror(AMSIP_FILE_NOT_EXIST)));
		osip_mutex_unlock(udp_connection_mutex);
		return AMSIP_FILE_NOT_EXIST;
	}
#endif

	data->out_command_current = UDPFTP_DOWNLOADFILE;
	data->out_command_acked = 0;
	data->sent = -1;
	data->byte_sent = -1;
	data->received = 0;
	data->byte_received = 0;

	data->did = did;
	data->cid = cid;
	osip_mutex_unlock(udp_connection_mutex);

	return AMSIP_SUCCESS;
}

#if 0
PPL_DECLARE (int) am_session_file_release(int cid, int did)
{
	struct udp_connection *data = NULL;
	int i;

	if (udp_connection_mutex==NULL)
		return AMSIP_UNDEFINED_ERROR;
	
	osip_mutex_lock(udp_connection_mutex);
	for (i = 0; i < MAX_UDPFTP_CONTEXT; i++) {
		data = &ops[i];

		if (data->cid == cid) {
			break;
		}
		data = NULL;
	}

	if (data == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "udpftp: am_session_file_release. not enough context\n"));
		osip_mutex_unlock(udp_connection_mutex);
		return AMSIP_NOTFOUND;
	}

	if (data->cid > 0)
		am_session_stop(data->cid, data->did, 486);

#ifndef WIN32
	if (data->fd > 0)
		close(data->fd);
	data->fd = -1;
#else
	if (data->fd != INVALID_HANDLE_VALUE)
		CloseHandle(data->fd);
	data->fd = INVALID_HANDLE_VALUE;
#endif

	/* release all blocks */
	while (osip_list_size(&data->sender_packets) != 0) {
		MST140Block *blk_sent = osip_list_get(&data->sender_packets, 0);

		osip_list_remove(&data->sender_packets, 0);
		am_free_t140block(blk_sent);
		memset(blk_sent, 0, sizeof(MST140Block));
	}

	memset(data, 0, sizeof(struct udp_connection));

	osip_gettimeofday(&data->timeout, NULL);
	add_gettimeofday(&data->timeout, 1000);

	osip_mutex_unlock(udp_connection_mutex);
	return AMSIP_SUCCESS;
}
#endif

PPL_DECLARE (int)
am_session_file_info(int cid, int did, struct am_fileinfo *fileinfo)
{
	struct udp_connection *data = NULL;

	int i;

	if (udp_connection_mutex==NULL)
		return AMSIP_UNDEFINED_ERROR;
	
	memset(fileinfo, 0, sizeof(struct am_fileinfo));

	osip_mutex_lock(udp_connection_mutex);
	for (i = 0; i < MAX_UDPFTP_CONTEXT; i++) {
		data = &ops[i];

		if (data->cid == cid) {
			break;
		}
		data = NULL;
	}

	if (data == NULL) {
		for (i = 0; i < MAX_UDPFTP_CONTEXT; i++) {
			data = &ops[i];

			if (data->cid <= 0) {
				data->cid = cid;
				data->did = did;
				data->sent = -1;
				data->byte_sent = -1;
				data->received = -1;
				data->byte_received = -1;
				osip_mutex_unlock(udp_connection_mutex);
				return AMSIP_SUCCESS;
			}
			data = NULL;
		}
	}

	if (data == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "udpftp: am_session_file_info. not enough context\n"));
		osip_mutex_unlock(udp_connection_mutex);
		return AMSIP_NOTFOUND;
	}

	if (data->filename_short != '\0')
		snprintf(fileinfo->filename, sizeof(fileinfo->filename), "%s", data->filename_short);
	fileinfo->file_size = data->file_size;
	data->did = did;
	data->cid = cid;

	/* if fileinfo->bytes_sent==-1 && fileinfo->bytes_received = -1 -> transfer NOT started */
	fileinfo->bytes_sent = data->byte_sent;
	if (data->byte_sent >= 0) { /* means transfer at least started */
		if (osip_list_size(&data->sender_packets) > 0
			&& data->byte_sent == data->file_size)
			fileinfo->bytes_sent = data->byte_sent - 1;	/* wait for ALL acknowledge packets */
	}
	/* either -1, 0 or >0 */
	fileinfo->bytes_received = data->byte_received;

	osip_mutex_unlock(udp_connection_mutex);

	return AMSIP_SUCCESS;
}

int _am_udpftp_start_thread()
{
	struct udp_connection *data = NULL;

	int i;

	memset(&ops[0], 0, sizeof(ops));
	for (i = 0; i < MAX_UDPFTP_CONTEXT; i++) {
		data = &ops[i];
		osip_gettimeofday(&data->timeout, NULL);
		add_gettimeofday(&data->timeout, 1000);
	}
	
	if (udp_connection_mutex==NULL)
	{
		udp_connection_mutex = osip_mutex_init();
		if (udp_connection_mutex == NULL) {
			run_thread = 0;
			return AMSIP_UNDEFINED_ERROR;
		}		
	}

	run_thread = 1;
	if (udpftp_thread == NULL) {

		udpftp_thread =
			osip_thread_create(20000, udpftp_thread_func, data);
		if (udpftp_thread == NULL) {
			run_thread = 0;
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"amsip: Cannot start thread for udpftp transfer!\n"));
			return AMSIP_UNDEFINED_ERROR;
		}
	}
	return AMSIP_SUCCESS;
}

int _am_udpftp_stop_thread()
{
	struct udp_connection *data = NULL;

	int i;

	if (udp_connection_mutex==NULL)
		return AMSIP_SUCCESS;
	
	if (udpftp_thread != NULL) {
		run_thread = 0;
		osip_thread_join(udpftp_thread);
		osip_free(udpftp_thread);
		udpftp_thread = NULL;
	}

	for (i = 0; i < MAX_UDPFTP_CONTEXT; i++) {
		data = &ops[i];

		if (data->cid > 0) {
			am_session_stop(data->cid, data->did, 486);
		}
#ifndef WIN32
		if (data->fd > 0)
			close(data->fd);
		data->fd = -1;
#else
		if (data->fd != INVALID_HANDLE_VALUE)
			CloseHandle(data->fd);
		data->fd = INVALID_HANDLE_VALUE;
#endif

		/* release all blocks */
		while (osip_list_size(&data->sender_packets) != 0) {
			MST140Block *blk_sent =
				osip_list_get(&data->sender_packets, 0);

			osip_list_remove(&data->sender_packets, 0);
			am_free_t140block(blk_sent);
			memset(blk_sent, 0, sizeof(MST140Block));
		}

		memset(data, 0, sizeof(struct udp_connection));
		data = NULL;
	}

	return AMSIP_SUCCESS;
}
