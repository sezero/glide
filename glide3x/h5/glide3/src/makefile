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
#
# $Log:
#

!include $(BUILD_ROOT)\config.mak

# build the lib and dll
#FX_DLL_BUILD=1

TAG_REGEXP      = $(BUILD_ROOT)\$(FX_GLIDE_HW)\glide3\src\glide3.rx

#FX_GLIDE_ALT_TAB=1

!if "$(FX_CHRIS_DENIS_ANTHONY_HACK)" == "1"
LCDEFS          =       $(LCDEFS) -DCHRIS_DENIS_ANTHONY_HACK=1
!endif

!if "$(FX_GLIDE_ALT_TAB)" == "1"
LCDEFS          =       $(LCDEFS) -DGLIDE_ALT_TAB=1 -DFX_GLIDE_ALT_TAB=1
#LADEFS          =       $(LADEFS) -DGLIDE_ALT_TAB
!endif

!if "$(FX_TARGET)" == "WIN32"
LCINCS         = -I$(DIRECTXSDK)\include
!endif

# This is a bit of a red herring
!if "$(FX_GLIDE_DIRECT_WRITE)" == "1"
FX_GLIDE_DIRECT_WRITE   = 1
FX_GLIDE_PACKET_FIFO    = 0
!if "$(HAL_CSIM)" != "1"
LCDEFS          =       $(LCDEFS) -DDIRECT_IO=1
#!else
ATSOPTS         =       $(ATSOPTS) -DDIRECT_IO=1
!endif
!else
FX_GLIDE_PACKET_FIFO    = 1
!endif

#FX_GLIDE_CTRISETUP =    0

# Compile for specific hardware
!if "$(HAL_CSIM)" == "1"
!if "$(FX_GLIDE_DIRECT_WRITE)" == "1"
FX_GLIDE_PACKET_FIFO    = 0
FX_GLIDE_DIRECT_WRITE   = 1
FX_GLIDE_CTRISETUP      = 1
!else
FX_GLIDE_PACKET_FIFO    = 1
FX_GLIDE_DIRECT_WRITE   = 0
FX_GLIDE_CTRISETUP      = 1
!endif
!endif

!if "$(FX_GLIDE_PACKET_FIFO)" == "1"
!if "$(FX_GLIDE_SW_SETUP)" == "1"
CMDXPORTDEFS    =       -DGLIDE_PACKET3_TRI_SETUP=1 -DUSE_PACKET_FIFO=1
!else
CMDXPORTDEFS    =       -DGLIDE_PACKET3_TRI_SETUP=1 -DUSE_PACKET_FIFO=1 -DGLIDE_HW_TRI_SETUP=1
!endif
!else 
CMDXPORTDEFS    =       -DGLIDE_HW_TRI_SETUP=1 -DGLIDE_PACKET3_TRI_SETUP=0
FX_GLIDE_DEBUG_FIFO=1
!endif

#FX_GLIDE_REAL_HW=       1
#FX_GLIDE_NO_FIFO=       1

HWSPEC          =       fifo.c
LCDEFS          =       $(LCDEFS) -DH3=1 $(CMDXPORTDEFS)
INITLIB         =       $(BUILD_ROOT)\$(FX_GLIDE_HW)\$(FX_GLIDE_HW)init.lib

!if "$(FX_GLIDE_VTUNE)"!=""
OTSOPTS = /Zi
LLDOPTS = $(LLDOPTS) /DEBUG
!endif

!if "$(DEBUG)" == "1"
DBGOPTS          =      -DGLIDE_DEBUG=1 -DGDBG_INFO_ON=1
GLIDE_SANITY_ALL =      1
!endif

!if "$(FX_GLIDE_HW)" == "h5"
LCDEFS          =       $(LCDEFS) -DFX_GLIDE_NAPALM=1 -DH4=1 -DFX_GLIDE_H5_CSIM=1 #-DA0_AA_SWAP_WORKAROUND=1 -DCHECK_SLAVE_SWAPCMD=1 -DTACO_MEMORY_FIFO_HACK=1
!endif

