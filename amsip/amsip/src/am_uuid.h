/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

/*
** Copyright (c) 1990- 1993, 1996 Open Software Foundation, Inc.
** Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, Ca. &
** Digital Equipment Corporation, Maynard, Mass.
** Copyright (c) 1998 Microsoft.
** To anyone who acknowledges that this file is provided "AS IS"
** without any express or implied warranty: permission to use, copy,
** modify, and distribute this file for any purpose is hereby
** granted without fee, provided that the above copyright notices and
** this notice appears in all source code copies, and that none of
** the names of Open Software Foundation, Inc., Hewlett-Packard
** Company, Microsoft, or Digital Equipment Corporation be used in
** advertising or publicity pertaining to distribution of the software
** without specific, written prior permission. Neither Open Software
** Foundation, Inc., Hewlett-Packard Company, Microsoft, nor Digital
** Equipment Corporation makes any representations about the
** suitability of this software for any purpose.
*/

#ifndef __AM_UUID_H__
#define __AM_UUID_H__

#ifndef NOUUID

#undef am_uuid_t
typedef struct {
	unsigned32 time_low;
	unsigned16 time_mid;
	unsigned16 time_hi_and_version;
	unsigned8 clock_seq_hi_and_reserved;
	unsigned8 clock_seq_low;
	byte node[6];
} am_uuid_t;

/* uuid_create -- generate a UUID */
int uuid_create(am_uuid_t * uuid);

/* uuid_create_md5_from_name -- create a version 3 (MD5) UUID using a
   "name" from a "name space" */
void uuid_create_md5_from_name(am_uuid_t * uuid,	/* resulting UUID */
							   am_uuid_t nsid,	/* UUID of the namespace */
							   void *name,	/* the name from which to generate a UUID */
							   int namelen	/* the length of the name */
	);

/* uuid_create_sha1_from_name -- create a version 5 (SHA-1) UUID
   using a "name" from a "name space" */
void uuid_create_sha1_from_name(am_uuid_t * uuid,	/* resulting UUID */
								am_uuid_t nsid,	/* UUID of the namespace */
								void *name,	/* the name from which to generate a UUID */
								int namelen	/* the length of the name */
	);

/* am_uuid_compare --  Compare two UUID's "lexically" and return
        -1   u1 is lexically before u2
         0   u1 is equal to u2
         1   u1 is lexically after u2
   Note that lexical ordering is not temporal ordering!
*/
int am_uuid_compare(am_uuid_t * u1, am_uuid_t * u2);

void am_puid(am_uuid_t urn);

#endif

#endif
