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

!include $(BUILD_ROOT)\config.mak

LDIRT= *.exe *.map *.sys *.obj *.lib

# Special case rush because its built off of the sst1 tree
!if "$(FX_GLIDE_HW)" == "SST96"
GLIDE_ROOT      =       $(BUILD_ROOT)\sst1
!else
GLIDE_ROOT      =       $(BUILD_ROOT)\$(FX_GLIDE_HW)
!endif

LCINCS  = $(LCINCS)  -I$(GLIDE_ROOT)\include -WX

!if "$(FX_GLIDE_HW)" == "h5"
LCDEFS          =       $(LCDEFS) -DFX_GLIDE_NAPALM=1
!endif

!if "$(FX_COMPILER)"=="WATCOM"
LIBOBJS = tlib.lib
!else
LIBOBJS = tlib.obj
!endif

GLIDELIB        =       glide3x.lib

LLDLIBS = $(LIBOBJS) $(GLIDE_ROOT)\lib\$(GLIDELIB)\
          $(BUILD_ROOT_SWLIBS)\lib\fxmisc.lib
!if "$(HAL_CSIM)" == "1"
LLDLIBS = $(LLDLIBS) $(GLIDE_ROOT)\lib\h4hal.lib
!endif

PRIVATE_HEADERS = tlib.h tlib.c tldata.inc

CFILES   = test00.c \
           test01.c \
           test02.c \
           test03.c \
           test04.c \
           test05.c \
           test06.c \
           test07.c \
           test08.c \
           test09.c \
           test10.c \
           test11.c \
           test12.c \
           test13.c \
           test14.c \
           test15.c \
           test16.c \
           test17.c \
           test18.c \
           test19.c \
           test20.c \
           test21.c \
           test22.c \
           test23.c \
           test24.c \
           test25.c \
           test26.c \
           test27.c \
           test28.c \
           test29.c \
           test30.c \
           test31.c \
           test32.c \
           test33.c \
           test34.c \
           test35.c \
           test36.c \
           test37.c \
           test38.c \
           test39.c \
           test40.c \
           test41.c \
           test42.c \
           test43.c \
           test44.c \
           test45.c \
           test46.c \
           test47.c \
           test48.c \
           test49.c \
           display.c \
           sbench.c

PROGRAMS = $(CFILES:.c=.exe)

DATAFILES = alpha.3df decal1.3df lava.3df light.3df matt1.3df miro.3df

!include $(BUILD_ROOT)\swlibs\include\nmake\3dfx.mak

$(PROGRAMS):  $(LLDLIBS)

!if "$(FX_COMPILER)"=="WATCOM"
!if "$(FX_TARGET)"=="DOS"
tlib.lib: tlib.obj
        wlib -b -c -n -q -p=512 tlib tlib.obj
!else
tlib.lib: tlib.obj
        wlib -b -c -n -q -p=512 tlib tlib.obj
!endif
!endif

test00: test00.exe
test01: test01.exe
test02: test02.exe
test03: test03.exe
test04: test04.exe
test05: test05.exe
test06: test06.exe
test07: test07.exe
test08: test08.exe
test09: test09.exe
test10: test10.exe
test11: test11.exe
test12: test12.exe
test13: test13.exe
test14: test14.exe
test15: test15.exe
test16: test16.exe
test17: test17.exe
test18: test18.exe
test19: test19.exe
test20: test20.exe
test21: test21.exe
test22: test22.exe
test23: test23.exe
test24: test24.exe
test25: test25.exe
test26: test26.exe
test27: test27.exe
test28: test28.exe
test29: test29.exe
test30: test30.exe
test31: test31.exe
test32: test32.exe
test33: test33.exe
test34: test34.exe
test35: test35.exe
test36: test36.exe
test37: test37.exe
test38: test38.exe
test39: test39.exe
test40: test40.exe
test41: test41.exe
test42: test42.exe
test43: test43.exe
test44: test44.exe
test45: test45.exe
test46: test46.exe
test47: test47.exe
test48: test48.exe
test49: test49.exe
sbench: sbench.exe
display: display.exe
        