# So that we can get reasonable stuff from SoftIce etc even
# if we're building an optimized version. This should only
# add symbolics so it is really only a codesize issue.
!if "$(FX_GLIDE_SYMBOLICS)" == "1"
!if "$(DEBUG)" != "1"
OTSOPTS          = /Zi 
!endif
LLDOPTS          = /MAP:glide3x.map /DEBUG
!endif

!if "$(FX_DLL_BUILD)" == "1"
LLDOPTS         = $(LLDOPTS) /NODEFAULTLIB libcmt.lib kernel32.lib
!endif

!if "$(FX_COMPILER)" == "MICROSOFT"
DBGOPTS          =      $(DBGOPTS)
!endif

# 3DNow stuff
!if "$(HAL_CSIM)" == "1"
GL_AMD3D        =       
GL_MMX          =
GL_SSE          =
GL_SSE2         =
!else
!if "$(FX_GLIDE_CPUOPTS)" == "1"
GL_AMD3D        =       1
GL_MMX          =       1
GL_SSE          =       1
GL_SSE2         =       1
!else
GL_AMD3D        =
GL_MMX          =
GL_SSE          =
GL_SSE2         =
!endif
!endif

!if "$(FX_GLIDE_SW_SETUP)" == "1"
GL_AMD3D        =       
GL_MMX          =
GL_SSE          =
GL_SSE2         =
!endif

!if "$(GL_AMD3D)"!=""
CFILES          =       $(CFILES) xtexdl_def.c
LOBJECTS        =       xtexdl_3dnow.obj \
                        xdraw2_3dnow.obj \
                        xdraw3_3dnow.obj

!if "$(FX_GLIDE_CTRISETUP)" != "1"
LOBJECTS        =       $(LOBJECTS) \
                        xdraw2_def.obj xdraw3_def.obj
!endif

# KoolSmoky - check this
LCDEFS          =       $(LCDEFS) -DGL_AMD3D=1
!else
CFILES          =       $(CFILES) xtexdl_def.c
!endif # GL_AMD3D

!if "$(GL_MMX)"!=""
LCDEFS          =       $(LCDEFS) -DGL_MMX=1
LOBJECTS        =       $(LOBJECTS) \
                        xtexdl_mmx.obj
!endif

!if "$(GL_SSE)"!=""
LCDEFS          =       $(LCDEFS) -DGL_SSE=1
LOBJECTS        =       $(LOBJECTS) \
                        xdraw2_sse.obj \
                        xdraw3_sse.obj
!endif

!if "$(GL_SSE2)"!=""
LCDEFS          =       $(LCDEFS) -DGL_SSE2=1
LOBJECTS        =       $(LOBJECTS) \
                        xtexdl_sse2.obj
!endif

!if "$(FX_GLIDE_HW)" == ""
!error "FX_GLIDE_HW not defined"
!endif

# Display Options (Simulator, Hardware)
DSPOPTS =
SUBLIBRARIES =
!if ("$(HAL_CSIM)" != "")
DSPOPTS         =       $(DSPOPTS) -DHAL_CSIM=1 -DGLIDE_INIT_HAL=1 -DHOOPTI_LAB_LFB_BYPASS=1
#LCDEFS          =       $(LCDEFS) -DHAL_CSIM=1 -DGLIDE_INIT_HAL=1 -DHOOPTI_LAB_LFB_BYPASS=1
LCINCS          =       -I$(BUILD_ROOT)\$(FX_GLIDE_HW)\csim
SUBLIBRARIES    =       $(SUBLIBRARIES) $(BUILD_ROOT)\$(FX_GLIDE_HW)\lib\h4hal.lib
FX_GLIDE_REAL_HW=       1
!else
DSPOPTS         =       $(DSPOPTS) -DGLIDE_INIT_HWC=1
#LCDEFS          =       $(LCDEFS) -DGLIDE_INIT_HWC=1
!endif

!if ("$(HAL_HW)" == "1")
DSPOPTS         =       $(DSPOPTS) -DHAL_HW=1
#LCDEFS          =       $(LCDEFS) -DHAL_HW=1
SUBLIBRARIES    =       $(SUBLIBRARIES) 
!endif
!if !defined(DSPOPTS)
!error "Unknown HAL_* configuration"
!endif

