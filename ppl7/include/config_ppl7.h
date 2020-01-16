/* ppl7/include/config_ppl7.h.  Generated from config_ppl7.h.in by configure.  */
/*******************************************************************************
 * This file is part of "Patrick's Programming Library", Version 7 (PPL7).
 * Web: http://www.pfp.de/ppl/
 *
 * $Author$
 * $Revision$
 * $Date$
 * $Id$
 *
 *******************************************************************************
 * Copyright (c) 2013, Patrick Fedick <patrick@pfp.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright notice, this
 *       list of conditions and the following disclaimer. 
 *    2. Redistributions in binary form must reproduce the above copyright notice,
 *       this list of conditions and the following disclaimer in the documentation
 *       and/or other materials provided with the distribution. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER AND CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************/
 
#ifndef _PPL7_CONFIG
#define _PPL7_CONFIG

#define PACKAGE_VERSION "1.0.0"
#define PACKAGE_BUGREPORT "fedick@denic.de"
#define PACKAGE_NAME "udppingpong"
#define PACKAGE_STRING "udppingpong 1.0.0"

/* #undef MINGW32 */
/* #undef MINGW64 */
#define HAVE_AMD64 1

/* Define as 1 if you have unistd.h. */
#define HAVE_UNISTD_H 1

/* Define, if we have Thread Local Space */
#define HAVE_TLS 1

/* #undef HAVE_MKTIME */
/* #undef size_t */
/* #undef wchar_t */
/* #undef int8_t */
/* #undef int16_t */
/* #undef int32_t */
/* #undef int64_t */
/* #undef uint8_t */
/* #undef uint16_t */
/* #undef uint32_t */
/* #undef uint64_t */
/* #undef uintptr_t */


#define HAVE_ALARM 1
#define HAVE_BZERO 1
#define HAVE_FSEEKO 1
#define HAVE_GETHOSTBYNAME 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_INET_NTOA 1
#define HAVE_INTTYPES_H 1
#define HAVE_MALLOC 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMORY_H 1
#define HAVE_MEMSET 1
#define HAVE_MKDIR 1
#define HAVE_REALLOC 1
#define HAVE_SOCKET 1
/* #undef HAVE_SOCKADDR_SA_LEN */
#define HAVE_STDARG_H 1
#define HAVE_STDDEF_H 1
#define HAVE_STDBOOL_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_LOCALE_H 1
#define HAVE_WCHAR_H 1
/* #undef HAVE_WIDEC_H */
#define HAVE_WCTYPE_H 1
#define HAVE_STRDUP 1
#define HAVE_STRNDUP 1
#define HAVE_STRNCPY 1
/* #undef HAVE_STRLCPY */
#define HAVE_STRNCAT 1
/* #undef HAVE_STRLCAT */
#define HAVE_MATH_H 1
#define HAVE_SYSLOG_H 1

#define HAVE_STRERROR 1
#define HAVE_STRFTIME 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_STRNCASECMP 1
#define HAVE_STRCASECMP 1
#define HAVE_STRSTR 1
#define HAVE_STRCASESTR 1
#define HAVE_TIME_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_FILE_H 1
#define HAVE_FCNTL_H 1
#define HAVE_UNISTD_H 1
#define HAVE_ERRNO_H 1
#define HAVE_TIME_H 1
#define HAVE_SYS_SOCKET_H 1
#define HAVE_SYS_POLL_H 1
#define HAVE_NETINET_IN_H 1
#define HAVE_NETDB_H 1
#define HAVE_ARPA_INET_H 1
#define HAVE_SYS_MMAN_H 1
/* #undef HAVE_VALGRIND_HELGRIND_H */

