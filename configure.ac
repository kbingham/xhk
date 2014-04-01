AC_INIT([xhk], [1.0], [kieranbingham@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC_C99
AM_PROG_CC_C_O

AC_CONFIG_HEADERS([config.h])

PKG_CHECK_MODULES([X11], [x11 >= 1.6])
PKG_CHECK_MODULES([XI], [xi >= 1.6])
PKG_CHECK_MODULES([XTST], [xtst >= 1])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
])
AC_OUTPUT

