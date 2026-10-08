#ifndef CURLINC_SYSTEM_H
#define CURLINC_SYSTEM_H

#if defined(__DJGPP__) || defined(__GO32__)
#if defined(__DJGPP__) && (__DJGPP__ > 1)
#define CURL_TYPEOF_CURL_OFF_T long long
#define CURL_FORMAT_CURL_OFF_T "lld"
#define CURL_FORMAT_CURL_OFF_TU "llu"
#define CURL_SUFFIX_CURL_OFF_T LL
#define CURL_SUFFIX_CURL_OFF_TU ULL
#else
#define CURL_TYPEOF_CURL_OFF_T long
#define CURL_FORMAT_CURL_OFF_T "ld"
#define CURL_FORMAT_CURL_OFF_TU "lu"
#define CURL_SUFFIX_CURL_OFF_T L
#define CURL_SUFFIX_CURL_OFF_TU UL
#endif
#define CURL_TYPEOF_CURL_SOCKLEN_T int

#elif defined(__SALFORDC__)
#define CURL_TYPEOF_CURL_OFF_T long
#define CURL_FORMAT_CURL_OFF_T "ld"
#define CURL_FORMAT_CURL_OFF_TU "lu"
#define CURL_SUFFIX_CURL_OFF_T L
#define CURL_SUFFIX_CURL_OFF_TU UL
#define CURL_TYPEOF_CURL_SOCKLEN_T int

#elif defined(__BORLANDC__)
#if (__BORLANDC__ < 0x520)
#define CURL_TYPEOF_CURL_OFF_T long
#define CURL_FORMAT_CURL_OFF_T "ld"
#define CURL_FORMAT_CURL_OFF_TU "lu"
#define CURL_SUFFIX_CURL_OFF_T L
#define CURL_SUFFIX_CURL_OFF_TU UL
#else
#define CURL_TYPEOF_CURL_OFF_T __int64
#define CURL_FORMAT_CURL_OFF_T "I64d"
#define CURL_FORMAT_CURL_OFF_TU "I64u"
#define CURL_SUFFIX_CURL_OFF_T i64
#define CURL_SUFFIX_CURL_OFF_TU ui64
#endif
#define CURL_TYPEOF_CURL_SOCKLEN_T int

#elif defined(__TURBOC__)
#define CURL_TYPEOF_CURL_OFF_T long
#define CURL_FORMAT_CURL_OFF_T "ld"
#define CURL_FORMAT_CURL_OFF_TU "lu"
#define CURL_SUFFIX_CURL_OFF_T L
#define CURL_SUFFIX_CURL_OFF_TU UL
#define CURL_TYPEOF_CURL_SOCKLEN_T int

#elif defined(__POCC__)
#if (__POCC__ < 280)
#define CURL_TYPEOF_CURL_OFF_T long
#define CURL_FORMAT_CURL_OFF_T "ld"
#define CURL_FORMAT_CURL_OFF_TU "lu"
#define CURL_SUFFIX_CURL_OFF_T L
#define CURL_SUFFIX_CURL_OFF_TU UL
#elif defined(_MSC_VER)
#define CURL_TYPEOF_CURL_OFF_T __int64
#define CURL_FORMAT_CURL_OFF_T "I64d"
#define CURL_FORMAT_CURL_OFF_TU "I64u"
#define CURL_SUFFIX_CURL_OFF_T i64
#define CURL_SUFFIX_CURL_OFF_TU ui64
#else
#define CURL_TYPEOF_CURL_OFF_T long long
#define CURL_FORMAT_CURL_OFF_T "lld"
#define CURL_FORMAT_CURL_OFF_TU "llu"
#define CURL_SUFFIX_CURL_OFF_T LL
#define CURL_SUFFIX_CURL_OFF_TU ULL
#endif
#define CURL_TYPEOF_CURL_SOCKLEN_T int

