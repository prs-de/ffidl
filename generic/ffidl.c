/*****************************************
 * ffidl
 *
 * A combination of libffi or ffcall, for foreign function
 * interface, and libdl, for dynamic library loading and
 * symbol listing,  packaged with hints from ::dll, and
 * exported to Tcl.
 *
 * Ffidl - Copyright (c) 1999 by Roger E Critchlow Jr,
 * Santa Fe, NM, USA, rec@elf.org
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the ``Software''), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL ROGER E CRITCHLOW JR BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Note that this distribution of Ffidl contains copies of libffi and
 * ffcall, each of which has its own Copyright notice and License.
 *
 */

/*
 * Changes since ffidl 0.5:
 *  - updates for 2005 versions of libffi & ffcall
 *  - TEA 3.2 buildsystem, testsuite
 *  - support for Tcl 8.4, Tcl_WideInt, TclpDlopen
 *  - support for Darwin PowerPC
 *  - fixes for 64bit (LP64)
 *  - callouts & callbacks are created/used relative to current namespace (for unqualified names)
 *  - addition of [ffidl::stubsymbol] for Tcl/Tk symbol resolution via stubs tables
 *  - callbacks can be called anytime, not just from inside callouts (using Tcl_BackgroundError to report errors)
 * These changes are under BSD License and are
 * Copyright (c) 2005, Daniel A. Steffen <das@users.sourceforge.net>
 *
 * Changes since ffidl 0.6:
 *  - Build support for Tcl 8.6. (by pooryorick)
 *
 * Changes since ffidl 0.6:
 *  - Updates for API changes in 2015 version of libffi.
 *  - Fixes for Tcl_WideInt.
 *  - Update build system to TEA 3.9.
 * These changes are under BSD License and are
 * Copyright (c) 2015, Patzschke + Rasp Software GmbH, Wiesbaden
 * Author: Adrián Medraño Calvo <amcalvo@prs.de>
 *
 * Changes since ffidl 0.7:
 *  - Support for LLP64 (Win64).
 *  - Support specifying callback's command prefix.
 *  - Fix usage of libffi's return value API.
 *  - Disable long double support if longer than double
 *  - ... see doc/ffidl.html
 * These changes are under BSD License and are
 * Copyright (c) 2020, Patzschke + Rasp Software GmbH, Wiesbaden
 * Author: Adrián Medraño Calvo <amcalvo@prs.de>
 */

#include <ffidlConfig.h>

#include <tcl.h>
#include <tclInt.h>
#include <tclPort.h>

#if defined(LOOKUP_TK_STUBS)
static const char *MyTkInitStubs(Tcl_Interp *interp, char *version, int exact);
static void *tkStubsPtr, *tkPlatStubsPtr, *tkIntStubsPtr, *tkIntPlatStubsPtr, *tkIntXlibStubsPtr;
#else
#define tkStubsPtr NULL
#define tkPlatStubsPtr NULL
#define tkIntStubsPtr NULL
#define tkIntPlatStubsPtr NULL
#define tkIntXlibStubsPtr NULL
#endif

/*
 * Windows needs to know which symbols to export.  Unix does not.
 * BUILD_Ffidl should be undefined for Unix.
 */

#if defined(BUILD_Ffidl)
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT
#endif /* BUILD_Ffidl */

#include <string.h>
#include <stdlib.h>
/* Needed for MSVC (error C2065: 'INT64_MIN' : undeclared identifier) */
#include <stdint.h>

/* AC_C_CHAR_UNSIGNED */
#include <limits.h>
#if CHAR_MIN == 0
# define CHAR_UNSIGNED 1
#endif

/*
 * We can use either
 * libffi, with a no strings attached license,
 * or ffcall, with a GPL license.
 */

#if defined USE_LIBFFI

/* workaround for ffi.h bug on certain platforms: */
#include <stddef.h>
#include <limits.h>
#if HAVE_LONG_LONG
#  ifndef LONG_LONG_MAX
#    if SIZEOF_LONG_LONG == 4
#        define LONG_LONG_MAX 2147483647
#    elif SIZEOF_LONG_LONG == 8
#        define LONG_LONG_MAX 9223372036854775807
#    endif
#  endif
#endif

/*
 * We use two defines from ffi.h:
 *  FFI_NATIVE_RAW_API which indicates support for raw ffi api.
 *  FFI_CLOSURES which indicates support for callbacks.
 * libffi-1.20 doesn't define the latter, so we default it.
 */
#include <ffi.h>

#define USE_LIBFFI_RAW_API FFI_NATIVE_RAW_API


#ifndef FFI_CLOSURES
#define HAVE_CLOSURES 0
#else
#define HAVE_CLOSURES FFI_CLOSURES
#endif

#if defined(HAVE_LONG_DOUBLE) && defined(HAVE_LONG_DOUBLE_WIDER)
/*
 * Cannot support wider long doubles because they don't fit in Tcl_Obj.
 */
#  undef HAVE_LONG_DOUBLE
#endif

#define lib_type_void	&ffi_type_void
#define lib_type_uint8	&ffi_type_uint8
#define lib_type_sint8	&ffi_type_sint8
#define lib_type_uint16	&ffi_type_uint16
#define lib_type_sint16	&ffi_type_sint16
#define lib_type_uint32	&ffi_type_uint32
#define lib_type_sint32	&ffi_type_sint32
#define lib_type_uint64	&ffi_type_uint64
#define lib_type_sint64	&ffi_type_sint64
#define lib_type_float	&ffi_type_float
#define lib_type_double	&ffi_type_double
#define lib_type_longdouble	&ffi_type_longdouble
#define lib_type_pointer	&ffi_type_pointer

#define lib_type_schar	&ffi_type_schar
#define lib_type_uchar	&ffi_type_uchar
#define lib_type_ushort	&ffi_type_ushort
#define lib_type_sshort	&ffi_type_sshort
#define lib_type_uint	&ffi_type_uint
#define lib_type_sint	&ffi_type_sint
/* ffi_type_ulong & ffi_type_slong are always 64bit ! */
#if SIZEOF_LONG == 2
#define lib_type_ulong	&ffi_type_uint16
#define lib_type_slong	&ffi_type_sint16
#elif SIZEOF_LONG == 4
#define lib_type_ulong	&ffi_type_uint32
#define lib_type_slong	&ffi_type_sint32
#elif SIZEOF_LONG == 8
#define lib_type_ulong	&ffi_type_uint64
#define lib_type_slong	&ffi_type_sint64
#endif
#if HAVE_LONG_LONG
#if SIZEOF_LONG_LONG == 2
#define lib_type_ulonglong	&ffi_type_uint16
#define lib_type_slonglong	&ffi_type_sint16
#elif SIZEOF_LONG_LONG == 4
#define lib_type_ulonglong	&ffi_type_uint32
#define lib_type_slonglong	&ffi_type_sint32
#elif SIZEOF_LONG_LONG == 8
#define lib_type_ulonglong	&ffi_type_uint64
#define lib_type_slonglong	&ffi_type_sint64
#endif
#endif

#if defined(CHAR_UNSIGNED)
#define lib_type_char	&ffi_type_uint8
#else
#define lib_type_char	&ffi_type_sint8
#endif

#endif /* #if defined USE_LIBFFI */

#if defined USE_LIBFFCALL
#include <avcall.h>
#include <callback.h>

/* Compatibility for libffcall < 2.0 */
#if LIBFFCALL_VERSION < 0x0200
#define callback_t __TR_function
#define callback_function_t __VA_function
#endif

#define HAVE_CLOSURES 1
#undef HAVE_LONG_DOUBLE		/* no support in ffcall */

#define lib_type_void	__AVvoid
#define lib_type_char	__AVchar
#define lib_type_schar	__AVschar
#define lib_type_uchar	__AVuchar
#define lib_type_sshort	__AVshort
#define lib_type_ushort	__AVushort
#define lib_type_sint	__AVint
#define lib_type_uint	__AVuint
#define lib_type_slong	__AVlong
#define lib_type_ulong	__AVulong
#define lib_type_slonglong	__AVlonglong
#define lib_type_ulonglong	__AVulonglong
#define lib_type_float	__AVfloat
#define lib_type_double	__AVdouble
#define lib_type_pointer	__AVvoidp
#define lib_type_struct	__AVstruct

#define av_sint	av_int
#define av_slong	av_long
#define av_slonglong	av_longlong
#define av_sshort	av_short
#define av_start_sint	av_start_int
#define av_start_slong	av_start_long
#define av_start_slonglong	av_start_longlong
#define av_start_sshort	av_start_short
 
/* NB, abbreviated to the most usual, add cases as required */
#if SIZEOF_CHAR == 1
#define lib_type_uint8	lib_type_uchar
#define av_start_uint8	av_start_uchar
#define av_uint8	av_uchar
#define va_start_uint8	va_start_uchar
#define va_arg_uint8	va_arg_uchar
#define va_return_uint8	va_return_uchar
#define lib_type_sint8	lib_type_schar
#define av_start_sint8	av_start_schar
#define av_sint8	av_schar
#define va_start_sint8	va_start_schar
#define va_arg_sint8	va_arg_schar
#define va_return_sint8	va_return_schar
#else
#error "no 8 bit int"
#endif

#if SIZEOF_SHORT == 2
#define lib_type_uint16	lib_type_ushort
#define av_start_uint16	av_start_ushort
#define av_uint16	av_ushort
#define va_start_uint16	va_start_ushort
#define va_arg_uint16	va_arg_ushort
#define va_return_uint16	va_return_ushort
#define lib_type_sint16	lib_type_sshort
#define av_start_sint16	av_start_sshort
#define av_sint16	av_sshort
#define va_start_sint16	va_start_short
#define va_arg_sint16	va_arg_short
#define va_return_sint16	va_return_short
#else
#error "no 16 bit int"
#endif

#if SIZEOF_INT == 4
#define lib_type_uint32	lib_type_uint
#define av_start_uint32	av_start_uint
#define av_uint32	av_uint
#define va_start_uint32	va_start_uint
#define va_arg_uint32	va_arg_uint
#define va_return_uint32	va_return_uint
#define lib_type_sint32	lib_type_sint
#define av_start_sint32	av_start_sint
#define av_sint32	av_sint
#define va_start_sint32	va_start_int
#define va_arg_sint32	va_arg_int
#define va_return_sint32	va_return_int
#else
#error "no 32 bit int"
#endif

#if SIZEOF_LONG == 8
#define lib_type_uint64	lib_type_ulong
#define av_start_uint64	av_start_ulong
#define av_uint64	av_ulong
#define va_start_uint64	va_start_ulong
#define va_arg_uint64	va_arg_ulong
#define va_return_uint64	va_return_ulong
#define lib_type_sint64	lib_type_slong
#define av_start_sint64	av_start_slong
#define av_sint64	av_slong
#define va_start_sint64	va_start_long
#define va_arg_sint64	va_arg_long
#define va_return_sint64	va_return_long
#elif HAVE_LONG_LONG && SIZEOF_LONG_LONG == 8
#define lib_type_uint64	lib_type_ulonglong
#define av_start_uint64	av_start_ulonglong
#define av_uint64	av_ulonglong
#define va_start_uint64	va_start_ulonglong
#define va_arg_uint64	va_arg_ulonglong
#define va_return_uint64	va_return_ulonglong
#define lib_type_sint64	lib_type_slonglong
#define av_start_sint64	av_start_slonglong
#define av_sint64	av_slonglong
#define va_start_sint64	va_start_longlong
#define va_arg_sint64	va_arg_longlong
#define va_return_sint64	va_return_longlong
#endif

#endif	/* #if defined USE_LIBFFCALL */
/*
 * Turn callbacks off if they're not implemented
 */
#if defined USE_CALLBACKS
#if ! HAVE_CLOSURES
#undef USE_CALLBACKS
#endif
#endif

/*****************************************
 *				  
 * ffidlopen, ffidlsym, and ffidlclose abstractions
 * of dlopen(), dlsym(), and dlclose().
 */
#if defined(USE_TCL_DLOPEN)

typedef Tcl_LoadHandle ffidl_LoadHandle;
typedef Tcl_FSUnloadFileProc *ffidl_UnloadProc;

#elif defined(USE_TCL_LOADFILE)

typedef Tcl_LoadHandle ffidl_LoadHandle;
typedef Tcl_FSUnloadFileProc *ffidl_UnloadProc;

#else

typedef void *ffidl_LoadHandle;
typedef void *ffidl_UnloadProc;

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#else

#ifndef NO_DLFCN_H
#include <dlfcn.h>

/*
 * In some systems, like SunOS 4.1.3, the RTLD_NOW flag isn't defined
 * and this argument to dlopen must always be 1.  The RTLD_GLOBAL
 * flag is needed on some systems (e.g. SCO and UnixWare) but doesn't
 * exist on others;  if it doesn't exist, set it to 0 so it has no effect.
 */

#ifndef RTLD_NOW
#   define RTLD_NOW 1
#endif

#ifndef RTLD_GLOBAL
#   define RTLD_GLOBAL 0
#endif
#endif	/* NO_DLFCN_H */
#endif	/* _WIN32 */
#endif	/* USE_TCL_DLOPEN */

enum ffidl_load_binding {
  FFIDL_LOAD_BINDING_NONE,
  FFIDL_LOAD_BINDING_NOW,
  FFIDL_LOAD_BINDING_LAZY
};

enum ffidl_load_visibility {
  FFIDL_LOAD_VISIBILITY_NONE,
  FFIDL_LOAD_VISIBILITY_LOCAL,
  FFIDL_LOAD_VISIBILITY_GLOBAL
};

struct ffidl_load_flags {
  enum ffidl_load_binding binding;
  enum ffidl_load_visibility visibility;
};
typedef struct ffidl_load_flags ffidl_load_flags;

/*****************************************
 *				  
 * Functions exported from this file.
 */

EXTERN void *ffidl_pointer_pun (void *p);
EXTERN void *ffidl_copy_bytes (void *dst, void *src, size_t len);
EXTERN int   Ffidl_Init (Tcl_Interp * interp);

/*****************************************
 *
 * Definitions.
 */
/*
 * values for ffidl_type.type
 */
enum ffidl_typecode {
  FFIDL_VOID		=  0,
    FFIDL_INT		=  1,
    FFIDL_FLOAT		=  2,
    FFIDL_DOUBLE	=  3,
#if HAVE_LONG_DOUBLE
    FFIDL_LONGDOUBLE	=  4,
#endif
    FFIDL_UINT8		=  5,
    FFIDL_SINT8		=  6,
    FFIDL_UINT16	=  7,
    FFIDL_SINT16	=  8,
    FFIDL_UINT32	=  9,
    FFIDL_SINT32	= 10,
    FFIDL_UINT64	= 11,
    FFIDL_SINT64	= 12,
    FFIDL_STRUCT	= 13,
    FFIDL_PTR		= 14,	/* integer value pointer */
    FFIDL_PTR_BYTE	= 15,	/* byte array pointer */
    FFIDL_PTR_UTF8	= 16,	/* UTF-8 string pointer */
    FFIDL_PTR_UTF16	= 17,	/* UTF-16 string pointer */
    FFIDL_PTR_VAR	= 18,	/* byte array in variable */
    FFIDL_PTR_OBJ	= 19,	/* Tcl_Obj pointer */
    FFIDL_PTR_PROC	= 20,	/* Pointer to Tcl proc */

/*
 * aliases for unsized type names
 */
#if defined(CHAR_UNSIGNED)
    FFIDL_CHAR	= FFIDL_UINT8,
#else
    FFIDL_CHAR	= FFIDL_SINT8,
#endif

    FFIDL_SCHAR	= FFIDL_SINT8,
    FFIDL_UCHAR	= FFIDL_UINT8,

#if SIZEOF_SHORT == 2
    FFIDL_USHORT	= FFIDL_UINT16,
    FFIDL_SSHORT	= FFIDL_SINT16,
#elif SIZEOF_SHORT == 4
    FFIDL_USHORT	= FFIDL_UINT32,
    FFIDL_SSHORT	= FFIDL_SINT32,
#elif SIZEOF_SHORT == 8
    FFIDL_USHORT	= FFIDL_UINT64,
    FFIDL_SSHORT	= FFIDL_SINT64,
#else
#error "no short type"
#endif

#if SIZEOF_INT == 2
    FFIDL_UINT	= FFIDL_UINT16,
    FFIDL_SINT	= FFIDL_SINT16,
#elif SIZEOF_INT == 4
    FFIDL_UINT	= FFIDL_UINT32,
    FFIDL_SINT	= FFIDL_SINT32,
#elif SIZEOF_INT == 8
    FFIDL_UINT	= FFIDL_UINT64,
    FFIDL_SINT	= FFIDL_SINT64,
#else
#error "no int type"
#endif

#if SIZEOF_LONG == 2
    FFIDL_ULONG	= FFIDL_UINT16,
    FFIDL_SLONG	= FFIDL_SINT16,
#elif SIZEOF_LONG == 4
    FFIDL_ULONG	= FFIDL_UINT32,
    FFIDL_SLONG	= FFIDL_SINT32,
#elif SIZEOF_LONG == 8
    FFIDL_ULONG	= FFIDL_UINT64,
    FFIDL_SLONG	= FFIDL_SINT64,
#else
#error "no long type"
#endif

#if HAVE_LONG_LONG
#if SIZEOF_LONG_LONG == 2
    FFIDL_ULONGLONG	= FFIDL_UINT16,
    FFIDL_SLONGLONG	= FFIDL_SINT16
#elif SIZEOF_LONG_LONG == 4
    FFIDL_ULONGLONG	= FFIDL_UINT32,
    FFIDL_SLONGLONG	= FFIDL_SINT32
#elif SIZEOF_LONG_LONG == 8
    FFIDL_ULONGLONG	= FFIDL_UINT64,
    FFIDL_SLONGLONG	= FFIDL_SINT64
#else
#error "no long long type"
#endif
#endif
};

/*
 * Once more through, decide the alignment and C types
 * for the sized ints
 */

#define ALIGNOF_INT8	1
#define UINT8_T		unsigned char
#define SINT8_T		signed char

#if SIZEOF_SHORT == 2
#define ALIGNOF_INT16	ALIGNOF_SHORT
#define UINT16_T	unsigned short
#define SINT16_T	signed short
#elif SIZEOF_INT == 2
#define ALIGNOF_INT16	ALIGNOF_INT
#define UINT16_T	unsigned int
#define SINT16_T	signed int
#elif SIZEOF_LONG == 2
#define ALIGNOF_INT16	ALIGNOF_LONG
#define UINT16_T	unsigned long
#define SINT16_T	signed long
#else
#error "no 16 bit int"
#endif

#if SIZEOF_SHORT == 4
#define ALIGNOF_INT32	ALIGNOF_SHORT
#define UINT32_T	unsigned short
#define SINT32_T	signed short
#elif SIZEOF_INT == 4
#define ALIGNOF_INT32	ALIGNOF_INT
#define UINT32_T	unsigned int
#define SINT32_T	signed int
#elif SIZEOF_LONG == 4
#define ALIGNOF_INT32	ALIGNOF_LONG
#define UINT32_T	unsigned long
#define SINT32_T	signed long
#else
#error "no 32 bit int"
#endif

