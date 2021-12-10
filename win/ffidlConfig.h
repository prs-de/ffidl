/*
 * ffidlConfig.h.
 *
 * Unlike the TEA based builds, this file is NOT automatically generated.
 * It is intended to be used only with the nmake builds with VC++
 * (tested with VS 2017, earlier releases may or may not work).
 *
 * This file is includable by both x86 and x64 builds.
 */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* The normal alignment of `char', in bytes. */
#define ALIGNOF_CHAR 1

/* The normal alignment of `double', in bytes. */
#define ALIGNOF_DOUBLE 8

/* The normal alignment of `float', in bytes. */
#define ALIGNOF_FLOAT 4

/* The normal alignment of `int', in bytes. */
#define ALIGNOF_INT 4

/* The normal alignment of `long', in bytes. */
#define ALIGNOF_LONG 4

/* The normal alignment of `long double', in bytes. */
#define ALIGNOF_LONG_DOUBLE 8

/* The normal alignment of `long long', in bytes. */
#define ALIGNOF_LONG_LONG 8

/* The normal alignment of `short', in bytes. */
#define ALIGNOF_SHORT 2

/* The normal alignment of `void *', in bytes. */
#ifdef _WIN64
#define ALIGNOF_VOID_P 8
#else
#define ALIGNOF_VOID_P 4
#endif

/* Build windows export dll */
/* #define BUILD_Ffidl 1 - not needed - already defined by nmake build system */

/* Canonical host name */
#ifdef _WIN64
#define CANONICAL_HOST "x86_64-pc-cl"
#else
#define CANONICAL_HOST "i386-pc-cl"
#endif

/* Defined when cygwin/mingw does not support EXCEPTION DISPOSITION */
/* #undef EXCEPTION_DISPOSITION */

/* Defined when compiler supports casting to union type. */
/* #undef HAVE_CAST_TO_UNION */

#ifdef _WIN64

/* libffi supports FFI_WIN64 on this platform. */
#define HAVE_FFI_WIN64 /**/

#else /*  ! _WIN64 */

/* libffi supports FFI_FASTCALL on this platform. */
#define HAVE_FFI_FASTCALL /**/

/* libffi supports FFI_MS_CDECL on this platform. */
#define HAVE_FFI_MS_CDECL /**/

/* libffi supports FFI_STDCALL on this platform. */
#define HAVE_FFI_STDCALL /**/

/* libffi supports FFI_THISCALL on this platform. */
#define HAVE_FFI_THISCALL /**/

#endif /* _WIN64 */

/* libffi supports FFI_EFI64 on this platform. */
/* #undef HAVE_FFI_EFI64 */

/* libffi supports FFI_GNUW64 on this platform. */
/* #undef HAVE_FFI_GNUW64 - Visual C++ does not support 128 bit long double */

/* libffi supports FFI_REGISTER on this platform. */
/* #undef HAVE_FFI_REGISTER */

/* libffi supports FFI_SYSV on this platform. */
/* #undef HAVE_FFI_SYSV */

/* libffi supports FFI_UNIX64 on this platform. */
/* #undef HAVE_FFI_UNIX64 */

/* libffi supports FFI_PASCAL on this platform. */
/* #undef HAVE_FFI_PASCAL */

/* Compiler support for module scope symbols */
/* #undef HAVE_HIDDEN */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Do we have <limits.h>? */
/* #undef HAVE_LIMITS_H */

/* Define to 1 if the system has the type `long double'. */
#define HAVE_LONG_DOUBLE 1

/* Define to 1 if the type `long double' works and has more range or precision
   than `double'. */
/* #undef HAVE_LONG_DOUBLE_WIDER */

/* Define to 1 if the system has the type `long long'. */
#define HAVE_LONG_LONG 1

/* Define to 1 if you have the `lseek64' function. */
/* #undef HAVE_LSEEK64 */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Do we have <net/errno.h>? */
/* #undef HAVE_NET_ERRNO_H */

/* Defined when mingw does not support SEH */
/* #undef HAVE_NO_SEH */

/* Define to 1 if you have the `open64' function. */
/* #undef HAVE_OPEN64 */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
/* #undef HAVE_STRINGS_H */

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Is 'struct dirent64' in <sys/types.h>? */
/* #undef HAVE_STRUCT_DIRENT64 */

/* Is 'struct stat64' in <sys/stat.h>? */
/* #undef HAVE_STRUCT_STAT64 */

/* Define to 1 if you have the <sys/param.h> header file. */
/* #undef HAVE_SYS_PARAM_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Is off64_t in <sys/types.h>? */
/* #undef HAVE_TYPE_OFF64_T */

