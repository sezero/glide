#
# global configuration environment
#
# $Log:
# 1   ve3d      1.0         KoolSmoky    global config
#

# compiler defs
FX_COMPILER=MICROSOFT
FX_TARGET=WIN32
FX_HOST=WIN32
FX_PCI_TARGET=NT
# FX_TARGET_MINOR=WIN95

INCLUDE=$(INCLUDE)
INSTALL_DESTINATION=$(BUILD_ROOT)\$(FX_GLIDE_HW)

# Make a static link library.
FX_DLL_BUILD=1

# Define beta release
# LCDEFS = $(LCDEFS) -DBETA=1

# Define debuging
# DEBUG = 1

# Enables us to get stuff from SoftIce etc even when building 
# an optimized version. 
# FX_GLIDE_SYMBOLICS=1

# Supports minimal debug info on default even when building an
# optimized version. Use this to disable all debug info.
# CEASE_ALL_GDBG=1

# Use output to nt kernel debugger
# KERNEL_NT=1

# Use Intel VTune
# FX_GLIDE_VTUNE=1

# HAL_CSIM has to be 1 or NULL (Simulator=1, Hardware=NULL) 
HAL_CSIM=

# Use CPU optimizations.
# HAL_CSIM must be NULL for this to work.
FX_GLIDE_CPUOPTS=1

# disable 3dfx watermark
FX_GLIDE_NO_PLUG=0

# disable 3dfx splashscreen
FX_GLIDE_NO_SPLASH=0

# it's now faster to cull backfaced/zero area triangles in sw before the fifo
# forced to sw when using amd asm optimization.
# FX_GLIDE_HW_CULL=1

# Stick one more slot in the dataList so that the clamping loop does 
# not go one more iteration.
# GLIDE_FP_CLAMP=1

# if we're going to stick one more 0 in the texture datalist we need to
# have GLIDE_FP_CLAMP=1 ofcourse
# GLIDE_FP_CLAMP_TEX=1

# use C trisetup instead of asm setup.
# FX_GLIDE_CTRISETUP=1

# setting this to 1 will disable hw TSU for triangle rendering.
# FX_GLIDE_SW_SETUP=1

# use direct i/o writes
# FX_GLIDE_DIRECT_WRITE=1

# FX_GLIDE_NO_FIFO=1
# FX_GLIDE_DEBUG_FIFO=1
# LCDEFS = $(LCDEFS) -D_FIFODUMP=1

# are we compiling for glide3x?
!if "$(FX_HW_PROJECTS)"=="glide3"
FX_GLIDE3 = 1
!endif

# use dxdrvr.c instead of win_mode.c for minihwc.lib
# HWC_DXDRVR = 1

# if 1, HWC_EXT_INIT is not used
# HWC_INIT_VXD = 1

# VXD stuff: need the LCDEFS options for win95 DDK compatiblity, just in case.
HWC_WIN40COMPAT = 1

# ABJ- hack
# use the minivdd to setup SLI/AA mode for hwcRestoreVideo().
# This is need till the not lazy stuff is properly done on a multichip comp.
HWC_MINIVDD_HACK = 1

# Obsolete. We now use Colourless's Alt Tab fix for winnt5.1
# LCDEFS          =       $(LCDEFS) -DIS_ALT_TAB=1

# Colourless
# Remove this line to disable the WinXP Alt Tab fix. There will be a speed up
# Shouldn't use this anymore. Faster generally works better, in particular for SDL
# app. Doing it this way doesn't always detect vid mode switches
# LCDEFS = $(LCDEFS) -DWINXP_ALT_TAB_FIX=1

# Colourless
# Remove this line to disable the faster WinXP Alt Tab fix. There will be a minor speed up
LCDEFS = $(LCDEFS) -DWINXP_FASTER_ALT_TAB_FIX=1

# profiling
# LLDOPTS = /PROFILE

#
# misc options
#
LCDEFS = $(LCDEFS) -DSTRICT -DWIN32_LEAN_AND_MEAN
LADEFS = $(LADEFS) -DSTRICT
LCDEFS = $(LCDEFS) -D_MSC_VER=1200 -DWINVER=0x0400
# LCDEFS = $(LCDEFS) -DGLIDE3_VERTEX_LAYOUT=1
# LCDEFS = $(LCDEFS) -DGLIDE_VERTEX_TABLE=1
# LCDEFS = $(LCDEFS) -DGLIDE3_SCALER=1
# LCDEFS = $(LCDEFS) -DGLIDE_TEST_TEXTURE_ALIGNMENT=1
# LCDEFS = $(LCDEFS) -DLFB_DISABLE_SLAVE_FIFO=1
# LCDEFS = $(LCDEFS) -DGLIDE_MULTIPLATFORM=1
# LCDEFS = $(LCDEFS) -DPCI_BUMP_N_GRIND=1 machintosh?
# LCDEFS = $(LCDEFS) -DGLIDE_USE_ALT_REGMAP=1
#
# Sandro's fast C clipping routine. GLIDE_USE_C_TRISETUP=1 must be set.
# LCDEFS = $(LCDEFS) -DFAST_C_CLIP=1
#
# Check for trilinearness
# LCDEFS = $(LCDEFS) -DGLIDE_CHECK_TRILINEAR=1
#
# Protected hacks for Glide/Win. Working 1-window command fifo.
# enables 1 window glide in window. removed! we'll never use it.
# LCDEFS = $(LCDEFS) -DTACO_MEMORY_FIFO_HACK=1
#
# for WINNT Global structure used to store app state, used by the display driver
# LCDEFS = $(LCDEFS) -DWINNT=1 -D_WIN32_WINNT=0x0500 
#
# Called once when we start using the command fifo in order to tell
# NT / W2K to hold off on unmapping that view of the frame buffer and
# then called again once Glide knows it's lost its context so that
# the driver knows that it's now ok to unmap the memory.
# need WINNT=1 to work.
# LCDEFS = $(LCDEFS) -DENABLE_V3_W2K_GLIDE_CHANGES=1
# LCDEFS = $(LCDEFS) -DSLI_AA=1

#
# We branched glide starting w/ Voodoo2(aka CVG) so 
# make sure that the right thing happens. If this sort
# of branching happens again then add it to the list below.
#
!if "$(FX_GLIDE_HW)" == "sst1"
SUBDIRS = swlibs sst1
!else if  "$(FX_GLIDE_HW)" == "sst96"
SUBDIRS = swlibs sst1
!else if "$(FX_GLIDE_HW)" == "cvg"
FX_GLIDE_CVG=1
SUBDIRS	= swlibs cvg
!ifndef HAL_HW
HAL_HW = 1
!endif
!else if "$(FX_GLIDE_HW)" == "h3"
FX_GLIDE_H3=1
SUBDIRS	= swlibs h3
!else if "$(FX_GLIDE_HW)" == "h5"
FX_GLIDE_H5=1
SUBDIRS = swlibs h5	  
!endif
