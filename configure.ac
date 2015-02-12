#!/bin/bash -norc
dnl	This file is an input file used by the GNU "autoconf" program to
dnl	generate the file "configure", which is run during Tcl installation
dnl	to configure the system for the local environment.
#
# RCS: @(#) $Id: configure.in,v 1.40 2005/04/28 13:20:09 das Exp $

#-----------------------------------------------------------------------
# Sample configure.in for Tcl Extensions.  The only places you should
# need to modify this file are marked by the string __CHANGE__
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
# __CHANGE__
# Set your package name and version numbers here.
#
# This initializes the environment with PACKAGE_NAME and PACKAGE_VERSION
# set as provided.  These will also be added as -D defs in your Makefile
# so you can encode the package version directly into the source files.
#-----------------------------------------------------------------------

AC_INIT([Ffidl], [0.6])

AC_PREREQ(2.59b)

#--------------------------------------------------------------------
# Call TEA_INIT as the first TEA_ macro to set up initial vars.
# This will define a ${TEA_PLATFORM} variable == "unix" or "windows"
# as well as PKG_LIB_FILE and PKG_STUB_LIB_FILE.
#--------------------------------------------------------------------

TEA_INIT([3.9])

AC_CONFIG_AUX_DIR(tclconfig)

#--------------------------------------------------------------------
# ffidl autoheader
#--------------------------------------------------------------------

AC_CONFIG_HEADERS([ffidlConfig.h])
AC_CONFIG_COMMANDS_PRE([DEFS=-DHAVE_FFIDL_CONFIG_H])
TEA_ADD_INCLUDES([-imacros ffidlConfig.h])
CONFIG_CLEAN_FILES=ffidlConfig.h
AC_SUBST(CONFIG_CLEAN_FILES)

#--------------------------------------------------------------------
# Load the tclConfig.sh file
#--------------------------------------------------------------------

TEA_PATH_TCLCONFIG
TEA_LOAD_TCLCONFIG

#--------------------------------------------------------------------
# Load the tkConfig.sh file if necessary (Tk extension)
#--------------------------------------------------------------------

#TEA_PATH_TKCONFIG
#TEA_LOAD_TKCONFIG

#-----------------------------------------------------------------------
# Handle the --prefix=... option by defaulting to what Tcl gave.
# Must be called after TEA_LOAD_TCLCONFIG and before TEA_SETUP_COMPILER.
#-----------------------------------------------------------------------

TEA_PREFIX

#-----------------------------------------------------------------------
# Standard compiler checks.
# This sets up CC by using the CC env var, or looks for gcc otherwise.
# This also calls AC_PROG_CC, AC_PROG_INSTALL and a few others to create
# the basic setup necessary to compile executables.
#-----------------------------------------------------------------------

TEA_SETUP_COMPILER

#-----------------------------------------------------------------------
# __CHANGE__
# Specify the C source files to compile in TEA_ADD_SOURCES,
# public headers that need to be installed in TEA_ADD_HEADERS,
# stub library C source files to compile in TEA_ADD_STUB_SOURCES,
# and runtime Tcl library files in TEA_ADD_TCL_SOURCES.
# This defines PKG(_STUB)_SOURCES, PKG(_STUB)_OBJECTS, PKG_HEADERS
# and PKG_TCL_SOURCES.
#-----------------------------------------------------------------------

TEA_ADD_SOURCES([ffidl.c])
TEA_ADD_HEADERS([])
TEA_ADD_INCLUDES([])
TEA_ADD_LIBS([])
TEA_ADD_CFLAGS([])
TEA_ADD_STUB_SOURCES([])
TEA_ADD_TCL_SOURCES([library/ffidlrt.tcl])
AC_DEFINE(LOOKUP_TK_STUBS, 1, 
    [ffidl::stubsymbol can lookup in Tk stubs tables])

#--------------------------------------------------------------------
# __CHANGE__
# A few miscellaneous platform-specific items:
#
# Define a special symbol for Windows (BUILD_sample in this case) so
# that we create the export library with the dll.
#
# Windows creates a few extra files that need to be cleaned up.
# You can add more files to clean if your extension creates any extra
# files.
#
# TEA_ADD_* any platform specific compiler/build info here.
#--------------------------------------------------------------------