#define HAVE_VPRINTF 1
#define HAVE_ASPRINTF 1
#define HAVE_VASPRINTF 1
/* #undef HAVE___MINGW_VASPRINTF */
/* #undef HAVE_WORKING_VSNPRINTF */
#define HAVE__BOOL 1
#define FPOS_T_STRUCT 1
#define HAVE_PCRE 1
#define HAVE_FLOCK 1
#define HAVE_FCNTL 1
#define HAVE_FPUTS 1
#define HAVE_FPUTWS 1
#define HAVE_FGETS 1
#define HAVE_FGETWS 1
#define HAVE_FPUTC 1
#define HAVE_FPUTWC 1
#define HAVE_FGETC 1
#define HAVE_FGETWC 1
#define STRUCT_TM_HAS_GMTOFF 1
#define HAVE_ATOLL 1
#define HAVE_STRTOLL 1
#define HAVE_STRTOULL 1
/* #undef HAVE_SLEEP */
/* #undef HAVE_NAP */
#define HAVE_USLEEP 1
#define HAVE_NANOSLEEP 1
#define HAVE_STRTOK_R 1
#define HAVE_SIGNAL 1
#define HAVE_SIGNAL_H 1
#define HAVE_DIRENT_H 1
#define HAVE_FNMATCH_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_LIMITS_H 1
#define HAVE_STRTOK_R 1
#define HAVE_GETPID 1
#define HAVE_GETPPID 1
#define HAVE_GETUID 1
#define HAVE_GETEUID 1
#define HAVE_LOCALTIME 1
#define HAVE_LOCALTIME_R 1
#define HAVE_GMTIME 1
#define HAVE_GMTIME_R 1
#define HAVE_CTYPE_H 1
#define HAVE_MKSTEMP 1
#define HAVE_WCSCASECMP 1
#define HAVE_WCSNCASECMP 1
#define HAVE_WCSCMP 1
#define HAVE_WCSNCMP 1
#define HAVE_WCSTOL 1
#define HAVE_WCSTOLL 1
#define HAVE_WCSLEN 1
#define HAVE_WCSSTR 1
#define HAVE_WCSTOUL 1
#define HAVE_WCSTOULL 1
#define HAVE_WCSTOMBS 1
#define HAVE_WCSRTOMBS 1
#define HAVE_WCSNRTOMBS 1
#define HAVE_MBSTOWCS 1
#define HAVE_MBSRTOWCS 1
#define HAVE_MBSNRTOWCS 1
#define HAVE_WPRINTF 1
#define HAVE_FWPRINTF 1
#define HAVE_SWPRINTF 1
#define HAVE_VWPRINTF 1
#define HAVE_VFWPRINTF 1
#define HAVE_VSWPRINTF 1
#define HAVE_WCSTOD 1
#define HAVE_WCSTOF 1
/* #undef HAVE_WSTOL */
/* #undef HAVE_WSTOLL */
/* #undef HAVE_WATOI */
/* #undef HAVE_WATOLL */
/* #undef HAVE_WSTOD */
/* #undef HAVE_WATOF */
#define HAVE_TRUNCATE 1
#define HAVE_FTRUNCATE 1
#define HAVE_STRCMP 1
#define HAVE_STRNCMP 1
#define HAVE_STRNLEN 1
#define HAVE_BCOPY 1
#define HAVE_SYNC 1
#define HAVE_FSYNC 1
#define HAVE_MMAP 1
#define HAVE_MUNMAP 1
/* #undef HAVE_PAGESIZE */
#define HAVE_SYSCONF 1
#define HAVE_STAT 1
#define HAVE_OPENDIR 1
#define HAVE_CLOSEDIR 1
#define HAVE_READDIR 1
#define HAVE_READDIR_R 1
/* #undef HAVE_HTOL */
/* #undef HAVE_HTOLL */
#define HAVE_ATOI 1
#define HAVE_ATOL 1
#define HAVE_ATOLL 1
#define HAVE_ATOF 1
/* #undef HAVE_STRTOLOWER */
/* #undef HAVE_STRTOUPPER */
#define HAVE_UNLINK 1
#define HAVE_REMOVE 1
#define HAVE_SYSLOG 1
#define HAVE_OPENLOG 1
#define HAVE_CLOSELOG 1



