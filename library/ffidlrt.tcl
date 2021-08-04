#
# Ffidl interface to Tcl8.2
#
# Run time support for Ffidl.
#
# NOTE: Remember to update FFIDLRT_VERSION in configure.ac when changing this
#       version number.
package provide Ffidlrt 0.4
package require Ffidl

namespace eval ::ffidl:: {}

proc ::ffidl::find-pkg-lib {pkg} {
    package require $pkg
    foreach i [::info loaded {}] {
        foreach {l p} $i {}
        if {$p eq "$pkg"} {
            return $l
        }
    }
    # ignore errors when running under pkg_mkIndex:
    if {![llength [info commands __package_orig]] } {
        return -code error "Library for package $pkg not found"
    }
}

namespace eval ::ffidl:: {
    set ffidl_lib [find-pkg-lib Ffidl]
    array set libs [list ffidl [list $ffidl_lib] ffidl_test [list $ffidl_lib]]
    unset ffidl_lib
    
    # 'libs' array is used by the ::ffidl::find-lib
    # abstraction to store the resolved lib paths
    #
    # 'types' and 'typedefs' arrays are used by the ::ffidl::find-type
    # abstraction to store resolved system types
    # and whether they have already been defined
    # with ::ffidl::typedef
    array set typedefs {}
    switch -exact $tcl_platform(platform) {
	unix {
	    switch -glob $tcl_platform(os) {
                Darwin {
                    array set libs {
                        c System.framework/System
                        m System.framework/System
                        gdbm {}
                        gmp {}
                        mathswig libmathswig0.5.dylib
                    }
                    array set types {
                        size_t {{unsigned long}}
                        clock_t {{unsigned long}}
                        time_t long
                        timeval {uint32 uint32}
                    }
                }
                Linux {
                    if {$tcl_platform(wordSize) == 8} {
                        if {$tcl_platform(machine) eq "alpha"} {
                            array set libs {
                                c /lib/libc.so.6.1
                                m /lib/libm.so.6.1
                                gdbm /usr/lib/libgdbm.so
                                gmp {/usr/local/lib/libgmp.so /usr/lib/libgmp.so.2}
                                mathswig libmathswig0.5.so
                            }
                            array set types {
                                size_t long
                                clock_t long
                                time_t long
                                timeval {time_t time_t}
                            }                        
                        } else {
                            array set libs {
                                c {
				    /lib64/libc.so.6
				    /lib/x86_64-linux-gnu/libc.so.6
				}
                                m {
				    /lib64/libm.so.6
				    /lib/x86_64-linux-gnu/libm.so.6
				}
                                gdbm {
				    /usr/lib64/libgdbm.so
				    /usr/lib/x86_64-linux-gnu/libgdbm.so
				}
                                gmp {
				    /usr/lib/x86_64-linux-gnu/libgmp.so
				    /usr/local/lib64/libgmp.so
				    /usr/lib64/libgmp.so.2
				}
                                mathswig libmathswig0.5.so
                            }
                            array set types {
                                size_t long
                                clock_t long
                                time_t long
                                timeval {time_t time_t}
                            }
                        }
                    } else {
                        array set libs {
                            c {
                                    /lib/libc.so.6
                                    /lib/i386-linux-gnu/libc.so.6
                            }
                            m {
                                    /lib/libm.so.6
                                    /lib/i386-linux-gnu/libm.so.6
                            }
                            gdbm {
                                    /usr/lib/libgdbm.so
                                    /usr/lib/i386-linux-gnu/libgdbm.so.3
                            }
                            gmp {
                                   /usr/lib/i386-linux-gnu/libgmp.so.2
                                   /usr/local/lib/libgmp.so
                                   /usr/lib/libgmp.so.2
                            }
                            mathswig libmathswig0.5.so
                        }
                        array set types {
                            size_t int
                            clock_t long
                            time_t long
                            timeval {time_t time_t}
                        }
                    }
                }
                *BSD {
                    array set libs {
                        c {/usr/lib/libc.so /usr/lib/libc.so.30.1}
                        m {/usr/lib/libm.so /usr/lib/libm.so.1.0}
                        gdbm libgdbm.so
                        gmp libgmp.so
                        mathswig libmathswig0.5.so
                    }
                    array set types {
                        size_t int
                        clock_t long
                        time_t long
                        timeval {time_t time_t}
                    }
                }
                default {
                    array set libs {
                        c /lib/libc.so
                        m /lib/libm.so
                        gdbm libgdbm.so
                        gmp libgmp.so
                        mathswig libmathswig0.5.so
                    }
                    array set types {
                        size_t int
                        clock_t long
                        time_t long
                        timeval {time_t time_t}
                    }
                }
	    }
	}
	windows {
            #
            # found libraries
            # this array is used by the ::ffidl::find-lib
            # abstraction to store the resolved lib paths
            #
            # CHANGE - put your resolved lib paths here
            #
	    array set libs {
		c msvcrt.dll
		m msvcrt.dll
		gdbm {}
		gmp gmp202.dll
		mathswig mathswig05.dll
	    }
            #
            # found types
            # these arrays are used by the ::ffidl::find-type
            # abstraction to store resolved system types
            # and whether they have already been defined
            # with ::ffidl::typedef
            #
            # CHANGE - put your resolved system types here
            #
            array set types {
                size_t int
                clock_t long
                time_t long
                timeval {time_t time_t}
            }
            array set typedefs {
            }
	}
    }
}

#
# find a shared library given a root name
# this is an abstraction in search of a
# solution.
#
# currently wired for my linux box
#
proc ::ffidl::find-lib {root} {
    upvar \#0 ::ffidl::libs libs
    if { ! [::info exists libs($root)] || [llength libs($root)] == 0} {
	error "::ffidl::find-lib $root - no mapping defined for $root"
    }
    if {[llength $libs($root)] > 1} {
	foreach l $libs($root) {
	    if {[file exists $l]} {
		set libs($root) $l
		break
	    }
	}
    }
    lindex $libs($root) 0
}

