#
# THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
# PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
# TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
# INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
# DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
# THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
# EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
# FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
# 
# USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
# RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
# TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
# AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
# SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
# THE UNITED STATES.  
# 
# COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED

TAG_REGEXP      = $(BUILD_ROOT)\$(FX_GLIDE_HW)\glide\src\glide.rx

!if "$(FX_CHRIS_DENIS_ANTHONY_HACK)" == "1"
LCDEFS          =       $(LCDEFS) -DCHRIS_DENIS_ANTHONY_HACK=1
!endif

!if "$(FX_TARGET)" == "WIN32"
LCINCS         = -I$(DXDDK)\inc -I$(DXDDK)\inc32
!endif

FX_TACO_MEMORY_FIFO_HACK = 
!if "$(FX_TACO_MEMORY_FIFO_HACK)" != ""
LCDEFS          =       $(LCDEFS) -DTACO_MEMORY_FIFO_HACK
!endif

!if "$(DIAG_BUILD)" != ""
LCDEFS          =       $(LCDEFS) -DDIAG_BUILD
!endif

# This is a bit of a red herring
!if "$(FX_GLIDE_DIRECT_WRITE)" == "1"
FX_GLIDE_DIRECT_WRITE   = 1
FX_GLIDE_CTRISETUP      = 1
LCDEFS          =       $(LCDEFS) -DDIRECT_IO=1
!else
FX_GLIDE_PACKET_FIFO    = 1
!endif

# Make a static link library for things like the diags.
!if "$(FX_DLL_BUILD)" == "1"
FX_DLL_LIBRARY  =       1
!else
LCDEFS          =       $(LCDEFS) -DFX_STATIC_BUILD
FX_GLIDE_CTRISETUP = 1
!endif

!if "$(FX_GLIDE_PACKET_FIFO)" == "1"
CMDXPORTDEFS    =       -DGLIDE_PACKET3_TRI_SETUP=1 -DUSE_PACKET_FIFO=1 -DGLIDE_HW_TRI_SETUP=1
!else 
CMDXPORTDEFS    =       -DUSE_PACKET_FIFO=0 -DGLIDE_HW_TRI_SETUP=1 -DGLIDE_PACKET3_TRI_SETUP=0
FX_GLIDE_DEBUG_FIFO=1
!endif

FX_GLIDE_REAL_HW=       1
FX_GLIDE_NO_FIFO=       1

!if "$(FX_GLIDE_CTRISETUP)" != "1"
FX_GLIDE_CTRISETUP =    0
!endif
HWSPEC          =       fifo.c
LCDEFS          =       $(LCDEFS) -DH3 $(CMDXPORTDEFS)
INITLIB         =       $(BUILD_ROOT)\$(FX_GLIDE_HW)\$(FX_GLIDE_HW)init.lib

!if "$(FX_GLIDE_VTUNE)"!=""
OTSOPTS = /Zi
LLDOPTS = /DEBUG
!endif

!if "$(DEBUG)" == "1"
DBGOPTS          =      -DGLIDE_DEBUG -DGDBG_INFO_ON
GLIDE_SANITY_ALL =      1
!endif

# So that we can get reasonable stuff from SoftIce etc even
# if we're building an optimized version. This should only
# add symbolics so it is really only a codesize issue.
!if "$(FX_GLIDE_SYMBOLICS)" == "1"
!if "$(DEBUG)" != "1"
OTSOPTS          = /Zi 
!endif
LLDOPTS          = /MAP:glide2x.map /DEBUG
!endif

!if "$(FX_COMPILER)" == "MICROSOFT"
DBGOPTS          =      $(DBGOPTS) /WX
!endif

!if "$(FX_GLIDE_HW)" == ""
!error "FX_GLIDE_HW not defined"
!endif

# Display Options
DSPOPTS =
SUBLIBRARIES =
DSPOPTS         =       $(DSPOPTS) -DGLIDE_INIT_HWC

