#
# Process this file with autoconf to produce a configure script

AC_PREREQ([2.63])

AC_INIT([itpl2dirtree], [1.0.0])
AC_CONFIG_MACRO_DIRS([m4])

AM_INIT_AUTOMAKE

dnl AM_CONFIG_HEADER(config.h)

dnl AM_MAINTAINER_MODE

# Checks for programs.
dnl AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
dnl AC_EXEEXT
dnl AC_LANG_CPLUSPLUS
AM_PROG_LIBTOOL

# Library Version
LTVERSION="1:0:0"
AC_SUBST(LTVERSION)

# Checks for libraries.
AC_CHECK_FUNCS([strlcpy])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_ARG_WITH(expat-includes,
             [  --with-expat-includes=DIR  set a path for the header file of expat ],
             [ac_expat_hpath=$withval], [ac_expat_hpath="/usr/include"] )

oCFLAGS="$CFLAGS"
CFLAGS="-I$ac_expat_hpath"
AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM([[#include <expat.h>]], [[/*BODY*/]])],
        [AC_MSG_RESULT(expat header files are in $ac_expat_hpath)
         EXPAT_INCLUDES="-I$ac_expat_hpath"],
        [AC_MSG_RESULT(expat header path not found)] )
AC_SUBST(EXPAT_INCLUDES)
AM_SUBST_NOTMAKE(EXPAT_INCLUDES)

AC_ARG_WITH(expat-libraries,
             [  --with-expat-libraries=DIR  set a path for the library file of expat ],
             [ac_expat_lpath=$withval], [ac_expat_lpath="/usr/lib"] )

oLIBS="$LIBS"
LIBS="-L$ac_expat_lpath -lexpat -Xlinker -rpath -Xlinker $ac_expat_lpath"
AC_LINK_IFELSE(
        [AC_LANG_PROGRAM([[#include <expat.h>]], [[/*BODY*/]])],
        [AC_MSG_RESULT(expat library files are in $ac_expat_lpath)
         EXPAT_LDADD="$ac_expat_lpath"],
        [AC_MSG_RESULT(expat library path not found)] )
AC_SUBST(EXPAT_LDADD)
AM_SUBST_NOTMAKE(EXPAT_LDADD)

LIBS="$oLIBS"
CFLAGS="$oCFLAGS"

dnl
dnl C comipiler flags
dnl
AC_ARG_ENABLE(
        debug,
        [ --enable-debug=ARG debugging mode ],
        [
                if test "$enableval" = yes ; then
                  CFLAGS="$CFLAGS -O0 -g -Wall -DDEBUG"
                else
                  CFLAGS="$CFLAGS -O0 -g -Wall -DDEBUG=$enableval"
                fi
        ],
        [
                CFLAGS="-O3 -g -Wall -DNDEBUG"
        ])

AC_OUTPUT([Makefile
           src/Makefile])
