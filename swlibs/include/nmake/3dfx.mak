#
# $Log:
# 1   ve3d      1.0         11/24/01 KoolSmoky    initial win32 global makefile
#

#
# library source sets FX_DLL_DEFINITION before including fxdll.h to indicate
# next header is interface file
#
!if "$(FX_TARGET)" == "WIN32" && "$(FX_DLL_BUILD)"=="1" && "$(FX_DLL_LIBRARY)"=="1"
FX_DLL_COMPILE  = 1
LINK_DLL        = 1
!endif

!if "$(FX_TARGET)" == "WIN32" && "$(FX_DLL_BUILD)"=="1" && "$(FX_DLL_COMPILE)"=="1"
COMPILE_DLL     = 1
!endif

#
# compiler flags:
#
!if "$(FX_COMPILER)" == "MICROSOFT"

!if "$(CC)"==""
CC             = cl
CPLUSPLUS      = cl
!endif
CDEBUG         = -Z7
CNODEBUG       = /Ox
LDEBUG         = -Z7
LINK           = $(CC)
GCOPTS         = /nologo /G6 /W3 /c
GCDEFS         = -D__MSC__=1 -D_X86_=1 -DNULL=0
!if "$(FX_TARGET_MINOR)" == "WIN95"
GLDOPTS        = -nologo /link /map -subsystem:windows,4.0 -debugtype:none -pdb:none -incremental:no -release
!else
!if "$(DEBUG)"=="1"
GLDOPTS        = -nologo /link -subsystem:console -debugtype:both -pdb:none
!else
GLDOPTS        = -link -pdb:none -incremental:no -release
!endif
!endif
CPP            = $(CC) -E /c
LNODEBUG       = $(CDEBUG)
GCINCS         = -I$(BUILD_ROOT_SWLIBS)\include 

!endif

#
# assembler flags:
#
AOPTS          = /coff
ASM_LIST_FLAGS = /FAsc
AR             = lib -nologo
!if "$(AS)"==""
AS             = ml
!endif
ADEBUG         = /Zi
AOPTS          = $(AOPTS) /I. /c /Cp 
ANODEBUG       = 

# if we are not debugging then replace debug flags with nodebug flags
!ifndef DEBUG
CDEBUG   = $(CNODEBUG)
LDEBUG   = $(LNODEBUG)
ADEBUG   = $(ANODEBUG)
!endif

#
# target environment configs:
#
!if "$(FX_TARGET)" == "WIN32"
GCDEFS     = $(GCDEFS) -D_WIN32 -DWIN32=1 -D__WIN32__=1

!ifdef COMPILE_DLL
GCDEFS     = $(GCDEFS) -DFX_DLL_ENABLE=1
!endif

baselibs   = $(optlibs) advapi32.lib #kernel32.lib
uilibs     = shell32.lib comdlg32.lib comctl32.lib ctl3d32.lib 
winlibs    = $(baselibs) $(uilibs) user32.lib gdi32.lib winspool.lib winmm.lib

!if "$(FX_COMPILER)" == "MICROSOFT"
libcdll    = /MT
!endif

guilibsdll = $(libcdll) $(winlibs)

!endif

#
# CFLAGS, we make sure we use both global and local flags:
#
#
GCFLAGS = $(GCINCS) $(GCDEFS) $(GCOPTS)
LCFLAGS = $(LCINCS) $(LCDEFS) $(LCOPTS)
VCFLAGS = $(VCINCS) $(VCDEFS) $(VCOPTS)

CFLAGS  = $(CDEBUG) $(GCFLAGS) $(LCFLAGS) $(VCFLAGS)

#
# global linker flags (LDFLAGS) and libraries (LDLIBS)
#       NOTE: local libs are before global libs
#
GLDOPTS = $(LDEBUG) $(GLDOPTS)
GLDLIBS = $(guilibsdll)
LDFLAGS = $(GLDOPTS) $(LLDOPTS)
LDLIBS  = $(LLDLIBS) $(GLDLIBS)

#
# build up global assembler flags
#
AFLAGS = $(LADEFS) $(ADEBUG) $(LAINCS) $(AOPTS) $(LAOPTS) $(AINCLUDES)

#
# OS commands:
#
!if "$(FX_HOST)" == "WIN32"
XCOPY=@xcopy
CALL=@call
!endif

CHMODWR = attrib -r
RM      = del /Q
INSTALL = $(XCOPY) /d /k /r /i /s /q

#
# default rule (make all):
#
default:: all