if test "${TEA_PLATFORM}" = "windows" ; then
    AC_DEFINE(BUILD_Ffidl, 1, [Build windows export dll])
    CLEANFILES="pkgIndex.tcl *.lib *.dll *.exp *.ilk *.pdb vc*.pch"
else
    CLEANFILES="pkgIndex.tcl"
fi
AC_SUBST(CLEANFILES)

#--------------------------------------------------------------------
# __CHANGE__
# Choose which headers you need.  Extension authors should try very
# hard to only rely on the Tcl public header files.  Internal headers
# contain private data structures and are subject to change without
# notice.
# This MUST be called after TEA_LOAD_TCLCONFIG / TEA_LOAD_TKCONFIG
#--------------------------------------------------------------------

#TEA_PUBLIC_TCL_HEADERS
TEA_PRIVATE_TCL_HEADERS

#TEA_PUBLIC_TK_HEADERS
#TEA_PRIVATE_TK_HEADERS
#TEA_PATH_X

#--------------------------------------------------------------------
# ffidl specific configuration
#--------------------------------------------------------------------

AC_MSG_CHECKING([which library to use for foreign function calls])

AC_ARG_ENABLE(libffi, 
    AC_HELP_STRING([--enable-libffi],
        [use libffi for foreign function calls [--enable-libffi]]), 
    [libffi=$enableval], [libffi=default])
AC_ARG_ENABLE(ffcall, 
    AC_HELP_STRING([--enable-ffcall],
        [use ffcall for foreign function calls [--disable-ffcall]]), 
    [ffcall=$enableval], [ffcall=no])
case $libffi,$ffcall in
  yes,no|default,no)
	AC_DEFINE(USE_LIBFFI, 1, [Use libffi for foreign function calls])
	TEA_ADD_INCLUDES([-Ilibffi/include])
	TEA_ADD_LIBS([libffi/.libs/libffi.a])
	FFIDL_FFLIB=libffi
	AC_MSG_RESULT([libffi])
  ;;
  no,yes|default,yes)
	AC_DEFINE(USE_FFCALL, 1, [Use ffcall for foreign function calls])
	TEA_ADD_INCLUDES([-Iffcall/avcall -Iffcall/callback])
	TEA_ADD_LIBS([ffcall/avcall/.libs/libavcall.a 
	              ffcall/callback/.libs/libcallback.a])
	FFIDL_FFLIB=ffcall
	AC_MSG_RESULT([ffcall])
  ;;
  yes,yes)
          AC_MSG_ERROR([You cannot use both --enable-libffi and --enable-ffcall.])
  ;;
esac
AC_SUBST(FFIDL_FFLIB)

AC_MSG_CHECKING([whether to implement callbacks])
AC_ARG_ENABLE(callbacks, 
    AC_HELP_STRING([--enable-callbacks],
        [implement callbacks, if possible [--enable-callbacks]]), 
    [tcl_ok=$enableval], [tcl_ok=yes])
AC_MSG_RESULT([$tcl_ok])
if test "$tcl_ok" = "yes"; then
    AC_DEFINE(USE_CALLBACKS, 1, [Implement callbacks])
fi

AC_MSG_CHECKING([whether to build ffidl test functions])
AC_ARG_ENABLE(test, 
    AC_HELP_STRING([--enable-test],
        [build ffidl test functions [--disable-test]]), 
    [tcl_ok=$enableval], [tcl_ok=yes])
AC_MSG_RESULT([$tcl_ok])
if test "$tcl_ok" = "yes"; then
    TEA_ADD_SOURCES([ffidl_test.c])
    TEA_ADD_LIBS([-lgcc])
    FFIDL_TEST=1
fi
AC_SUBST(FFIDL_TEST)