#if SIZEOF_SHORT == 8
#define ALIGNOF_INT64	ALIGNOF_SHORT
#define UINT64_T	unsigned short
#define SINT64_T	signed short
#elif SIZEOF_INT == 8
#define ALIGNOF_INT64	ALIGNOF_INT
#define UINT64_T	unsigned int
#define SINT64_T	signed int
#elif SIZEOF_LONG == 8
#define ALIGNOF_INT64	ALIGNOF_LONG
#define UINT64_T	unsigned long
#define SINT64_T	signed long
#elif HAVE_LONG_LONG && SIZEOF_LONG_LONG == 8
#define ALIGNOF_INT64	ALIGNOF_LONG_LONG
#define UINT64_T	unsigned long long
#define SINT64_T	signed long long
#endif

#if defined(ALIGNOF_INT64)
#define HAVE_INT64	1
#endif

#if defined(HAVE_INT64)
#  if defined(TCL_WIDE_INT_IS_LONG)
#    define HAVE_WIDE_INT		0
#    define Ffidl_NewInt64Obj		Tcl_NewLongObj
#    define Ffidl_GetInt64FromObj	Tcl_GetLongFromObj
#    define Ffidl_Int64			long
#  else
#    define HAVE_WIDE_INT		1
#    define Ffidl_NewInt64Obj		Tcl_NewWideIntObj
#    define Ffidl_GetInt64FromObj	Tcl_GetWideIntFromObj
#    define Ffidl_Int64			Tcl_WideInt
#  endif
#endif

/*
 * values for ffidl_type.class
 */
#define FFIDL_ARG		0x001	/* type parser in argument context */
#define FFIDL_RET		0x002	/* type parser in return context */
#define FFIDL_ELT		0x004	/* type parser in element context */
#define FFIDL_CBARG		0x008	/* type parser in callback argument context */
#define FFIDL_CBRET		0x010	/* type parser in callback return context */
#define FFIDL_ALL		(FFIDL_ARG|FFIDL_RET|FFIDL_ELT|FFIDL_CBARG|FFIDL_CBRET)
#define FFIDL_ARGRET		(FFIDL_ARG|FFIDL_RET)
#define FFIDL_GETINT		0x020	/* arg needs an int value */
#define FFIDL_GETDOUBLE		0x040	/* arg needs a double value */
#define FFIDL_GETWIDEINT	0x080	/* arg needs a wideInt value */
#define FFIDL_STATIC_TYPE	0x100	/* do not free this type */

/*
 * Tcl object type used for representing pointers within Tcl.
 *
 * We wrap an existing "expr"-compatible Tcl_ObjType, in order to easily support
 * pointer arithmetic and formatting withing Tcl.  The size of the Tcl_ObjType
 * needs to match the pointer size of the platform: long on LP64, Tcl_WideInt on
 * LLP64 (e.g. WIN64).
 */
#if SIZEOF_VOID_P == SIZEOF_LONG
#  define FFIDL_POINTER_IS_LONG 1
#elif SIZEOF_VOID_P == 8 && defined(HAVE_WIDE_INT)
#  define FFIDL_POINTER_IS_LONG 0
#else
#  error "pointer size not supported"
#endif

#if FFIDL_POINTER_IS_LONG
static Tcl_Obj *Ffidl_NewPointerObj(void *ptr) {
  return Tcl_NewLongObj((long)ptr);
}
static int Ffidl_GetPointerFromObj(Tcl_Interp *interp, Tcl_Obj *obj, void **ptr) {
  int status;
  long l;
  status = Tcl_GetLongFromObj(interp, obj, &l);
  *ptr = (void *)l;
  return status;
}
#  define FFIDL_GETPOINTER FFIDL_GETINT
#else
static Tcl_Obj *Ffidl_NewPointerObj(void *ptr) {
  return Tcl_NewWideIntObj((Tcl_WideInt)ptr);
}
static int Ffidl_GetPointerFromObj(Tcl_Interp *interp, Tcl_Obj *obj, void **ptr) {
  int status;
  Tcl_WideInt w;
  status = Tcl_GetWideIntFromObj(interp, obj, &w);
  *ptr = (void *)w;
  return status;
}
#  define FFIDL_GETPOINTER FFIDL_GETWIDEINT
#endif

/*****************************************
 * Due to an ancient libffi defect, not fixed due to compatibility concerns,
 * return values for types smaller than the architecture's register size need to
 * be treated specially.  This must only be done for return values of integral
 * types, and not for arguments.  The recommended approach is to use a buffer of
 * size ffi_arg for these values, disregarding the type size.
 *
 * The following macros select, for each architecture, the appropriate member of
 * ffidl_value to use for a return value of a particular type.
 */
#define FFIDL_FITS_INTO_ARG(type) FFI_SIZEOF_ARG <= SIZEOF_##type

#define FFIDL_RVALUE_TYPE_INT int
#if USE_LIBFFI && !FFIDL_FITS_INTO_ARG(INT)
#  define FFIDL_RVALUE_WIDENED_TYPE_INT ffi_arg
#else
#  define FFIDL_RVALUE_WIDENED_TYPE_INT FFIDL_RVALUE_TYPE_INT
#endif

#define FFIDL_RVALUE_TYPE_UINT8 UINT8_T
#if USE_LIBFFI && !FFIDL_FITS_INTO_ARG(UINT8)
#  define FFIDL_RVALUE_WIDENED_TYPE_UINT8 ffi_arg
#else
#  define FFIDL_RVALUE_WIDENED_TYPE_UINT8 FFIDL_RVALUE_TYPE_UINT8
#endif

#define FFIDL_RVALUE_TYPE_SINT8 SINT8_T
#if USE_LIBFFI && !FFIDL_FITS_INTO_ARG(SINT8)
#  define FFIDL_RVALUE_WIDENED_TYPE_SINT8 ffi_arg
#else
#  define FFIDL_RVALUE_WIDENED_TYPE_SINT8 FFIDL_RVALUE_TYPE_SINT8
#endif

#define FFIDL_RVALUE_TYPE_UINT16 UINT16_T
#if USE_LIBFFI && !FFIDL_FITS_INTO_ARG(UINT16)
#  define FFIDL_RVALUE_WIDENED_TYPE_UINT16 ffi_arg
#else
#  define FFIDL_RVALUE_WIDENED_TYPE_UINT16 FFIDL_RVALUE_TYPE_UINT16
#endif

#define FFIDL_RVALUE_TYPE_SINT16 SINT16_T
#if USE_LIBFFI && !FFIDL_FITS_INTO_ARG(SINT16)
#  define FFIDL_RVALUE_WIDENED_TYPE_SINT16 ffi_arg
#else
#  define FFIDL_RVALUE_WIDENED_TYPE_SINT16 FFIDL_RVALUE_TYPE_SINT16
#endif

#define FFIDL_RVALUE_TYPE_UINT32 UINT32_T
#if USE_LIBFFI && !FFIDL_FITS_INTO_ARG(UINT32)
#  define FFIDL_RVALUE_WIDENED_TYPE_UINT32 ffi_arg
#else
#  define FFIDL_RVALUE_WIDENED_TYPE_UINT32 FFIDL_RVALUE_TYPE_UINT32
#endif

#define FFIDL_RVALUE_TYPE_SINT32 SINT32_T
#if USE_LIBFFI && !FFIDL_FITS_INTO_ARG(SINT32)
#  define FFIDL_RVALUE_WIDENED_TYPE_SINT32 ffi_arg
#else
#  define FFIDL_RVALUE_WIDENED_TYPE_SINT32 FFIDL_RVALUE_TYPE_SINT32
#endif

#if HAVE_INT64
#  define FFIDL_RVALUE_TYPE_UINT64 UINT64_T
#  if USE_LIBFFI && !FFIDL_FITS_INTO_ARG(UINT64)
#    define FFIDL_RVALUE_WIDENED_TYPE_UINT64 ffi_arg
#  else
#    define FFIDL_RVALUE_WIDENED_TYPE_UINT64 FFIDL_RVALUE_TYPE_UINT64
#  endif

#  define FFIDL_RVALUE_TYPE_SINT64 SINT64_T
#  if USE_LIBFFI && !FFIDL_FITS_INTO_ARG(SINT64)
#    define FFIDL_RVALUE_WIDENED_TYPE_SINT64 ffi_arg
#  else
#    define FFIDL_RVALUE_WIDENED_TYPE_SINT64 FFIDL_RVALUE_TYPE_SINT64
#  endif
#endif	/* HAVE_INT64 */

/* Only integral types are affected, see above comment. */
#define FFIDL_RVALUE_TYPE_FLOAT float
#define FFIDL_RVALUE_WIDENED_TYPE_FLOAT FFIDL_RVALUE_TYPE_FLOAT
#define FFIDL_RVALUE_TYPE_DOUBLE double
#define FFIDL_RVALUE_WIDENED_TYPE_DOUBLE FFIDL_RVALUE_TYPE_DOUBLE
#if HAVE_LONG_DOUBLE
#  define FFIDL_RVALUE_TYPE_LONGDOUBLE long double
#  define FFIDL_RVALUE_WIDENED_TYPE_LONGDOUBLE FFIDL_RVALUE_TYPE_LONGDOUBLE
#endif

#define FFIDL_RVALUE_TYPE_PTR void *
#define FFIDL_RVALUE_WIDENED_TYPE_PTR FFIDL_RVALUE_TYPE_PTR

#define FFIDL_RVALUE_TYPE_STRUCT void *
#define FFIDL_RVALUE_WIDENED_TYPE_STRUCT FFIDL_RVALUE_TYPE_STRUCT

#define QUOTECONCAT(a, b) a ## b
#define CONCAT(a, b) QUOTECONCAT(a, b)
#define FFIDL_RVALUE_TYPE(type)   CONCAT(FFIDL_RVALUE_TYPE_,   type)
#define FFIDL_RVALUE_WIDENED_TYPE(type)  CONCAT(FFIDL_RVALUE_WIDENED_TYPE_,   type)
/* Retrieve and cast a widened return value to the return value. */
#define FFIDL_RVALUE_PEEK_UNWIDEN(type, rvalue) ((FFIDL_RVALUE_TYPE(type))*(FFIDL_RVALUE_WIDENED_TYPE(type) *)rvalue)
/* Cast a return value to the return widened return value, put it at dst. */
#define FFIDL_RVALUE_POKE_WIDENED(type, dst, src) (*(FFIDL_RVALUE_WIDENED_TYPE(type) *)dst = (FFIDL_RVALUE_TYPE(type))src)


/*****************************************
 *
 * Type definitions for ffidl.
 */
/*
 * forward declarations.
 */
typedef enum ffidl_typecode ffidl_typecode;
typedef union ffidl_value ffidl_value;
typedef union ffidl_tclobj_value ffidl_tclobj_value;
typedef struct ffidl_type ffidl_type;
typedef struct ffidl_client ffidl_client;
typedef struct ffidl_cif ffidl_cif;
typedef struct ffidl_callout ffidl_callout;
typedef struct ffidl_callback ffidl_callback;
typedef struct ffidl_closure ffidl_closure;
typedef struct ffidl_lib ffidl_lib;

/*
 * Can hold the (C) values extracted from Tcl_Objs, as specified by the type's
 * FFIDL_GETINT, FFIDL_GETDOUBLE, FFIDL_GETWIDEINT.
 */
union ffidl_tclobj_value {
  double v_double;
  long v_long;
#if HAVE_INT64
  Ffidl_Int64 v_wideint;
#endif
};

/*
 * The ffidl_value structure contains a union used
 * for converting to/from Tcl type.
 */
union ffidl_value {
  int v_int;
  float v_float;
  double v_double;
#if HAVE_LONG_DOUBLE
  long double v_longdouble;
#endif
  UINT8_T v_uint8;
  SINT8_T v_sint8;
  UINT16_T v_uint16;
  SINT16_T v_sint16;
  UINT32_T v_uint32;
  SINT32_T v_sint32;
#if HAVE_INT64
  UINT64_T v_uint64;
  SINT64_T v_sint64;
#endif
  void *v_struct;
  void *v_pointer;
#if USE_LIBFFI
  ffi_arg v_ffi_arg;
#endif
};

/*
 * The ffidl_type structure contains a type code, a class,
 * the size of the type, the structure element alignment of
 * the class, and a pointer to the underlying ffi_type.
 */
struct ffidl_type {
   int refs;			/* Reference counting */
   size_t size;			/* Type's size */
   ffidl_typecode typecode;	/* Type identifier */
   unsigned short class;	/* Type's properties */
   unsigned short alignment;	/* Type's alignment */
   unsigned short nelts;	/* Number of elements */
   ffidl_type **elements;	/* Pointer to element types */
#if USE_LIBFFI
   ffi_type *lib_type;		/* libffi's type data */
#elif USE_LIBFFCALL
   enum __AVtype lib_type;	/* ffcall's type data */
   int splittable;
#endif
};

/*
 * The ffidl_client contains
 * a hashtable for ffidl::typedef definitions,
 * a hashtable for ffidl::callout definitions,
 * a hashtable for cif's keyed by signature,
 * a hashtable of libs loaded by ffidl::symbol,
 * a hashtable of callbacks keyed by proc name
 */
struct ffidl_client {
  Tcl_HashTable types;
  Tcl_HashTable cifs;
  Tcl_HashTable callouts;
  Tcl_HashTable libs;
  Tcl_HashTable callbacks;
};

/*
 * The ffidl_cif structure contains an ffi_cif,
 * an array of ffidl_types used to construct the
 * cif and convert arguments, and an array of void*
 * used to pass converted arguments into ffi_call.
 */
struct ffidl_cif {
   int refs;		   /* Reference counting. */
   ffidl_client *client;   /* Backpointer to the ffidl_client. */
   int protocol;	   /* Calling convention. */
   ffidl_type *rtype;	   /* Type of return value. */
   int argc;		   /* Number of arguments. */
   ffidl_type **atypes;	   /* Type of each argument. */
#if USE_LIBFFI
   ffi_type **lib_atypes;	/* Pointer to storage area for libffi's internal
				 * argument types. */
   ffi_cif lib_cif;		/* Libffi's internal data. */
#endif
};

/*
 * The ffidl_callout contains a cif pointer,
 * a function address, the ffidl_client
 * which defined the callout, and a usage
 * string.
 */
struct ffidl_callout {
  ffidl_cif *cif;
  void (*fn)(void);
  ffidl_client *client;
  void *ret;		   /* Where to store the return value. */
  void **args;		   /* Where to store each of the arguments' values. */
  char *usage;
#if USE_LIBFFI && USE_LIBFFI_RAW_API
  int use_raw_api;		/* Whether to use libffi's raw API. */
#endif
};

#if USE_CALLBACKS
/*
 * The ffidl_closure contains a ffi_closure structure,
 * a Tcl_Interp pointer, and a pointer to the callback binding.
 */
struct ffidl_closure {
#if USE_LIBFFI
   ffi_closure *lib_closure;	/* Points to the writtable part of the closure. */
   void *executable;		/* Points to the executable address of the closure. */
#elif USE_LIBFFCALL
   callback_t lib_closure;
#endif
};
/*
 * The ffidl_callback binds a ffidl_cif pointer to
 * a Tcl proc name, it defines the signature of the
 * c function call to the Tcl proc.
 */
struct ffidl_callback {
  ffidl_cif *cif;
  int cmdc;			/* Number of command prefix words. */
  Tcl_Obj **cmdv;		/* Command prefix Tcl_Objs. */
  Tcl_Interp *interp;
  ffidl_closure closure;
#if USE_LIBFFI_RAW_API
  int use_raw_api;		/* Whether to use libffi's raw API. */
  ptrdiff_t *offsets;		/* Raw argument offsets. */
#endif
};
#endif

struct ffidl_lib {
  ffidl_LoadHandle loadHandle;
  ffidl_UnloadProc unloadProc;
};

/*****************************************
 *
 * Data defined in this file.
 * In addition to the version string above
 */

static const Tcl_ObjType *ffidl_bytearray_ObjType;
static const Tcl_ObjType *ffidl_int_ObjType;
#if HAVE_WIDE_INT
static const Tcl_ObjType *ffidl_wideInt_ObjType;
#endif
static const Tcl_ObjType *ffidl_double_ObjType;

/*
 * base types, the ffi base types and some additional bits.
 *
 * NOTE: we no longer set the "libtype" field here, because MSVC forbids
 * initializing a static data pointer with the address of a data object declared
 * with the dllimport attribute, as is the case such with libffi's types.  See
 * https://docs.microsoft.com/en-us/cpp/c-language/rules-and-limitations-for-dllimport-dllexport
 * for details.  Instead, we initialize it in the client_alloc procedure.
 */
#define init_type(size,type,class,alignment) { 1/*refs*/, size, type, class|FFIDL_STATIC_TYPE, alignment, 0/*nelts*/, 0/*elements*/, 0/*libtype*/}