#define HAVE_GETHOSTNAME 1
#define HAVE_SETHOSTNAME 1
#define HAVE_GETDOMAINNAME 1
#define HAVE_SETDOMAINNAME 1
#define HAVE_UNAME 1
#define HAVE_INET_NTOP 1
#define HAVE_INET_PTON 1
#define HAVE_INET_ATON 1
#define HAVE_INET_NTOA 1
/* #undef HAVE_INET_NTOA_R */
#define HAVE_INET_ADDR 1
#define HAVE_INET_NETWORK 1
#define HAVE_INET_MAKEADDR 1

#define HAVE_RESOLV_H 1
#define HAVE_ARPA_NAMESER_H 1



/* #undef _FILE_OFFSET_BITS */


/* #undef HAVE_LIBLDNS */
#define HAVE_LIBBIND 1
#define HAVE_RES_QUERY 1
#define HAVE_RES_SEARCH 1
#define HAVE_RES_QUERYDOMAIN 1
#define HAVE_RES_MKQUERY 1
#define HAVE_RES_SEND 1
#define HAVE_DN_COMP 1
#define HAVE_DN_EXPAND 1
#define HAVE_NS_INITPARSE 1

/* #undef HAVE_LIBMICROHTTPD */
/* #undef HAVE_LIBIDN */
/* #undef HAVE_LIBIDN2 */
/* #undef HAVE_LIBIDN2_IDN2_NO_TR46 */


/*
 * Cryptography
 */
/* #undef HAVE_OPENSSL */
/* #undef HAVE_TLS_METHOD */
/* #undef HAVE_TLS_SERVER_METHOD */
/* #undef HAVE_TLS_CLIENT_METHOD */
/* #undef HAVE_SSLV23_METHOD */
/* #undef HAVE_SSLV23_SERVER_METHOD */
/* #undef HAVE_SSLV23_CLIENT_METHOD */
/* #undef HAVE_EVP_AES_128_ECB */
/* #undef HAVE_EVP_AES_192_ECB */
/* #undef HAVE_EVP_AES_256_ECB */
/* #undef HAVE_EVP_ARIA_128_ECB */
/* #undef HAVE_EVP_ARIA_192_ECB */
/* #undef HAVE_EVP_ARIA_256_ECB */
/* #undef HAVE_EVP_BF_ECB */
/* #undef HAVE_EVP_CAMELLIA_128_ECB */
/* #undef HAVE_EVP_CAMELLIA_192_ECB */
/* #undef HAVE_EVP_CAMELLIA_256_ECB */
/* #undef HAVE_EVP_CAST5_ECB */
/* #undef HAVE_EVP_DES_ECB */
/* #undef HAVE_EVP_DES_EDE3_ECB */
/* #undef HAVE_EVP_IDEA_ECB */
/* #undef HAVE_EVP_RC2_ECB */
/* #undef HAVE_EVP_RC5_32_12_16_ECB */


 
 
/*
 * Assembler
 */
/* #undef HAVE_YASM */
/* #undef HAVE_NASM */
/* #undef HAVE_X86_ASSEMBLER */

/*
 * Character encoding
 */
/* #undef HAVE_ICONV */
/* #undef ICONV_CONST */


/*
 * Endianess
 */
/* #undef __BIG_ENDIAN__ */
#define __LITTLE_ENDIAN__ 1

/*
 * Compression
 */

/* #undef HAVE_LIBZ */
/* #undef HAVE_BZIP2 */

/*
 * lib curl
 */