DSPOPTS         =       $(DSPOPTS) -DHAL_HW=1
SUBLIBRARIES    =       $(SUBLIBRARIES) 

!if !defined(DSPOPTS)
!error "Unknown HAL_* configuration"
!endif

SUBLIBRARIES    =       $(SUBLIBRARIES) $(BUILD_ROOT)\h3\lib\minihwc.lib

# 3DNow stuff
GL_AMD3D        =       1
!if "$(GL_AMD3D)"!=""
LCDEFS          =       $(LCDEFS) -DGL_AMD3D
LOBJECTS        =       $(LOBJECTS) xtexdl_3dnow.obj
CFILES          =       $(CFILES) xtexdl_def.c
!endif # GL_AMD3D

# Do culling test in sw for independent triangles
CULL_MODE       =
!if "$(FX_GLIDE_HW_CULL)" != "1"
CULL_MODE       =       -DGLIDE_TRI_CULLING=1
!endif
DSPOPTS         =       $(DSPOPTS) $(CULL_MODE)

# Send a single DWORD ARGB rather than 4 fp values, at
# the cost of doing the conversion.
!if "$(FX_GLIDE_PACK_RGB)" == "1"
LCDEFS          =       $(LCDEFS) -DGLIDE_PACKED_RGB=1
!endif

!if "$(FX_GLIDE_NO_FIFO)" != "1"
FIFODEFS = -DUSE_PACKET_FIFO=1

!if "$(FX_GLIDE_DEBUG_FIFO)" == "1"
# GLIDE_USE_DEBUG_FIFO:   Run w/ the small fifo to cause me/glide more stress
# FIFO_ASSERT_FULL:       Check hw depth/fifo a lot (slow)
LCDEFS          =       $(LCDEFS)  \
                        -DGLIDE_USE_DEBUG_FIFO=1 -DFIFO_ASSERT_FULL=1 -DASSERT_FAULT=0 -DGLIDE_SANITY_SIZE=1
!endif
!endif

# Optimization Options
# This is for turning on and off algorithmic optimizations,
# not flags to the C compiler.  Usually this involves
# enabling/disabling assembly language code, but it can also   
# change the way C code works, or how C code generates data to be 
# used by various pieces of code.
# Usually these are set with environment variables or arguments to 
# nmake.
#

# Turn on/off assembly language trisetup code.
# (C on is Assembly off) (A ssembly T ri S etup OPTS)
!if "$(FX_GLIDE_CTRISETUP)" == "1"
ASMTRISETUP     =
ATSOPTS         =       -DGLIDE_USE_C_TRISETUP 
CFILES          =       $(CFILES) gxdraw.c
!else
DSPOPTS         =       $(DSPOPTS) -DGLIDE_DISPATCH_SETUP=1 -DGLIDE_PACKED_RGB=0

!if "$(GL_AMD3D)"==""
ASMTRISETUP     =       xdraw2.asm
!else
LOBJECTS        =       $(LOBJECTS) \
                        xdraw2_def.obj xdraw2_3dnow.obj
!endif # GL_AMD3D
!endif

OPTOPTS         =       $(GRMOPTS) $(OTSOPTS) $(ATSOPTS)

# local defines, begin with basics and then add on
LCDEFS          =       $(LCDEFS) -DGLIDE_LIB

# Turn Off/On compilation of shameless plug
!if "$(FX_GLIDE_NO_PLUG)" == "1"
!else
LCDEFS          =       $(LCDEFS) -DGLIDE_PLUG
!endif

# Turn Off/On splash screen
!if "$(FX_GLIDE_NO_SPLASH)" == "1"
!else
LCDEFS          =       $(LCDEFS) -DGLIDE_SPLASH
!endif

!if "$(FX_PCI_TARGET)" == "NT"
LCDEFS          =       $(LCDEFS) -DNT_BUILD
!endif

#Glide 3 Stuff, for migration all
!if "$(FX_GLIDE3)" != ""
LCDEFS          =       $(LCDEFS) -DGLIDE3
GLIDE3FILES     =       gstrip.c distrip.c distate.c diget.c
!endif

