REM
REM THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
REM PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
REM TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
REM INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
REM DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
REM THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
REM EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
REM FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
REM 
REM USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
REM RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
REM TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
REM AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
REM SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
REM THE UNITED STATES.  
REM 
REM COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
REM

@echo off
REM
REM if FX_GLIDE_TEST_SRCIMG is not initiated, the default directory is
REM \3dfx\devel\sst1\glide\images
REM if FX_GLIDE_TEST_DSTIMG is not initiated, the default directory is
REM \3dfx\devel\sst1\glide\images
REM if FX_GLIDE_TEST_DIFFIMG is not initiated, the default directory is
REM \3dfx\devel\sst1\glide\images
REM
if [%FX_GLIDE_TEST_DIFFIMG%]==[] goto Default1

goto Output1

:Default1
set FX_GLIDE_TEST_DIFFIMG=%BUILD_ROOT_SST1%\glide\images

:Output1
set ddir=%FX_GLIDE_TEST_DIFFIMG%

if [%FX_GLIDE_TEST_SRCIMG%]==[] goto Default2

goto Output2

:Default2
set FX_GLIDE_TEST_SRCIMG=%BUILD_ROOT_SST1%\glide\images

:Output2
set sdir=%FX_GLIDE_TEST_SRCIMG%

if [%FX_GLIDE_TEST_DSTIMG%]==[] goto Default3

goto Output3

:Default3
set FX_GLIDE_TEST_DSTIMG=%BUILD_ROOT_SST1%\glide\images

:Output3
set tdir=%FX_GLIDE_TEST_DSTIMG%

@echo on
isub -v %sdir%\test00.vgm %tdir%\test00.tst %ddir%\test00.tga
isub -v %sdir%\test01.vgm %tdir%\test01.tst %ddir%\test01.tga
isub -v %sdir%\test02.vgm %tdir%\test02.tst %ddir%\test02.tga
isub -v %sdir%\test03.vgm %tdir%\test03.tst %ddir%\test03.tga
isub -v %sdir%\test04.vgm %tdir%\test04.tst %ddir%\test04.tga
isub -v %sdir%\test05.vgm %tdir%\test05.tst %ddir%\test05.tga
isub -v %sdir%\test06.vgm %tdir%\test06.tst %ddir%\test06.tga
isub -v %sdir%\test07.vgm %tdir%\test07.tst %ddir%\test07.tga
isub -v %sdir%\test08.vgm %tdir%\test08.tst %ddir%\test08.tga
isub -v %sdir%\test09_0.vgm %tdir%\test09_0.tst %ddir%\test09_0.tga
isub -v %sdir%\test09_1.vgm %tdir%\test09_1.tst %ddir%\test09_1.tga
isub -v %sdir%\test10_0.vgm %tdir%\test10_0.tst %ddir%\test10_0.tga
isub -v %sdir%\test10_1.vgm %tdir%\test10_1.tst %ddir%\test10_1.tga
isub -v %sdir%\test13.vgm %tdir%\test13.tst %ddir%\test13.tga
isub -v %sdir%\test16.vgm %tdir%\test16.tst %ddir%\test16.tga
isub -v %sdir%\test17_0.vgm %tdir%\test17_0.tst %ddir%\test17_0.tga
isub -v %sdir%\test17_1.vgm %tdir%\test17_1.tst %ddir%\test17_1.tga
isub -v %sdir%\test17_2.vgm %tdir%\test17_2.tst %ddir%\test17_2.tga
isub -v %sdir%\test17_3.vgm %tdir%\test17_3.tst %ddir%\test17_3.tga
isub -v %sdir%\test17_4.vgm %tdir%\test17_4.tst %ddir%\test17_4.tga
isub -v %sdir%\test18.vgm %tdir%\test18.tst %ddir%\test18.tga
isub -v %sdir%\test19_0.vgm %tdir%\test19_0.tst %ddir%\test19_0.tga
isub -v %sdir%\test19_1.vgm %tdir%\test19_1.tst %ddir%\test19_1.tga
isub -v %sdir%\test19_2.vgm %tdir%\test19_2.tst %ddir%\test19_2.tga
isub -v %sdir%\test19_3.vgm %tdir%\test19_3.tst %ddir%\test19_3.tga
