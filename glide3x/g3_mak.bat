@echo off
rem ///////////////////////////////////////////////////////////////////
rem 03/01/2003 batchfile for win32 Glide builds
rem KoolSmoky koolsmoky@3dfxzone.it
rem http://www.3dfxzone.it/koolsmoky/
rem ///////////////////////////////////////////////////////////////////
echo.
echo #
echo # 3dfx Glide win32 makefile
echo #
echo # The Glide Open Source Project
echo # http://glide.sourceforge.net/
echo #
echo # "g3_mak.bat [ all | gendate | minihwc | dll ] [ clean ]"
echo #
rem 
rem 
rem Build Environment
rem Configure these to suite your environment.
rem The source tree should be as follows:
rem
rem BUILD_ROOT-+-h5-+-bin
rem            |    |
rem            |    +-glide3---src
rem            |    |
rem            |    +-include
rem            |    |
rem            |    +-incsrc
rem            |    |
rem            |    +-lib
rem            |    |
rem            |    +-minihwc
rem            |
rem            +-swlibs-+-fxmisc
rem                     |
rem                     +-include---nmake
rem                     |
rem                     +-newpci---pcilib
rem
rem ///////////////////////////////////////////////////////////////////
set BUILD_ROOT=%cd%
set BUILD_ROOT_SWLIBS=%BUILD_ROOT%\swlibs
set FX_GLIDE_HW=h5
set FX_HW_PROJECTS=glide3
rem
rem ///////////////////////////////////////////////////////////////////
rem Compiler Environment
rem These are usually preconfigured in the command prompt environment.
rem ///////////////////////////////////////////////////////////////////
rem set DXSDKROOT=c:\dxsdk
rem set CC=%C32_ROOT%\bin\cl.exe
rem set AS=%MASM_ROOT%\bin\ml.exe
rem
if "%1" == "" goto error
goto %1
REM ----------build routine from here -------------
:all
if "%2" == "clean" goto clean
:gendate
if "%2" == "clean" goto clean_gendate
cd %BUILD_ROOT%\%FX_GLIDE_HW%\incsrc
nmake -f %BUILD_ROOT%\%FX_GLIDE_HW%\incsrc\makefile
if "%1" == "gendate" goto end
:minihwc
if "%2" == "clean" goto clean_minihwc
cd %BUILD_ROOT_SWLIBS%\fxmisc
nmake -f %BUILD_ROOT_SWLIBS%\fxmisc\makefile
cd %BUILD_ROOT_SWLIBS%\newpci\pcilib
nmake -f %BUILD_ROOT_SWLIBS%\newpci\pcilib\makefile
cd %BUILD_ROOT%\%FX_GLIDE_HW%\minihwc
nmake -f %BUILD_ROOT%\%FX_GLIDE_HW%\minihwc\makefile
if "%1" == "minihwc" goto end
:dll
if "%2" == "clean" goto clean_dll
cd %BUILD_ROOT%\%FX_GLIDE_HW%\%FX_HW_PROJECTS%\src
nmake -f %BUILD_ROOT%\%FX_GLIDE_HW%\%FX_HW_PROJECTS%\src\makefile
if "%1" == "dll" goto end
goto end
REM ----------clean routine from here -------------
:clean
cd %BUILD_ROOT%\%FX_GLIDE_HW%\include
nmake -f %BUILD_ROOT%\%FX_GLIDE_HW%\include\makefile clean
cd %BUILD_ROOT%\%FX_GLIDE_HW%\lib
nmake -f %BUILD_ROOT%\%FX_GLIDE_HW%\lib\makefile clean
cd %BUILD_ROOT%\%FX_GLIDE_HW%\bin
nmake -f %BUILD_ROOT%\%FX_GLIDE_HW%\bin\makefile clean
cd %BUILD_ROOT_SWLIBS%\include
nmake -f %BUILD_ROOT_SWLIBS%\include\makefile clean
:clean_gendate
cd %BUILD_ROOT%\%FX_GLIDE_HW%\incsrc
nmake -f %BUILD_ROOT%\%FX_GLIDE_HW%\incsrc\makefile clean
if "%1" == "gendate" goto end
:clean_minihwc
cd %BUILD_ROOT_SWLIBS%\fxmisc
nmake -f %BUILD_ROOT_SWLIBS%\fxmisc\makefile clean
cd %BUILD_ROOT_SWLIBS%\newpci\pcilib
nmake -f %BUILD_ROOT_SWLIBS%\newpci\pcilib\makefile clean
cd %BUILD_ROOT%\%FX_GLIDE_HW%\minihwc
nmake -f %BUILD_ROOT%\%FX_GLIDE_HW%\minihwc\makefile clean
if "%1" == "minihwc" goto end
:clean_dll
cd %BUILD_ROOT%\%FX_GLIDE_HW%\%FX_HW_PROJECTS%\src
nmake -f %BUILD_ROOT%\%FX_GLIDE_HW%\%FX_HW_PROJECTS%\src\makefile clean
if "%1" == "dll" goto end
goto end
:error
echo # oops!
echo #
echo.
goto realend
:end
echo.
echo #
echo # 3dfx Glide win32 makefile
echo #
echo # The Glide Open Source Project
echo # http://glide.sourceforge.net/
echo #
echo.
goto realend
:realend
cd %BUILD_ROOT%
