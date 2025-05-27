
##### BUILD can be DEBUG or RELEASE.
#
BUILD = DEBUG

HV3_POLIPO = C:\Users\billa\Downloads\hv3_polipo.exe

##### Version of and path to the Tcl installation to use.
#
TCLVERSION = 86
TCL = /mingw64

##### Flags passed to the C-compiler to link to Tcl.
#
# TCLLIB = -L$(TCL)/lib -ltclstub$(TCLVERSION) -ltkstub$(TCLVERSION)
TCLLIB = -L$(TCL)/lib -ltcl$(TCLVERSION) -ltk$(TCLVERSION) -ltclstub$(TCLVERSION) -ltkstub$(TCLVERSION)

##### Extra libraries used by Tcl on Linux. These flags are only required to
#     staticly link Tcl into an executable
#
# TCLLIB_DEBUG += -L/usr/X11R6/lib/ -lX11 -ldl -lm

BCC = x86_64-w64-mingw32-gcc
CC = gcc

CFLAGS_RELEASE = -O2 -DNDEBUG 
CFLAGS_DEBUG   = -g
CFLAGS = $(CFLAGS_$(BUILD))
CFLAGS += -DUSE_TCL_STUBS=1 -DUSE_TK_STUBS=1

##### The name of the shared library file to build.
#
SHARED_LIB_DEBUG = Tkhtml30g.dll
SHARED_LIB_RELEASE = Tkhtml30.dll
SHARED_LIB = $(SHARED_LIB_$(BUILD))

##### Command to build a shared library from a set of object files. The
#     command executed will be:
# 
#         $(MKSHLIB) $(OBJS) $(TCLSTUBSLIB) -o $(SHARED_LIB)
#
MKSHLIB = $(CC) -shared 
TCLSTUBSLIB =  "$(TCL)/lib/libtclstub$(TCLVERSION).a" 
TCLSTUBSLIB += "$(TCL)/lib/libtkstub$(TCLVERSION).a" 
TCLSTUBSLIB += -LC:/Tcl/lib

##### Commands to run tclsh on the build platform (to generate C files
#     to be passed to $(CC) from tcl scripts).
#
TCLSH = tclsh

##### Strip the shared library
#
STRIP_RELEASE = strip
STRIP_DEBUG = true
STRIP = $(STRIP_$(BUILD))

STARKITRT = /home/billa/work/tclkitsh-win32.upx.exe
MKSTARKIT = $(STARKITRT) /home/billa/sdx.kit wrap

##### Javascript libaries - libgc.a and libsee.a
#
JS_SHARED_LIB = libTclsee.dll

JSLIB   = /home/billa/SEE-mirror/libsee/.libs/libsee.a
JSLIB  += /mingw64/lib/libgc.a
JSFLAGS = -I/home/billa/SEE-mirror/include

#
# End of configuration section.
###########################################################################

default: binaries

hv3-win32.exe: hv3_img.kit
	cp $(STARKITRT) starkit_runtime
	$(MKSTARKIT) hv3_img.bin -runtime ./starkit_runtime
	mv hv3_img.bin hv3-win32.exe
	chmod 644 hv3-win32.exe

###############################################################################

##### Top of the Tkhtml source tree - the directory with this file in it.
#
TOP = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

include $(TOP)/main.mk