/* NOTE: remember to update the "initialize types" section in client_alloc(). */
static ffidl_type ffidl_type_void = init_type(0, FFIDL_VOID, FFIDL_RET|FFIDL_CBRET, 0);
static ffidl_type ffidl_type_char = init_type(SIZEOF_CHAR, FFIDL_CHAR, FFIDL_ALL|FFIDL_GETINT, ALIGNOF_CHAR);
static ffidl_type ffidl_type_schar = init_type(SIZEOF_CHAR, FFIDL_SCHAR, FFIDL_ALL|FFIDL_GETINT, ALIGNOF_CHAR);
static ffidl_type ffidl_type_uchar = init_type(SIZEOF_CHAR, FFIDL_UCHAR, FFIDL_ALL|FFIDL_GETINT, ALIGNOF_CHAR);
static ffidl_type ffidl_type_sshort = init_type(SIZEOF_SHORT, FFIDL_SSHORT, FFIDL_ALL|FFIDL_GETINT, ALIGNOF_SHORT);
static ffidl_type ffidl_type_ushort = init_type(SIZEOF_SHORT, FFIDL_USHORT, FFIDL_ALL|FFIDL_GETINT, ALIGNOF_SHORT);
static ffidl_type ffidl_type_sint = init_type(SIZEOF_INT, FFIDL_SINT, FFIDL_ALL|FFIDL_GETINT, ALIGNOF_INT);
static ffidl_type ffidl_type_uint = init_type(SIZEOF_INT, FFIDL_UINT, FFIDL_ALL|FFIDL_GETINT, ALIGNOF_INT);
#if SIZEOF_LONG == 8
static ffidl_type ffidl_type_slong = init_type(SIZEOF_LONG, FFIDL_SLONG, FFIDL_ALL|FFIDL_GETWIDEINT, ALIGNOF_LONG);
static ffidl_type ffidl_type_ulong = init_type(SIZEOF_LONG, FFIDL_ULONG, FFIDL_ALL|FFIDL_GETWIDEINT, ALIGNOF_LONG);
#else
static ffidl_type ffidl_type_slong = init_type(SIZEOF_LONG, FFIDL_SLONG, FFIDL_ALL|FFIDL_GETINT, ALIGNOF_LONG);
static ffidl_type ffidl_type_ulong = init_type(SIZEOF_LONG, FFIDL_ULONG, FFIDL_ALL|FFIDL_GETINT, ALIGNOF_LONG);
#endif
#if HAVE_LONG_LONG
static ffidl_type ffidl_type_slonglong = init_type(SIZEOF_LONG_LONG, FFIDL_SLONGLONG, FFIDL_ALL|FFIDL_GETWIDEINT, ALIGNOF_LONG_LONG);
static ffidl_type ffidl_type_ulonglong = init_type(SIZEOF_LONG_LONG, FFIDL_ULONGLONG, FFIDL_ALL|FFIDL_GETWIDEINT, ALIGNOF_LONG_LONG);
#endif
static ffidl_type ffidl_type_float = init_type(SIZEOF_FLOAT, FFIDL_FLOAT, FFIDL_ALL|FFIDL_GETDOUBLE, ALIGNOF_FLOAT);
static ffidl_type ffidl_type_double = init_type(SIZEOF_DOUBLE, FFIDL_DOUBLE, FFIDL_ALL|FFIDL_GETDOUBLE, ALIGNOF_DOUBLE);
#if HAVE_LONG_DOUBLE
static ffidl_type ffidl_type_longdouble = init_type(SIZEOF_LONG_DOUBLE, FFIDL_LONGDOUBLE, FFIDL_ALL|FFIDL_GETDOUBLE, ALIGNOF_LONG_DOUBLE);
#endif
static ffidl_type ffidl_type_sint8 = init_type(1, FFIDL_SINT8, FFIDL_ALL|FFIDL_GETINT, ALIGNOF_INT8);
static ffidl_type ffidl_type_uint8 = init_type(1, FFIDL_UINT8, FFIDL_ALL|FFIDL_GETINT, ALIGNOF_INT8);
static ffidl_type ffidl_type_sint16 = init_type(2, FFIDL_SINT16, FFIDL_ALL|FFIDL_GETINT, ALIGNOF_INT16);
static ffidl_type ffidl_type_uint16 = init_type(2, FFIDL_UINT16, FFIDL_ALL|FFIDL_GETINT, ALIGNOF_INT16);
static ffidl_type ffidl_type_sint32 = init_type(4, FFIDL_SINT32, FFIDL_ALL|FFIDL_GETINT, ALIGNOF_INT32);
static ffidl_type ffidl_type_uint32 = init_type(4, FFIDL_UINT32, FFIDL_ALL|FFIDL_GETINT, ALIGNOF_INT32);
#if HAVE_INT64
static ffidl_type ffidl_type_sint64 = init_type(8, FFIDL_SINT64, FFIDL_ALL|FFIDL_GETWIDEINT, ALIGNOF_INT64);
static ffidl_type ffidl_type_uint64 = init_type(8, FFIDL_UINT64, FFIDL_ALL|FFIDL_GETWIDEINT, ALIGNOF_INT64);
#endif
static ffidl_type ffidl_type_pointer       = init_type(SIZEOF_VOID_P, FFIDL_PTR,       FFIDL_ALL|FFIDL_GETPOINTER,           ALIGNOF_VOID_P);
static ffidl_type ffidl_type_pointer_obj   = init_type(SIZEOF_VOID_P, FFIDL_PTR_OBJ,   FFIDL_ARGRET|FFIDL_CBARG|FFIDL_CBRET, ALIGNOF_VOID_P);
static ffidl_type ffidl_type_pointer_utf8  = init_type(SIZEOF_VOID_P, FFIDL_PTR_UTF8,  FFIDL_ARGRET|FFIDL_CBARG,             ALIGNOF_VOID_P);
static ffidl_type ffidl_type_pointer_utf16 = init_type(SIZEOF_VOID_P, FFIDL_PTR_UTF16, FFIDL_ARGRET|FFIDL_CBARG,             ALIGNOF_VOID_P);
static ffidl_type ffidl_type_pointer_byte  = init_type(SIZEOF_VOID_P, FFIDL_PTR_BYTE,  FFIDL_ARG,                            ALIGNOF_VOID_P);
static ffidl_type ffidl_type_pointer_var   = init_type(SIZEOF_VOID_P, FFIDL_PTR_VAR,   FFIDL_ARG,                            ALIGNOF_VOID_P);
#if USE_CALLBACKS
static ffidl_type ffidl_type_pointer_proc = init_type(SIZEOF_VOID_P, FFIDL_PTR_PROC, FFIDL_ARG, ALIGNOF_VOID_P);
#endif

/*****************************************
 *
 * Functions defined in this file.
 */

/*
 * Dynamic loading
 */
#if !defined(USE_TCL_DLOPEN) && !defined(USE_TCL_LOADFILE)
static int ffidlsymfallback(ffidl_LoadHandle handle,
			    char *nativeSymbolName,
			    void **address,
			    char *error)
{
  int status = TCL_OK;
#if defined(_WIN32)
  /*
   * Ack, what about data?  I guess they're not particular,
   * some windows headers declare data as dll import, eg
   * vc98/include/math.h: _CRTIMP extern double _HUGE;
   */
  *address = GetProcAddress(handle, nativeSymbolName);
  if (!*address) {
    unknown = "unknown error";
    status = TCL_ERROR;
  }
#else
  dlerror();			/* clear any old error. */
  *address = dlsym(handle, nativeSymbolName);
  error = dlerror();
  if (error) {
    status = TCL_ERROR;
  }
#endif /* _WIN32 */
  return status;
}
#endif /* !USE_TCL_DLOPEN && !USE_TCL_LOADFILE */

static int ffidlsym(Tcl_Interp *interp,
		    ffidl_LoadHandle handle,
		    Tcl_Obj *symbolNameObj,
		    void **address)
{
  int status = TCL_OK;
  char *error = NULL;
  char *symbolName = NULL;
  char *nativeSymbolName = NULL;
  Tcl_DString nds;

  symbolName = Tcl_GetString(symbolNameObj);
  nativeSymbolName = Tcl_UtfToExternalDString(NULL, symbolName, -1, &nds);
#if defined(USE_TCL_DLOPEN)
  *address = TclpFindSymbol(interp, (Tcl_LoadHandle)handle, nativeSymbolName);
  if (!*address) {
    error = "TclpFindSymbol() failed";
    status = TCL_ERROR;
  }
#elif defined(USE_TCL_LOADFILE)
  *address = Tcl_FindSymbol(interp, (Tcl_LoadHandle)handle, nativeSymbolName);
  if (!*address) {
    error = "Tcl_FindSymbol() failed";
    status = TCL_ERROR;
  }
#else
  status = ffidlsymfallback(handle, nativeSymbolName, address, error);
  if (status != TCL_OK) {
    /*
     * Some platforms still add an underscore to the beginning of symbol
     * names.  If we can't find a name without an underscore, try again
     * with the underscore.
     */
    char *newNativeSymbolName = NULL;
    Tcl_DString uds;
    char *ignoreerror = NULL;

    Tcl_DStringInit(&uds);
    Tcl_DStringAppend(&uds, "_", 1);
    Tcl_DStringAppend(&uds, Tcl_DStringValue(&nds), Tcl_DStringLength(&nds));
    newNativeSymbolName = Tcl_DStringValue(&uds);

    status = ffidlsymfallback(handle, newNativeSymbolName, address, ignoreerror);

    Tcl_DStringFree(&uds);
  }
#endif /* USE_TCL_{LOADFILE,DLOPEN} */

  Tcl_DStringFree(&nds);

  if (error) {
    Tcl_AppendResult(interp, "couldn't find symbol \"", symbolName, "\" : ", error, NULL);
  }

  return status;
}

static int ffidlopen(Tcl_Interp *interp,
		     Tcl_Obj *libNameObj,
		     ffidl_load_flags flags,
		     ffidl_LoadHandle *handle,
		     ffidl_UnloadProc *unload)
{
  int status = TCL_OK;
#if defined(USE_TCL_DLOPEN)
  if (flags.binding != FFIDL_LOAD_BINDING_NONE ||
      flags.visibility != FFIDL_LOAD_VISIBILITY_NONE) {
    char *libraryName = NULL;
    libraryName = Tcl_GetString(libNameObj);
    Tcl_AppendResult(interp, "couldn't load file \"", libraryName, "\" : ",
		     "loading flags are not supported with USE_TCL_DLOPEN configuration",
		     (char *) NULL);
    status = TCL_ERROR;
  } else {
    status = TclpDlopen(interp, libNameObj, handle, unload);
  }
#elif defined(USE_TCL_LOADFILE)
  {
    int tclflags =
      (flags.visibility == FFIDL_LOAD_VISIBILITY_GLOBAL? TCL_LOAD_GLOBAL : 0) |
      (flags.binding == FFIDL_LOAD_BINDING_LAZY? TCL_LOAD_LAZY : 0);
    if (Tcl_LoadFile(interp, libNameObj, NULL, tclflags, NULL, handle) != TCL_OK) {
      status = TCL_ERROR;
    }
  }
  *unload = NULL;
#else
  Tcl_DString ds;
  char *libraryName = NULL;
  char *nativeLibraryName = NULL;
  char *error = NULL;

  libraryName = Tcl_GetString(libNameObj);
  nativeLibraryName = Tcl_UtfToExternalDString(NULL, libraryName, -1, &ds);
  nativeLibraryName = strlen(nativeLibraryName) ? nativeLibraryName : NULL;

#if defined(_WIN32)
  if (flags.binding != FFIDL_LOAD_BINDING_NONE ||
      flags.visibility != FFIDL_LOAD_VISIBILITY_NONE) {
    error = "loading flags are not supported under windows";
    status = TCL_ERROR;
  } else {
    *handle = LoadLibraryA(nativeLibraryName);
    if (!*handle) {
      error = "unknown error";
    }
  }
#else
  {
    int dlflags =
      (flags.visibility == FFIDL_LOAD_VISIBILITY_LOCAL? RTLD_LOCAL : RTLD_GLOBAL) |
      (flags.binding == FFIDL_LOAD_BINDING_LAZY? RTLD_LAZY : RTLD_NOW);
    *handle = dlopen(nativeLibraryName, dlflags);
    /* dlopen returns NULL when it fails. */
    if (!*handle) {
      error = dlerror();
    }
  }
#endif

  if (*handle == NULL) {
    Tcl_AppendResult(interp, "couldn't load file \"", libraryName, "\" : ",
		     error, (char *) NULL);
    status = TCL_ERROR;
  } else {
    *unload = NULL;
  }

  Tcl_DStringFree(&ds);
#endif

  return status;
}

static int ffidlclose(Tcl_Interp *interp,
		      char *libraryName,
		      ffidl_LoadHandle handle,
		      ffidl_UnloadProc unload)
{
  int status = TCL_OK;
  const char *error = NULL;
#if defined(USE_TCL_DLOPEN)
  /* NOTE: no error reporting. */
  ((Tcl_FSUnloadFileProc*)unload)((Tcl_LoadHandle)handle);
#elif defined(USE_TCL_LOADFILE)
  status = Tcl_FSUnloadFile(interp, (Tcl_LoadHandle)handle);
  if (status != TCL_OK) {
    error = Tcl_GetStringResult(interp);
  }
#else
#if defined(_WIN32)
  if (!FreeLibrary(handle)) {
    status = TCL_ERROR;
    error = "unknown error";
  }
#else
  if (dlclose(handle)) {
    status = TCL_ERROR;
    error = dlerror();
  }
#endif
#endif
  if (status != TCL_OK) {
    Tcl_AppendResult(interp, "couldn't unload lib \"", libraryName, "\": ",
		     error, (char *) NULL);
  }
  return status;
}


/*
 * hash table management
 */
/* define a hashtable entry */
static void entry_define(Tcl_HashTable *table, char *name, void *datum)
{
  int dummy;
  Tcl_SetHashValue(Tcl_CreateHashEntry(table,name,&dummy), datum);
}
/* lookup an existing entry */
static void *entry_lookup(Tcl_HashTable *table, char *name)
{
  Tcl_HashEntry *entry = Tcl_FindHashEntry(table,name);
  return entry ? Tcl_GetHashValue(entry) : NULL;
}
/* find an entry by it's hash value */
static Tcl_HashEntry *entry_find(Tcl_HashTable *table, void *datum)
{
  Tcl_HashSearch search;
  Tcl_HashEntry *entry = Tcl_FirstHashEntry(table, &search);
  while (entry != NULL) {
    if (Tcl_GetHashValue(entry) == datum)
      return entry;
    entry = Tcl_NextHashEntry(&search);
  }
  return NULL;
}
/*
 * type management
 */
/* define a new type */
static void type_define(ffidl_client *client, char *tname, ffidl_type *ttype)
{
  entry_define(&client->types,tname,(void*)ttype);
}
/* lookup an existing type */
static ffidl_type *type_lookup(ffidl_client *client, char *tname)
{
  return entry_lookup(&client->types,tname);
}
/* find a type by it's ffidl_type */
/*
static Tcl_HashEntry *type_find(ffidl_client *client, ffidl_type *type)
{
  return entry_find(&client->types,(void *)type);
}
*/

/* Determine correct binary formats */
#if defined WORDS_BIGENDIAN
#define FFIDL_WIDEINT_FORMAT	"W"
#define FFIDL_INT_FORMAT	"I"
#define FFIDL_SHORT_FORMAT	"S"
#else
#define FFIDL_WIDEINT_FORMAT	"w"
#define FFIDL_INT_FORMAT	"i"
#define FFIDL_SHORT_FORMAT	"s"
#endif

/* build a binary format string */
static int type_format(Tcl_Interp *interp, ffidl_type *type, size_t *offset)
{
  int i;
  char buff[128];
  /* Handle void case. */
  if (type->size == 0) {
    Tcl_SetResult(interp, "", TCL_STATIC);
    return TCL_OK;
  }
  /* Insert alignment padding */
  while ((*offset % type->alignment) != 0) {
    Tcl_AppendResult(interp, "x", NULL);
    *offset += 1;
  }
  switch (type->typecode) {
  case FFIDL_INT:
  case FFIDL_UINT8:
  case FFIDL_SINT8:
  case FFIDL_UINT16:
  case FFIDL_SINT16:
  case FFIDL_UINT32:
  case FFIDL_SINT32:
#if HAVE_INT64
  case FFIDL_UINT64:
  case FFIDL_SINT64:
#endif
  case FFIDL_PTR:
  case FFIDL_PTR_BYTE:
  case FFIDL_PTR_OBJ:
  case FFIDL_PTR_UTF8:
  case FFIDL_PTR_UTF16:
  case FFIDL_PTR_VAR:
  case FFIDL_PTR_PROC:
    switch (type->size) {
    case sizeof(Ffidl_Int64):
      *offset += 8;
      Tcl_AppendResult(interp, FFIDL_WIDEINT_FORMAT, NULL);
      return TCL_OK;
    case sizeof(int):
      *offset += 4;
      Tcl_AppendResult(interp, FFIDL_INT_FORMAT, NULL);
      return TCL_OK;
    case sizeof(short):
      *offset += 2;
      Tcl_AppendResult(interp, FFIDL_SHORT_FORMAT, NULL);
      return TCL_OK;
    case sizeof(char):
      *offset += 1;
      Tcl_AppendResult(interp, "c", NULL);
      return TCL_OK;
    default:
      *offset += type->size;
      sprintf(buff, "c%lu", (long)(type->size));
      Tcl_AppendResult(interp, buff, NULL);
      return TCL_OK;
    }
  case FFIDL_FLOAT:
  case FFIDL_DOUBLE:
#if HAVE_LONG_DOUBLE
  case FFIDL_LONGDOUBLE:
#endif
    if (type->size == sizeof(double)) {
      *offset += 8;
      Tcl_AppendResult(interp, "d", NULL);
      return TCL_OK;
    } else if (type->size == sizeof(float)) {
      *offset += 4;
      Tcl_AppendResult(interp, "f", NULL);
      return TCL_OK;
    } else {
      *offset += type->size;
      sprintf(buff, "c%lu", (long)(type->size));
      Tcl_AppendResult(interp, buff, NULL);
      return TCL_OK;
    }
  case FFIDL_STRUCT:
    for (i = 0; i < type->nelts; i += 1)
      if (type_format(interp, type->elements[i], offset) != TCL_OK)
	return TCL_ERROR;
    /* Insert tail padding */
    while (*offset < type->size) {
      Tcl_AppendResult(interp, "x", NULL);
      *offset += 1;
    }
    return TCL_OK;
  default:
    sprintf(buff, "cannot format ffidl_type: %d", type->typecode);
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, buff, NULL);
    return TCL_ERROR;
  }
}
static ffidl_type *type_alloc(ffidl_client *client, int nelts)
{
  ffidl_type *newtype;
  newtype = (ffidl_type *)Tcl_Alloc(sizeof(ffidl_type)
				  +nelts*sizeof(ffidl_type*)
#if USE_LIBFFI
				  +sizeof(ffi_type)+(nelts+1)*sizeof(ffi_type *)
#endif
				  );
  if (newtype == NULL) {
    return NULL;
  }
  /* initialize aggregate type */
  newtype->size = 0;
  newtype->typecode = FFIDL_STRUCT;
  newtype->class = FFIDL_ALL;
  newtype->alignment = 0;
  newtype->refs = 0;
  newtype->nelts = nelts;
  newtype->elements = (ffidl_type **)(newtype+1);
#if USE_LIBFFI
  newtype->lib_type = (ffi_type *)(newtype->elements+nelts);
  newtype->lib_type->size = 0;
  newtype->lib_type->alignment = 0;
  newtype->lib_type->type = FFI_TYPE_STRUCT;
  newtype->lib_type->elements = (ffi_type **)(newtype->lib_type+1);
#endif
  return newtype;
}
/* free a type */
static void type_free(ffidl_type *type)
{
  Tcl_Free((void *)type);
}
/* maintain reference counts on type's */
static void type_inc_ref(ffidl_type *type)
{
  type->refs += 1;
}
static void type_dec_ref(ffidl_type *type)
{
  if (--type->refs == 0) {
    type_free(type);
  }
}
/* prep a type for use by the library */
static int type_prep(ffidl_type *type)
{
#if USE_LIBFFI
  ffi_cif cif;
  int i;
  for (i = 0; i < type->nelts; i += 1)
    type->lib_type->elements[i] = type->elements[i]->lib_type;
  type->lib_type->elements[i] = NULL;
  /* try out new type in a temporary cif, which should set size and alignment */
  if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 0, type->lib_type, NULL) != FFI_OK)
    return TCL_ERROR;
  if (type->size != type->lib_type->size) {
    fprintf(stderr, "ffidl disagrees with libffi about aggregate size of type %u! %lu != %lu\n", type->typecode, (long)(type->size), (long)(type->lib_type->size));
  }
  if (type->alignment != type->lib_type->alignment) {
    fprintf(stderr, "ffidl disagrees with libffi about aggregate alignment of type  %u! %hu != %hu\n", type->typecode, type->alignment, type->lib_type->alignment);
  }
#elif USE_LIBFFCALL
  /* decide if the structure can be split into parts for register return */
  /* Determine whether a struct type is word-splittable, i.e. whether each of
   * its components fit into a register.
   * These macros are adapted from ffcall-1.6/avcall/avcall.h/av_word_splittable*()
   */
#define ffidl_word_splittable_1(slot1)  \
  (__ffidl_offset1(slot1)/sizeof(__avword) == (__ffidl_offset1(slot1)+__ffidl_sizeof(slot1)-1)/sizeof(__avword))
#define ffidl_word_splittable_2(slot1,slot2)  \
  ((__ffidl_offset1(slot1)/sizeof(__avword) == (__ffidl_offset1(slot1)+__ffidl_sizeof(slot1)-1)/sizeof(__avword)) \
   && (__ffidl_offset2(slot1,slot2)/sizeof(__avword) == (__ffidl_offset2(slot1,slot2)+__ffidl_sizeof(slot2)-1)/sizeof(__avword)) \
  )
