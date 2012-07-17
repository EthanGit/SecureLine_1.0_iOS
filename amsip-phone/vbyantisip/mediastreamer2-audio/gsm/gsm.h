/*
 * Copyright 1992 by Jutta Degener and Carsten Bormann, Technische
 * Universitaet Berlin.  See the accompanying file "COPYRIGHT" for
 * details.  THERE IS ABSOLUTELY NO WARRANTY FOR THIS SOFTWARE.
 */

/*$Header: /antisip/cvs/amsip/codecs/gsm/inc/gsm.h,v 1.2 2007-10-12 13:48:15 jack Exp $*/

#ifndef	GSM_H
#define	GSM_H

#ifdef __cplusplus
#	define	NeedFunctionPrototypes	1
#endif

#if __STDC__
#	define	NeedFunctionPrototypes	1
#endif

#ifdef _NO_PROTO
#	undef	NeedFunctionPrototypes
#endif

#ifdef NeedFunctionPrototypes
#   include	<stdio.h>		/* for FILE * 	*/
#endif

#undef GSM_P
#ifdef NeedFunctionPrototypes
#	define	GSM_P( protos )	protos
#else
#	define  GSM_P( protos )	( /* protos */ )
#endif

/*
 *	Interface
 */

typedef struct gsm_state * 	gsm;
typedef short		   	gsm_signal;		/* signed 16 bit */
typedef unsigned char		gsm_byte;
typedef gsm_byte 		gsm_frame[33];		/* 33 * 8 bits	 */

#define	GSM_MAGIC		0xD		  	/* 13 kbit/s RPE-LTP */

#define	GSM_PATCHLEVEL		10
#define	GSM_MINOR		0
#define	GSM_MAJOR		1

#define	GSM_OPT_VERBOSE		1
#define	GSM_OPT_FAST		2
#define	GSM_OPT_LTP_CUT		3
#define	GSM_OPT_WAV49		4
#define	GSM_OPT_FRAME_INDEX	5
#define	GSM_OPT_FRAME_CHAIN	6

#ifdef __cplusplus
extern "C"
{
#endif

gsm  gsm_create 	GSM_P((void));
void gsm_destroy GSM_P((gsm));	

int  gsm_print   GSM_P((FILE *, gsm, gsm_byte  *));
int  gsm_option  GSM_P((gsm, int, int *));

void gsm_encode  GSM_P((gsm, gsm_signal *, gsm_byte  *));
int  gsm_decode  GSM_P((gsm, gsm_byte   *, gsm_signal *));

int  gsm_explode GSM_P((gsm, gsm_byte   *, gsm_signal *));
void gsm_implode GSM_P((gsm, gsm_signal *, gsm_byte   *));

#ifdef __cplusplus
}
#endif

#undef	GSM_P

#endif	/* GSM_H */