#
# find a typedef for a standard type
# and define it with ::ffidl::typedef
# if not already done
#
# currently wired for my linux box
#
proc ::ffidl::find-type {type} {
    upvar \#0 ::ffidl::types types
    upvar \#0 ::ffidl::typedefs typedefs
    if { ! [::info exists types($type)]} {
	error "::ffidl::find-type $type - no mapping defined for $type"
    }
    if { ! [::info exists typedefs($type)]} {
	eval ::ffidl::typedef $type $types($type)
	set typedefs($type) 1
    }
}
    
#
# get the address of the string rep of a Tcl_Obj
# get the address of the unicode rep of a Tcl_Obj
# get the address of the bytearray rep of a Tcl_Obj
#
# CAUTION - anything which alters the Tcl_Obj may
# invalidate the results of this function.  Use
# only in circumstances where the Tcl_Obj will not
# be modified in any way.
#
# CAUTION - the memory pointed to by the addresses
# returned by ::ffidl::get-string and ::ffidl::get-unicode
# is managed by Tcl, the contents should never be
# modified.
#
# The memory pointed to by ::ffidl::get-bytearray may
# be modified if care is taken to respect its size,
# and if shared references to the bytearray object
# are known to be compatible with the modification.
#

::ffidl::callout ::ffidl::get-string {pointer-obj} pointer [::ffidl::stubsymbol tcl stubs 340]; #Tcl_GetString
::ffidl::callout ::ffidl::get-unicode {pointer-obj} pointer [::ffidl::stubsymbol tcl stubs 382]; #Tcl_GetUnicode
::ffidl::callout ::ffidl::get-bytearray-from-obj {pointer-obj pointer-var} pointer [::ffidl::stubsymbol tcl stubs 33]; #Tcl_GetByteArrayFromObj

proc ::ffidl::get-bytearray {obj} {
    set len [binary format [::ffidl::info format int] 0]
    ::ffidl::get-bytearray-from-obj $obj len
}

#
# create a new string Tcl_Obj
# create a new unicode Tcl_Obj
# create a new bytearray Tcl_Obj
#
# I'm not sure if these are actually useful
#

::ffidl::callout ::ffidl::new-string {pointer int} pointer-obj [::ffidl::stubsymbol tcl stubs 56]; #Tcl_NewStringObj
::ffidl::callout ::ffidl::new-unicode {pointer int} pointer-obj [::ffidl::stubsymbol tcl stubs 378]; #Tcl_NewUnicodeObj
::ffidl::callout ::ffidl::new-bytearray {pointer int} pointer-obj [::ffidl::stubsymbol tcl stubs 50]; #Tcl_NewByteArrayObj

::ffidl::find-type size_t
if {1} {
    # Tcl's allocator: malloc, free, realloc.
    ::ffidl::callout ::ffidl::malloc {unsigned} pointer [::ffidl::stubsymbol tcl stubs 3]; #Tcl_Alloc
    ::ffidl::callout ::ffidl::realloc {pointer unsigned} pointer [::ffidl::stubsymbol tcl stubs 5]; #Tcl_Realloc
    ::ffidl::callout ::ffidl::free {pointer} void [::ffidl::stubsymbol tcl stubs 4]; #Tcl_Free
} else {
    # access the standard allocator: malloc, free, realloc.
    ::ffidl::callout ::ffidl::malloc {size_t} pointer [::ffidl::symbol [::ffidl::find-lib c] malloc]
    ::ffidl::callout ::ffidl::realloc {pointer size_t} pointer [::ffidl::symbol [::ffidl::find-lib c] realloc]
    ::ffidl::callout ::ffidl::free {pointer} void [::ffidl::symbol [::ffidl::find-lib c] free]
}

#
# Copy some memory at some location into a Tcl bytearray.
#
# Needless to say, this can be very hazardous to your
# program's health if things aren't sized correctly.
#
::ffidl::callout ::ffidl::memcpy {pointer-var pointer size_t} pointer [::ffidl::symbol [::ffidl::find-lib ffidl] ffidl_copy_bytes];

#
# Regular memcpy working on pointers.  ::ffidl::memcpy kept as is for compatibilitiy.
#
::ffidl::callout ::ffidl::memcpy2 {pointer pointer size_t} pointer [::ffidl::symbol [::ffidl::find-lib ffidl] ffidl_copy_bytes];

#
# Create a Tcl bytearray with a copy of the contents some memory location.
#
proc ::ffidl::peek {address nbytes} {
    set dst [binary format x$nbytes]
    ::ffidl::memcpy dst $address $nbytes
    set dst
}

#
# Copy the contents of a Tcl bytearray to some memory location.
#
proc ::ffidl::poke {dst src} {
    set n [string length $bytes];
    set src [::ffidl::get-bytearray $bytes];
    ::ffidl::memcpy2 $dst $src $n;
}

#
# convert raw pointers, as integers, into Tcl_Obj's
#
::ffidl::callout ::ffidl::pointer-into-string {pointer} pointer-utf8 [::ffidl::symbol [::ffidl::find-lib ffidl] ffidl_pointer_pun]
::ffidl::callout ::ffidl::pointer-into-unicode {pointer} pointer-utf16 [::ffidl::symbol [::ffidl::find-lib ffidl] ffidl_pointer_pun]
# ::ffidl::pointer-into-bytearray is deprecated. Use ::ffidl::peek instead.
interp alias {} ::ffidl::pointer-into-bytearray {} ::ffidl::peek;