# !if (("$(HAL_CSIM)" == "1") && ("$(HAL_HW)" == "1"))
SUBLIBRARIES    =       $(SUBLIBRARIES) $(BUILD_ROOT)\$(FX_GLIDE_HW)\lib\minihwc.lib
# !endif

# If we're running w/ the CSIM then we cannot do direct
# lfb writes through the info ptr.
!if (("$(HAL_HW)" == "1") && ("$(HAL_CSIM)" == "1"))
LCDEFS          =       $(LCDEFS) -DHOOPTI_LAB_LFB_BYPASS=1
!endif

# GLIDE_HW_TRI_SETUP:   Use the hw TSU for triangle rendering. 
# GLIDE_TRI_CULLING:    Cull backfaced/zero area triangles in sw before the fifo.
#                       NB: This must be set if GLIDE_HW_TRI_SETUP=0
!if "$(FX_GLIDE_SW_SETUP)" == "1"
LCDEFS          =       $(LCDEFS) -DGLIDE_HW_TRI_SETUP=0 -DGLIDE_TRI_CULLING=1
FX_GLIDE_CTRISETUP = 1
!else # HW Setup

# Stick one more slots in the dataList so that the clamping loop does 
# not go one more iteration.
!if "$(GLIDE_FP_CLAMP)" == "1"
LCDEFS          =       $(LCDEFS) -DGLIDE_FP_CLAMP=1
!if "$(GLIDE_FP_CLAMP_TEX)" == "1"
LCDEFS          =       $(LCDEFS) -DGLIDE_FP_CLAMP_TEX=1
!endif
!endif

# Do culling test in sw for independent triangles
CULL_MODE       =
!if "$(FX_GLIDE_HW_CULL)" != "1"
CULL_MODE       =       -DGLIDE_TRI_CULLING=1
!endif
DSPOPTS         =       $(DSPOPTS) $(CULL_MODE)
#LCDEFS          =       $(LCDEFS) $(CULL_MODE)

!endif # HW Tri Setup

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

# Turn on/off assembly language specializations for rendering
# primatives. Calling this trisetup is a bit of a misnomer, but
# has remained because i was too lazy to change it when I ganked
# this makefile from Chris's sst1 glide2x tree.
!if "$(FX_GLIDE_CTRISETUP)" == "1"
ASMTRISETUP     =
ATSOPTS         =       -DGLIDE_USE_C_TRISETUP=1
CFILES          =       $(CFILES) gxdraw.c
!else

# There are now special build rules to get specialized
# function versions of the functions taht these files
# originally contained. See the end of the file.
!if "$(GL_AMD3D)"==""
ASMTRISETUP     =       $(ASMTRISETUP) xdraw2.asm xdraw3.asm
!endif

DSPOPTS         =       $(DSPOPTS) -DGLIDE_PACKED_RGB=0 -DGLIDE_TRI_CULLING=1
#LCDEFS          =       $(LCDEFS) -DGLIDE_PACKED_RGB=0 -DGLIDE_TRI_CULLING=1
!endif

OPTOPTS         =       $(GRMOPTS) $(OTSOPTS) $(ATSOPTS)

# local defines, begin with basics and then add on
LCDEFS          =       $(LCDEFS) -DGLIDE_LIB=1
# KoolSmoky - the above isn't use anywhere for H5

# Turn Off/On compilation of shameless plug
!if "$(FX_GLIDE_NO_PLUG)" == "1"
!else
LCDEFS          =       $(LCDEFS) -DGLIDE_PLUG=1
!endif

# Turn Off/On splash screen
!if "$(FX_GLIDE_NO_SPLASH)" == "1"
!else
LCDEFS          =       $(LCDEFS) -DGLIDE_SPLASH=1
!endif

!if "$(FX_PCI_TARGET)" == "NT"
LCDEFS          =       $(LCDEFS) -DNT_BUILD=1
!endif

LCDEFS          =       $(LCDEFS) -DGLIDE3=1 -DGLIDE3_ALPHA=1
GLIDE3FILES     =       distate.c gstrip.c distrip.c diget.c