all: incs libs bins

RESFILE = $(RCFILE:.rc=.res)
OBJECTS = $(CFILES:.c=.obj) $(CPPFILES:.cpp=.obj) $(AFILES:.asm=.obj) $(RESFILE) $(LOBJECTS)

#
# INCS rule:
#
incs: $(BUILD_TOOLS) $(HEADERS)
!ifdef HEADERS
!ifdef FX_INSTALL_DESTINATION
!ifdef INSTALL_DESTINATION
    !$(INSTALL) $** $(INSTALL_DESTINATION)\include
    !attrib +r $(INSTALL_DESTINATION)\include\$**
    !$(INSTALL) $** $(FX_INSTALL_DESTINATION)\include
    !attrib +r $(FX_INSTALL_DESTINATION)\include\$**
!else
    !$(INSTALL) $** $(FX_INSTALL_DESTINATION)\include
    !attrib +r $(FX_INSTALL_DESTINATION)\include\$**
!endif
!else ifdef INSTALL_DESTINATION
    !$(INSTALL) $** $(INSTALL_DESTINATION)\include
    !attrib +r $(INSTALL_DESTINATION)\include\$**
!else
   @echo INSTALL_DESTINATION not defined, not installing HEADERS
!endif
!endif

#
# LIBRARIES rules:
#
!ifdef LIBRARIES
LIBPARTS = $(OBJECTS) $(SUBLIBRARIES)

!ifdef LINK_DLL

!if "$(FX_TARGET)" == "WIN32"
DLLS = $(LIBRARIES:.lib=.dll)
EXPS = $(LIBRARIES:.lib=.exp)
!endif

!if "$(FX_COMPILER)"=="MICROSOFT"
$(LIBRARIES) : $(LIBPARTS)
    $(LINK) /o $* /LD $(LIBPARTS) $(LDLIBS) $(LDFLAGS)
!endif

$(DLLS) : $(LIBRARIES)

!if defined(LIBRARIES) || defined(DLLS)
libs: $(LIBRARIES) $(DLLS)
    echo FX_INSTALL_DESTINATION=$(FX_INSTALL_DESTINATION)
!ifdef FX_INSTALL_DESTINATION
!ifdef INSTALL_DESTINATION
    !$(INSTALL) $(LIBRARIES) $(INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)
    !$(INSTALL) $(DLLS) $(INSTALL_DESTINATION)\bin
    !attrib +r $(INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)\$(LIBRARIES)
    !attrib +r $(INSTALL_DESTINATION)\bin\$(DLLS)
    !$(INSTALL) $(LIBRARIES) $(FX_INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)
    !$(INSTALL) $(DLLS) $(FX_INSTALL_DESTINATION)\bin
    !attrib +r $(FX_INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)\$(LIBRARIES)
    !attrib +r $(FX_INSTALL_DESTINATION)\bin\$(DLLS)
!else
    !$(INSTALL) $(LIBRARIES) $(FX_INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)
    !$(INSTALL) $(DLLS) $(FX_INSTALL_DESTINATION)\bin
    !attrib +r $(FX_INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)\$(LIBRARIES)
    !attrib +r $(FX_INSTALL_DESTINATION)\bin\$(DLLS)
!endif
!else ifdef INSTALL_DESTINATION
    !$(INSTALL) $(LIBRARIES) $(INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)
    !$(INSTALL) $(DLLS) $(INSTALL_DESTINATION)\bin
    !attrib +r $(INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)\$(LIBRARIES)
    !attrib +r $(INSTALL_DESTINATION)\bin\$(DLLS)
!else
   @echo INSTALL_DESTINATION not defined, not installing LIBRARIES
!endif
!else
libs:
!endif

!else

$(LIBRARIES) : $(LIBPARTS)
   $(AR) /out:$*.lib $(LIBPARTS)

libs: $(LIBRARIES)
!ifdef FX_INSTALL_DESTINATION
!ifdef INSTALL_DESTINATION
    !$(INSTALL) $** $(INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)
    !attrib +r $(INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)\$**
    !$(INSTALL) $** $(FX_INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)
    !attrib +r $(FX_INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)\$**
!else
    !$(INSTALL) $** $(FX_INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)
    !attrib +r $(FX_INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)\$**
!endif
!else ifdef INSTALL_DESTINATION
    !$(INSTALL) $** $(INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)
    !attrib +r $(INSTALL_DESTINATION)\lib$(FX_COMPLIB_DIR)\$**
