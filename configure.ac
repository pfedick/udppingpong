dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.69)
AC_INIT([dnsperftest], [1.0.0], [fedick@denic.de], [dnsperftest-1.0.0.tar.bz2])
AC_CONFIG_AUX_DIR([autoconf])

#AC_CONFIG_MACRO_DIR(autoconf)
AC_CONFIG_SRCDIR([CommonLib/include/dnsperftest_sensor.h])
#AC_CONFIG_SRCDIR([include/config.h.in])

AC_CONFIG_FILES([Makefile])
#AC_CONFIG_FILES([tests/Makefile])
#AC_CONFIG_FILES([ppl7-config])
AC_CONFIG_HEADERS([include/config.h])


###############################################################################
# Includes
###############################################################################
#sinclude(autoconf/ax_cxx_compile_stdcxx.m4)
sinclude(autoconf/ppl7.m4)


AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE

OLD_CFLAGS=$CFLAGS
# Checks for programs.
AC_PROG_CC
CFLAGS=$OLD_CFLAGS
AC_PROG_CXX
dnl AX_CXX_COMPILE_STDCXX(11) 

dnl AC_LANG(C++)

dnl AX_GCC_ARCHFLAG

case "$host_os" in
	freebsd* )
		CFLAGS="-I/usr/local/include $CFLAGS"
		LDFLAGS="-L/usr/local/lib $LDFLAGS"
  		;;
esac
 
AC_C_BIGENDIAN(
	report_byteorder="BIG ENDIAN"
	AC_DEFINE(__BIG_ENDIAN__),
	report_byteorder="LITTLE ENDIAN"
	AC_DEFINE(__LITTLE_ENDIAN__))

AC_CHECK_SIZEOF(char)
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(wchar_t)
AC_CHECK_SIZEOF(size_t)
AC_CHECK_SIZEOF(float)
AC_CHECK_SIZEOF(double)
AC_CHECK_SIZEOF(void*)

#--------------------------------------------------------------------
#       Check for various typedefs and provide substitutes if
#       they don't exist.
#--------------------------------------------------------------------

AC_HEADER_STDBOOL
AC_HEADER_TIME
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UID_T
AC_TYPE_OFF_T

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

AX_TM_HAS_GMTOFF()

AC_PATH_XTRA

AX_CHECK_GNU_MAKE(,
   [AC_MSG_ERROR([No gmake found])]
)
if test x$_cv_gnu_make_command = x ; then
   AC_MSG_ERROR(No gmake found)
else 
   MAKECMD=$_cv_gnu_make_command
fi

AC_SUBST(MAKECMD)


AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([stdarg.h])
AC_CHECK_HEADERS([stddef.h])
AC_CHECK_HEADERS([stdio.h])
AC_CHECK_HEADERS([math.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([sys/file.h])
AC_CHECK_HEADERS([time.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([sys/types.h])
AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([sys/poll.h])
AC_CHECK_HEADERS([netinet/in.h])
AC_CHECK_HEADERS([arpa/inet.h])
AC_CHECK_HEADERS([strings.h])
AC_CHECK_HEADERS([errno.h])
AC_CHECK_HEADERS([signal.h])
AC_CHECK_HEADERS([dirent.h])
AC_CHECK_HEADERS([fnmatch.h])
AC_CHECK_HEADERS([sys/stat.h])
AC_CHECK_HEADERS([limits.h])
AC_CHECK_HEADERS([locale.h])
AC_CHECK_HEADERS([syslog.h])

AC_CHECK_TYPE(size_t, unsigned)
AC_CHECK_TYPE(wchar_t, unsigned)

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STAT
AC_CHECK_FUNCS([socket signal])
AC_CHECK_FUNCS([fcntl ioctl])
AC_CHECK_FUNCS([signal syslog openlog closelog])
AC_CHECK_FUNCS([select pselect])
AC_CACHE_CHECK([for sa_len in struct sockaddr], ac_cv_struct_sa_len,
[AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/socket.h>], [struct sockaddr s; s.sa_len;],
ac_cv_struct_sa_len=yes, ac_cv_struct_sa_len=no)])
if test $ac_cv_struct_sa_len = yes; then
  AC_DEFINE(HAVE_SOCKADDR_SA_LEN)
fi


dnl do we need to specify -lm explicitly?
AC_CHECK_FUNC(sin,,[AC_CHECK_LIB(m,sin)])


AX_PATH_LIB_PPL7(7.0.0,
	AC_DEFINE(HAVE_PPL7, 1, [ Define if you have ppl7. ])
	, :)


SRCDIR=`pwd`
AC_SUBST(SRCDIR)


WINDOWS_WINSOCK_LIBS=""
case "$host_os" in
	*mingw32* )
		MINGW32=yes
		AC_DEFINE(MINGW32, 1, [Define if you compile on Windows MINGW32])
		#LIBS="$LIBS -lws2_32"
		WINDOWS_WINSOCK_LIBS="-lws2_32"
  		;;
  	*mingw64*|msys )
		MINGW32=yes
		MINGW64=yes
		AC_DEFINE(MINGW32, 1, [Define if you compile on Windows MINGW32])
		AC_DEFINE(MINGW64, 1, [Define if you compile on Windows MINGW64])
		WINDOWS_WINSOCK_LIBS="-lws2_32"
  		;;
	* )
		MINGW32=no
		;;
esac

AC_SUBST(WINDOWS_WINSOCK_LIBS)


###############################################################################
# Output
###############################################################################

AC_OUTPUT



