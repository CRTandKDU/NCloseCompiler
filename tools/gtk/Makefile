##===- projects/sample/tools/sample/Makefile ---------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

#
# Give the name of the tool.
#
TOOLNAME=nkb-couchgtk

#
# Includes needed
#
xerces_builddir=/home/jmc/xerces-c-3.1.1
CPPFLAGS += `pkg-config --cflags /usr/lib/pkgconfig/gtk+-2.0.pc` -fexceptions -I. -I$(xerces_builddir) -I$(xerces_builddir)/src/ 

#
# List libraries that we'll need
# We use LIBS because sample is a dynamic library.
#
# USEDLIBS = nkb-compile.a
ExtraLibs =  `pkg-config --libs /usr/lib/pkgconfig/gtk+-2.0.pc` -L/usr/local/lib  -lxerces-c-3.1 -lnsl  -lpthread -ljansson -lcurl -Wl,-Bsymbolic-functions -lgssapi_krb5 -lm -L/usr/lib -licui18n -licuuc -licudata -lm  
LINK_COMPONENTS += core jit native interpreter codegen asmparser


#
# Include Makefile.common so we know what to do.
#
include $(LEVEL)/Makefile.common