#define ffidl_word_splittable_3(slot1,slot2,slot3)  \
  ((__ffidl_offset1(slot1)/sizeof(__avword) == (__ffidl_offset1(slot1)+__ffidl_sizeof(slot1)-1)/sizeof(__avword)) \
   && (__ffidl_offset2(slot1,slot2)/sizeof(__avword) == (__ffidl_offset2(slot1,slot2)+__ffidl_sizeof(slot2)-1)/sizeof(__avword)) \
   && (__ffidl_offset3(slot1,slot2,slot3)/sizeof(__avword) == (__ffidl_offset3(slot1,slot2,slot3)+__ffidl_sizeof(slot3)-1)/sizeof(__avword)) \
  )
#define ffidl_word_splittable_4(slot1,slot2,slot3,slot4)  \
  ((__ffidl_offset1(slot1)/sizeof(__avword) == (__ffidl_offset1(slot1)+__ffidl_sizeof(slot1)-1)/sizeof(__avword)) \
   && (__ffidl_offset2(slot1,slot2)/sizeof(__avword) == (__ffidl_offset2(slot1,slot2)+__ffidl_sizeof(slot2)-1)/sizeof(__avword)) \
   && (__ffidl_offset3(slot1,slot2,slot3)/sizeof(__avword) == (__ffidl_offset3(slot1,slot2,slot3)+__ffidl_sizeof(slot3)-1)/sizeof(__avword)) \
   && (__ffidl_offset4(slot1,slot2,slot3,slot4)/sizeof(__avword) == (__ffidl_offset4(slot1,slot2,slot3,slot4)+__ffidl_sizeof(slot4)-1)/sizeof(__avword)) \
  )
#define __ffidl_offset1(slot1)  \
  0
#define __ffidl_offset2(slot1,slot2)  \
  ((__ffidl_offset1(slot1)+__ffidl_sizeof(slot1)+__ffidl_alignof(slot2)-1) & -(long)__ffidl_alignof(slot2))
#define __ffidl_offset3(slot1,slot2,slot3)  \
  ((__ffidl_offset2(slot1,slot2)+__ffidl_sizeof(slot2)+__ffidl_alignof(slot3)-1) & -(long)__ffidl_alignof(slot3))
#define __ffidl_offset4(slot1,slot2,slot3,slot4)  \
  ((__ffidl_offset3(slot1,slot2,slot3)+__ffidl_sizeof(slot3)+__ffidl_alignof(slot4)-1) & -(long)__ffidl_alignof(slot4))
#define __ffidl_alignof(slot) slot->alignment
#define __ffidl_sizeof(slot) slot->size
  if (type->size <= sizeof(__avword))
    type->splittable = 1;
  else if (type->size > 2*sizeof(__avword))
    type->splittable = 0;
  else if (type->nelts == 1)
    type->splittable = ffidl_word_splittable_1(type->elements[0]);
  else if (type->nelts == 2)
    type->splittable = ffidl_word_splittable_2(type->elements[0],type->elements[1]);
  else if (type->nelts == 3)
    type->splittable = ffidl_word_splittable_3(type->elements[0],type->elements[1],type->elements[2]);
  else if (type->nelts == 4)
    type->splittable = ffidl_word_splittable_4(type->elements[0],type->elements[1],type->elements[2],type->elements[3]);
  else
    type->splittable = 0;
#endif
  return TCL_OK;
}
/*
 * cif, ie call signature, management.
 */
/* define a new cif */
static void cif_define(ffidl_client *client, char *cname, ffidl_cif *cif)
{
  entry_define(&client->cifs,cname,(void*)cif);
}
/* lookup an existing cif */
static ffidl_cif *cif_lookup(ffidl_client *client, char *cname)
{
  return entry_lookup(&client->cifs,cname);
}
/* find a cif by it's ffidl_cif */
static Tcl_HashEntry *cif_find(ffidl_client *client, ffidl_cif *cif)
{
  return entry_find(&client->cifs,(void *)cif);
}
/* allocate a cif and its parts */
static ffidl_cif *cif_alloc(ffidl_client *client, int argc)
{
  /* allocate storage for:
     the ffidl_cif,
     the argument ffi_type pointers,
     the argument ffidl_types,
     the argument values,
     and the argument value pointers. */
  ffidl_cif *cif;
  cif = (ffidl_cif *)Tcl_Alloc(sizeof(ffidl_cif)
			       +argc*sizeof(ffidl_type*) /* atypes */
#if USE_LIBFFI
			       +argc*sizeof(ffi_type*) /* lib_atypes */
#endif /* USE_LIBFFI */
    );
  if (cif == NULL) {
    return NULL;
  }
  /* initialize the cif */
  cif->refs = 0;
  cif->client = client;
  cif->argc = argc;
  cif->atypes = (ffidl_type **)(cif+1);
#if USE_LIBFFI
  cif->lib_atypes = (ffi_type **)(cif->atypes+argc);
#endif /* USE_LIBFFI */
  return cif;
}
/* free a cif */
void cif_free(ffidl_cif *cif)
{
  Tcl_Free((void *)cif);
}
/* maintain reference counts on cif's */
static void cif_inc_ref(ffidl_cif *cif)
{
  cif->refs += 1;
}
static void cif_dec_ref(ffidl_cif *cif)
{
  if (--cif->refs == 0) {
    Tcl_DeleteHashEntry(cif_find(cif->client, cif));
    cif_free(cif);
  }
}
/**
 * Parse an argument or return type specification.
 *
 * After invoking this function, @p type points to the @c ffidl_type
 * corresponding to @p typename, and @p argp is initialized
 *
 * @param[in] interp Tcl interpreter.
 * @param[in] client Ffidle data.
 * @param[in] context Context where the type has been found.
 * @param[in] typename Tcl_Obj whose string representation is a type name.
 * @param[out] typePtr Points to the place to store the pointer to the parsed @c
 *     ffidl_type.
 * @param[in] valueArea Points to the area where values @p argp values should be
 *     stored.
 * @param[out] valuePtr Points to the place to store the address within @p valueArea
 *     where the arguments are to be retrieved or the return value shall be
 *     placed upon callout.
 * @return TCL_OK if successful, TCL_ERROR otherwise.
 */
static int cif_type_parse(Tcl_Interp *interp, ffidl_client *client, Tcl_Obj *typename, ffidl_type **typePtr)
{
  char *arg = Tcl_GetString(typename);

  /* lookup the type */
  *typePtr = type_lookup(client, arg);
  if (*typePtr == NULL) {
    Tcl_AppendResult(interp, "no type defined for: ", arg, NULL);
    return TCL_ERROR;
  }
  return TCL_OK;
}

/**
 * Check whether a type may be used in a particular context.
 * @param[in] interp Tcl interpreter.
 * @param[in] context The context the type must be allowed to be used.
 * @param[in] type A ffidl_type.
 * @param[in] typeNameObj The type's name.
 */
static int cif_type_check_context(Tcl_Interp *interp, unsigned context,
				  Tcl_Obj *typeNameObj, ffidl_type *typePtr)
{
  if ((context & typePtr->class) == 0) {
    char *typeName = Tcl_GetString(typeNameObj);
    Tcl_AppendResult(interp, "type ", typeName, " is not permitted in ",
		     (context&FFIDL_ARG) ? "argument" :  "return",
		     " context.", NULL);
    return TCL_ERROR;
  }
  return TCL_OK;
}

#if USE_LIBFFI_RAW_API
/**
 * Check whether we can support the raw API on the @p cif.
 */
static int cif_raw_supported(ffidl_cif *cif)
{
  int i;
  int raw_api_supported;

  raw_api_supported = cif->rtype->typecode != FFIDL_STRUCT;

  for (i = 0; i < cif->argc; i++) {
    ffidl_type *atype = cif->atypes[i];
    /* No raw API for structs and long double (fails assertion on
       libffi < 3.3). */
    if (atype->typecode == FFIDL_STRUCT
#if HAVE_LONG_DOUBLE
	|| atype->typecode == FFIDL_LONGDOUBLE
#endif
      )
      raw_api_supported = 0;
  }
  return raw_api_supported;
}

/**
 * Prepare the arguments and return value areas and pointers.
 */
static int cif_raw_prep_offsets(ffidl_cif *cif, ptrdiff_t *offsets)
{
  int i;
  ptrdiff_t offset = 0;
  size_t bytes = ffi_raw_size(&cif->lib_cif);

  for (i = 0; i < cif->argc; i += 1) {
    offsets[i] = offset;
    offset += cif->atypes[i]->size;
    /* align offset, so total bytes is correct */
    if (offset & (FFI_SIZEOF_ARG-1))
      offset = (offset|(FFI_SIZEOF_ARG-1))+1;
  }
  if (offset != bytes) {
    fprintf(stderr, "ffidl and libffi disagree about bytes of argument! %td != %zd\n", offset, bytes);
    return TCL_ERROR;
  }
  return TCL_OK;
}
#endif

/* do any library dependent prep for this cif */
static int cif_prep(ffidl_cif *cif)
{
#if USE_LIBFFI
  ffi_type *lib_rtype;
  int i;
  ffi_type **lib_atypes;
  lib_rtype = cif->rtype->lib_type;
  lib_atypes = cif->lib_atypes;
  for (i = 0; i < cif->argc; i += 1) {
    lib_atypes[i] = cif->atypes[i]->lib_type;
  }
  if (ffi_prep_cif(&cif->lib_cif, cif->protocol, cif->argc, lib_rtype, lib_atypes) != FFI_OK) {
    return TCL_ERROR;
  }
#endif
  return TCL_OK;
}

struct ffidl_protocolmap {
  char *protocolname;
  enum ffi_abi protocol;
};
typedef struct ffidl_protocolmap ffidl_protocolmap;
static const ffidl_protocolmap protocolmap[] = {
  {"", FFI_DEFAULT_ABI},
  {"default", FFI_DEFAULT_ABI},
#if defined(HAVE_FFI_EFI64)
  {"efi64", FFI_EFI64},
#endif
#if defined(HAVE_FFI_FASTCALL)
  {"fastcall", FFI_FASTCALL},
#endif
#if defined(HAVE_FFI_GNUW64)
  {"gnuw64", FFI_GNUW64},
#endif
#if defined(HAVE_FFI_MS_CDECL)
  {"mscdecl", FFI_MS_CDECL},
#endif
#if defined(HAVE_FFI_PASCAL)
  {"pascal", FFI_PASCAL},
#endif
#if defined(HAVE_FFI_REGISTER)
  {"register", FFI_REGISTER},
#endif
#if defined(HAVE_FFI_STDCALL)
  {"stdcall", FFI_STDCALL},
#endif
#if defined(HAVE_FFI_SYSV)
  {"cdecl", FFI_SYSV},
  {"sysv", FFI_SYSV},
#endif
#if defined(HAVE_FFI_THISCALL)
  {"thiscall", FFI_THISCALL},
#endif
#if defined(HAVE_FFI_UNIX64)
  {"unix64", FFI_UNIX64},
#endif
#if defined(HAVE_FFI_WIN64)
  {"win64", FFI_WIN64},
#endif
  {NULL,}
};

/* find the protocol, ie abi, for this cif */
static int cif_protocol(Tcl_Interp *interp, Tcl_Obj *obj, int *protocolp, char **protocolnamep)
{
#if USE_LIBFFI
  if (obj) {
    int idx = 0;
    if (TCL_OK != Tcl_GetIndexFromObjStruct(interp, obj, &protocolmap,
					    sizeof(ffidl_protocolmap),
					    "protocol", 0, &idx)) {
      return TCL_ERROR;
    }
    *protocolp = protocolmap[idx].protocol;
    if (*protocolp == FFI_DEFAULT_ABI) {
      *protocolnamep = NULL;
    } else {
      *protocolnamep = protocolmap[idx].protocolname;
    }
  } else {
    *protocolp = FFI_DEFAULT_ABI;
    *protocolnamep = NULL;
  }
#elif USE_LIBFFCALL
  *protocolp = 0;
  *protocolnamep = NULL;
#endif	/* USE_LIBFFCALL */
  return TCL_OK;
}
/*
 * parse a cif argument list, return type, and protocol,
 * and find or create it in the cif table.
 */
static int cif_parse(Tcl_Interp *interp, ffidl_client *client, Tcl_Obj *args, Tcl_Obj *ret, Tcl_Obj *pro, ffidl_cif **cifp)
{
  int argc, protocol, i;
  Tcl_Obj **argv;
  char *protocolname;
  Tcl_DString signature;
  ffidl_cif *cif = NULL;
  /* fetch argument types */
  if (Tcl_ListObjGetElements(interp, args, &argc, &argv) == TCL_ERROR) return TCL_ERROR;
  /* fetch protocol */
  if (cif_protocol(interp, pro, &protocol, &protocolname) == TCL_ERROR) return TCL_ERROR;
  /* build the cif signature key */
  Tcl_DStringInit(&signature);
  if (protocolname != NULL) {
    Tcl_DStringAppend(&signature, protocolname, -1);
    Tcl_DStringAppend(&signature, " ", 1);
  }
  Tcl_DStringAppend(&signature, Tcl_GetString(ret), -1);
  Tcl_DStringAppend(&signature, "(", 1);
  for (i = 0; i < argc; i += 1) {
    if (i != 0) Tcl_DStringAppend(&signature, ",", 1);
    Tcl_DStringAppend(&signature, Tcl_GetString(argv[i]), -1);
  }
  Tcl_DStringAppend(&signature, ")", 1);
  /* lookup the signature in the cif hash */
  cif = cif_lookup(client, Tcl_DStringValue(&signature));
  if (cif == NULL) {
    cif = cif_alloc(client, argc);
    cif->protocol = protocol;
    if (cif == NULL) {
      Tcl_AppendResult(interp, "couldn't allocate the ffidl_cif", NULL); 
      goto error;
    }
    /* parse return value spec */
    if (cif_type_parse(interp, client, ret, &cif->rtype) == TCL_ERROR) {
      goto error;
    }
    /* parse arg specs */
    for (i = 0; i < argc; i += 1)
      if (cif_type_parse(interp, client, argv[i], &cif->atypes[i]) == TCL_ERROR) {
	goto error;
      }
    /* see if we done right */
    if (cif_prep(cif) != TCL_OK) {
      Tcl_AppendResult(interp, "type definition error", NULL);
      goto error;
    }
    /* define the cif */
    cif_define(client, Tcl_DStringValue(&signature), cif);
    Tcl_ResetResult(interp);
  }
  /* free the signature string */
  Tcl_DStringFree(&signature);
  /* mark the cif as referenced */
  cif_inc_ref(cif);
  /* return success */
  *cifp = cif;
  return TCL_OK;
error:
  if (cif) {
    cif_free(cif);
  }
  Tcl_DStringFree(&signature);
  return TCL_ERROR;
}
/*
 * callout management
 */
/* define a new callout */
static void callout_define(ffidl_client *client, char *pname, ffidl_callout *callout)
{
  entry_define(&client->callouts,pname,(void*)callout);
}
/* lookup an existing callout */
static ffidl_callout *callout_lookup(ffidl_client *client, char *pname)
{
  return entry_lookup(&client->callouts,pname);
}
/* find a callout by it's ffidl_callout */
static Tcl_HashEntry *callout_find(ffidl_client *client, ffidl_callout *callout)
{
  return entry_find(&client->callouts,(void *)callout);
}
/* cleanup on ffidl_callout_call deletion */
static void callout_delete(ClientData clientData)
{
  ffidl_callout *callout = (ffidl_callout *)clientData;
  Tcl_HashEntry *entry = callout_find(callout->client, callout);
  if (entry) {
    cif_dec_ref(callout->cif);
    Tcl_Free((void *)callout);
    Tcl_DeleteHashEntry(entry);
  }
}
/**
 * Parse an argument or return type specification.
 *
 * After invoking this function, @p type points to the @c ffidl_type
 * corresponding to @p typename, and @p argp is initialized
 *
 * @param[in] interp Tcl interpreter.
 * @param[in] client Ffidle data.
 * @param[in] context Context where the type has been found.
 * @param[in] typename Tcl_Obj whose string representation is a type name.
 * @param[out] typePtr Points to the place to store the pointer to the parsed @c
 *     ffidl_type.
 * @param[in] valueArea Points to the area where values @p argp values should be
 *     stored.
 * @param[out] valuePtr Points to the place to store the address within @p valueArea
 *     where the arguments are to be retrieved or the return value shall be
 *     placed upon callout.
 * @return TCL_OK if successful, TCL_ERROR otherwise.
 */
static int callout_prep_value(Tcl_Interp *interp, unsigned context,
			      Tcl_Obj *typeNameObj, ffidl_type *typePtr,
			      ffidl_value *valueArea, void **valuePtr)
{
  char buff[128];

  /* test the context */
  if (cif_type_check_context(interp, context, typeNameObj, typePtr) != TCL_OK) {
    return TCL_ERROR;
  }
  /* set arg value pointer */
  switch (typePtr->typecode) {
  case FFIDL_VOID:
    /* libffi depends on this being NULL on some platforms ! */
    *valuePtr = NULL;
    break;
  case FFIDL_STRUCT:
    /* Will be set to a pointer to the structure's contents. */
    *valuePtr = NULL;
    break;
  case FFIDL_INT:
  case FFIDL_FLOAT:
  case FFIDL_DOUBLE:
#if HAVE_LONG_DOUBLE
  case FFIDL_LONGDOUBLE:
#endif
  case FFIDL_UINT8:
  case FFIDL_SINT8:
  case FFIDL_UINT16:
  case FFIDL_SINT16:
  case FFIDL_UINT32:
  case FFIDL_SINT32:
#if HAVE_INT64
  case FFIDL_UINT64:
  case FFIDL_SINT64:
#endif
  case FFIDL_PTR:
  case FFIDL_PTR_BYTE:
  case FFIDL_PTR_OBJ:
  case FFIDL_PTR_UTF8:
  case FFIDL_PTR_UTF16:
  case FFIDL_PTR_VAR:
  case FFIDL_PTR_PROC:
    *valuePtr = (void *)valueArea;
    break;
  default:
    sprintf(buff, "unknown ffidl_type.t = %d", typePtr->typecode);
    Tcl_AppendResult(interp, buff, NULL);
    return TCL_ERROR;
  }
  return TCL_OK;
}

/*
 * Get a C value of specified type (FFIDL_GETINT, FFIDL_GETDOUBLE,
 * FFIDL_GETWIDEINT) from a Tcl_Obj .  Note that this value must still be
 * interpreted according to the type, whether it's in argument or return
 * position (libffi quirk), etc.
 */