#elif defined(__LCC__)
#if defined(__MCST__) 
#define CURL_TYPEOF_CURL_OFF_T long
#define CURL_FORMAT_CURL_OFF_T "ld"
#define CURL_FORMAT_CURL_OFF_TU "lu"
#define CURL_SUFFIX_CURL_OFF_T L
#define CURL_SUFFIX_CURL_OFF_TU UL
#define CURL_TYPEOF_CURL_SOCKLEN_T socklen_t
#define CURL_PULL_SYS_TYPES_H 1
#define CURL_PULL_SYS_SOCKET_H 1
#else 
#define CURL_TYPEOF_CURL_OFF_T long
#define CURL_FORMAT_CURL_OFF_T "ld"
#define CURL_FORMAT_CURL_OFF_TU "lu"
#define CURL_SUFFIX_CURL_OFF_T L
#define CURL_SUFFIX_CURL_OFF_TU UL
#define CURL_TYPEOF_CURL_SOCKLEN_T int
#endif

#elif defined(__SYMBIAN32__)
#if defined(__EABI__) 
#define CURL_TYPEOF_CURL_OFF_T long long
#define CURL_FORMAT_CURL_OFF_T "lld"
#define CURL_FORMAT_CURL_OFF_TU "llu"
#define CURL_SUFFIX_CURL_OFF_T LL
#define CURL_SUFFIX_CURL_OFF_TU ULL
#elif defined(__CW32__)
#pragma longlong on
#define CURL_TYPEOF_CURL_OFF_T long long
#define CURL_FORMAT_CURL_OFF_T "lld"
#define CURL_FORMAT_CURL_OFF_TU "llu"
#define CURL_SUFFIX_CURL_OFF_T LL
#define CURL_SUFFIX_CURL_OFF_TU ULL
#elif defined(__VC32__)
#define CURL_TYPEOF_CURL_OFF_T __int64
#define CURL_FORMAT_CURL_OFF_T "lld"
#define CURL_FORMAT_CURL_OFF_TU "llu"
#define CURL_SUFFIX_CURL_OFF_T LL
#define CURL_SUFFIX_CURL_OFF_TU ULL
#endif
#define CURL_TYPEOF_CURL_SOCKLEN_T unsigned int

#elif defined(__MWERKS__)
#define CURL_TYPEOF_CURL_OFF_T long long
#define CURL_FORMAT_CURL_OFF_T "lld"
#define CURL_FORMAT_CURL_OFF_TU "llu"
#define CURL_SUFFIX_CURL_OFF_T LL
#define CURL_SUFFIX_CURL_OFF_TU ULL
#define CURL_TYPEOF_CURL_SOCKLEN_T int

#elif defined(_WIN32_WCE)
#define CURL_TYPEOF_CURL_OFF_T __int64
#define CURL_FORMAT_CURL_OFF_T "I64d"
#define CURL_FORMAT_CURL_OFF_TU "I64u"
#define CURL_SUFFIX_CURL_OFF_T i64
#define CURL_SUFFIX_CURL_OFF_TU ui64
#define CURL_TYPEOF_CURL_SOCKLEN_T int

#elif defined(__MINGW32__)
#define CURL_TYPEOF_CURL_OFF_T long long
#define CURL_FORMAT_CURL_OFF_T "I64d"
#define CURL_FORMAT_CURL_OFF_TU "I64u"
#define CURL_SUFFIX_CURL_OFF_T LL
#define CURL_SUFFIX_CURL_OFF_TU ULL
#define CURL_TYPEOF_CURL_SOCKLEN_T socklen_t
#define CURL_PULL_SYS_TYPES_H 1
#define CURL_PULL_WS2TCPIP_H 1

#elif defined(__VMS)
#if defined(__VAX)
#define CURL_TYPEOF_CURL_OFF_T long
#define CURL_FORMAT_CURL_OFF_T "ld"
#define CURL_FORMAT_CURL_OFF_TU "lu"
#define CURL_SUFFIX_CURL_OFF_T L
#define CURL_SUFFIX_CURL_OFF_TU UL
#else
#define CURL_TYPEOF_CURL_OFF_T long long
#define CURL_FORMAT_CURL_OFF_T "lld"
#define CURL_FORMAT_CURL_OFF_TU "llu"
#define CURL_SUFFIX_CURL_OFF_T LL
#define CURL_SUFFIX_CURL_OFF_TU ULL
#endif
#define CURL_TYPEOF_CURL_SOCKLEN_T unsigned int