!if (("$(GLIDE_SANITY_ALL)" != "") || ("$(GLIDE_SANITY_SIZE)" != ""))
LCDEFS          =       $(LCDEFS) -DGLIDE_SANITY_SIZE=1
!endif
!if (("$(GLIDE_SANITY_ALL)" != "") || ("$(GLIDE_SANITY_ASSERT)" != ""))
LCDEFS          =       $(LCDEFS) -DGLIDE_SANITY_ASSERT=1
!endif

# Local Defs, Includes, and Options (C)
LCINCS          =       $(LCINCS) -I$(BUILD_ROOT)\$(FX_GLIDE_HW)\include 
LCOPTS          =       $(LCOPTS) $(DBGOPTS) $(DSPOPTS) $(OPTOPTS)

# Local Defs, Includes, and Options (ASM)
LADEFS          =       $(LADEFS) $(ASM_REGMAP_DEFS)
LAINCS          =       -I$(BUILD_ROOT)\$(FX_GLIDE_HW)\include
LAOPTS          =       $(DBGOPTS) $(DSPOPTS) $(OPTOPTS)

#AFILES          =       $(ASMTRISETUP) cpudtect.asm

# sources
HEADERS         =       glide.h glidesys.h glideutl.h g3ext.h
PRIVATE_HEADERS =       fxglide.h gsstdef.h fxinline.h gsfc.h fxcmd.h fxsplash.h

INSTALL_DESTINATION =   $(BUILD_ROOT)\$(FX_GLIDE_HW)

CFILES              =   $(CFILES) gsplash.c g3df.c  gu.c gthread.c \
                        gpci.c diglide.c disst.c ditex.c gsfc.c\
                        gbanner.c gerror.c gaa.c gdraw.c gglide.c $(GLIDE3FILES) \
                        glfb.c gsst.c gtex.c gtexdl.c $(HWSPEC) cpuid.c
                        
CPPFILES        =       $(CPPFILES) winsurf.cpp

OFILES          =       $(CFILES:.c=.obj) $(CPPFILES:.cpp=.obj)

OTHER_CFILES    =       fxgasm.c fxbldno.c fxinline.h

CODFILES        =       $(CFILES:.c=.cod)

# targets
LDIRT           =       fxgasm.obj fxgasm.exe fxgasm.h fxinline.h fxbldno.obj fxbldno.exe fxbldno.h *.lst *.obj

LIBRARIES       =       glide3x.lib
RCFILE          =       glide.rc

# Make a static link library for things like the diags.
!if "$(FX_DLL_BUILD)" == "1"
FX_DLL_LIBRARY  =       1
!else
LCDEFS          =       $(LCDEFS) -DFX_STATIC_BUILD=1
!endif


LCDEFS          =       $(LCDEFS) $(FIFODEFS)

!include $(BUILD_ROOT)\swlibs\include\nmake\3dfx.mak

codfiles:   $(CODFILES)
xdraw2.obj : xdraw2.asm xdraw2.inc fxgasm.h fxinline.h

ctags:  $(CFILES)
        ctags $(CFILES)

!if "$(FX_TARGET)" == "WIN32"
!if "$(FX_DLL_BUILD)" != ""
glide3x.dll:
        del fxbldno.h
!endif
!endif

!if "$(FX_TARGET)" == "DOS"
!if "$(FX_DLL_BUILD)" != ""

glimport.obj: glimport.asm

IMPORT_DEPS =   $(BUILD_ROOT)\swlibs\lib\dllload.lib \
                $(DLLLOADS) glimport.obj 

glide3x.lib:   $(IMPORT_DEPS)
        if exist glide3x.lib del glide3x.lib
        wlib glide3x.lib +$(BUILD_ROOT)\swlibs\lib\dllload.lib +glimport.obj 
!if "$(FX_WATCOM_REG_CALL)" != "1"
        wlib glide3x.lib +$(BUILD_ROOT)\swlibs\lib\dllloads.obj
!endif            
        if exist $(BUILD_ROOT)\$(FX_GLIDE_HW)\lib\glide3x.lib attrib -r $(BUILD_ROOT)\$(FX_GLIDE_HW)\lib\glide3x.lib
        if exist $(BUILD_ROOT)\$(FX_GLIDE_HW)\lib\glide3x.lib del $(BUILD_ROOT)\$(FX_GLIDE_HW)\lib\glide3x.lib
        xcopy /d /k /r /i /s /q glide3x.lib $(BUILD_ROOT)\$(FX_GLIDE_HW)\lib
        attrib +r $(BUILD_ROOT)\$(FX_GLIDE_HW)\lib\glide3x.lib


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
        -I$(BUILD_ROOT)\$(FX_GLIDE_HW)\include -I$(BUILD_ROOT)\swlibs\include
