#
# global configuration environment
#
# $Log:
# 1   ve3d      1.0         KoolSmoky    global config
#

# moved this to win32mak.bat
# BUILD_DRIVE = $(HOMEDRIVE)

# use this for 95 DDK compatiblity
#!include $(BUILD_DRIVE)\98DDK\inc\master.mk

# moved this to win32mak.bat
# BUILD_ROOT=$(BUILD_DRIVE)\work\Glide3x

INCLUDE=$(INCLUDE)

# moved this to win32mak.bat
# BUILD_ROOT_SWLIBS=$(BUILD_ROOT)\swlibs

# moved this to win32mak.bat
# DIRECTXSDK=$(BUILD_DRIVE)\DXSDK

#
# Variables local to the build.
#
# moved this to win32mak.bat
# FX_HW_PROJECTS=glide
#
# --enable-fx-glide-hw chooses which ASIC to build for.
#
# FX_GLIDE_HW=h5
#
INSTALL_DESTINATION=$(BUILD_ROOT)\$(FX_GLIDE_HW)
#
# --enable-fx-dri-build chooses to build with or without DRI.
#
#DRI_BUILD=0
#
# This is makefile.linux behavior.  I don't know exactly
# what this does.
#
# if voodoo2 use HAL_HW=true
#HAL_HW=1
#
# Define debuging
# FX_GLIDE_SYMBOLICS=1 enables us to get stuff from SoftIce etc even when 
# building an optimized version. Supports minimal debug info on default. if you don't 
# want any, CEASE_ALL_GDBG=1 comments out #define GDBG_INFO_ON in 
# minihwc.c and win_mode.c .
#FX_GLIDE_SYMBOLICS=1
#DEBUG = 1
#CEASE_ALL_GDBG=1
#BETA = 1
!if "$(CEASE_ALL_GDBG)"=="1" && "$(DEBUG)"!="1"
LCDEFS          =       $(LCDEFS) -DCEASE_ALL_GDBG=1
!endif
!if "$(BETA)"=="1"
LCDEFS          =       $(LCDEFS) -DBETA=1
!endif
#
# More debuging
# use output to nt kernel debugger
#KERNEL_NT=1
# Yet more debuging
#FX_GLIDE_VTUNE=
#
# Define --enable-amd3d
#
#enable_amd3d=false
#GL_AMD3D=1
#
# --enable-fx-texlib=texus2 enables new texus library.
#
TEXTURE_UTILITIES_DIR=texus2
#
# These are conditional variables whose value is set by
# one of the other enable macros.
#
#FX_GLIDE_H3=0
FX_GLIDE_H5=1
#FX_GLIDE_CVG=0
#
# Architecture
# current, i386,i486,i586,i685,alpha
# default=current
FX_GLIDE_BUILD_ARCHITECTURE=i686
#
# Various tests needed at points in the build
# First, we set defaults.
#
#FX_GLIDE_PACKET_FIFO=1
#FX_GLIDE3=1
#
# There is no entry for this for H5
#FX_CHRIS_DENIS_ANTHONY_HACK=1
#
# FX_GLIDE_ALT_TAB already in makefile of h5\glide3x\src but commented 
# out there. Need IS_ALT_TAB=1 to complete alt-tab in NT5.1
GLIDE_ALT_TAB=1
#FX_GLIDE_ALT_TAB=1 
#IS_ALT_TAB=1
#!if "$(FX_GLIDE_ALT_TAB)"=="1" && "$(IS_ALT_TAB)"=="1"
#LCDEFS          =       $(LCDEFS) -DIS_ALT_TAB=1
#!endif

# Colourless
# Remove this line to disable the WinXP Alt Tab fix. There will be a speed up
# Shouldn't use this anymore. Faster generally works better, in particular for SDL
# app. Doing it this way doesn't always detect vid mode switches
#LCDEFS = $(LCDEFS) -DWINXP_ALT_TAB_FIX=1