#elif defined(__OS400__)
#if defined(__ILEC400__)
#define CURL_TYPEOF_CURL_OFF_T long long
#define CURL_FORMAT_CURL_OFF_T "lld"
#define CURL_FORMAT_CURL_OFF_TU "llu"
#define CURL_SUFFIX_CURL_OFF_T LL
#define CURL_SUFFIX_CURL_OFF_TU ULL
#define CURL_TYPEOF_CURL_SOCKLEN_T socklen_t
#define CURL_PULL_SYS_TYPES_H 1
#define CURL_PULL_SYS_SOCKET_H 1
#endif

#elif defined(__MVS__)
#if defined(__IBMC__) || defined(__IBMCPP__)
#if defined(_ILP32)
#elif defined(_LP64)
#endif
#if defined(_LONG_LONG)
#define CURL_TYPEOF_CURL_OFF_T long long
#define CURL_FORMAT_CURL_OFF_T "lld"
#define CURL_FORMAT_CURL_OFF_TU "llu"
#define CURL_SUFFIX_CURL_OFF_T LL
#define CURL_SUFFIX_CURL_OFF_TU ULL
#elif defined(_LP64)
#define CURL_TYPEOF_CURL_OFF_T long
#define CURL_FORMAT_CURL_OFF_T "ld"
#define CURL_FORMAT_CURL_OFF_TU "lu"
#define CURL_SUFFIX_CURL_OFF_T L
#define CURL_SUFFIX_CURL_OFF_TU UL
#else
#define CURL_TYPEOF_CURL_OFF_T long
#define CURL_FORMAT_CURL_OFF_T "ld"
#define CURL_FORMAT_CURL_OFF_TU "lu"
#define CURL_SUFFIX_CURL_OFF_T L
#define CURL_SUFFIX_CURL_OFF_TU UL
#endif
#define CURL_TYPEOF_CURL_SOCKLEN_T socklen_t
#define CURL_PULL_SYS_TYPES_H 1
#define CURL_PULL_SYS_SOCKET_H 1
#endif

#elif defined(__370__)
#if defined(__IBMC__) || defined(__IBMCPP__)
#if defined(_ILP32)
#elif defined(_LP64)
#endif
#if defined(_LONG_LONG)
#define CURL_TYPEOF_CURL_OFF_T long long
#define CURL_FORMAT_CURL_OFF_T "lld"
#define CURL_FORMAT_CURL_OFF_TU "llu"
#define CURL_SUFFIX_CURL_OFF_T LL
#define CURL_SUFFIX_CURL_OFF_TU ULL
#elif defined(_LP64)
#define CURL_TYPEOF_CURL_OFF_T long
#define CURL_FORMAT_CURL_OFF_T "ld"
#define CURL_FORMAT_CURL_OFF_TU "lu"
#define CURL_SUFFIX_CURL_OFF_T L
#define CURL_SUFFIX_CURL_OFF_TU UL
#else
#define CURL_TYPEOF_CURL_OFF_T long
#define CURL_FORMAT_CURL_OFF_T "ld"
#define CURL_FORMAT_CURL_OFF_TU "lu"
#define CURL_SUFFIX_CURL_OFF_T L
#define CURL_SUFFIX_CURL_OFF_TU UL
#endif
#define CURL_TYPEOF_CURL_SOCKLEN_T socklen_t
#define CURL_PULL_SYS_TYPES_H 1
#define CURL_PULL_SYS_SOCKET_H 1
#endif

#elif defined(TPF)
#define CURL_TYPEOF_CURL_OFF_T long
#define CURL_FORMAT_CURL_OFF_T "ld"
#define CURL_FORMAT_CURL_OFF_TU "lu"
#define CURL_SUFFIX_CURL_OFF_T L
#define CURL_SUFFIX_CURL_OFF_TU UL
#define CURL_TYPEOF_CURL_SOCKLEN_T int