static inline int value_convert_to_c(Tcl_Interp *interp, ffidl_type *type, Tcl_Obj *obj, ffidl_tclobj_value *out)
{
  double dtmp = 0;
  long ltmp = 0;
#if HAVE_INT64
  Ffidl_Int64 wtmp = 0;
#endif
  if (type->class & FFIDL_GETINT) {
    if (obj->typePtr == ffidl_double_ObjType) {
      if (Tcl_GetDoubleFromObj(interp, obj, &dtmp) == TCL_ERROR) {
	goto fail;
      }
      /* Avoid undefined behaviour when casting a non-representable value. */
      if (dtmp >= LONG_MIN && dtmp <= LONG_MAX) {
	ltmp = (long)dtmp;
      }
      if (dtmp != ltmp) {
	/* Does not round-trip; use Tcl's conversion. */
	if (Tcl_GetLongFromObj(interp, obj, &ltmp) == TCL_ERROR) {
	  goto fail;
	}
      }
    } else if (Tcl_GetLongFromObj(interp, obj, &ltmp) == TCL_ERROR) {
      goto fail;
    }
    out->v_long = ltmp;
#if HAVE_INT64
  } else if (type->class & FFIDL_GETWIDEINT) {
    if (obj->typePtr == ffidl_double_ObjType) {
      if (Tcl_GetDoubleFromObj(interp, obj, &dtmp) == TCL_ERROR) {
	goto fail;
      }
      /* Avoid undefined behaviour when casting a non-representable value. */
      if (dtmp >= INT64_MIN && dtmp <= INT64_MAX) {
	wtmp = (Ffidl_Int64)dtmp;
      }
      if (dtmp != wtmp) {
	/* Does not round-trip; use Tcl's conversion. */
	if (Ffidl_GetInt64FromObj(interp, obj, &wtmp) == TCL_ERROR) {
	  goto fail;
	}
      }
    } else if (Ffidl_GetInt64FromObj(interp, obj, &wtmp) == TCL_ERROR) {
      goto fail;
    }
    out->v_wideint = wtmp;
#endif
  } else if (type->class & FFIDL_GETDOUBLE) {
    if (obj->typePtr == ffidl_int_ObjType) {
      if (Tcl_GetLongFromObj(interp, obj, &ltmp) == TCL_ERROR) {
	goto fail;
      }
      /* Avoid undefined behaviour when casting a non-representable value. */
      if (ltmp >= DBL_MIN && ltmp <= DBL_MAX) {
	dtmp = (double)ltmp;
      }
      if (dtmp != ltmp) {
	/* Does not round-trip; use Tcl's conversion. */
	if (Tcl_GetDoubleFromObj(interp, obj, &dtmp) == TCL_ERROR) {
	  goto fail;
	}
      }
#if HAVE_WIDE_INT
    } else if (obj->typePtr == ffidl_wideInt_ObjType) {
      if (Tcl_GetWideIntFromObj(interp, obj, &wtmp) == TCL_ERROR) {
	goto fail;
      }
      /* Avoid undefined behaviour when casting a non-representable value. */
      if (wtmp >= DBL_MIN && wtmp <= DBL_MAX) {
	dtmp = (double)wtmp;
      }
      if (dtmp != wtmp) {
	/* Does not round-trip; use Tcl's conversion. */
	if (Tcl_GetDoubleFromObj(interp, obj, &dtmp) == TCL_ERROR) {
	  goto fail;
	}
      }
#endif
    } else if (Tcl_GetDoubleFromObj(interp, obj, &dtmp) == TCL_ERROR) {
      goto fail;
    }
    out->v_double = dtmp;
  }
  return TCL_OK;
fail:
  return TCL_ERROR;
}


static int callout_prep(ffidl_callout *callout)
{
#if USE_LIBFFI_RAW_API
  int i;
  ffidl_cif *cif = callout->cif;

  callout->use_raw_api = cif_raw_supported(cif);

  if (callout->use_raw_api) {
    /* rewrite callout->args[i] into a stack image */
    ptrdiff_t *offsets;
    offsets = (ptrdiff_t *)Tcl_Alloc(sizeof(ptrdiff_t) * cif->argc);
    if (TCL_OK != cif_raw_prep_offsets(cif, offsets)) {
      Tcl_Free((void *)offsets);
      return TCL_ERROR;
    }
    /* fprintf(stderr, "using raw api for %d args\n", cif->argc); */
    for (i = 0; i < cif->argc; i++) {
      /* set args[i] to args[0]+offset */
      /* fprintf(stderr, "  arg[%d] was %08x ...", i, callout->args[i]); */
      callout->args[i] = (void *)(((char *)callout->args[0])+offsets[i]);
      /* fprintf(stderr, " becomes %08x\n", callout->args[i]); */
    }
    /* fprintf(stderr, "  final offset %d, bytes %d\n", offset, bytes); */
    Tcl_Free((void *)offsets);
  }
#endif
  return TCL_OK;
}

/* make a call */
/* consider what happens if we reenter using the same cif */  
static void callout_call(ffidl_callout *callout)
{
  ffidl_cif *cif = callout->cif;
#if USE_LIBFFI
#if USE_LIBFFI_RAW_API
  if (callout->use_raw_api)
    ffi_raw_call(&cif->lib_cif, callout->fn, callout->ret, (ffi_raw *)callout->args[0]);
  else
    ffi_call(&cif->lib_cif, callout->fn, callout->ret, callout->args);
#else
  ffi_call(&cif->lib_cif, callout->fn, callout->ret, callout->args);
#endif
#elif USE_LIBFFCALL
  av_alist alist;
  int i;
  switch (cif->rtype->typecode) {
  case FFIDL_VOID:
    av_start_void(alist,callout->fn);
    break;
  case FFIDL_INT:
    av_start_int(alist,callout->fn,callout->ret);
    break;
  case FFIDL_FLOAT:
    av_start_float(alist,callout->fn,callout->ret);
    break;
  case FFIDL_DOUBLE:
    av_start_double(alist,callout->fn,callout->ret);
    break;
  case FFIDL_UINT8:
    av_start_uint8(alist,callout->fn,callout->ret);
    break;
  case FFIDL_SINT8:
    av_start_sint8(alist,callout->fn,callout->ret);
    break;
  case FFIDL_UINT16:
    av_start_uint16(alist,callout->fn,callout->ret);
    break;
  case FFIDL_SINT16:
    av_start_sint16(alist,callout->fn,callout->ret);
    break;
  case FFIDL_UINT32:
    av_start_uint32(alist,callout->fn,callout->ret);
    break;
  case FFIDL_SINT32:
    av_start_sint32(alist,callout->fn,callout->ret);
    break;
#if HAVE_INT64
  case FFIDL_UINT64:
    av_start_uint64(alist,callout->fn,callout->ret);
    break;
  case FFIDL_SINT64:
    av_start_sint64(alist,callout->fn,callout->ret);
    break;
#endif
  case FFIDL_STRUCT:
    _av_start_struct(alist,callout->fn,cif->rtype->size,cif->rtype->splittable,callout->ret);
    break;
  case FFIDL_PTR:
  case FFIDL_PTR_OBJ:
  case FFIDL_PTR_UTF8:
  case FFIDL_PTR_UTF16:
  case FFIDL_PTR_BYTE:
  case FFIDL_PTR_VAR:
#if USE_CALLBACKS
  case FFIDL_PTR_PROC:
#endif
    av_start_ptr(alist,callout->fn,void *,callout->ret);
    break;
  }

  for (i = 0; i < cif->argc; i += 1) {
    switch (cif->atypes[i]->typecode) {
    case FFIDL_VOID:
      continue;
    case FFIDL_INT:
      av_int(alist,*(int *)callout->args[i]);
      continue;
    case FFIDL_FLOAT:
      av_float(alist,*(float *)callout->args[i]);
      continue;
    case FFIDL_DOUBLE:
      av_double(alist,*(double *)callout->args[i]);
      continue;
    case FFIDL_UINT8:
      av_uint8(alist,*(UINT8_T *)callout->args[i]);
      continue;
    case FFIDL_SINT8:
      av_sint8(alist,*(SINT8_T *)callout->args[i]);
      continue;
    case FFIDL_UINT16:
      av_uint16(alist,*(UINT16_T *)callout->args[i]);
      continue;
    case FFIDL_SINT16:
      av_sint16(alist,*(SINT16_T *)callout->args[i]);
      continue;
    case FFIDL_UINT32:
      av_uint32(alist,*(UINT32_T *)callout->args[i]);
      continue;
    case FFIDL_SINT32:
      av_sint32(alist,*(SINT32_T *)callout->args[i]);
      continue;
#if HAVE_INT64
    case FFIDL_UINT64:
      av_uint64(alist,*(UINT64_T *)callout->args[i]);
      continue;
    case FFIDL_SINT64:
      av_sint64(alist,*(SINT64_T *)callout->args[i]);
      continue;
#endif
    case FFIDL_STRUCT:
      _av_struct(alist,cif->atypes[i]->size,cif->atypes[i]->alignment,callout->args[i]);
      continue;
    case FFIDL_PTR:
    case FFIDL_PTR_OBJ:
    case FFIDL_PTR_UTF8:
    case FFIDL_PTR_UTF16:
    case FFIDL_PTR_BYTE:
    case FFIDL_PTR_VAR:
#if USE_CALLBACKS
    case FFIDL_PTR_PROC:
#endif
      av_ptr(alist,void *,*(void **)callout->args[i]);
      continue;
    }
    /* Note: change "continue" to "break" if further work must be done here. */
  }
  av_call(alist);
#endif
}
/*
 * lib management, but note we never free a lib
 * because we cannot know how often it is used.
 */
/* define a new lib */
static void lib_define(ffidl_client *client, char *lname, void *handle, void* unload)
{
  ffidl_lib *libentry = (ffidl_lib *)Tcl_Alloc(sizeof(ffidl_lib));
  libentry->loadHandle = handle;
  libentry->unloadProc = unload;
  entry_define(&client->libs,lname,libentry);
}
/* lookup an existing type */
static ffidl_LoadHandle lib_lookup(ffidl_client *client,
				   char *lname,
				   ffidl_UnloadProc *unload)
{
  ffidl_lib *libentry = entry_lookup(&client->libs,lname);
  if (libentry) {
    if (unload) {
      *unload = libentry->unloadProc;
    }
    return libentry->loadHandle;
  } else {
    return NULL;
  }
}
#if USE_CALLBACKS
/*
 * callback management
 */
/* free a defined callback */
static void callback_free(ffidl_callback *callback)
{
  if (callback) {
    int i;
    cif_dec_ref(callback->cif);
    for (i = 0; i < callback->cmdc; i++) {
      Tcl_DecrRefCount(callback->cmdv[i]);
    }
#if USE_LIBFFI
    ffi_closure_free(callback->closure.lib_closure);
#elif USE_LIBFFCALL
    free_callback(callback->closure.lib_closure);
#endif
    Tcl_Free((void *)callback);
  }
}
/* define a new callback */
static void callback_define(ffidl_client *client, char *cname, ffidl_callback *callback)
{
  ffidl_callback *old_callback = NULL;
  /* if callback is already defined, clean it up. */
  old_callback = entry_lookup(&client->callbacks,cname);
  callback_free(old_callback);
  entry_define(&client->callbacks,cname,(void*)callback);
}
/* lookup an existing callback */
static ffidl_callback *callback_lookup(ffidl_client *client, char *cname)
{
  return entry_lookup(&client->callbacks,cname);
}
/* find a callback by it's ffidl_callback */
/*
static Tcl_HashEntry *callback_find(ffidl_client *client, ffidl_callback *callback)
{
  return entry_find(&client->callbacks,(void *)callback);
}
*/
/* delete a callback definition */
/*
static void callback_delete(ffidl_client *client, ffidl_callback *callback)
{
  Tcl_HashEntry *entry = callback_find(client, callback);
  if (entry) {
    callback_free(callback);
    Tcl_DeleteHashEntry(entry);
  }
}
*/
#if USE_LIBFFI
/* call a tcl proc from a libffi closure */
static void callback_callback(ffi_cif *fficif, void *ret, void **args, void *user_data)
{
  ffidl_callback *callback = (ffidl_callback *)user_data;
  Tcl_Interp *interp = callback->interp;
  ffidl_cif *cif = callback->cif;
  Tcl_Obj **objv, *obj;
  char buff[128];
  int i, status;
  ffidl_tclobj_value obj_value = {0};
  /* test for valid scope */
  if (interp == NULL) {
    Tcl_Panic("callback called out of scope!\n");
  }
  /* initialize argument list */
  objv = callback->cmdv+callback->cmdc;
  /* fetch and convert argument values */
  for (i = 0; i < cif->argc; i += 1) {
    void *argp;
#if USE_LIBFFI_RAW_API
    if (callback->use_raw_api) {
      ptrdiff_t offset = callback->offsets[i] - callback->offsets[0];
      argp = (void *)(((char *)args)+offset);
    } else {
      argp = args[i];
    }
#else
    argp = args[i];
#endif
    switch (cif->atypes[i]->typecode) {
    case FFIDL_INT:
      objv[i] = Tcl_NewLongObj((long)(*(int *)argp));
      break;
    case FFIDL_FLOAT:
      objv[i] = Tcl_NewDoubleObj((double)(*(float *)argp));
      break;
    case FFIDL_DOUBLE:
      objv[i] = Tcl_NewDoubleObj(*(double *)argp);
      break;
#if HAVE_LONG_DOUBLE
    case FFIDL_LONGDOUBLE:
      objv[i] = Tcl_NewDoubleObj((double)(*(long double *)argp));
      break;
#endif
    case FFIDL_UINT8:
      objv[i] = Tcl_NewLongObj((long)(*(UINT8_T *)argp));
      break;
    case FFIDL_SINT8:
      objv[i] = Tcl_NewLongObj((long)(*(SINT8_T *)argp));
      break;
    case FFIDL_UINT16:
      objv[i] = Tcl_NewLongObj((long)(*(UINT16_T *)argp));
      break;
    case FFIDL_SINT16:
      objv[i] = Tcl_NewLongObj((long)(*(SINT16_T *)argp));
      break;
    case FFIDL_UINT32:
      objv[i] = Tcl_NewLongObj((long)(*(UINT32_T *)argp));
      break;
    case FFIDL_SINT32:
      objv[i] = Tcl_NewLongObj((long)(*(SINT32_T *)argp));
      break;
#if HAVE_INT64
    case FFIDL_UINT64:
      objv[i] = Ffidl_NewInt64Obj((Ffidl_Int64)(*(UINT64_T *)argp));
      break;
    case FFIDL_SINT64:
      objv[i] = Ffidl_NewInt64Obj((Ffidl_Int64)(*(SINT64_T *)argp));
      break;
#endif
    case FFIDL_STRUCT:
      objv[i] = Tcl_NewByteArrayObj((unsigned char *)argp, cif->atypes[i]->size);
      break;
    case FFIDL_PTR:
      objv[i] = Ffidl_NewPointerObj((*(void **)argp));
      break;
    case FFIDL_PTR_OBJ:
      objv[i] = *(Tcl_Obj **)argp;
      break;
    case FFIDL_PTR_UTF8:
      objv[i] = Tcl_NewStringObj(*(char **)argp, -1);
      break;
    case FFIDL_PTR_UTF16:
      objv[i] = Tcl_NewUnicodeObj(*(Tcl_UniChar **)argp, -1);
      break;
    default:
      sprintf(buff, "unimplemented type for callback argument: %d", cif->atypes[i]->typecode);
      Tcl_AppendResult(interp, buff, NULL);
      while (i-- >= 0) {
	Tcl_DecrRefCount(objv[i]);
      }
      goto escape;
    }
    Tcl_IncrRefCount(objv[i]);
  }
  /* call */
  status = Tcl_EvalObjv(interp, callback->cmdc+cif->argc, callback->cmdv, TCL_EVAL_GLOBAL);
  /* clean up arguments */
  for (i = 0; i < cif->argc; i++) {
    Tcl_DecrRefCount(objv[i]);
  }
  if (status == TCL_ERROR) {
    goto escape;
  }
  /* fetch return value */
  obj = Tcl_GetObjResult(interp);
  if (TCL_OK != value_convert_to_c(interp, cif->rtype, obj, &obj_value)) {
    Tcl_AppendResult(interp, ", converting callback return value", NULL);
    goto escape;
  }
  /* convert return value */
  switch (cif->rtype->typecode) {
  case FFIDL_VOID:	break;
  case FFIDL_INT:	FFIDL_RVALUE_POKE_WIDENED(INT, ret, obj_value.v_long); break;
  case FFIDL_FLOAT:	FFIDL_RVALUE_POKE_WIDENED(FLOAT, ret, obj_value.v_double); break;
  case FFIDL_DOUBLE:	FFIDL_RVALUE_POKE_WIDENED(DOUBLE, ret, obj_value.v_double); break;
#if HAVE_LONG_DOUBLE
  case FFIDL_LONGDOUBLE:FFIDL_RVALUE_POKE_WIDENED(LONGDOUBLE, ret, obj_value.v_long); break;
#endif
  case FFIDL_UINT8:	FFIDL_RVALUE_POKE_WIDENED(UINT8, ret, obj_value.v_long); break;
  case FFIDL_SINT8:	FFIDL_RVALUE_POKE_WIDENED(SINT8, ret, obj_value.v_long); break;
  case FFIDL_UINT16:	FFIDL_RVALUE_POKE_WIDENED(UINT16, ret, obj_value.v_long); break;
  case FFIDL_SINT16:	FFIDL_RVALUE_POKE_WIDENED(SINT16, ret, obj_value.v_long); break;
  case FFIDL_UINT32:	FFIDL_RVALUE_POKE_WIDENED(UINT32, ret, obj_value.v_long); break;
  case FFIDL_SINT32:	FFIDL_RVALUE_POKE_WIDENED(SINT32, ret, obj_value.v_long); break;
#if HAVE_INT64
  case FFIDL_UINT64:	FFIDL_RVALUE_POKE_WIDENED(UINT64, ret, obj_value.v_wideint); break;
  case FFIDL_SINT64:	FFIDL_RVALUE_POKE_WIDENED(SINT64, ret, obj_value.v_wideint); break;
#endif
  case FFIDL_STRUCT:
    {
      int len;
      void *bytes = Tcl_GetByteArrayFromObj(obj, &len);
      if (len != cif->rtype->size) {
	Tcl_ResetResult(interp);
	sprintf(buff, "byte array for callback struct return has %u bytes instead of %lu", len, (long)(cif->rtype->size));
	Tcl_AppendResult(interp, buff, NULL);
	goto escape;
      }
      memcpy(ret, bytes, cif->rtype->size);
      break;
    }
#if FFIDL_POINTER_IS_LONG
  case FFIDL_PTR:	FFIDL_RVALUE_POKE_WIDENED(PTR, ret, obj_value.v_long); break;
#else
  case FFIDL_PTR:	FFIDL_RVALUE_POKE_WIDENED(PTR, ret, obj_value.v_wideint); break;
#endif
  case FFIDL_PTR_OBJ:	FFIDL_RVALUE_POKE_WIDENED(PTR, ret, obj); break;
  default:
    Tcl_ResetResult(interp);
    sprintf(buff, "unimplemented type for callback return: %d", cif->rtype->typecode);
    Tcl_AppendResult(interp, buff, NULL);
    goto escape;
  }
  /* done */
  return;
escape:
  Tcl_BackgroundError(interp);
  memset(ret, 0, cif->rtype->size);
}
#elif USE_LIBFFCALL
static void callback_callback(void *user_data, va_alist alist)
{
  ffidl_callback *callback = (ffidl_callback *)user_data;
  Tcl_Interp *interp = callback->interp;
  ffidl_cif *cif = callback->cif;
  Tcl_Obj **objv, *obj;
  char buff[128];
  int i, status;
  ffidl_tclobj_value obj_value = {0};
  /* test for valid scope */
  if (interp == NULL) {
    Tcl_Panic("callback called out of scope!\n");
  }
  /* initialize argument list */
  objv = callback->cmdv+callback->cmdc;
  /* start */
  switch (cif->rtype->typecode) {
  case FFIDL_VOID:	va_start_void(alist); break;
  case FFIDL_INT:	va_start_int(alist); break;
  case FFIDL_FLOAT:	va_start_float(alist); break;
  case FFIDL_DOUBLE:	va_start_double(alist); break;
  case FFIDL_UINT8:	va_start_uint8(alist); break;
  case FFIDL_SINT8:	va_start_sint8(alist); break;
  case FFIDL_UINT16:	va_start_uint16(alist); break;
  case FFIDL_SINT16:	va_start_sint16(alist); break;
  case FFIDL_UINT32:	va_start_uint32(alist); break;
  case FFIDL_SINT32:	va_start_sint32(alist); break;
#if HAVE_INT64
  case FFIDL_UINT64:	va_start_uint64(alist); break;
  case FFIDL_SINT64:	va_start_sint64(alist); break;
#endif
  case FFIDL_STRUCT:	_va_start_struct(alist,cif->rtype->size,cif->rtype->alignment,cif->rtype->splittable); break;
  case FFIDL_PTR:	va_start_ptr(alist,void *); break;
  case FFIDL_PTR_OBJ:	va_start_ptr(alist,Tcl_Obj *); break;
  default:
    Tcl_ResetResult(interp);
    sprintf(buff, "unimplemented type for callback return: %d", cif->rtype->typecode);
    Tcl_AppendResult(interp, buff, NULL);
    goto escape;
  }
  /* fetch and convert argument values */
  for (i = 0; i < cif->argc; i++) {
    switch (cif->atypes[i]->typecode) {
    case FFIDL_INT:
      objv[i] = Tcl_NewLongObj((long)va_arg_int(alist));
      break;
    case FFIDL_FLOAT:
      objv[i] = Tcl_NewDoubleObj((double)va_arg_float(alist));
      break;
    case FFIDL_DOUBLE:
      objv[i] = Tcl_NewDoubleObj(va_arg_double(alist));
      break;
    case FFIDL_UINT8:
      objv[i] = Tcl_NewLongObj((long)va_arg_uint8(alist));
      break;
    case FFIDL_SINT8:
      objv[i] = Tcl_NewLongObj((long)va_arg_sint8(alist));
      break;
    case FFIDL_UINT16:
      objv[i] = Tcl_NewLongObj((long)va_arg_uint16(alist));
      break;
    case FFIDL_SINT16:
      objv[i] = Tcl_NewLongObj((long)va_arg_sint16(alist));
      break;
    case FFIDL_UINT32:
      objv[i] = Tcl_NewLongObj((long)va_arg_uint32(alist));
      break;
    case FFIDL_SINT32:
      objv[i] = Tcl_NewLongObj((long)va_arg_sint32(alist));
      break;
#if HAVE_INT64
    case FFIDL_UINT64:
      objv[i] = Ffidl_NewInt64Obj((long)va_arg_uint64(alist));
      break;
    case FFIDL_SINT64:
      objv[i] = Ffidl_NewInt64Obj((long)va_arg_sint64(alist));
      break;
#endif
    case FFIDL_STRUCT:
      objv[i] = Tcl_NewByteArrayObj(_va_arg_struct(alist,
						   cif->atypes[i]->size,
						   cif->atypes[i]->alignment),
				    cif->atypes[i]->size);
      break;
    case FFIDL_PTR:
      objv[i] = Ffidl_NewPointerObj(va_arg_ptr(alist,void *));
      break;
    case FFIDL_PTR_OBJ:
      objv[i] = va_arg_ptr(alist,Tcl_Obj *);
      break;
    case FFIDL_PTR_UTF8:
      objv[i] = Tcl_NewStringObj(va_arg_ptr(alist,char *), -1);
      break;
    case FFIDL_PTR_UTF16:
      objv[i] = Tcl_NewUnicodeObj(va_arg_ptr(alist,Tcl_UniChar *), -1);
      break;
    default:
      sprintf(buff, "unimplemented type for callback argument: %d", cif->atypes[i]->typecode);
      Tcl_AppendResult(interp, buff, NULL);
      while (i-- >= 0) {
	Tcl_DecrRefCount(objv[i]);
      }
      goto escape;
    }
    Tcl_IncrRefCount(objv[i]);
  }
  /* call */
  status = Tcl_EvalObjv(interp, callback->cmdc+cif->argc, callback->cmdv, TCL_EVAL_GLOBAL);
  /* clean up arguments */
  for (i = 0; i < cif->argc; i++) {
    Tcl_DecrRefCount(objv[i]);
  }
  if (status == TCL_ERROR) {
    goto escape;
  }
  /* fetch return value */
  obj = Tcl_GetObjResult(interp);
  if (TCL_OK != value_convert_to_c(interp, cif->rtype, obj, &obj_value)) {
    Tcl_AppendResult(interp, ", converting callback return value", NULL);
    goto escape;
  }
  
  /* convert return value */
  switch (cif->rtype->typecode) {
  case FFIDL_VOID:	va_return_void(alist); break;
  case FFIDL_INT:	va_return_int(alist, obj_value.v_long); break;
  case FFIDL_FLOAT:	va_return_float(alist, obj_value.v_double); break;
  case FFIDL_DOUBLE:	va_return_double(alist, obj_value.v_double); break;
  case FFIDL_UINT8:	va_return_uint8(alist, obj_value.v_long); break;
  case FFIDL_SINT8:	va_return_sint8(alist, obj_value.v_long); break;
  case FFIDL_UINT16:	va_return_uint16(alist, obj_value.v_long); break;
  case FFIDL_SINT16:	va_return_sint16(alist, obj_value.v_long); break;
  case FFIDL_UINT32:	va_return_uint32(alist, obj_value.v_long); break;
  case FFIDL_SINT32:	va_return_sint32(alist, obj_value.v_long); break;
#if HAVE_INT64
  case FFIDL_UINT64:	va_return_uint64(alist, obj_value.v_wideint); break;
  case FFIDL_SINT64:	va_return_sint64(alist, obj_value.v_wideint); break;
#endif
  case FFIDL_STRUCT:	
    {
      int len;
      void *bytes = Tcl_GetByteArrayFromObj(obj, &len);
      if (len != cif->rtype->size) {
	Tcl_ResetResult(interp);
	sprintf(buff, "byte array for callback struct return has %u bytes instead of %lu", len, (long)(cif->rtype->size));
	Tcl_AppendResult(interp, buff, NULL);
	goto escape;
      }
      _va_return_struct(alist, cif->rtype->size, cif->rtype->alignment, bytes);
      break;
    }
  case FFIDL_PTR:	va_return_ptr(alist, void *, obj_value.v_long); break;
  case FFIDL_PTR_OBJ:	va_return_ptr(alist, Tcl_Obj *, obj); break;
  default:
    Tcl_ResetResult(interp);
    sprintf(buff, "unimplemented type for callback return: %d", cif->rtype->typecode);
    Tcl_AppendResult(interp, buff, NULL);
    goto escape;
  }
  /* done */
  return;
escape:
  Tcl_BackgroundError(interp);
}
#endif
#endif
/*
 * Client management.
 */