/* Define to 1 if you have the <unistd.h> header file. */
/* #undef HAVE_UNISTD_H */

/* Defined when cygwin/mingw ignores VOID define in winnt.h */
/* #undef HAVE_WINNT_IGNORE_VOID */

/* ffidl::stubsymbol can lookup in Tk stubs tables */
#define LOOKUP_TK_STUBS 1

/* Do we have <dirent.h>? */
/* #undef NO_DIRENT_H */

/* Do we have <dlfcn.h>? */
/* #undef NO_DLFCN_H */

/* Do we have <errno.h>? */
/* #undef NO_ERRNO_H */

/* Do we have <limits.h>? */
/* #undef NO_LIMITS_H */

/* Do we have <stdlib.h>? */
/* #undef NO_STDLIB_H */

/* Do we have <string.h>? */
/* #undef NO_STRING_H */

/* Do we have <sys/wait.h>? */
/* #undef NO_SYS_WAIT_H */

/* Do we have <values.h>? */
/* #undef NO_VALUES_H */

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
/* #define PACKAGE_NAME "Ffidl" - defined by nmake build system */

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "ffidl"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
/* #define PACKAGE_VERSION "0.9b0" - defined by nmake build system */

/* Define to the full name and version of this package. */
#define PACKAGE_STRING PACKAGE_NAME " " PACKAGE_VERSION

/* The size of `char', as computed by sizeof. */
#define SIZEOF_CHAR 1

/* The size of `double', as computed by sizeof. */
#define SIZEOF_DOUBLE 8

/* The size of `float', as computed by sizeof. */
#define SIZEOF_FLOAT 4

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* The size of `long double', as computed by sizeof. */
#define SIZEOF_LONG_DOUBLE 8

/* The size of `long long', as computed by sizeof. */
#define SIZEOF_LONG_LONG 8

/* The size of `short', as computed by sizeof. */
#define SIZEOF_SHORT 2

/* The size of `void *', as computed by sizeof. */
#ifdef _WIN64
#define SIZEOF_VOID_P 8
#else
#define SIZEOF_VOID_P 4
#endif

/* This a static build */
/* #undef STATIC_BUILD - automatically defined by nmake build system */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Is memory debugging enabled? */
/* #undef TCL_MEM_DEBUG - defined by nmake build system */

/* Are we building with threads enabled? */
/* #define TCL_THREADS 1 - defined by nmake build system */

/* Are wide integers to be implemented with C 'long's? */
/* #undef TCL_WIDE_INT_IS_LONG */

/* What type should be used to define wide integers? */
#define TCL_WIDE_INT_TYPE __int64

/* UNDER_CE version */
/* #undef UNDER_CE */

/* Implement callbacks */
#define USE_CALLBACKS 1

/* Use libffcall for foreign function calls */
/* #undef USE_LIBFFCALL */

/* Use libffi for foreign function calls */
#define USE_LIBFFI 1

/* Use TclOO stubs */
#define USE_TCLOO_STUBS 1

/* Use Tcl*Dlopen() API to load code */
/* #undef USE_TCL_DLOPEN */

/* Use Tcl_LoadFile() API to load code */
#define USE_TCL_LOADFILE 1

/* Use Tcl stubs */
/* #define USE_TCL_STUBS 1 - defined by nmake build system */

/* Do we want to use the threaded memory allocator? */
/* #undef USE_THREAD_ALLOC - defined by nmake build system */

/* Use Tk stubs */
/* #undef USE_TK_STUBS */

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Add the _ISOC99_SOURCE flag when building */
/* #undef _ISOC99_SOURCE */

/* Add the _LARGEFILE64_SOURCE flag when building */
/* #undef _LARGEFILE64_SOURCE */

/* Add the _LARGEFILE_SOURCE64 flag when building */
/* #undef _LARGEFILE_SOURCE64 */

/* # needed in sys/socket.h Should OS/390 do the right thing with sockets? */
/* #undef _OE_SOCKETS */

/* Do we really want to follow the standard? Yes we do! */
/* #undef _POSIX_PTHREAD_SEMANTICS */

/* Do we want the reentrant OS API? */
/* #undef _REENTRANT */

/* Do we want the thread-safe OS API? */
/* #undef _THREAD_SAFE */

/* _WIN32_WCE version */
/* #undef _WIN32_WCE */

/* Do we want to use the XOPEN network library? */
/* #undef _XOPEN_SOURCE_EXTENDED */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#define inline __inline
#endif

/* Define to 1 if type `char' is unsigned and you are not using gcc.  */
#ifndef __CHAR_UNSIGNED__
/* # undef __CHAR_UNSIGNED__ */
#endif