#elif defined(__TINYC__) 
#define CURL_TYPEOF_CURL_OFF_T long long
#define CURL_FORMAT_CURL_OFF_T "lld"
#define CURL_FORMAT_CURL_OFF_TU "llu"
#define CURL_SUFFIX_CURL_OFF_T LL
#define CURL_SUFFIX_CURL_OFF_TU ULL
#define CURL_TYPEOF_CURL_SOCKLEN_T socklen_t
#define CURL_PULL_SYS_TYPES_H 1
#define CURL_PULL_SYS_SOCKET_H 1

#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC) 
#if !defined(__LP64) && (defined(__ILP32) || defined(__i386) || defined(__sparcv8) || defined(__sparcv8plus))
#define CURL_TYPEOF_CURL_OFF_T long long
#define CURL_FORMAT_CURL_OFF_T "lld"
#define CURL_FORMAT_CURL_OFF_TU "llu"
#define CURL_SUFFIX_CURL_OFF_T LL
#define CURL_SUFFIX_CURL_OFF_TU ULL
#elif defined(__LP64) || defined(__amd64) || defined(__sparcv9)
#define CURL_TYPEOF_CURL_OFF_T long
#define CURL_FORMAT_CURL_OFF_T "ld"
#define CURL_FORMAT_CURL_OFF_TU "lu"
#define CURL_SUFFIX_CURL_OFF_T L
#define CURL_SUFFIX_CURL_OFF_TU UL
#endif
#define CURL_TYPEOF_CURL_SOCKLEN_T socklen_t
#define CURL_PULL_SYS_TYPES_H 1
#define CURL_PULL_SYS_SOCKET_H 1

#elif defined(__xlc__) 
#if !defined(_LP64)
#define CURL_TYPEOF_CURL_OFF_T long long
#define CURL_FORMAT_CURL_OFF_T "lld"
#define CURL_FORMAT_CURL_OFF_TU "llu"
#define CURL_SUFFIX_CURL_OFF_T LL
#define CURL_SUFFIX_CURL_OFF_TU ULL
#else
#define CURL_TYPEOF_CURL_OFF_T long
#define CURL_FORMAT_CURL_OFF_T "ld"
#define CURL_FORMAT_CURL_OFF_TU "lu"
#define CURL_SUFFIX_CURL_OFF_T L
#define CURL_SUFFIX_CURL_OFF_TU UL
#endif
#define CURL_TYPEOF_CURL_SOCKLEN_T socklen_t
#define CURL_PULL_SYS_TYPES_H 1
#define CURL_PULL_SYS_SOCKET_H 1

#elif defined(_MSC_VER)
#if (_MSC_VER >= 900) && (_INTEGRAL_MAX_BITS >= 64)
#define CURL_TYPEOF_CURL_OFF_T __int64
#define CURL_FORMAT_CURL_OFF_T "I64d"
#define CURL_FORMAT_CURL_OFF_TU "I64u"
#define CURL_SUFFIX_CURL_OFF_T i64
#define CURL_SUFFIX_CURL_OFF_TU ui64
#else
#define CURL_TYPEOF_CURL_OFF_T long
#define CURL_FORMAT_CURL_OFF_T "ld"
#define CURL_FORMAT_CURL_OFF_TU "lu"
#define CURL_SUFFIX_CURL_OFF_T L
#define CURL_SUFFIX_CURL_OFF_TU UL
#endif
#define CURL_TYPEOF_CURL_SOCKLEN_T int

#elif defined(__GNUC__) && !defined(_SCO_DS)
#if !defined(__LP64__) && (defined(__ILP32__) || defined(__i386__) || defined(__hppa__) || defined(__ppc__) || defined(__powerpc__) || defined(__arm__) || defined(__sparc__) || defined(__mips__) ||                 \
                           defined(__sh__) || defined(__XTENSA__) || (defined(__SIZEOF_LONG__) && __SIZEOF_LONG__ == 4) || (defined(__LONG_MAX__) && __LONG_MAX__ == 2147483647L))