AC_C_CHAR_UNSIGNED
AC_CHECK_SIZEOF(char)
AC_CHECK_ALIGNOF(char)
AC_CHECK_SIZEOF(short)
AC_CHECK_ALIGNOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_ALIGNOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_ALIGNOF(long)
AC_CHECK_TYPES(long long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_ALIGNOF(long long)
AC_CHECK_SIZEOF(float)
AC_CHECK_ALIGNOF(float)
AC_CHECK_SIZEOF(double)
AC_CHECK_ALIGNOF(double)
AC_C_LONG_DOUBLE
AC_CHECK_SIZEOF(long double)
AC_CHECK_ALIGNOF(long double)
AC_CHECK_SIZEOF(void *)
AC_CHECK_ALIGNOF(void *)

AC_CANONICAL_HOST
eval AC_DEFINE_UNQUOTED(CANONICAL_HOST, "${host}", 
    [Canonical host name])

#--------------------------------------------------------------------
# Check whether --enable-threads or --disable-threads was given.
# This auto-enables if Tcl was compiled threaded.
#--------------------------------------------------------------------

TEA_ENABLE_THREADS

#--------------------------------------------------------------------
# The statement below defines a collection of symbols related to
# building as a shared library instead of a static library.
#--------------------------------------------------------------------

TEA_ENABLE_SHARED

#--------------------------------------------------------------------
# This macro figures out what flags to use with the compiler/linker
# when building shared/static debug/optimized objects.  This information
# can be taken from the tclConfig.sh file, but this figures it all out.
#--------------------------------------------------------------------

TEA_CONFIG_CFLAGS

if test "${TEA_PLATFORM}" != "windows" ; then
    if test "${TCL_MAJOR_VERSION}" -gt 8 -o \
        \( "${TCL_MAJOR_VERSION}" -gt 7 -a "${TCL_MINOR_VERSION}" -gt 5 \)
    then
        AC_DEFINE(USE_TCL_LOADFILE, 1, [Use Tcl_LoadFile() API to load code])
        PKG_DL_OBJS=
    else
        AC_DEFINE(USE_TCL_DLOPEN, 1, [Use Tcl*Dlopen() API to load code])
        TEA_ADD_LIBS([${DL_LIBS}])
        PKG_DL_OBJS=${DL_OBJS}
    fi
fi
AC_SUBST(PKG_DL_OBJS)

#--------------------------------------------------------------------
# Set the default compiler switches based on the --enable-symbols option.
#--------------------------------------------------------------------

TEA_ENABLE_SYMBOLS

#--------------------------------------------------------------------
# Everyone should be linking against the Tcl stub library.  If you
# can't for some reason, remove this definition.  If you aren't using
# stubs, you also need to modify the SHLIB_LD_LIBS setting below to
# link against the non-stubbed Tcl library.  Add Tk too if necessary.
#--------------------------------------------------------------------

AC_DEFINE(USE_TCL_STUBS, 1, [Use Tcl stubs])
#AC_DEFINE(USE_TK_STUBS, 1, [Use Tk stubs])

#--------------------------------------------------------------------
# This macro generates a line to use when building a library.  It
# depends on values set by the TEA_ENABLE_SHARED, TEA_ENABLE_SYMBOLS,
# and TEA_LOAD_TCLCONFIG macros above.
#--------------------------------------------------------------------

TEA_MAKE_LIB

#--------------------------------------------------------------------
# Find tclsh so that we can run pkg_mkIndex to generate the pkgIndex.tcl
# file during the install process.  Don't run the TCLSH_PROG through
# ${CYGPATH} because it's being used directly by make.
# Require that we use a tclsh shell version 8.2 or later since earlier
# versions have bugs in the pkg_mkIndex routine.
# Add WISH as well if this is a Tk extension.
#--------------------------------------------------------------------

TEA_PROG_TCLSH
#TEA_PROG_WISH

#--------------------------------------------------------------------
# Finally, substitute all of the various values into the Makefile.
# You may alternatively have a special pkgIndex.tcl.in or other files
# which require substituting th AC variables in.  Include these here.
#--------------------------------------------------------------------

AC_OUTPUT([GNUmakefile])
