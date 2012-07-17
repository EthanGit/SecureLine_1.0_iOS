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

