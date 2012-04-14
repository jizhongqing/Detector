#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([gadget], [0.0.1], [a0079636@nus.edu.sg])

AC_CONFIG_SRCDIR([gadget.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([.])

LT_INIT
AC_LANG([C++])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CXX

CPPFLAGS="$CPPFLAGS"
LIBS="$LIBS -lelf"

# Checks for dependencies.
PKG_CHECK_MODULES(	ARGTABLE2,
					argtable2,
					CFLAGS="$CFLAGS $ARGTABLE2_CFLAGS"; CPPFLAGS="$CPPFLAGS $ARGTABLE2_CFLAGS"; LIBS="$LIBS $ARGTABLE2_LIBS"	)

# Checks for header files.
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL

# Done
AC_CONFIG_FILES([
	Makefile
])

AC_OUTPUT