/* #undef HAVE_LIBCURL */
/* #undef LIBCURL_FEATURE_SSL */
/* #undef LIBCURL_FEATURE_IPV6 */
/* #undef LIBCURL_FEATURE_LIBZ */
/* #undef LIBCURL_FEATURE_ASYNCHDNS */
/* #undef LIBCURL_FEATURE_GSS_API */
/* #undef LIBCURL_FEATURE_SPNEGO */
/* #undef LIBCURL_FEATURE_NTLM */
/* #undef LIBCURL_FEATURE_NTLM_WB */
/* #undef LIBCURL_FEATURE_TLS_SRP */
/* #undef LIBCURL_PROTOCOL_DICT */
/* #undef LIBCURL_PROTOCOL_FILE */
/* #undef LIBCURL_PROTOCOL_FTP */
/* #undef LIBCURL_PROTOCOL_FTPS */
/* #undef LIBCURL_PROTOCOL_GOPHER */
/* #undef LIBCURL_PROTOCOL_HTTP */
/* #undef LIBCURL_PROTOCOL_HTTPS */
/* #undef LIBCURL_PROTOCOL_IMAP */
/* #undef LIBCURL_PROTOCOL_IMAPS */
/* #undef LIBCURL_PROTOCOL_POP3 */
/* #undef LIBCURL_PROTOCOL_POP3S */
/* #undef LIBCURL_PROTOCOL_RTSP */
/* #undef LIBCURL_PROTOCOL_SMTP */
/* #undef LIBCURL_PROTOCOL_SMTPS */
/* #undef LIBCURL_PROTOCOL_TELNET */
/* #undef LIBCURL_PROTOCOL_TFTP */


/*
 * Audio
 */
/* #undef HAVE_LAME_LAME_H */
/* #undef HAVE_LAME */
/* #undef HAVE_LAME_HIP_DECODE */
/* #undef HAVE_MAD_H */
/* #undef HAVE_LIBMAD */
/* #undef HAVE_MPG123 */
/* #undef HAVE_MPG123_H */
/* #undef HAVE_LIBSHOUT */
/* #undef HAVE_LIBOGG */

/* #undef HAVE_LIBCDIO */
/* #undef HAVE_LIBCDDB */


/*
 * pthread
 */
#define HAVE_PTHREADS 1



/*
 * Grafiklibraries
 */
/* #undef HAVE_TIFF */
/* #undef HAVE_JPEG */
/* #undef HAVE_LIBJPEGTURBO */
/* #undef HAVE_PNG */
/* #undef HAVE_IMAGEMAGICK */
/* #undef HAVE_FREETYPE2 */
/* #undef HAVE_SDL2 */
/* #undef HAVE_SDL12 */
/* #undef HAVE_X11 */

/*
 * Databases
 */
/* #undef HAVE_MYSQL */
/* #undef HAVE_POSTGRESQL */
/* #undef POSTGRESQL_HAVE_PQsetSingleRowMode */
/* #undef HAVE_SQLITE3 */


#define ICONV_UNICODE	undefined

#ifdef _WIN32
	#ifdef _M_ALPHA
		#ifndef __BIG_ENDIAN__
			#define __BIG_ENDIAN__
		#endif
	#else
		#ifndef __LITTLE_ENDIAN__
			#define __LITTLE_ENDIAN__ 1
		#endif
	#endif

	#define WIN32_LEAN_AND_MEAN		// Keine MFCs
	#define _WIN32_WINNT 0x0600
	// Ab Visual Studio 2005 (Visual C++ 8.0) gibt es einige �nderungen
	#if _MSC_VER >= 1400
		#define _CRT_SECURE_NO_WARNINGS
		#define _CRT_SECURE_NO_DEPRECATE 
		#define _AFX_SECURE_NO_DEPRECATE 
		#define _ATL_SECURE_NO_DEPRECATE
		#pragma warning(disable : 4996)
		#pragma warning(disable : 4800)

		/*
		#define unlink _unlink
		#define strdup _strdup
		#define fileno _fileno
		#define mkdir _mkdir
		#define open _open
		#define close _close
		#define read _read
		*/
	#endif
	//#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
	//#include <windows.h>
#endif

#endif