#define CURL_TYPEOF_CURL_OFF_T long long
#define CURL_FORMAT_CURL_OFF_T "lld"
#define CURL_FORMAT_CURL_OFF_TU "llu"
#define CURL_SUFFIX_CURL_OFF_T LL
#define CURL_SUFFIX_CURL_OFF_TU ULL
#elif defined(__LP64__) || defined(__x86_64__) || defined(__ppc64__) || defined(__sparc64__) || defined(__e2k__) || (defined(__SIZEOF_LONG__) && __SIZEOF_LONG__ == 8) ||                                             \
    (defined(__LONG_MAX__) && __LONG_MAX__ == 9223372036854775807L)
#define CURL_TYPEOF_CURL_OFF_T long
#define CURL_FORMAT_CURL_OFF_T "ld"
#define CURL_FORMAT_CURL_OFF_TU "lu"
#define CURL_SUFFIX_CURL_OFF_T L
#define CURL_SUFFIX_CURL_OFF_TU UL
#endif
#define CURL_TYPEOF_CURL_SOCKLEN_T socklen_t
#define CURL_PULL_SYS_TYPES_H 1
#define CURL_PULL_SYS_SOCKET_H 1

#else

#define CURL_TYPEOF_CURL_OFF_T long
#define CURL_FORMAT_CURL_OFF_T "ld"
#define CURL_FORMAT_CURL_OFF_TU "lu"
#define CURL_SUFFIX_CURL_OFF_T L
#define CURL_SUFFIX_CURL_OFF_TU UL
#define CURL_TYPEOF_CURL_SOCKLEN_T int
#endif

#ifdef _AIX

#define CURL_PULL_SYS_POLL_H
#endif

#ifdef CURL_PULL_WS2TCPIP_H
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#endif

#ifdef CURL_PULL_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef CURL_PULL_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef CURL_PULL_SYS_POLL_H
#include <sys/poll.h>
#endif

#ifdef CURL_TYPEOF_CURL_SOCKLEN_T
typedef CURL_TYPEOF_CURL_SOCKLEN_T curl_socklen_t;
#endif

#ifdef CURL_TYPEOF_CURL_OFF_T
typedef CURL_TYPEOF_CURL_OFF_T curl_off_t;
#endif

#if defined(__STDC__) || defined(_MSC_VER) || defined(__cplusplus) || defined(__HP_aCC) || defined(__BORLANDC__) || defined(__LCC__) || defined(__POCC__) || defined(__SALFORDC__) || defined(__HIGHC__) ||           \
    defined(__ILEC400__)

#define CURL_ISOCPP
#else

#undef CURL_ISOCPP
#endif

#if defined(__BORLANDC__) && (__BORLANDC__ == 0x0551)
#define CURLINC_OFF_T_C_HLPR2(x) x
#define CURLINC_OFF_T_C_HLPR1(x) CURLINC_OFF_T_C_HLPR2(x)
#define CURL_OFF_T_C(Val) CURLINC_OFF_T_C_HLPR1(Val)##CURLINC_OFF_T_C_HLPR1(CURL_SUFFIX_CURL_OFF_T)
#define CURL_OFF_TU_C(Val) CURLINC_OFF_T_C_HLPR1(Val)##CURLINC_OFF_T_C_HLPR1(CURL_SUFFIX_CURL_OFF_TU)
#else
#ifdef CURL_ISOCPP
#define CURLINC_OFF_T_C_HLPR2(Val, Suffix) Val##Suffix
#else
#define CURLINC_OFF_T_C_HLPR2(Val, Suffix) Val  Suffix
#endif
#define CURLINC_OFF_T_C_HLPR1(Val, Suffix) CURLINC_OFF_T_C_HLPR2(Val, Suffix)
#define CURL_OFF_T_C(Val) CURLINC_OFF_T_C_HLPR1(Val, CURL_SUFFIX_CURL_OFF_T)
#define CURL_OFF_TU_C(Val) CURLINC_OFF_T_C_HLPR1(Val, CURL_SUFFIX_CURL_OFF_TU)
#endif

#endif 
