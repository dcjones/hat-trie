
AC_INIT([hat-trie], [0.1.0], [dcjones@cs.washington.edu])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_HEADERS([config.h])

CFLAGS="-Wall -Wextra -std=c99 -pedantic -g -O0"

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_DISABLE_SHARED
AC_PROG_LIBTOOL

AC_C_BIGENDIAN([AC_MSG_ERROR([Big-endian systems are not currently supported.])])
AC_CHECK_HEADERS([stdint.h stdlib.h])
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

AC_CONFIG_FILES([Makefile src/Makefile test/Makefile])
AC_OUTPUT