#Glide 3. remove migration stuff
!if "$(FX_GLIDE3_ALPHA)" != ""
LCDEFS          =       $(LCDEFS) -DGLIDE3_ALPHA
GLIDE3FILES     =       gstrip.c distrip.c distate.c diget.c
!endif

!if defined(GLIDE_SANITY_ALL) || defined(GLIDE_SANITY_SIZE)
LCDEFS          =       $(LCDEFS) -DGLIDE_SANITY_SIZE
!endif
!if defined(GLIDE_SANITY_ALL) || defined(GLIDE_SANITY_ASSERT)
LCDEFS          =       $(LCDEFS) -DGLIDE_SANITY_ASSERT
!endif

# Local Defs, Includes, and Options (C)
LCINCS          =       $(LCINCS) -I$(BUILD_ROOT)\$(FX_GLIDE_HW)\include 
LCOPTS          =       $(LCOPTS) $(DBGOPTS) $(DSPOPTS) $(OPTOPTS)

# Local Defs, Includes, and Options (ASM)
LADEFS          =       $(LADEFS) $(ASM_REGMAP_DEFS)
LAINCS          =       -I$(BUILD_ROOT)\$(FX_GLIDE_HW)\include
LAOPTS          =       $(DBGOPTS) $(DSPOPTS) $(OPTOPTS) /Fl /Sa

AFILES          =       $(ASMTRISETUP) cpudtect.asm

# sources
HEADERS         =       glide.h gump.h glidesys.h glideutl.h
PRIVATE_HEADERS =       fxglide.h gsstdef.h fxinline.h fxsplash.h

INSTALL_DESTINATION =   $(BUILD_ROOT)\$(FX_GLIDE_HW)

CFILES              =   $(CFILES) gsplash.c g3df.c  gu.c guclip.c\
                        gpci.c gump.c diglide.c disst.c ditex.c\
                        gbanner.c gerror.c gmovie.c digutex.c ddgump.c\
                        gaa.c gdraw.c gglide.c glfb.c gsst.c gtex.c\
                        gtexdl.c gutex.c $(HWSPEC) $(GLIDE3FILES)

OFILES          =       $(CFILES:.c=.obj)

OTHER_CFILES    =       fxgasm.c fxbldno.c fxinline.h

CODFILES        =       $(CFILES:.c=.cod)

# targets
LDIRT           =       fxgasm.obj fxgasm.exe fxgasm.h fxinline.h fxbldno.obj fxbldno.exe fxbldno.h

!if "$(FX_GLIDE3)" == "1"
LIBRARIES       =       glide3x.lib
!else
LIBRARIES       =       glide2x.lib
!endif
RCFILE          =       glide.rc

LCDEFS          =       $(LCDEFS) $(FIFODEFS)

!include $(BUILD_ROOT)\swlibs/include/nmake/3dfx.mak

codfiles:   $(CODFILES)
xdraw2.obj : xdraw2.asm xdraw2.inc fxgasm.h fxinline.h

ctags:  $(CFILES)
        ctags $(CFILES)

gsplash.obj : splshdat.c

!if "$(FX_TARGET)" == "WIN32"
!if "$(FX_DLL_BUILD)" != ""
glide2x.dll:
        del fxbldno.h
!endif
!endif

!if "$(FX_TARGET)" == "DOS"
!if "$(FX_DLL_BUILD)" != ""

glimport.obj: glimport.asm

IMPORT_DEPS =   $(BUILD_ROOT)\swlibs\lib\dllload.lib \
                $(DLLLOADS) glimport.obj 

glide2x.lib:   $(IMPORT_DEPS)
        if exist glide2x.lib del glide2x.lib
        wlib glide2x.lib +$(BUILD_ROOT)\swlibs\lib\dllload.lib +glimport.obj 