# Colourless
# Remove this line to disable the faster WinXP Alt Tab fix. There will be a minor speed up
LCDEFS = $(LCDEFS) -DWINXP_FASTER_ALT_TAB_FIX=1
#
# use direct i/o writes
#FX_GLIDE_DIRECT_WRITE=1
#
# HAL_CSIM has to be 1 or NULL (Simulator=1, Hardware=NULL) 
HAL_CSIM=
#
# 1 turns on cpu optimizations.
# HAL_CSIM must be NULL for this to work.
FX_GLIDE_CPUOPTS=1
#
#FX_GLIDE_NO_FIFO=1
#
!if "$(DEBUG)"=="1"
#FX_GLIDE_DEBUG_FIFO=1
!endif
#
#LCDEFS = $(LCDEFS) -D_FIFODUMP=1
#
# disable 3dfx watermark
FX_GLIDE_NO_PLUG=0
#
# disable 3dfx splashscreen
FX_GLIDE_NO_SPLASH=0
#
#
# Make a static link library.
FX_DLL_BUILD=1
#
# HW setup
# it's now faster to cull backfaced/zero area triangles in sw before the fifo
# forced to sw when using amd asm optimization.
#FX_GLIDE_HW_CULL=1
# Stick one more slot in the dataList so that the clamping loop does 
# not go one more iteration.
#GLIDE_FP_CLAMP=1
# if we're going to stick one more 0 in the texture datalist we need to
# have GLIDE_FP_CLAMP=1 ofcourse
#GLIDE_FP_CLAMP_TEX=1
#
# use C trisetup instead of asm setup.
#FX_GLIDE_CTRISETUP=1
#
#setting this to 1 will disable hw TSU for triangle rendering.
#FX_GLIDE_SW_SETUP=1
#
#
# compiler defs
FX_COMPILER=MICROSOFT
FX_TARGET=WIN32
FX_HOST=WIN32
FX_PCI_TARGET=NT
#FX_TARGET_MINOR=WIN95
#
#
#
#THISDIR = devel
#
# We branched glide starting w/ Voodoo2(aka CVG) so 
# make sure that the right thing happens. If this sort
# of branching happens again then add it to the list below.
#
!if "$(FX_GLIDE_HW)" == "sst1"
SUBDIRS = swlibs sst1
!else
!if  "$(FX_GLIDE_HW)" == "sst96"
SUBDIRS = swlibs sst1
!else
!if "$(FX_GLIDE_HW)" == "cvg"
SUBDIRS	= swlibs cvg
!ifndef HAL_HW
HAL_HW = 1
!endif
!else
!if "$(FX_GLIDE_HW)" == "h3"
SUBDIRS	= swlibs h3
!else 
!if "$(FX_GLIDE_HW)" == "h5"
SUBDIRS = swlibs h5	  
!else
FX_GLIDE_HW = h3
SUBDIRS = swlibs h3
!endif
!endif
!endif
!endif
!endif

#
# Other variables we need to set
#
!if "$(FX_HW_PROJECTS)"=="glide3"
FX_GLIDE3 = 1
!endif

############################
#
# variables added
#
############################
# use dxdrvr.c instead of win_mode.c for minihwc.lib
#HWC_DXDRVR = 1
#
#
# VXD stuff: need the LCDEFS options for win95 DDK compatiblity, just in case.
HWC_WIN40COMPAT = 1
#
#
# ABJ- hack
# use the minivdd to setup SLI/AA mode for hwcRestoreVideo().
# This is need till the not lazy stuff is properly done on a multichip comp.
HWC_MINIVDD_HACK = 1
#
#
# Save the initial vidScreenSize, lfbMemoryConfig, and bpp at hwcinitvideo
# and restore them at resetVideo().
HWC_WINXP_MINIVDD_HACK = 0
!if "$(HWC_MINIVDD_HACK)"!="1"
HWC_WINXP_MINIVDD_HACK = 0
!endif
#
#
# if 1, HWC_EXT_INIT is not used
#HWC_INIT_VXD = 1
#
#
# STRICT
STRICT=1
!if "$(STRICT)"=="1"
LCDEFS          =       $(LCDEFS) -DSTRICT -DWIN32_LEAN_AND_MEAN
#LADEFS          =       $(LADEFS) -DSTRICT
!endif
#
#
# other stuff
LCDEFS = $(LCDEFS) -D_MSC_VER=1200 -DWINVER=0x0400
#
!if "$(FX_GLIDE3)"=="1"
LCDEFS = $(LCDEFS) -DGLIDE3_VERTEX_LAYOUT=1 -DGLIDE3_SCALER=1
!endif
#
#-DGLIDE_VERTEX_TABLE=1
#-DGLIDE3_SCALER=1
#-D_MSC_VER=1200
#-DWINVER=0x0501
#-DGLIDE_TEST_TEXTURE_ALIGNMENT=1
#-DGLIDE_CHECK_TRILINEAR=1 use this if we want to check for trilinearness
#-DGLIDE3_VERTEX_LAYOUT=1 this was removed.
#-DLFB_DISABLE_SLAVE_FIFO=1
#-DGLIDE_MULTIPLATFORM=1
#-DPCI_BUMP_N_GRIND=1 machintosh?
#-DGLIDE_USE_ALT_REGMAP=1
#
# Protected hacks for Glide/Win. Working 1-window command fifo.
# enables 1 window glide in window. removed! we'll never use it.
#-DTACO_MEMORY_FIFO_HACK=1
#
# for WINNT Global structure used to store app state, used by the display driver
#LCDEFS = $(LCDEFS) -DWINNT=1 -D_WIN32_WINNT=0x0500 
# Called once when we start using the command fifo in order to tell
# NT / W2K to hold off on unmapping that view of the frame buffer and
# then called again once Glide knows it's lost its context so that
# the driver knows that it's now ok to unmap the memory.
# need WINNT=1 to work.
#ENABLE_V3_W2K_GLIDE_CHANGES = 1
!if "$(ENABLE_V3_W2K_GLIDE_CHANGES)"=="1"
LCDEFS          =       $(LCDEFS) -DENABLE_V3_W2K_GLIDE_CHANGES=1
!endif
# need WINNT=1 to work.
#SLI_AA = 1
!if "$(SLI_AA)"=="1"
LCDEFS          =       $(LCDEFS) -DSLI_AA=1
!endif
#
#LLDOPTS = /PROFILE
