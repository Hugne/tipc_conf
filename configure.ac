#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.68])
AC_INIT([tipcconf], [0.1.0], [erik.hugne@ericsson.com])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_HEADERS([config.h])
# Checks for programs.

AC_PROG_CC
AC_CHECK_PROG([has_pkg_config],[pkg-config],[yes])
if test "x$has_pkg_config" != "xyes"; then
	AC_MSG_ERROR([pkg-config not found])
fi
CFLAGS="$CFLAGS -m64 -std=gnu99 -pedantic -fno-strict-aliasing \
	-Wall -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes \
	-Wmissing-prototypes -Werror"

# Checks for libraries.
PKG_CHECK_MODULES([LIBMNL], [libmnl])
PKG_CHECK_MODULES([LIBJANSSON], [jansson])
PKG_CHECK_MODULES([LIBAVAHICORE], [avahi-core])
#Fix for broken AVAHI pkg-config, avahi-core depends on avahi-common
LIBAVAHICORE_LIBS+="-lavahi-common"

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
# Automake options
AC_ARG_ENABLE([static],[AC_HELP_STRING([--enable-static],
	      [Enable static linking])],
	      [enable_static=yes; echo "Enable static linking"],
	      [])
AM_CONDITIONAL([STATIC],
	       [test "x$enable_static" = "xyes"])

#AC_ARG_ENABLE([static],
#	      [  --enable-static         Enable static linking])
#AS_IF([test "x$enable_foo" == "xyes"],
#      [echo "Will do static linking"; AM_CONDITIONAL(STATIC, true)])
AC_CONFIG_FILES([Makefile
		client/Makefile])
AC_OUTPUT
