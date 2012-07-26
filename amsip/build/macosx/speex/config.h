#define SIZEOF_INT 4
#define SIZEOF_SHORT 2

#if defined(__i386__)
/* 32bit */
#define SIZEOF_LONG 4
#define SIZEOF_UNSIGNED_LONG 4
#define SIZEOF_UNSIGNED_LONG_LONG 8
#define HAVE_X86 1
#else
/* 64b */
#define SIZEOF_LONG 8
#define SIZEOF_UNSIGNED_LONG 8
#define SIZEOF_UNSIGNED_LONG_LONG 8
#define HAVE_X86 1
#endif

/*gsm*/
#define	HAS_STDLIB_H	1		/* /usr/include/stdlib.h	*/
#define	HAS_LIMITS_H	1		/* /usr/include/limits.h	*/
#define	HAS_FCNTL_H	1		/* /usr/include/fcntl.h		*/
#define	HAS_ERRNO_DECL	1		/* errno.h declares errno	*/

#define	HAS_FSTAT 	1		/* fstat syscall		*/
#define	HAS_FCHMOD 	1		/* fchmod syscall		*/
#define	HAS_CHMOD 	1		/* chmod syscall		*/
#define	HAS_FCHOWN 	1		/* fchown syscall		*/
#define	HAS_CHOWN 	1		/* chown syscall		*/
//define	HAS__FSETMODE 	1		/* _fsetmode -- set file mode	*/

#define	HAS_STRING_H 	1		/* /usr/include/string.h 	*/
//define	HAS_STRINGS_H	1		/* /usr/include/strings.h 	*/

#define	HAS_UNISTD_H	1		/* /usr/include/unistd.h	*/
#define	HAS_UTIME	1		/* POSIX utime(path, times)	*/
//define	HAS_UTIMES	1		/* use utimes()	syscall instead	*/
#define	HAS_UTIME_H	1		/* UTIME header file		*/
#define	HAS_UTIMBUF	1		/* struct utimbuf		*/
//define	HAS_UTIMEUSEC   1		/* microseconds in utimbuf?	*/

/*speex*/
#define FLOATING_POINT
#define EXPORT __attribute__((visibility("default")))
#define HAVE_ALLOCA_H 1
#define SPEEX_EXTRA_VERSION ""
#define SPEEX_MAJOR_VERSION 1
#define SPEEX_MICRO_VERSION 16
#define SPEEX_MINOR_VERSION 1
#define SPEEX_VERSION "1.2rc1"
#define USE_SMALLFT 
#define VAR_ARRAYS 
#define _USE_SSE 
#define restrict __restrict

#define asm __asm


/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.in by autoheader.  */

/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Define to 1 if you have the <arpa/nameser.h> header file. */
#define HAVE_ARPA_NAMESER_H 1

/* Define to 1 if you have the <assert.h> header file. */
#define HAVE_ASSERT_H 1

/* Define to 1 if you have the <ctype.h> header file. */
#define HAVE_CTYPE_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `getifaddrs' function. */
#define HAVE_GETIFADDRS 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <malloc.h> header file. */
/* #undef HAVE_MALLOC_H */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <nameser8_compat.h> header file. */
//#define HAVE_NAMESER8_COMPAT_H 1

/* Define to 1 if you have the <arpa/nameser_compat.h> header file. */
#define HAVE_ARPA_NAMESER_COMPAT_H 1

/* Define to 1 if you have the <netdb.h> header file. */
#define HAVE_NETDB_H 1

/* Define to 1 if you have the <openssl/ssl.h> header file. */
#define HAVE_OPENSSL_SSL_H 1

/* Define if you have POSIX threads libraries and header files. */
#define HAVE_PTHREAD 1

/* Define to 1 if you have the <resolv.h> header file. */
#define HAVE_RESOLV_H 1

/* Define to 1 if you have the <semaphore.h> header file. */
#define HAVE_SEMAPHORE_H 1

/* Define to 1 if you have the <signal.h> header file. */
#define HAVE_SIGNAL_H 1

/* Define to 1 if you have the <stdarg.h> header file. */
#define HAVE_STDARG_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/select.h> header file. */
#define HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/sem.h> header file. */
#define HAVE_SYS_SEM_H 1

/* Define to 1 if you have the <sys/signal.h> header file. */
#define HAVE_SYS_SIGNAL_H 1

/* Define to 1 if you have the <sys/soundcard.h> header file. */
/* #undef HAVE_SYS_SOUNDCARD_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <varargs.h> header file. */
/* #undef HAVE_VARARGS_H */

/* Name of package */
#define PACKAGE "libeXosip2"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME ""

/* Define to the full name and version of this package. */
#define PACKAGE_STRING ""

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME ""

/* Define to the version of this package. */
#define PACKAGE_VERSION ""

/* Define to the necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "3.4.0"


//osip define
#define HAVE_STRUCT_TIMEVAL 1
#define HAVE_SYSLOG_H 1
#define HAVE_TIME_H 1
#define HAVE_LRAND48 1

//amsip define
#define HAVE_SYS_UN_H 1
#define HAVE_SYS_SOCKET_H 1
#define HAVE_STDDEF_H 1
#define HAVE_SRTP_SRTP_H 1
#define HAVE_NETINET_IN_H 1
#define HAVE_MACH_O_DYLD_H 1
#define HAVE_ERRNO_H 1

//srtp define
#define CPU_CISC 1
#define DEV_URANDOM "/dev/urandom"
#define ENABLE_DEBUGGING 1
#define ERR_REPORTING_STDOUT 1
#define HAVE_ARPA_INET_H 1
#define HAVE_INET_ATON 1
#define HAVE_INT16_T 1
#define HAVE_INT32_T 1
#define HAVE_INT8_T 1
#define HAVE_INTTYPES_H 1
#define HAVE_MACHINE_TYPES_H 1
#define HAVE_MEMORY_H 1
#define HAVE_NETINET_IN_H 1
#define HAVE_SOCKET 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_SYSLOG_H 1
#define HAVE_SYS_SOCKET_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_UIO_H 1
#define HAVE_UINT16_T 1
#define HAVE_UINT32_T 1
#define HAVE_UINT64_T 1
#define HAVE_UINT8_T 1
#define HAVE_UNISTD_H 1
#define HAVE_USLEEP 1
#define STDC_HEADERS 1