!else if "$(FX_COMPILER)" == "WATCOM"
        set WCC386=$(CFLAGS)
        wcl386 fxgasm.c 
!endif  
fxgasm.h: fxgasm.exe 
        fxgasm -hex > fxgasm.h
fxinline.h: fxgasm.exe
        fxgasm -inline > fxinline.h

glide3x.nms:  glide3x.dll
        nmsym /trans:source,package,always /source:$(BUILD_ROOT)\$(FX_GLIDE_HW)\cinit;$(BUILD_ROOT)\$(FX_GLIDE_HW)\minihwc;$(BUILD_ROOT)\$(FX_GLIDE_HW)\glide3\src glide3x.dll

#--------------------------------------------------------------------------
# special rules for making FXBLDNO.H
#
fxbldno.exe: fxbldno.c $(HEADERS) $(PRIVATE_HEADERS)
!if "$(FX_COMPILER)" == "MICROSOFT"
        $(CC) -o $@ fxbldno.c $(GCDEFS) $(LCDEFS) $(VCDEFS) $(LCOPTS)
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

#--------------------------------------------------------------------------
# 3DNow!(tm) dependencies
# XXX_def.obj targets are the default targets
#
!if "$(FX_GLIDE_CTRISETUP)" != "1"
xdraw2_def.obj: xdraw2.asm xdraw2.inc fxgasm.h
        $(AS) $(AFLAGS) -o $@ xdraw2.asm

xdraw3_def.obj: xdraw3.asm fxgasm.h
        $(AS) $(AFLAGS) -o $@ xdraw3.asm
!endif

!if "$(GL_AMD3D)"!=""
#xdraw2_def.obj: xdraw2.asm xdraw2.inc fxgasm.h
#        $(AS) $(AFLAGS) -o $@ xdraw2.asm

xdraw2_3dnow.obj: xdraw2.asm xdraw2.inc fxgasm.h
        $(AS) $(AFLAGS) -DGL_AMD3D=1 -DUSE_PACKET_FIFO=1 -o $@ xdraw2.asm

#xdraw3_def.obj: xdraw3.asm fxgasm.h
#        $(AS) $(AFLAGS) -o $@ xdraw3.asm

xdraw3_3dnow.obj: xdraw3.asm fxgasm.h
        $(AS) $(AFLAGS) -DGL_AMD3D=1 -DUSE_PACKET_FIFO=1 -o $@ xdraw3.asm

xtexdl_3dnow.obj: xtexdl.asm fxgasm.h
        $(AS) $(AFLAGS) -DGL_AMD3D=1 -DUSE_PACKET_FIFO=1 -o $@ xtexdl.asm

!endif # GL_AMD3D

!if "$(GL_MMX)"!=""
xtexdl_mmx.obj: xtexdl.asm fxgasm.h
        $(AS) $(AFLAGS) -DGL_MMX=1 -DUSE_PACKET_FIFO=1 -o $@ xtexdl.asm
!endif # GL_MMX

!if "$(GL_SSE)"!=""
xdraw2_sse.obj: xdraw2.asm xdraw2.inc fxgasm.h
        $(AS) $(AFLAGS) -DGL_SSE=1 -DUSE_PACKET_FIFO=1 -o $@ xdraw2.asm

xdraw3_sse.obj: xdraw3.asm fxgasm.h
        $(AS) $(AFLAGS) -DGL_SSE=1 -DUSE_PACKET_FIFO=1 -o $@ xdraw3.asm
!endif # GL_SSE

!if "$(GL_SSE2)"!=""
xtexdl_sse2.obj: xtexdl.asm fxgasm.h
        $(AS) $(AFLAGS) -DGL_SSE2=1 -DUSE_PACKET_FIFO=1 -o $@ xtexdl.asm
!endif # GL_SSE2
