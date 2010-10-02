##===- projects/sample/tools/sample/Makefile ---------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

#
# Give the name of the tool.
#
TOOLNAME=nkb-knowcess

#
# Includes needed
#
xerces_builddir=/home/jmc/xerces-c-3.1.1
CPPFLAGS += -fexceptions -I. -I$(xerces_builddir) -I$(xerces_builddir)/src/ 

#
# List libraries that we'll need
# We use LIBS because sample is a dynamic library.
#
# USEDLIBS = nkb-compile.a
ExtraLibs =  -L/usr/local/lib  -lxerces-c-3.1 -lnsl  -lpthread -lcurl -Wl,-Bsymbolic-functions -lgssapi_krb5 -lm -L/usr/lib -licui18n -licuuc -licudata -lm  
LINK_COMPONENTS += core jit native interpreter codegen asmparser


#
# Include Makefile.common so we know what to do.
#
include $(LEVEL)/Makefile.common