/* client interp deletion callback for cleanup */
static void client_delete(ClientData clientData, Tcl_Interp *interp)
{
  ffidl_client *client = (ffidl_client *)clientData;
  Tcl_HashSearch search;
  Tcl_HashEntry *entry;

  /* there should be no callouts left */
  for (entry = Tcl_FirstHashEntry(&client->callouts, &search); entry != NULL; entry = Tcl_NextHashEntry(&search)) {
    char *name = Tcl_GetHashKey(&client->callouts, entry);
    /* Couldn't do this while traversing the hash table anyway */
    /* Tcl_DeleteCommand(interp, name); */
    fprintf(stderr, "error - dangling callout in client_delete: %s\n", name);
  }

#if USE_CALLBACKS
  /* free all callbacks */
  for (entry = Tcl_FirstHashEntry(&client->callbacks, &search); entry != NULL; entry = Tcl_NextHashEntry(&search)) {
    ffidl_callback *callback = Tcl_GetHashValue(entry);
    callback_free(callback);
  }
#endif

  /* there should be no cifs left */
  for (entry = Tcl_FirstHashEntry(&client->cifs, &search); entry != NULL; entry = Tcl_NextHashEntry(&search)) {
    char *signature = Tcl_GetHashKey(&client->cifs, entry);
    fprintf(stderr, "error - dangling ffidl_cif in client_delete: %s\n",signature);
  }

  /* free all allocated typedefs */
  for (entry = Tcl_FirstHashEntry(&client->types, &search); entry != NULL; entry = Tcl_NextHashEntry(&search)) {
    ffidl_type *type = Tcl_GetHashValue(entry);
    if ((type->class & FFIDL_STATIC_TYPE) == 0) {
      type_dec_ref(type);
    }
  }

  /* free all libs */
  for (entry = Tcl_FirstHashEntry(&client->libs, &search); entry != NULL; entry = Tcl_NextHashEntry(&search)) {
    char *libraryName = Tcl_GetHashKey(&client->libs, entry);
    ffidl_lib *libentry = Tcl_GetHashValue(entry);
    ffidlclose(interp, libraryName, libentry->loadHandle, libentry->unloadProc);
    Tcl_Free((void *)libentry);
  }

  /* free hashtables */
  Tcl_DeleteHashTable(&client->callouts);
#if USE_CALLBACKS
  Tcl_DeleteHashTable(&client->callbacks);
#endif
  Tcl_DeleteHashTable(&client->cifs);
  Tcl_DeleteHashTable(&client->types);
  Tcl_DeleteHashTable(&client->libs);

  /* free client structure */
  Tcl_Free((void *)client);
}
/* client allocation and initialization */
static ffidl_client *client_alloc(Tcl_Interp *interp)
{
  ffidl_client *client;

  /* allocate client data structure */
  client = (ffidl_client *)Tcl_Alloc(sizeof(ffidl_client));

  /* allocate hashtables for this load */
  Tcl_InitHashTable(&client->types, TCL_STRING_KEYS);
  Tcl_InitHashTable(&client->callouts, TCL_STRING_KEYS);
  Tcl_InitHashTable(&client->cifs, TCL_STRING_KEYS);
  Tcl_InitHashTable(&client->libs, TCL_STRING_KEYS);
#if USE_CALLBACKS
  Tcl_InitHashTable(&client->callbacks, TCL_STRING_KEYS);
#endif

  /* initialize types */
  ffidl_type_void.lib_type = lib_type_void;
  ffidl_type_char.lib_type = lib_type_char;
  ffidl_type_schar.lib_type = lib_type_schar;
  ffidl_type_uchar.lib_type = lib_type_uchar;
  ffidl_type_sshort.lib_type = lib_type_sshort;
  ffidl_type_ushort.lib_type = lib_type_ushort;
  ffidl_type_sint.lib_type = lib_type_sint;
  ffidl_type_uint.lib_type = lib_type_uint;
  ffidl_type_slong.lib_type = lib_type_slong;
  ffidl_type_ulong.lib_type = lib_type_ulong;
#if HAVE_LONG_LONG
  ffidl_type_slonglong.lib_type = lib_type_slonglong;
  ffidl_type_ulonglong.lib_type = lib_type_ulonglong;
#endif
  ffidl_type_float.lib_type = lib_type_float;
  ffidl_type_double.lib_type = lib_type_double;
#if HAVE_LONG_DOUBLE
  ffidl_type_longdouble.lib_type = lib_type_longdouble;
#endif
  ffidl_type_sint8.lib_type = lib_type_sint8;
  ffidl_type_uint8.lib_type = lib_type_uint8;
  ffidl_type_sint16.lib_type = lib_type_sint16;
  ffidl_type_uint16.lib_type = lib_type_uint16;
  ffidl_type_sint32.lib_type = lib_type_sint32;
  ffidl_type_uint32.lib_type = lib_type_uint32;
#if HAVE_INT64
  ffidl_type_sint64.lib_type = lib_type_sint64;
  ffidl_type_uint64.lib_type = lib_type_uint64;
#endif
  ffidl_type_pointer.lib_type       = lib_type_pointer;
  ffidl_type_pointer_obj.lib_type   = lib_type_pointer;
  ffidl_type_pointer_utf8.lib_type  = lib_type_pointer;
  ffidl_type_pointer_utf16.lib_type = lib_type_pointer;
  ffidl_type_pointer_byte.lib_type  = lib_type_pointer;
  ffidl_type_pointer_var.lib_type   = lib_type_pointer;
#if USE_CALLBACKS
  ffidl_type_pointer_proc.lib_type = lib_type_pointer;
#endif

  type_define(client, "void", &ffidl_type_void);
  type_define(client, "char", &ffidl_type_char);
  type_define(client, "signed char", &ffidl_type_schar);
  type_define(client, "unsigned char", &ffidl_type_uchar);
  type_define(client, "short", &ffidl_type_sshort);
  type_define(client, "unsigned short", &ffidl_type_ushort);
  type_define(client, "int", &ffidl_type_sint);
  type_define(client, "unsigned", &ffidl_type_uint);
  type_define(client, "long", &ffidl_type_slong);
  type_define(client, "unsigned long", &ffidl_type_ulong);
#if HAVE_LONG_LONG
  type_define(client, "long long", &ffidl_type_slonglong);
  type_define(client, "unsigned long long", &ffidl_type_ulonglong);
#endif
  type_define(client, "float", &ffidl_type_float);
  type_define(client, "double", &ffidl_type_double);
#if HAVE_LONG_DOUBLE
  type_define(client, "long double", &ffidl_type_longdouble);
#endif
  type_define(client, "sint8", &ffidl_type_sint8);
  type_define(client, "uint8", &ffidl_type_uint8);
  type_define(client, "sint16", &ffidl_type_sint16);
  type_define(client, "uint16", &ffidl_type_uint16);
  type_define(client, "sint32", &ffidl_type_sint32);
  type_define(client, "uint32", &ffidl_type_uint32);
#if HAVE_INT64
  type_define(client, "sint64", &ffidl_type_sint64);
  type_define(client, "uint64", &ffidl_type_uint64);
#endif
  type_define(client, "pointer", &ffidl_type_pointer);
  type_define(client, "pointer-obj", &ffidl_type_pointer_obj);
  type_define(client, "pointer-utf8", &ffidl_type_pointer_utf8);
  type_define(client, "pointer-utf16", &ffidl_type_pointer_utf16);
  type_define(client, "pointer-byte", &ffidl_type_pointer_byte);
  type_define(client, "pointer-var", &ffidl_type_pointer_var);
#if USE_CALLBACKS
  type_define(client, "pointer-proc", &ffidl_type_pointer_proc);
#endif

  /* arrange for cleanup on interpreter deletion */
  Tcl_CallWhenDeleted(interp, client_delete, (ClientData)client);

  /* finis */
  return client;
}
/*****************************************
 *
 * Functions exported as tcl commands.
 */

/* usage: ::ffidl::info option ?...? */
static int tcl_ffidl_info(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  enum {
    command_ix,
    option_ix,
    minargs
  };

  int i;
  char *arg;
  Tcl_HashTable *table;
  Tcl_HashSearch search;
  Tcl_HashEntry *entry;
  ffidl_type *type;
  ffidl_client *client = (ffidl_client *)clientData;
  static const char *options[] = {
#define INFO_ALIGNOF 0
    "alignof",
#define INFO_CALLBACKS 1
    "callbacks",
#define INFO_CALLOUTS 2
    "callouts",
#define INFO_CANONICAL_HOST 3
    "canonical-host",
#define INFO_FORMAT 4
    "format",
#define INFO_HAVE_INT64 5
    "have-int64",
#define INFO_HAVE_LONG_DOUBLE 6
    "have-long-double",
#define INFO_HAVE_LONG_LONG 7
    "have-long-long",
#define INFO_INTERP 8
    "interp",
#define INFO_LIBRARIES 9
    "libraries",
#define INFO_SIGNATURES 10
    "signatures",
#define INFO_SIZEOF 11
    "sizeof",
#define INFO_TYPEDEFS 12
    "typedefs",
#define INFO_USE_CALLBACKS 13
    "use-callbacks",
#define INFO_USE_FFCALL 14
    "use-ffcall",
#define INFO_USE_LIBFFCALL 15
    "use-libffcall",
#define INFO_USE_LIBFFI 16
    "use-libffi",
#define INFO_USE_LIBFFI_RAW 17
    "use-libffi-raw",
#define INFO_NULL 18
    "NULL",
    NULL
  };

  if (objc < minargs) {
    Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
    return TCL_ERROR;
  }

  if (Tcl_GetIndexFromObj(interp, objv[option_ix], options, "option", TCL_EXACT, &i) == TCL_ERROR)
    return TCL_ERROR;

  switch (i) {
  case INFO_CALLOUTS:		/* return list of callout names */
    table = &client->callouts;
  list_table_keys:		/* list the keys in a hash table */
    if (objc != 2) {
      Tcl_WrongNumArgs(interp,2,objv,"");
      return TCL_ERROR;
    }
    for (entry = Tcl_FirstHashEntry(table, &search); entry != NULL; entry = Tcl_NextHashEntry(&search))
      Tcl_ListObjAppendElement(interp, Tcl_GetObjResult(interp), Tcl_NewStringObj(Tcl_GetHashKey(table,entry),-1));
    return TCL_OK;
  case INFO_TYPEDEFS:		/* return list of typedef names */
    table = &client->types;
    goto list_table_keys;
  case INFO_SIGNATURES:		/* return list of ffi signatures */
    table = &client->cifs;
    goto list_table_keys;
  case INFO_LIBRARIES:		/* return list of lib names */
    table = &client->libs;
    goto list_table_keys;
  case INFO_CALLBACKS:		/* return list of callback names */
#if USE_CALLBACKS
    table = &client->callbacks;
    goto list_table_keys;
#else
    Tcl_AppendResult(interp, "callbacks are not supported in this configuration", NULL);
    return TCL_ERROR;
#endif

  case INFO_SIZEOF:		/* return sizeof type */
  case INFO_ALIGNOF:		/* return alignof type */
  case INFO_FORMAT:		/* return binary format of type */
    if (objc != 3) {
      Tcl_WrongNumArgs(interp,2,objv,"type");
      return TCL_ERROR;
    }
    arg = Tcl_GetString(objv[2]);
    type = type_lookup(client, arg);
    if (type == NULL) {
      Tcl_AppendResult(interp, "undefined type: ", arg, NULL);
      return TCL_ERROR;
    }
    if (i == INFO_SIZEOF) {
      Tcl_SetObjResult(interp, Tcl_NewIntObj(type->size));
      return TCL_OK;
    }
    if (i == INFO_ALIGNOF) {
      Tcl_SetObjResult(interp, Tcl_NewIntObj(type->alignment));
      return TCL_OK;
    }
    if (i == INFO_FORMAT) {
      size_t offset = 0;
      return type_format(interp, type, &offset);
    }
    Tcl_AppendResult(interp, "lost in ::ffidl::info?", NULL);
    return TCL_ERROR;
  case INFO_INTERP:
    /* return the interp as integer */
    if (objc != 2) {
      Tcl_WrongNumArgs(interp,2,objv,"");
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Ffidl_NewPointerObj(interp));
    return TCL_OK;
  case INFO_USE_FFCALL:
  case INFO_USE_LIBFFCALL:
#if USE_LIBFFCALL
    Tcl_SetObjResult(interp, Tcl_NewIntObj(1));
#else
    Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
#endif
    return TCL_OK;
  case INFO_USE_LIBFFI:
#if USE_LIBFFI
    Tcl_SetObjResult(interp, Tcl_NewIntObj(1));
#else
    Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
#endif
    return TCL_OK;
  case INFO_USE_CALLBACKS:
#if USE_CALLBACKS
    Tcl_SetObjResult(interp, Tcl_NewIntObj(1));
#else
    Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
#endif
    return TCL_OK;
  case INFO_HAVE_LONG_LONG:
#if HAVE_LONG_LONG
    Tcl_SetObjResult(interp, Tcl_NewIntObj(1));
#else
    Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
#endif
    return TCL_OK;
  case INFO_HAVE_LONG_DOUBLE:
#if HAVE_LONG_DOUBLE
    Tcl_SetObjResult(interp, Tcl_NewIntObj(1));
#else
    Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
#endif
    return TCL_OK;
  case INFO_USE_LIBFFI_RAW:
#if USE_LIBFFI_RAW_API
    Tcl_SetObjResult(interp, Tcl_NewIntObj(1));
#else
    Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
#endif
    return TCL_OK;
  case INFO_HAVE_INT64:
#if HAVE_INT64
    Tcl_SetObjResult(interp, Tcl_NewIntObj(1));
#else
    Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
#endif
    return TCL_OK;
  case INFO_CANONICAL_HOST:
    Tcl_SetObjResult(interp, Tcl_NewStringObj(CANONICAL_HOST,-1));
    return TCL_OK;
  case INFO_NULL:
    Tcl_SetObjResult(interp, Ffidl_NewPointerObj(NULL));
    return TCL_OK;
  }
  
  /* return an error */
  Tcl_AppendResult(interp, "missing option implementation: ", Tcl_GetString(objv[option_ix]), NULL);
  return TCL_ERROR;
}

