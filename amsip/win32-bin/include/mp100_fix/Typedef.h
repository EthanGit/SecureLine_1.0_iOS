#ifndef __typedefH__
#define __typedefH__

#include <windows.h>

typedef BYTE		 	UINT8;
typedef WORD	 	UINT16;
//typedef DWORD		UINT32;
typedef ULONGLONG	UINT64;

typedef short int		INT16;
//typedef long			INT32;
typedef __int64		INT64;

typedef UINT8*		PUINT8;
typedef UINT16*		PUINT16;
typedef UINT32*		PUINT32;
typedef UINT64*		PUINT64;

typedef INT16*		PINT16;
typedef INT32*		PINT32;
typedef INT64*		PINT64;

typedef void**		PPVOID;
// PVOID, PCHAR, BOOL was already defined in Windows.h
// in LINUX, it should be defined
#define STDIFACE  UINT32 WINAPI

// Define callback functions pParam1 : ptr to object, pParam2 : callback param
typedef UINT32 (*PCallbackFunc)(PVOID pParam1, PVOID pParam2);

#if !defined (MIN)
	#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#if !defined (MAX)
	#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

enum InfoSelector2    // 2nd version of information selector with categories
{
	AUDIO_INFO = 0xa0000300,  // audio info
};

enum AudioInfoSelector
{
	AUDIOINFO_VOLUME = AUDIO_INFO,   // audio volume
	AUDIOINFO_BALANCE,               // balance
	AUDIOINFO_CHANNEL,               // 2CH analog/ 5.1CH analgo/ SPDIF
	AUDIOINFO_TYPE_DATARATE,         // 192KHz/224KHz/384KHz/ AC3/ AAC/ MPEG1/ MPEG2
	AUDIOINFO_SAMPLINGRATE,          // 32/ 44/ 48KHz
	AUDIOINFO_SPDIF_EXIST,           // SPDIF exists or not

	AUDIOINFO_MIXER_SOURCE,          // analog audio mixer source (CD/ LINE-IN/ AUX)
	AUDIOINFO_MIXER_ATTENUATION,     // analog audio mixer attenuation value
	AUDIOINFO_MIXER_MUTE,            // analog audio mixer mute or not

	AUDIOINFO_MAXVOLUMELEVEL
};
#endif