!if "$(FX_WATCOM_REG_CALL)" != "1"
        wlib glide2x.lib +$(BUILD_ROOT)\swlibs\lib\dllloads.obj
!endif            
        if exist $(BUILD_ROOT)\$(FX_GLIDE_HW)\lib\glide2x.lib attrib -r $(BUILD_ROOT)\$(FX_GLIDE_HW)\lib\glide2x.lib
        if exist $(BUILD_ROOT)\$(FX_GLIDE_HW)\lib\glide2x.lib del $(BUILD_ROOT)\$(FX_GLIDE_HW)\lib\glide2x.lib
        xcopy /d /k /r /i /s /q glide2x.lib $(BUILD_ROOT)\$(FX_GLIDE_HW)\lib
        attrib +r $(BUILD_ROOT)\$(FX_GLIDE_HW)\lib\glide2x.lib


!endif
!endif

!if "$(FX_COMPILER)" == "WATCOM"
#------------------------------------------------------------
# special rul for makeing glideapi.obj under WATCOM
#

glideapi.obj : glideapi.c
        set $(CC_ENV_VAR)=-d2 $(GCFLAGS) $(LCFLAGS) $(VCFLAGS)
        $(CC) glideapi.c
!endif

#--------------------------------------------------------------------------
# special rules for making FXGASM.H
#
fxgasm.exe: fxgasm.c $(HEADERS) gsstdef.h fxglide.h Makefile
!if "$(FX_COMPILER)" == "MICROSOFT"
        $(CC) -o $@ fxgasm.c $(GCDEFS) $(LCDEFS) $(VCDEFS) $(LCOPTS) \
        -I$(BUILD_ROOT)\$(FX_GLIDE_HW)\include -I$(BUILD_ROOT)\swlibs\include $(LDFLAGS)
!else if "$(FX_COMPILER)" == "WATCOM"
        set WCC386=$(CFLAGS)
        wcl386 fxgasm.c 
!endif  
fxgasm.h: fxgasm.exe 
        fxgasm -hex > fxgasm.h
fxinline.h: fxgasm.exe
        fxgasm -inline > fxinline.h


glide2x.nms:  glide2x.dll
        nmsym /trans:source,package,always /source:$(BUILD_ROOT)/h3/cinit;$(BUILD_ROOT)/h3/minihwc;$(BUILD_ROOT)\h3\glide\src glide2x.dll

#--------------------------------------------------------------------------
# special rules for making FXBLDNO.H
#

fxbldno.exe: fxbldno.c $(HEADERS) $(PRIVATE_HEADERS)
!if "$(FX_COMPILER)" == "MICROSOFT"
        $(CC) -o $@ fxbldno.c $(GCDEFS) $(LCDEFS) $(VCDEFS) $(LCOPTS) $(LDFLAGS)
!else if "$(FX_COMPILER)" == "WATCOM"
        set WCC386=$(CFLAGS)
        wcl386 fxbldno.c 
!endif  

fxbldno.h: fxbldno.exe
        fxbldno > fxbldno.h

glide.res: rcver.h fxbldno.h
gglide.obj: rcver.h

!if "$(FX_GLIDE_CTRISETUP)" != "1"
gdraw.obj: fxinline.h
!endif

# 3DNow!(tm) dependencies
# XXX_def.obj targets are the default targets

!if "$(GL_AMD3D)"!=""
xdraw2_def.obj: xdraw2.asm xdraw2.inc fxgasm.h
        $(AS) $(AFLAGS) -Fo $@ xdraw2.asm

xdraw2_3dnow.obj: xdraw2.asm xdraw2.inc fxgasm.h
        $(AS) $(AFLAGS) -DGL_AMD3D -DUSE_PACKET_FIFO=1 -Fo $@ xdraw2.asm

xtexdl_3dnow.obj: xtexdl.asm fxgasm.h 
        $(AS) $(AFLAGS) -DGL_AMD3D -DUSE_PACKET_FIFO=1 -Fo $@ xtexdl.asm

!endif # GL_AMD3D