/* usage: ffidl::typedef name type1 ?type2 ...? */
static int tcl_ffidl_typedef(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  enum {
    command_ix,
    name_ix,
    type_ix,
    minargs
  };

  char *tname1, *tname2;
  ffidl_type *newtype, *ttype2;
  int nelts, i;
  ffidl_client *client = (ffidl_client *)clientData;

  /* check number of args */
  if (objc < minargs) {
    Tcl_WrongNumArgs(interp,1,objv,"name type ?...?");
    return TCL_ERROR;
  }
  /* fetch new type name, verify that it is new */
  tname1 = Tcl_GetString(objv[name_ix]);
  if (type_lookup(client, tname1) != NULL) {
    Tcl_AppendResult(interp, "type is already defined: ", tname1, NULL);
    return TCL_ERROR;
  }
  nelts = objc - 2;
  if (nelts == 1) {
    /* define tname1 as an alias for tname2 */
    tname2 = Tcl_GetString(objv[type_ix]);
    ttype2 = type_lookup(client, tname2);
    if (ttype2 == NULL) {
      Tcl_AppendResult(interp, "undefined type: ", tname2, NULL);
      return TCL_ERROR;
    }
    /* define alias */
    type_define(client, tname1, ttype2);
    type_inc_ref(ttype2);
  } else {
    /* allocate an aggregate type */
    newtype = type_alloc(client, nelts);
    if (newtype == NULL) {
      Tcl_AppendResult(interp, "couldn't allocate the ffi_type", NULL);
      return TCL_ERROR;
    }
    /* parse aggregate types */
    newtype->size = 0;
    newtype->alignment = 0;
    for (i = 0; i < nelts; i += 1) {
      tname2 = Tcl_GetString(objv[type_ix+i]);
      ttype2 = type_lookup(client, tname2);
      if (ttype2 == NULL) {
	type_free(newtype);
	Tcl_AppendResult(interp, "undefined element type: ", tname2, NULL);
	return TCL_ERROR;
      }
      if ((ttype2->class & FFIDL_ELT) == 0) {
	type_free(newtype);
	Tcl_AppendResult(interp, "type ", tname2, " is not permitted in element context", NULL);
	return TCL_ERROR;
      }
      newtype->elements[i] = ttype2;
      /* accumulate the aggregate size and alignment */
      /* align current size to element's alignment */
      if ((ttype2->alignment-1) & newtype->size) {
	newtype->size = ((newtype->size-1) | (ttype2->alignment-1)) + 1;
      }
      /* add the element's size */
      newtype->size += ttype2->size;
      /* bump the aggregate alignment as required */
      if (ttype2->alignment > newtype->alignment) {
	newtype->alignment = ttype2->alignment;
      }
    }
    newtype->size = ((newtype->size-1) | (newtype->alignment-1)) + 1; /* tail padding as in libffi */
    if (type_prep(newtype) != TCL_OK) {
      type_free(newtype);
      Tcl_AppendResult(interp, "type definition error", NULL);
      return TCL_ERROR;
    }
    /* define new type */
    type_define(client, tname1, newtype);
    type_inc_ref(newtype);
  }
  /* return success */
  return TCL_OK;
}

/* usage: depends on the signature defining the ffidl::callout */
static int tcl_ffidl_call(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  enum {
    command_ix,
    args_ix,
    minargs = args_ix
  };

  ffidl_callout *callout = (ffidl_callout *)clientData;
  ffidl_cif *cif = callout->cif;
  int i, itmp;
  Tcl_Obj *obj = NULL;
  char buff[128];
  ffidl_tclobj_value obj_value = {0};

  /* usage check */
  if (objc-args_ix != cif->argc) {
    Tcl_WrongNumArgs(interp, 1, objv, callout->usage);
    return TCL_ERROR;
  }
  /* fetch and convert argument values */
  for (i = 0; i < cif->argc; i += 1) {
    /* fetch object */
    obj = objv[args_ix+i];
    /* fetch value from object and store value into arg value array */
    if (TCL_OK != value_convert_to_c(interp, cif->atypes[i], obj, &obj_value)) {
      Tcl_AppendResult(interp, ", converting callout argument value", NULL);
      goto cleanup;
    }
    switch (cif->atypes[i]->typecode) {
    case FFIDL_INT:
      *(int *)callout->args[i] = (int)obj_value.v_long;
      continue;
    case FFIDL_FLOAT:
      *(float *)callout->args[i] = (float)obj_value.v_double;
      continue;
    case FFIDL_DOUBLE:
      *(double *)callout->args[i] = (double)obj_value.v_double;
      continue;
#if HAVE_LONG_DOUBLE
    case FFIDL_LONGDOUBLE:
      *(long double *)callout->args[i] = (long double)obj_value.v_double;
      continue;
#endif
    case FFIDL_UINT8:
      *(UINT8_T *)callout->args[i] = (UINT8_T)obj_value.v_long;
      continue;
    case FFIDL_SINT8:
      *(SINT8_T *)callout->args[i] = (SINT8_T)obj_value.v_long;
      continue;
    case FFIDL_UINT16:
      *(UINT16_T *)callout->args[i] = (UINT16_T)obj_value.v_long;
      continue;
    case FFIDL_SINT16:
      *(SINT16_T *)callout->args[i] = (SINT16_T)obj_value.v_long;
      continue;
    case FFIDL_UINT32:
      *(UINT32_T *)callout->args[i] = (UINT32_T)obj_value.v_long;
      continue;
    case FFIDL_SINT32:
      *(SINT32_T *)callout->args[i] = (SINT32_T)obj_value.v_long;
      continue;
#if HAVE_INT64
    case FFIDL_UINT64:
      *(UINT64_T *)callout->args[i] = (UINT64_T)obj_value.v_wideint;
      continue;
    case FFIDL_SINT64:
      *(SINT64_T *)callout->args[i] = (SINT64_T)obj_value.v_wideint;
      continue;
#endif
    case FFIDL_STRUCT:
      if (obj->typePtr != ffidl_bytearray_ObjType) {
	sprintf(buff, "parameter %d must be a binary string", i);
	Tcl_AppendResult(interp, buff, NULL);
	goto cleanup;
      }
      callout->args[i] = (void *)Tcl_GetByteArrayFromObj(obj, &itmp);
      if (itmp != cif->atypes[i]->size) {
	sprintf(buff, "parameter %d is the wrong size, %u bytes instead of %lu.", i, itmp, (long)(cif->atypes[i]->size));
	Tcl_AppendResult(interp, buff, NULL);
	goto cleanup;
      }
      continue;
    case FFIDL_PTR:
#if FFIDL_POINTER_IS_LONG
      *(void **)callout->args[i] = (void *)obj_value.v_long;
#else
      *(void **)callout->args[i] = (void *)obj_value.v_wideint;
#endif
      continue;
    case FFIDL_PTR_OBJ:
      *(void **)callout->args[i] = (void *)obj;
      continue;
    case FFIDL_PTR_UTF8:
      *(void **)callout->args[i] = (void *)Tcl_GetString(obj);
      continue;
    case FFIDL_PTR_UTF16:
      *(void **)callout->args[i] = (void *)Tcl_GetUnicode(obj);
      continue;
    case FFIDL_PTR_BYTE:
      if (obj->typePtr != ffidl_bytearray_ObjType) {
	sprintf(buff, "parameter %d must be a binary string", i);
	Tcl_AppendResult(interp, buff, NULL);
	goto cleanup;
      }
      *(void **)callout->args[i] = (void *)Tcl_GetByteArrayFromObj(obj, &itmp);
      continue;
    case FFIDL_PTR_VAR:
      obj = Tcl_ObjGetVar2(interp, objv[args_ix+i], NULL, TCL_LEAVE_ERR_MSG);
      if (obj == NULL) return TCL_ERROR;
      if (obj->typePtr != ffidl_bytearray_ObjType) {
	sprintf(buff, "parameter %d must be a binary string", i);
	Tcl_AppendResult(interp, buff, NULL);
	goto cleanup;
      }
      if (Tcl_IsShared(obj)) {
	obj = Tcl_ObjSetVar2(interp, objv[args_ix+i], NULL, Tcl_DuplicateObj(obj), TCL_LEAVE_ERR_MSG);
	if (obj == NULL) {
	  goto cleanup;
	}
      }
      *(void **)callout->args[i] = (void *)Tcl_GetByteArrayFromObj(obj, &itmp);
      /* printf("pointer-var -> %d\n", cif->avalues[i].v_pointer); */
      Tcl_InvalidateStringRep(obj);
      continue;
#if USE_CALLBACKS
    case FFIDL_PTR_PROC: {
      ffidl_callback *callback;
      ffidl_closure *closure;
      Tcl_DString ds;
      char *name = Tcl_GetString(objv[args_ix+i]);
      Tcl_DStringInit(&ds);
      if (!strstr(name, "::")) {
        Tcl_Namespace *ns;
        ns = Tcl_GetCurrentNamespace(interp);
        if (ns != Tcl_GetGlobalNamespace(interp)) {
          Tcl_DStringAppend(&ds, ns->fullName, -1);
        }
        Tcl_DStringAppend(&ds, "::", 2);
        Tcl_DStringAppend(&ds, name, -1);
        name = Tcl_DStringValue(&ds);
      }
      callback = callback_lookup(callout->client, name);
      Tcl_DStringFree(&ds);
      if (callback == NULL) {
	Tcl_AppendResult(interp, "no callback named \"", Tcl_GetString(objv[args_ix+i]), "\" is defined", NULL);
	goto cleanup;
      }
      closure = &(callback->closure);
#if USE_LIBFFI
      *(void **)callout->args[i] = (void *)closure->executable;
#elif USE_LIBFFCALL
      *(void **)callout->args[i] = (void *)closure->lib_closure;
#endif
    }
    continue;
#endif
    default:
      sprintf(buff, "unknown type for argument: %d", cif->atypes[i]->typecode);
      Tcl_AppendResult(interp, buff, NULL);
      goto cleanup;
    }
    /* Note: change "continue" to "break" if further work must be done here. */
  }
  /* prepare for structure return */
  if (cif->rtype->typecode == FFIDL_STRUCT) {
    obj = Tcl_NewByteArrayObj(NULL, 0);
    Tcl_SetByteArrayLength(obj, cif->rtype->size);
    Tcl_IncrRefCount(obj);
    callout->ret = Tcl_GetByteArrayFromObj(obj, &itmp);
  }
  /* call */
  callout_call(callout);
  /* convert return value */
  switch (cif->rtype->typecode) {
  case FFIDL_VOID:	break;
  case FFIDL_INT:	Tcl_SetObjResult(interp, Tcl_NewLongObj((long)FFIDL_RVALUE_PEEK_UNWIDEN(INT, callout->ret))); break;
  case FFIDL_FLOAT:	Tcl_SetObjResult(interp, Tcl_NewDoubleObj((double)FFIDL_RVALUE_PEEK_UNWIDEN(FLOAT, callout->ret))); break;
  case FFIDL_DOUBLE:	Tcl_SetObjResult(interp, Tcl_NewDoubleObj((double)FFIDL_RVALUE_PEEK_UNWIDEN(DOUBLE, callout->ret))); break;
#if HAVE_LONG_DOUBLE
  case FFIDL_LONGDOUBLE:Tcl_SetObjResult(interp, Tcl_NewDoubleObj((double)FFIDL_RVALUE_PEEK_UNWIDEN(LONGDOUBLE, callout->ret))); break;
#endif
  case FFIDL_UINT8:	Tcl_SetObjResult(interp, Tcl_NewLongObj((long)FFIDL_RVALUE_PEEK_UNWIDEN(UINT8, callout->ret))); break;
  case FFIDL_SINT8:	Tcl_SetObjResult(interp, Tcl_NewLongObj((long)FFIDL_RVALUE_PEEK_UNWIDEN(SINT8, callout->ret))); break;
  case FFIDL_UINT16:	Tcl_SetObjResult(interp, Tcl_NewLongObj((long)FFIDL_RVALUE_PEEK_UNWIDEN(UINT16, callout->ret))); break;
  case FFIDL_SINT16:	Tcl_SetObjResult(interp, Tcl_NewLongObj((long)FFIDL_RVALUE_PEEK_UNWIDEN(SINT16, callout->ret))); break;
  case FFIDL_UINT32:	Tcl_SetObjResult(interp, Tcl_NewLongObj((long)FFIDL_RVALUE_PEEK_UNWIDEN(UINT32, callout->ret))); break;
  case FFIDL_SINT32:	Tcl_SetObjResult(interp, Tcl_NewLongObj((long)FFIDL_RVALUE_PEEK_UNWIDEN(SINT32, callout->ret))); break;
#if HAVE_INT64
  case FFIDL_UINT64:	Tcl_SetObjResult(interp, Ffidl_NewInt64Obj((Ffidl_Int64)FFIDL_RVALUE_PEEK_UNWIDEN(UINT64, callout->ret))); break;
  case FFIDL_SINT64:	Tcl_SetObjResult(interp, Ffidl_NewInt64Obj((Ffidl_Int64)FFIDL_RVALUE_PEEK_UNWIDEN(SINT64, callout->ret))); break;
#endif
  case FFIDL_STRUCT:	Tcl_SetObjResult(interp, obj); Tcl_DecrRefCount(obj); break;
  case FFIDL_PTR:	Tcl_SetObjResult(interp, Ffidl_NewPointerObj(FFIDL_RVALUE_PEEK_UNWIDEN(PTR, callout->ret))); break;
  case FFIDL_PTR_OBJ:	Tcl_SetObjResult(interp, (Tcl_Obj *)FFIDL_RVALUE_PEEK_UNWIDEN(PTR, callout->ret)); break;
  case FFIDL_PTR_UTF8:	Tcl_SetObjResult(interp, Tcl_NewStringObj(FFIDL_RVALUE_PEEK_UNWIDEN(PTR, callout->ret), -1)); break;
  case FFIDL_PTR_UTF16:	Tcl_SetObjResult(interp, Tcl_NewUnicodeObj(FFIDL_RVALUE_PEEK_UNWIDEN(PTR, callout->ret), -1)); break;
  default:
    sprintf(buff, "Invalid return type: %d", cif->rtype->typecode);
    Tcl_AppendResult(interp, buff, NULL);
    goto cleanup;
  }    
  /* done */
  return TCL_OK;
  /* blew it */
 cleanup:
  return TCL_ERROR;
}

/* usage: ffidl::callout name {?argument_type ...?} return_type address ?protocol? */
static int tcl_ffidl_callout(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  enum {
    command_ix,
    name_ix,
    args_ix,
    return_ix,
    address_ix,
    protocol_ix,
    minargs = address_ix + 1,
    maxargs = protocol_ix + 1,
  };

  char *name;
  void (*fn)(void);
  int argc, i;
  Tcl_Obj **argv;
  Tcl_DString usage, ds;
  Tcl_Command res;
  ffidl_cif *cif = NULL;
  ffidl_callout *callout;
  ffidl_client *client = (ffidl_client *)clientData;
  ffidl_value *rvalue = NULL;
  ffidl_value *avalues = NULL;
  int has_protocol = objc - 1 >= protocol_ix;

  /* usage check */
  if (objc != minargs && objc != maxargs) {
    Tcl_WrongNumArgs(interp, 1, objv, "name {?argument_type ...?} return_type address ?protocol?");
    return TCL_ERROR;
  }
  Tcl_DStringInit(&ds);
  Tcl_DStringInit(&usage);
  /* fetch name */
  name = Tcl_GetString(objv[name_ix]);
  if (!strstr(name, "::")) {
    Tcl_Namespace *ns;
    ns = Tcl_GetCurrentNamespace(interp);
    if (ns != Tcl_GetGlobalNamespace(interp)) {
      Tcl_DStringAppend(&ds, ns->fullName, -1);
    }
    Tcl_DStringAppend(&ds, "::", 2);
    Tcl_DStringAppend(&ds, name, -1);
    name = Tcl_DStringValue(&ds);
  }
  /* fetch cif */
  if (cif_parse(interp, client,
		objv[args_ix],
		objv[return_ix],
		has_protocol ? objv[protocol_ix] : NULL,
		&cif) == TCL_ERROR) {
    goto error;
  }
  /* fetch function pointer */
  if (Ffidl_GetPointerFromObj(interp, objv[address_ix], (void **)&fn) == TCL_ERROR) {
    goto error;
  }
  /* if callout is already defined, redefine it */
  if ((callout = callout_lookup(client, name))) {
    Tcl_DeleteCommand(interp, name);
  }
  /* build the usage string */
  Tcl_ListObjGetElements(interp, objv[args_ix], &argc, &argv);
  for (i = 0; i < argc; i += 1) {
    if (i != 0) Tcl_DStringAppend(&usage, " ", 1);
    Tcl_DStringAppend(&usage, Tcl_GetString(argv[i]), -1);
  }
  /* allocate the callout structure, including:
     - usage string
     - argument value pointers
     - argument values */
  callout = (ffidl_callout *)Tcl_Alloc(sizeof(ffidl_callout)
				       +cif->argc*sizeof(void*) /* args */
				       +sizeof(ffidl_value)	/* rvalue */
				       +cif->argc*sizeof(ffidl_value) /* avalues */
				       +Tcl_DStringLength(&usage)+1); /* usage */
  if (callout == NULL) {
    Tcl_AppendResult(interp, "can't allocate ffidl_callout for: ", name, NULL);
    goto error;
  }
  /* initialize the callout */
  callout->cif = cif;
  callout->fn = fn;
  callout->client = client;
  /* set up return and argument pointers */
  callout->args = (void **)(callout+1);
  rvalue = (ffidl_value *)(callout->args+cif->argc);
  avalues = (ffidl_value *)(rvalue+1);
  /* prep return value */
  if (callout_prep_value(interp, FFIDL_RET, objv[return_ix], cif->rtype,
			 rvalue, &callout->ret) == TCL_ERROR) {
    goto error;
  }
  /* prep argument values */
  for (i = 0; i < argc; i += 1) {
    if (callout_prep_value(interp, FFIDL_ARG, argv[i], cif->atypes[i],
			   &avalues[i], &callout->args[i]) == TCL_ERROR) {
      goto error;
    }
  }
  callout_prep(callout);
  /* set up usage string */
  callout->usage = (char *)((avalues+cif->argc));
  strcpy(callout->usage, Tcl_DStringValue(&usage));
  /* free the usage string */
  Tcl_DStringFree(&usage);
  /* define the callout */
  callout_define(client, name, callout);
  /* create the tcl command */
  res = Tcl_CreateObjCommand(interp, name, tcl_ffidl_call, (ClientData) callout, callout_delete);
  Tcl_DStringFree(&ds);
  return (res ? TCL_OK : TCL_ERROR);
error:
  Tcl_DStringFree(&ds);
  Tcl_DStringFree(&usage);
  if (cif) {
    cif_dec_ref(cif);
  }
  return TCL_ERROR;
}

