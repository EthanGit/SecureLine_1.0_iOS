/*
   ITU-T G.729 Speech Coder ANSI-C Source Code
   Version 3.3    Last modified: December 26, 1995

   Copyright (c) 1996,
   AT&T, France Telecom, NTT, Universite de Sherbrooke, Lucent Technologies
   All rights reserved.
*/

/* WARNING: Make sure that the proper flags are defined for your system */

/*
   Types definitions
*/

#if defined(__BORLANDC__) || defined (__WATCOMC__) || defined(_MSC_VER) || defined(__ZTC__) || defined(__HIGHC__)
typedef  long  int   Word32   ;
typedef  short int   Word16   ;
typedef  short int   Flag  ;
#elif defined( __sun)
typedef short  Word16;
typedef long  Word32;
typedef int   Flag;
#elif defined(__unix__) || defined(__unix)
typedef short Word16;
typedef int   Word32;
typedef int   Flag;
#elif defined(VMS) || defined(__VMS)
typedef short  Word16;
typedef long  Word32;
typedef int   Flag;
#elif defined(ANDROID)
typedef short Word16;
typedef int   Word32;
typedef int   Flag;
#elif defined(__APPLE__)
typedef short Word16;
typedef int   Word32;
typedef int   Flag;
#else
#error  COMPILER NOT TESTED typedef.h needs to be updated, see readme
#endif