!else
   @echo INSTALL_DESTINATION not defined, not installing LIBRARIES
!endif
!endif
!else
libs:
!endif

#
# BINS rules:
#
!ifdef PROGRAM
$(PROGRAM) : $(LLDLIBS) $(OBJECTS) $(DEPDATA)
    $(LINK) -o $@ $(OBJECTS) $(LDLIBS) $(LDFLAGS)
!endif

!ifdef PROGRAMS
$(PROGRAMS) : $(LLDLIBS) $*.obj $(RESFILE)
        $(LINK) -o $@ $*.obj $(RESFILE) $(LDLIBS) $(LDFLAGS)
!endif

!if defined(PROGRAM) || defined(PROGRAMS) || defined(BATS)  || defined(DIAGS)
bins: $(PROGRAM) $(PROGRAMS) $(BATS) $(DATAFILES) $(DIAGS)
!ifdef FX_INSTALL_DESTINATION
!ifdef INSTALL_DESTINATION
    !$(INSTALL) $** $(INSTALL_DESTINATION)\bin
    !attrib +r $(INSTALL_DESTINATION)\bin\$**
    !$(INSTALL) $** $(FX_INSTALL_DESTINATION)\bin
    !attrib +r $(FX_INSTALL_DESTINATION)\bin\$**
!else
    !$(INSTALL) $** $(FX_INSTALL_DESTINATION)\bin
    !attrib +r $(FX_INSTALL_DESTINATION)\bin\$**
!endif
!else ifdef INSTALL_DESTINATION
    !$(INSTALL) $** $(INSTALL_DESTINATION)\bin
    !attrib +r $(INSTALL_DESTINATION)\bin\$**
!else
   @echo INSTALL_DESTINATION not defined, not installing PROGRAMS
!endif
!else
bins:
!endif

TARGETS = $(TARGETS) $(LIBRARIES) $(DLLS) $(EXPS) $(PROGRAM) $(PROGRAMS) $(DEPDATA) $(DIAGS)


#
# dependency:
#
MKDEPFILE = makedep

#
# make clean:
#
GDIRT   = *.cod *.bak *.pdb *.ilk *.map *.sym *.err *.i stderr.out tmp.opt *.evt
DIRT    = $(GDIRT) $(LDIRT)
JUNK    = __junk__

OBJECTS = $(OBJECTS)

clean:
!if EXISTS ($(MKDEPFILE))
    FOR %i IN ($(MKDEPFILE) $(JUNK)) DO $(CHMODWR) %i > NUL
    FOR %i IN ($(MKDEPFILE) $(JUNK)) DO $(RM) %i > NUL
!endif
    FOR %i IN ($(OBJECTS) $(JUNK)) DO $(CHMODWR) %i > NUL
    FOR %i IN ($(OBJECTS) $(JUNK)) DO $(RM) %i > NUL
    FOR %i IN ($(DIRT) $(JUNK)) DO $(CHMODWR) %i > NUL
    FOR %i IN ($(DIRT) $(JUNK)) DO $(RM) %i > NUL
    FOR %i IN ($(TARGETS) $(JUNK)) DO $(CHMODWR) %i > NUL
    FOR %i IN ($(TARGETS) $(JUNK)) DO $(RM) %i > NUL

.SUFFIXES: .cod .i

.c.obj:
        $(CC) $(CFLAGS) $<

.cpp.obj:
        $(CPLUSPLUS) $(CFLAGS) $<

.rc.res:
    rc $(GCINCS) $(LCINCS) $(VCINCS) $(GCDEFS) $(LCDEFS) $(VCDEFS) $<

.c.cod:
        $(CC) $(CFLAGS) $(ASM_LIST_FLAGS) $<

.c.i:
        $(CPP) $(CFLAGS) $< > $@

.asm.obj:
        $(AS) $(AFLAGS) $<

.asm.o:
    $(AS) $(AFLAGS) $<
    rename $*.obj $@


#
# makedepend rules:
#
!if EXISTS ($(MKDEPFILE))
!include $(MKDEPFILE)
!endif

!ifndef MAKEFILE
MAKEFILE = makefile
!endif

!ifdef CFILES
$(OBJECTS): $(HEADERS) $(PRIVATE_HEADERS) $(MAKEFILE)
!endif

!ifdef CPPFILES
$(OBJECTS): $(HEADERS) $(PRIVATE_HEADERS) $(MAKEFILE)
!endif