#if USE_CALLBACKS
/* usage: ffidl::callback name {?argument_type ...?} return_type ?protocol? ?cmdprefix? -> */
static int tcl_ffidl_callback(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  enum {
    command_ix,
    name_ix,
    args_ix,
    return_ix,
    protocol_ix,
    cmdprefix_ix,
    minargs = return_ix + 1,
    maxargs = cmdprefix_ix + 1,
  };

  char *name;
  Tcl_Obj *nameObj;
  ffidl_cif *cif = NULL;
  Tcl_Obj **cmdv = NULL;
  int cmdc;
  Tcl_DString ds;
  ffidl_callback *callback = NULL;
  ffidl_client *client = (ffidl_client *)clientData;
  ffidl_closure *closure = NULL;
  void (*fn)();
  int has_protocol = objc - 1 >= protocol_ix;
  int has_cmdprefix = objc - 1 >= cmdprefix_ix;
  int i, argc = 0;
  Tcl_Obj **argv = NULL;

  /* usage check */
  if (objc < minargs || objc > maxargs) {
    Tcl_WrongNumArgs(interp, 1, objv, "name {?argument_type ...?} return_type ?protocol? ?cmdprefix?");
    return TCL_ERROR;
  }
  /* fetch name */
  Tcl_DStringInit(&ds);
  name = Tcl_GetString(objv[name_ix]);
  if (!strstr(name, "::")) {
    Tcl_Namespace *ns;
    ns = Tcl_GetCurrentNamespace(interp);
    if (ns != Tcl_GetGlobalNamespace(interp)) {
      Tcl_DStringAppend(&ds, ns->fullName, -1);
    }
    Tcl_DStringAppend(&ds, "::", 2);
    Tcl_DStringAppend(&ds, name, -1);
    name = Tcl_DStringValue(&ds);
  }
  /* fetch cif */
  if (cif_parse(interp, client,
		objv[args_ix],
		objv[return_ix],
		has_protocol ? objv[protocol_ix] : NULL,
		&cif) == TCL_ERROR) {
      goto error;
  }
  /* check types */
  if (cif_type_check_context(interp, FFIDL_CBRET,
			     objv[return_ix], cif->rtype) == TCL_ERROR) {
    goto error;
  }
  Tcl_ListObjGetElements(interp, objv[args_ix], &argc, &argv);
  for (i = 0; i < argc; i += 1)
    if (cif_type_check_context(interp, FFIDL_ARG,
			       objv[args_ix], cif->atypes[i]) == TCL_ERROR) {
      goto error;
    }
  /* create Tcl proc */
  if (has_cmdprefix) {
    Tcl_Obj *cmdprefix = objv[cmdprefix_ix];
    Tcl_IncrRefCount(cmdprefix);
    if (Tcl_ListObjGetElements(interp, cmdprefix, &cmdc, &cmdv) != TCL_OK) {
      goto error;
    }
    for (i = 0; i < cmdc; i++) {
      Tcl_IncrRefCount(cmdv[i]);
    }
    Tcl_DecrRefCount(cmdprefix);
  } else {
    /* the callback name is the command */
    nameObj = Tcl_NewStringObj(name, -1);
    cmdv = &nameObj;
    cmdc = 1;
    Tcl_IncrRefCount(nameObj);
  }

  /* allocate the callback structure */
  callback = (ffidl_callback *)Tcl_Alloc(sizeof(ffidl_callback)
					 /* cmdprefix and argument Tcl_Objs */
					 +(cmdc+cif->argc)*sizeof(Tcl_Obj *)
#if USE_LIBFFI_RAW_API
					 /* raw argument offsets */
					 +cif->argc*sizeof(ptrdiff_t)
#endif
  );
  if (callback == NULL) {
    Tcl_AppendResult(interp, "can't allocate ffidl_callback for: ", name, NULL);
    goto error;
  }
  /* initialize the callback */
  callback->cif = cif;
  callback->interp = interp;
  /* store the command prefix' Tcl_Objs */
  callback->cmdc = cmdc;
  callback->cmdv = (Tcl_Obj **)(callback+1);
  memcpy(callback->cmdv, cmdv, cmdc*sizeof(Tcl_Obj *));
  closure = &(callback->closure);
#if USE_LIBFFI
  closure->lib_closure = ffi_closure_alloc(sizeof(ffi_closure), &(closure->executable));
#if USE_LIBFFI_RAW_API
  callback->offsets = (ptrdiff_t *)(callback->cmdv+cmdc+cif->argc);
  callback->use_raw_api = cif_raw_supported(cif);
  if (callback->use_raw_api &&
      ffi_prep_raw_closure_loc((ffi_raw_closure *)closure->lib_closure, &callback->cif->lib_cif,
				 (void (*)(ffi_cif*,void*,ffi_raw*,void*))callback_callback,
                                 (void *)callback, closure->executable) == FFI_OK) {
    if (TCL_OK != cif_raw_prep_offsets(callback->cif, callback->offsets)) {
      goto error;
    }
    /* Prepared successfully, continue. */
  }
  else
#endif
  {
#if USE_LIBFFI_RAW_API
    callback->use_raw_api = 0;
#endif
    if (ffi_prep_closure_loc(closure->lib_closure, &callback->cif->lib_cif,
                            (void (*)(ffi_cif*,void*,void**,void*))callback_callback,
                            (void *)callback, closure->executable) != FFI_OK) {
      Tcl_AppendResult(interp, "libffi can't make closure for: ", name, NULL);
      goto error;
    }
  }
#elif USE_LIBFFCALL
  closure->lib_closure = alloc_callback((callback_function_t)&callback_callback,
					(void *)callback);
#endif
  /* define the callback */
  callback_define(client, name, callback);
  Tcl_DStringFree(&ds);

  /* Return function pointer to the callback. */
#if USE_LIBFFI
  fn = (void (*)())closure->executable;
#elif USE_LIBFFCALL
  fn = (void (*)())closure->lib_closure;
#endif
  Tcl_SetObjResult(interp, Ffidl_NewPointerObj(fn));

  return TCL_OK;

error:
  Tcl_DStringFree(&ds);
  if (cif) {
    cif_dec_ref(cif);
  }
  if (cmdv) {
    for (i = 0; i < cmdc; i++) {
      Tcl_DecrRefCount(cmdv[i]);
    }
  }
  if (closure && closure->lib_closure) {
#if USE_LIBFFI
    ffi_closure_free(closure->lib_closure);
#elif USE_LIBFFCALL
    free_callback(closure->lib_closure);
#endif
  }
  if (callback) {
      Tcl_Free((void *)callback);
  }
  return TCL_ERROR;
}
#endif

/* usage: ffidl::library library ?options...?*/
static int tcl_ffidl_library(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  enum {
    command_ix,
    optional_ix,
    minargs
  };

   static const char *options[] = {
      "-binding",
      "-visibility",
      "--",
      NULL,
   };

  enum {
    option_binding,
    option_visibility,
    option_break,
  };

   static const char *bindingOptions[] = {
      "now",
      "lazy",
      NULL,
   };

   enum {
     binding_now,
     binding_lazy,
   };

   static const char *visibilityOptions[] = {
      "global",
      "local",
      NULL,
   };

  enum {
    visibility_global,
    visibility_local,
  };

  int i = 0;
  ffidl_load_flags flags = {FFIDL_LOAD_BINDING_NONE, FFIDL_LOAD_VISIBILITY_NONE};
  Tcl_Obj *libraryObj;
  char *libraryName;
  ffidl_LoadHandle handle;
  ffidl_UnloadProc unload;
  ffidl_client *client = (ffidl_client *)clientData;

  if (objc < minargs) {
    Tcl_WrongNumArgs(interp, 1, objv, "?flags? ?--? library");
    return TCL_ERROR;
  }

  for (i = optional_ix; i < objc; ++i) {
      int option;
      int status = Tcl_GetIndexFromObj(interp, objv[i], options,
                                       "option", 0, &option);
      if (status != TCL_OK) {
	/* No options. */
	Tcl_ResetResult(interp);
	break;
      }

      if (option_break == option) {
	/* End of options. */
	i++;
	break;
      }

      switch (option) {
	case option_binding:
	{
	  int bindingOption;
	  int status;

	  i++;
	  status = Tcl_GetIndexFromObj(interp, objv[i], bindingOptions,
				       "binding", 0, &bindingOption);
	  if (status != TCL_OK) {
	    return TCL_ERROR;
	  }

	  switch (bindingOption) {
	    case binding_lazy:
	      flags.binding = FFIDL_LOAD_BINDING_LAZY;
	      break;
	    case binding_now:
	      flags.binding = FFIDL_LOAD_BINDING_NOW;
	      break;
	  }
	  break;
	}
	case option_visibility:
	{
	  int visibilityOption;
	  int status;

	  i++;
	  status = Tcl_GetIndexFromObj(interp, objv[i], visibilityOptions,
				       "visibility", 0, &visibilityOption);
	  if (status != TCL_OK) {
	    return TCL_ERROR;
	  }

	  switch (visibilityOption) {
	    case visibility_global:
	      flags.visibility = FFIDL_LOAD_VISIBILITY_GLOBAL;
	      break;
	    case visibility_local:
	      flags.visibility = FFIDL_LOAD_VISIBILITY_LOCAL;
	      break;
	  }
	  break;
	}
	case option_break:
	  /* Already handled above */
	  break;
      }
  }

  libraryObj = objv[i];
  libraryName = Tcl_GetString(libraryObj);
  handle = lib_lookup(client, libraryName, NULL);

  if (handle != NULL) {
    Tcl_AppendResult(interp, "library \"", libraryName, "\" already loaded", NULL);
    return TCL_ERROR;
  }

  if (ffidlopen(interp, libraryObj, flags, &handle, &unload) != TCL_OK) {
    return TCL_ERROR;
  }

  lib_define(client, libraryName, handle, unload);

  return TCL_OK;
}

/* usage: ffidl::symbol library symbol -> address */
static int tcl_ffidl_symbol(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  enum {
    command_ix,
    library_ix,
    symbol_ix,
    nargs
  };

  char *library;
  void *address;
  ffidl_LoadHandle handle;
  ffidl_UnloadProc unload;
  ffidl_client *client = (ffidl_client *)clientData;

  if (objc != nargs) {
    Tcl_WrongNumArgs(interp,1,objv,"library symbol");
    return TCL_ERROR;
  }

  library = Tcl_GetString(objv[library_ix]);
  handle = lib_lookup(client, library, NULL);

  if (handle == NULL) {
    ffidl_load_flags flags = {FFIDL_LOAD_BINDING_NONE, FFIDL_LOAD_VISIBILITY_NONE};
    if (ffidlopen(interp, objv[library_ix], flags, &handle, &unload) != TCL_OK) {
      return TCL_ERROR;
    }
    lib_define(client, library, handle, unload);
  }

  if (ffidlsym(interp, handle, objv[symbol_ix], &address) != TCL_OK) {
    return TCL_ERROR;
  }

  Tcl_SetObjResult(interp, Ffidl_NewPointerObj(address));
  return TCL_OK;
}

/* usage: ffidl::stubsymbol library stubstable symbolnumber -> address */
static int tcl_ffidl_stubsymbol(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  enum {
    command_ix,
    library_ix,
    stubstable_ix,
    symbol_ix,
    nargs
  };

  int library, stubstable, symbolnumber; 
  void **stubs = NULL, *address;
  static const char *library_names[] = {
    "tcl", 
#if defined(LOOKUP_TK_STUBS)
    "tk",
#endif
    NULL
  };
  enum libraries {
    LIB_TCL,
#if defined(LOOKUP_TK_STUBS)
    LIB_TK,
#endif
  };
  static const char *stubstable_names[] = {
    "stubs", "intStubs", "platStubs", "intPlatStubs", "intXLibStubs", NULL
  };
  enum stubstables {
    STUBS, INTSTUBS, PLATSTUBS, INTPLATSTUBS, INTXLIBSTUBS,
  };

  if (objc != 4) {
    Tcl_WrongNumArgs(interp,1,objv,"library stubstable symbolnumber");
    return TCL_ERROR;
  }
  if (Tcl_GetIndexFromObj(interp, objv[library_ix], library_names, "library", 0, &library) != TCL_OK) {
    return TCL_ERROR;
  }
  if (Tcl_GetIndexFromObj(interp, objv[stubstable_ix], stubstable_names, "stubstable", 0, &stubstable) != TCL_OK) {
    return TCL_ERROR;
  }
  if (Tcl_GetIntFromObj(interp, objv[symbol_ix], &symbolnumber) != TCL_OK || symbolnumber < 0) {
    return TCL_ERROR;
  }

#if defined(LOOKUP_TK_STUBS)
  if (library == LIB_TK) {
    if (MyTkInitStubs(interp, "8.4", 0) == NULL) {
      return TCL_ERROR;
    }
  }
#endif
  switch (stubstable) {
    case STUBS:
      stubs = (void**)(library == LIB_TCL ? tclStubsPtr : tkStubsPtr); break;
    case INTSTUBS:
      stubs = (void**)(library == LIB_TCL ? tclIntStubsPtr : tkIntStubsPtr); break;
    case PLATSTUBS:
      stubs = (void**)(library == LIB_TCL ? tclPlatStubsPtr : tkPlatStubsPtr); break;
    case INTPLATSTUBS:
      stubs = (void**)(library == LIB_TCL ? tclIntPlatStubsPtr : tkIntPlatStubsPtr); break;
    case INTXLIBSTUBS:
      stubs = (void**)(library == LIB_TCL ? NULL : tkIntXlibStubsPtr); break;
  }

  if (!stubs) {
    Tcl_AppendResult(interp, "no stubs table \"", Tcl_GetString(objv[stubstable_ix]),
        "\" in library \"", Tcl_GetString(objv[library_ix]), "\"", NULL);
    return TCL_ERROR;
  }
  address = *(stubs + 2 + symbolnumber);
  if (!address) {
    Tcl_AppendResult(interp, "couldn't find symbol number ", Tcl_GetString(objv[symbol_ix]),
        " in stubs table \"", Tcl_GetString(objv[stubstable_ix]), "\"", NULL);
    return TCL_ERROR;
  }

  Tcl_SetObjResult(interp, Ffidl_NewPointerObj(address));
  return TCL_OK;
}

/*
 * One function exported for pointer punning with ffidl::callout.
 */
void *ffidl_pointer_pun(void *p) { return p; }
void *ffidl_copy_bytes(void *dst, void *src, size_t len) {
  return memmove(dst, src, len);
}

/*
 *--------------------------------------------------------------
 *
 * Ffidl_Init
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
int Ffidl_Init(Tcl_Interp *interp)
{
  ffidl_client *client;

  if (Tcl_InitStubs(interp, "8.4", 0) == NULL) {
    return TCL_ERROR;
  }
  if (Tcl_PkgRequire(interp, "Tcl", "8.4", 0) == NULL) {
      return TCL_ERROR;
  }
  if (Tcl_PkgProvide(interp, "Ffidl", PACKAGE_VERSION) != TCL_OK) {
    return TCL_ERROR;
  }

  /* allocate and initialize client for this interpreter */
  client = client_alloc(interp);

  /* initialize commands */
  Tcl_CreateObjCommand(interp,"::ffidl::info", tcl_ffidl_info, (ClientData) client, NULL);
  Tcl_CreateObjCommand(interp,"::ffidl::typedef", tcl_ffidl_typedef, (ClientData) client, NULL);
  Tcl_CreateObjCommand(interp,"::ffidl::library", tcl_ffidl_library, (ClientData) client, NULL);
  Tcl_CreateObjCommand(interp,"::ffidl::symbol", tcl_ffidl_symbol, (ClientData) client, NULL);
  Tcl_CreateObjCommand(interp,"::ffidl::stubsymbol", tcl_ffidl_stubsymbol, (ClientData) client, NULL);
  Tcl_CreateObjCommand(interp,"::ffidl::callout", tcl_ffidl_callout, (ClientData) client, NULL);
#if USE_CALLBACKS
  Tcl_CreateObjCommand(interp,"::ffidl::callback", tcl_ffidl_callback, (ClientData) client, NULL);
#endif

  /* determine Tcl_ObjType * for some types */
  ffidl_bytearray_ObjType = Tcl_GetObjType("bytearray");
  ffidl_int_ObjType = Tcl_GetObjType("int");
#if HAVE_WIDE_INT
  ffidl_wideInt_ObjType = Tcl_GetObjType("wideInt");
#endif
  ffidl_double_ObjType = Tcl_GetObjType("double");

  /* done */
  return TCL_OK;
}

#if defined(LOOKUP_TK_STUBS)
typedef struct MyTkStubHooks {
    void *tkPlatStubs;
    void *tkIntStubs;
    void *tkIntPlatStubs;
    void *tkIntXlibStubs;
} MyTkStubHooks;

typedef struct MyTkStubs {
    int magic;
    struct MyTkStubHooks *hooks;
} MyTkStubs;

/* private copy of Tk_InitStubs to avoid having to depend on Tk at build time */
static const char *
MyTkInitStubs(interp, version, exact)
    Tcl_Interp *interp;
    char *version;
    int exact;
{
    const char *actualVersion;

    actualVersion = Tcl_PkgRequireEx(interp, "Tk", version, exact,
		(ClientData *) &tkStubsPtr);
    if (!actualVersion) {
	return NULL;
    }

    if (!tkStubsPtr) {
	Tcl_SetResult(interp,
		"This implementation of Tk does not support stubs",
		TCL_STATIC);
	return NULL;
    }
    
    tkPlatStubsPtr =    ((MyTkStubs*)tkStubsPtr)->hooks->tkPlatStubs;
    tkIntStubsPtr =     ((MyTkStubs*)tkStubsPtr)->hooks->tkIntStubs;
    tkIntPlatStubsPtr = ((MyTkStubs*)tkStubsPtr)->hooks->tkIntPlatStubs;
    tkIntXlibStubsPtr = ((MyTkStubs*)tkStubsPtr)->hooks->tkIntXlibStubs;
    
    return actualVersion;
}
#endif

/* Local Variables: */
/* c-basic-offset: 2 */
/* End: */
