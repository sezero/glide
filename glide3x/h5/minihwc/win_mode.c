/*
** THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
** PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
** TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
** INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
** DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
** THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
** EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
** FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
** 
** USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
** RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
** TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
** AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
** SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
** THE UNITED STATES.  
** 
** COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
**
** $Log:
**  6    GlideXP   1.0.5           18/10/01 Ryan Nunn       Updating getModesRegPath
**  5    GlideXP   1.0.4           18/10/01 Ryan Nunn       Fixing another multimonitor bug
**  4    GlideXP   1.0.3           18/10/01 Ryan Nunn       Properly check refresh rates
**  3    GlideXP   1.0.2           15/10/01 Ryan Nunn       Fixing a multimonitor bug
**  2    GlideXP   1.0.1           14/10/01 Ryan Nunn       Check refresh rates
**  1    GlideXP   1.0             12/10/01 Ryan Nunn       Getting to compile
**       In windows for H5 Glide3. Copied from H3 Glide3.
**  7               1.11        11/30/02 KoolSmoky    moved CEASE_ALL_GDBG because we need
**        debug info for certain apps to run.
**  6     ve3d      1.10        06/24/02 KoolSmoky    ditch changing display modes with ddraw
**         and use changedisplaysettings instead. also removed ddraw from checkresolutions.
**  5     ve3d      1.04        12/27/01 KoolSmoky    added a switch to cease all debug info.
**  4     ve3d      1.03        11/30/01 KoolSmoky    output minimum debug info on release compiles.
**  3     ve3d      1.02        11/29/01 KoolSmoky    better displaymode enum. checks for bpp & refresh
**  2     ve3d      1.01        11/28/01 KoolSmoky    modified setVideoMode() to accept h3pixelSize
**         and checkResolutions() to accept stride from minihwc.c. Misc fixes to compile.
**  1     ve3d      1.00        11/25/01 KoolSmoky    copied from Glide\h3\minihwc\win_mode.c
**
*/

#if !defined(GDBG_INFO_ON) || (GDBG_INFO_ON == 0)
#if defined(GDBG_INFO_ON)
#undef GDBG_INFO_ON
#endif /* defined(GDBG_INFO_ON) */
#define GDBG_INFO_ON
#endif /* !defined(GDBG_INFO_ON) || (GDBG_INFO_ON == 0) */

#include <stdio.h>
#include <3dfx.h>
#include <gdebug.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h> 
#include <ddraw.h> 
#include <sst1vid.h>
#include "qmodes.h"
#include "setmode.h"
#define IS_32
#define Not_VxD
#include <minivdd.h>
#include <vmm.h>
#include <configmg.h>

/* Windows */
#define SEPARATOR '\\'
/* UNIX */
#define SEPARATOR2 '/'

/* Things Added */
#ifdef IS_ALT_TAB
static DEVMODE curDevMode;
static char *curDeviceName;
#endif

static HWND	hwndApp = 0;
//static GUID     fooGuid;
//static FxU32	_stride;

/* Direct Draw stuff */
LPDIRECTDRAW            lpDD1 = NULL;
LPDIRECTDRAW2           lpDD  = NULL;

#if WINXP_ALT_TAB_FIX
LPDIRECTDRAW4           lpDD4 = NULL;
#endif

char *opengl_regpath; /* KoolSmoky - registry path passed from grEnable */

#ifdef IS_ALT_TAB
FxBool is_opengl;     /* KoolSmoky - OpenGL flag */
FxBool is_fullscreen; /* KoolSmoky - full screen flag */
#endif

/*
 * parseFilename
 *
 *      Return the file name portion of a filename/path.
 */

static char *
_parseFilename(char *name)
{
    int i;

    if (name == NULL) 
      return NULL;
    for(i = strlen(name); i >= 0; i--)
        if ((name[i] == SEPARATOR) ||
            (name[i] == SEPARATOR2))
            return (name + i + 1);
    return name;
}  /* End of parseFilename*/


static int _set_exclusive_relaxed;
static int _set_vidmode_relaxed;

typedef struct _enumInfoStruct {
  GUID guid;
  HMONITOR hmon;
} EnumInfo;

static BOOL FAR PASCAL
ddEnumCbEx( GUID FAR *guid, LPSTR desc, LPSTR name, LPVOID ctx, HMONITOR hmon ) 
{
#if 0
  DWORD    *data  = (DWORD*)ctx;
  HMONITOR target = (HMONITOR)data[0];
  BOOL     rv     = DDENUMRET_OK;
#else
  EnumInfo* pEnumInfo = (EnumInfo*)ctx;
  BOOL     rv     = DDENUMRET_OK;
#endif
  
#if 0
  if ( target == hmon ) {
    if ( guid ) {
      fooGuid = *guid;
      data[1] = (DWORD)&fooGuid;
    } else {
      /* guid for primary display device */
      data[1] = 0;
    }
    rv     = DDENUMRET_CANCEL;
  }
#else
  if(pEnumInfo->hmon == hmon) {
    if ( guid ) CopyMemory(&pEnumInfo->guid, guid, sizeof(GUID));
    rv     = DDENUMRET_CANCEL;
  }
#endif
  
  return rv;
}

typedef struct _emcStruct {
  FxU32 xRes, yRes, Refresh;
  FxU32 bpp;
  FxBool modeOK;
} EMCData;

static HRESULT WINAPI
enumModesCallback(LPDDSURFACEDESC surfaceDesc, LPVOID lpContext)
{
  EMCData *emcData = (EMCData *) lpContext;
  
  GDBG_INFO(80, "enumModesCallbac:\n");
  GDBG_INFO(80, "\tsurfaceDesc->dwWidth == %d\n", surfaceDesc->dwWidth);
  GDBG_INFO(80, "\tsurfaceDesc->dwHeight == %d\n", surfaceDesc->dwHeight);
  GDBG_INFO(80, 
            "\tsurfaceDesc->dwRefreshRate == %d\n",surfaceDesc->dwRefreshRate);
  GDBG_INFO(80, "\tsurfaceDesc->ddpfPixelFormat.dwRGBBitCount == %d\n", 
            surfaceDesc->ddpfPixelFormat.dwRGBBitCount);
  if (
    (surfaceDesc->dwWidth == emcData->xRes) &&
    (surfaceDesc->dwHeight == emcData->yRes) &&
    (surfaceDesc->ddpfPixelFormat.dwRGBBitCount == emcData->bpp) &&
    ((surfaceDesc->dwRefreshRate == emcData->Refresh) ||
     (surfaceDesc->dwRefreshRate == 0) ||
     (emcData->Refresh == 0))) {
    emcData->modeOK = FXTRUE;
    return DDENUMRET_CANCEL;
  } else
    return DDENUMRET_OK;
  
} /* enumModesCallback */

static void
msgModeSetFailure(HRESULT hResult) 
{
  
  if (hResult& DDERR_GENERIC)
    GDBG_INFO(80, "Mode set failed due to DDERR_GENERIC\n");
  
  if (hResult & DDERR_INVALIDMODE)
    GDBG_INFO(80, "Mode set failed due to DDERR_INVALIDMODE\n");
  
  if (hResult & DDERR_INVALIDOBJECT)
    GDBG_INFO(80, "Mode set failed due to DDERR_INVALIDOBJECT\n");
  
  if (hResult & DDERR_INVALIDPARAMS)
    GDBG_INFO(80, "Mode set failed due to DDERR_INVALIDPARAMS\n");
  
  if (hResult & DDERR_LOCKEDSURFACES)
    GDBG_INFO(80, "Mode set failed due to DDERR_LOCKEDSURFACES\n");
  
  if (hResult & DDERR_NOEXCLUSIVEMODE)
    GDBG_INFO(80, "Mode set failed due to DDERR_NOEXCLUSIVEMODE\n");
  
  if (hResult & DDERR_SURFACEBUSY)
    GDBG_INFO(80, "Mode set failed due to DDERR_SURFACEBUSY\n");
  
  if (hResult & DDERR_UNSUPPORTED)
    GDBG_INFO(80, "Mode set failed due to DDERR_UNSUPPORTED\n");
  
  if (hResult & DDERR_UNSUPPORTEDMODE)
    GDBG_INFO(80, "Mode set failed due to DDERR_UNSUPPORTEDMODE\n");
  
  if (hResult & DDERR_WASSTILLDRAWING)
    GDBG_INFO(80, "Mode set failed due to DDERR_WASSTILLDRAWING\n");
  
} /* msgModeSetFailure */

static void
msgEnumDisplayModes(hResult)
{
  if (hResult & DDERR_INVALIDOBJECT)
    GDBG_INFO(80, "DDERR_INVALIDOBJECT\n");

  if (hResult & DDERR_INVALIDPARAMS)
    GDBG_INFO(80, "DDERR_INVALIDPARAMS\n");

} /* msgEnumDisplayModes */

#ifdef __WIN32__
static char *
getModesRegPath() 
{
  char *retVal = NULL;
  OSVERSIONINFO ovi;
  
  ovi.dwOSVersionInfoSize = sizeof ( ovi );
  GetVersionEx ( &ovi );
  if (ovi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
    HKEY hKey;
    DWORD type ;
    static char strval[255];
    DWORD szData = sizeof(strval) ;

/*    GDBG_INFO(80, "OS == WNT\n"); */
    if ( ovi.dwMajorVersion >= 5 && ovi.dwMinorVersion >= 1) {
      GDBG_INFO(80, "REGPATH: OS == NT5.1\n");
    } else {
      GDBG_INFO(80, "REGPATH: OS == NT4\n");
    }

    /* Go fishing for the registry path on Win2K */
    if (RegOpenKey(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\VIDEO", &hKey) == ERROR_SUCCESS)
    {
      if (RegQueryValueEx(hKey, "\\Device\\Video0", NULL, &type, strval, &szData) ==  ERROR_SUCCESS)
      {
        if (type != REG_SZ)
        {
          /* It is hardcoded on NT via Display Control code. see:
           * $/devel/swtools/bansheecp2 */
          retVal = "SYSTEM\\CurrentControlSet\\Services\\3Dfx\\Device0\\modes\\";
        }
         else
         {
          strcat(strval, "\\modes\\") ;
          retVal = (char*)((int)strval + strlen("\\REGISTRY\\Machine\\")) ;
         }
      }
       else
         retVal = "SYSTEM\\CurrentControlSet\\Services\\3Dfx\\Device0\\modes\\";

      RegCloseKey(hKey);
    }
  } else {
    QDEVNODE QDevNode;
    QIN Qin;
    int status;

    GDBG_INFO(80, "OS == W9X\n");

    Qin.dwSubFunc = QUERYDEVNODE;
    {
      HDC curDC = GetDC(NULL);

      status = ExtEscape ( (HDC)curDC, QUERYESCMODE, 
                           sizeof(Qin), (LPCSTR)&Qin, 
                           sizeof(QDevNode), (LPSTR)&QDevNode );
      ReleaseDC(NULL, curDC);
    }

    if ( status > 0 ) {
      static char regPath[255];

      CM_Get_DevNode_Key( QDevNode.dwDevNode, NULL, 
                          &regPath, sizeof(regPath), 
                          CM_REGISTRY_SOFTWARE );
      strcat(regPath, "\\modes\\");

      retVal = regPath;
    }
  }

  GDBG_INFO( 80, "REGPATH: %s\n", retVal );

  return retVal;
} /* getRegPath */
#endif

FxBool 
setVideoMode( HWND hwnd, int xRes, int yRes, int h3pixelSize, int refresh, void *hmon, char *regpath , char *devicename )
{
  LPGUID          ddGuid = NULL;
  HMODULE         ddraw = NULL;
#if (WINXP_FASTER_ALT_TAB_FIX || WINXP_ALT_TAB_FIX)
  DDSURFACEDESC   ddsd;
  EMCData emcData;               /* Enum Modes Callbac Data */
#endif
  HRESULT hResult;
  DEVMODE devMode;
  FxU32 bpp = 16;
  //HRESULT ddRVal;
  //DWORD style;
#ifdef IS_ALT_TAB
  OSVERSIONINFO ovi;

  /* reset fullscreen flag */
  is_fullscreen = FXFALSE;
#endif

#ifdef FX_GLIDE_NAPALM
  if (h3pixelSize == 4) bpp = 32;
  GDBG_INFO( 80, "Glide mode: %dx%dx%dbpp@%dHz\n",
             xRes,
             yRes,
             bpp,
             refresh);
#endif
  
  GDBG_INFO( 80, "setVideoMode sees hwnd %x\n", hwnd);
  hwndApp = ( hwnd == NULL ) ? GetActiveWindow() : hwnd;
  
  if ( hwndApp == NULL ) {
    GDBG_INFO( 80, "Couldn't get a valid window handle\n" );
  }

  /* KoolSmoky - Window style and dimensions are not important,
  ** since they will be adjusted elsewhere. It's important to call
  ** ShowWindow to ensure that any activity from other windows is
  ** obscured.*/
  //ShowWindow( hwndApp, SW_SHOW );
  //SetForegroundWindow(hwndApp);
  // HACK //
  //Sleep(100);

  ddGuid = NULL;
  ddraw = GetModuleHandle( "ddraw.dll" );      
  if ( ddraw != NULL ) {
    LPDIRECTDRAWENUMERATEEXA ddEnumEx;
    ddEnumEx = (void*)GetProcAddress( ddraw, "DirectDrawEnumerateExA" );
    if ( ddEnumEx ) {
#if 0
      DWORD   data[2];
      data[0] = (DWORD)hmon;
      data[1] = 0;
      ddEnumEx( ddEnumCbEx, data, DDENUM_ATTACHEDSECONDARYDEVICES );
      if ( data[1] ) {
        ddGuid = (LPGUID)data[1];
      }
#else
      EnumInfo enumInfo;

      ZeroMemory(&enumInfo, sizeof(enumInfo));
      ZeroMemory(&enumInfo.guid, sizeof(GUID));
      enumInfo.hmon = (HMONITOR)hmon;
      ddEnumEx( ddEnumCbEx, &enumInfo, DDENUM_ATTACHEDSECONDARYDEVICES );
      ddGuid = &enumInfo.guid;
#endif
    }
  }

  /* KoolSmoky - this only covers the PRIMARY VGA?  */
//  EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);
  /* KooSmoky - save display mode to restore at exit */
#ifdef IS_ALT_TAB
  curDeviceName=devicename;
  EnumDisplaySettings(curDeviceName, ENUM_REGISTRY_SETTINGS, &curDevMode);

  /* KoolSmoky - Hack for win95. make a disp struct if we don't get anything
   * from EnumDisplaySettings.
   */
  if ((curDevMode.dmBitsPerPel < 8UL) || (curDevMode.dmBitsPerPel > 32UL)) {
    HDC hdc = GetDC(NULL);
    curDevMode.dmSize = sizeof(DEVMODE) ;
    curDevMode.dmPelsWidth = GetSystemMetrics(SM_CXSCREEN) ;
    curDevMode.dmPelsHeight = GetSystemMetrics(SM_CYSCREEN) ;
    curDevMode.dmBitsPerPel = (DWORD)GetDeviceCaps(hdc, BITSPIXEL) ;
    curDevMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT ;
    ReleaseDC(NULL, hdc) ;
  }

  GDBG_INFO(80, "DeviceName: %s Display mode: %dx%dx%dbpp!\n", curDeviceName, curDevMode.dmPelsWidth, curDevMode.dmPelsHeight, curDevMode.dmBitsPerPel);

#endif

#if (WINXP_FASTER_ALT_TAB_FIX || WINXP_ALT_TAB_FIX)
   EnumDisplaySettings(devicename, ENUM_REGISTRY_SETTINGS, &devMode);

  /* KoolSmoky - Hack for win95. make a disp struct if we don't get anything
   * from EnumDisplaySettings.
   */
  if ((devMode.dmBitsPerPel < 8UL) || (devMode.dmBitsPerPel > 32UL)) {
    HDC hdc = GetDC(NULL);
    devMode.dmSize = sizeof(DEVMODE) ;
    devMode.dmPelsWidth = GetSystemMetrics(SM_CXSCREEN) ;
    devMode.dmPelsHeight = GetSystemMetrics(SM_CYSCREEN) ;
    devMode.dmBitsPerPel = (DWORD)GetDeviceCaps(hdc, BITSPIXEL) ;
    devMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT ;
    ReleaseDC(NULL, hdc) ;
  }

  GDBG_INFO(80, "DeviceName: %s Display mode: %dx%dx%dbpp!\n", devicename, devMode.dmPelsWidth, devMode.dmPelsHeight, devMode.dmBitsPerPel);

  /*
  **  Oh, this is lovely.  What we have here is a failure to
  **  communicate.  If the current mode is exactly the same as the
  **  mode in which Glide wishes to run, then the DirectDraw driver
  **  doesn't get called when we restore the video (or on an alt-tab)
  **  this would be fine, except Glide has its way with the video
  **  overlay registers.  So, if we detect this case, we ask DDraw for
  **  an 8-bit display, and fix up vidProcCfg in the minihwc.
  */
  /* KoolSmoky - what would happen if we run from 8bpp? */
  if ((devMode.dmPelsWidth == (FxU32) xRes) &&
      (devMode.dmPelsHeight ==  (FxU32) yRes)) {
      GDBG_INFO(80, "DDraw communication hack: setting to 8bpp!\n");
      bpp = 8;
  }
#endif

  
//  checkSpecialList();

#ifdef IS_ALT_TAB
  /* get os version */
  ovi.dwOSVersionInfoSize = sizeof ( ovi );
  GetVersionEx ( &ovi );
#endif
  
  if (lpDD == NULL) {
    /* only create directdraw object once */
    if ( DirectDrawCreate( ddGuid, &lpDD1, NULL ) != DD_OK) {
      GDBG_INFO(80, "DDraw Obj Create Failed!\n");
    }
    else GDBG_INFO(80, "DDraw Obj created!\n");
    if ( IDirectDraw_QueryInterface( lpDD1, &IID_IDirectDraw2, 
                                     (LPVOID*)&lpDD ) != DD_OK ) {
      IDirectDraw_Release( lpDD1 );
#if WINXP_ALT_TAB_FIX
      lpDD4 = NULL;
#endif
      lpDD1 = NULL;
      lpDD  = NULL;
      GDBG_INFO(80, "DDraw Obj Create Failed!\n");
      return FXFALSE;            
    } 
    else GDBG_INFO(80, "DDraw2 Obj created!\n");
#if WINXP_ALT_TAB_FIX
    if ( IDirectDraw_QueryInterface( lpDD1, &IID_IDirectDraw4, 
                                     (LPVOID*)&lpDD4 ) != DD_OK ) {
      IDirectDraw_Release( lpDD );
      IDirectDraw_Release( lpDD1 );
      lpDD4 = NULL;
      lpDD1 = NULL;
      lpDD  = NULL;
      GDBG_INFO(80, "DDraw Obj Create Failed!\n");
      return FXFALSE;            
    } 
    else GDBG_INFO(80, "DDraw4 Obj created!\n");
#endif
  }

  /* KoolSmoky - moves all of the pieces of surface memory
   * on the display card to a contiguous block to make the
   * largest single amount of free memory available.
  IDirectDraw2_Compact(lpDD);
  IDirectDraw_Compact(lpDD1);*/
  
  /* Set Exclusive Mode, change resolution,  */
  GDBG_INFO(80, "Setting Full screen exclusive mode!\n");
  GDBG_INFO(80, "Calling IDD2_SetCoop: 0x%x, 0x%x, 0x%x\n", lpDD, hwndApp, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN );

  hResult = IDirectDraw2_SetCooperativeLevel(lpDD, hwndApp,
                                               /*DDSCL_ALLOWREBOOT |*/
                                               DDSCL_EXCLUSIVE |
                                               DDSCL_FULLSCREEN);
  if(hResult != DD_OK)
  {
    GDBG_INFO(80, "Couldn't set cooperative level:  " );
    if (hResult & DDERR_EXCLUSIVEMODEALREADYSET)
      GDBG_INFO_MORE(80, "DDERR_EXCLUSIVEMODEALREADYSET\n" ); 
    
    if (hResult & DDERR_HWNDALREADYSET) {
      GDBG_INFO_MORE(80, "DDERR_HWNDALREADYSET\n" );
      if (hResult == DDERR_HWNDALREADYSET)
        _set_exclusive_relaxed = 1;
    }
    if (hResult & DDERR_HWNDSUBCLASSED)
      GDBG_INFO_MORE(80, "DDERR_HWNDSUBCLASSED\n" );
    
    if (hResult & DDERR_INVALIDOBJECT)
      GDBG_INFO_MORE(80, "DDERR_INVALIDOBJECT\n" );
    
    if (hResult & DDERR_INVALIDPARAMS)
      GDBG_INFO_MORE(80, "DDERR_INVALIDPARAMS\n" );
    
    if (hResult & DDERR_OUTOFMEMORY)
      GDBG_INFO_MORE(80, "DDERR_OUTOFMEMORY\n" );
    
    if (!_set_exclusive_relaxed)
      return FXFALSE;
  }
  
  GDBG_INFO(80, "FSEM Set\n" );
  GDBG_INFO(80, "Enumerating Display Modes.\n");
  
  /* Figure out if we can support the requested display mode.  If not,
     try to use the same x & y res, but the default refresh rate.*/
  /* KoolSmoky - ditch ddraw and use changedisplaysettings to switch modes */
  GDBG_INFO(80, "Setting Display Mode!\n");

#ifdef IS_ALT_TAB
  devMode.dmSize = sizeof(DEVMODE);
  devMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
  devMode.dmPelsWidth = xRes;
  devMode.dmPelsHeight = yRes;
  devMode.dmBitsPerPel = bpp;
  devMode.dmDisplayFrequency = refresh;

  if(((ovi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) && (ovi.dwMinorVersion < 10)) ||
     ((ovi.dwPlatformId == VER_PLATFORM_WIN32_NT) && (ovi.dwMajorVersion < 5)))
  {
    // win95,nt4
    if(ChangeDisplaySettings(&devMode, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL)
    {
      /*style = GetWindowLong(hwndApp, GWL_STYLE);
      style &= ~(WS_MAXIMIZE|WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX);
      style |= WS_POPUP;
      SetWindowLong(hwndApp, GWL_STYLE, style);*/

      is_fullscreen = FXTRUE;
      
      GDBG_INFO(80, "Display Mode Set\n" );
    } else {
      GDBG_INFO(80, "Setting display mode at default refresh\n");
      
      devMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT ;
      
      if(ChangeDisplaySettings(&devMode, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL)
      {
        /*style = GetWindowLong(hwndApp, GWL_STYLE);
        style &= ~(WS_MAXIMIZE|WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX);
        style |= WS_POPUP;
        SetWindowLong(hwndApp, GWL_STYLE, style);*/

        is_fullscreen = FXTRUE;
        
        GDBG_INFO(80, "Display Mode Set\n" );
      }
    }
    
  } else {
    // win98,win2k,winxp
    if(ChangeDisplaySettingsEx(curDeviceName, &devMode, NULL, CDS_FULLSCREEN, NULL) == DISP_CHANGE_SUCCESSFUL)
    {
      /*style = GetWindowLong(hwndApp, GWL_STYLE);
      style &= ~(WS_MAXIMIZE|WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX);
      style |= WS_POPUP;
      SetWindowLong(hwndApp, GWL_STYLE, style);*/

      is_fullscreen = FXTRUE;
      
      GDBG_INFO(80, "Display Mode Set\n" );
    } else {
      GDBG_INFO(80, "Setting display mode at default refresh\n");
      
      devMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT ;
      
      if(ChangeDisplaySettingsEx(curDeviceName, &devMode, NULL, CDS_FULLSCREEN, NULL) == DISP_CHANGE_SUCCESSFUL)
      {
        /*style = GetWindowLong(hwndApp, GWL_STYLE);
        style &= ~(WS_MAXIMIZE|WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX);
        style |= WS_POPUP;
        SetWindowLong(hwndApp, GWL_STYLE, style);*/

        is_fullscreen = FXTRUE;
        
        GDBG_INFO(80, "Display Mode Set\n" );
      }
    }
  }
#endif /* IS_ALT_TAB */

#if (WINXP_FASTER_ALT_TAB_FIX || WINXP_ALT_TAB_FIX)
  ddsd.dwSize = sizeof(ddsd);
  ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
  ddsd.dwWidth = xRes;
  ddsd.dwHeight = yRes;
  ddsd.dwRefreshRate = refresh;
  
  /* Set these for use by the callback */
  emcData.xRes = xRes;
  emcData.yRes = yRes;
  emcData.Refresh = refresh;
  emcData.modeOK = FXFALSE;
  emcData.bpp = bpp;
  
  hResult = IDirectDraw2_EnumDisplayModes(lpDD, 0, &ddsd, 
                                          (LPVOID) &emcData, enumModesCallback);
  
  if (hResult != DD_OK) {
    GDBG_INFO(80, "Warning:  EnumDisplayModes failed due to: \n");
    msgEnumDisplayModes(hResult);
  }      
  
  GDBG_INFO(80, "Setting Display Mode!\n");
  
  if (emcData.modeOK) {
    GDBG_INFO(80, "Found mode %dx%d.  Attempting at %dHz\n", xRes,
              yRes, refresh);
    hResult = IDirectDraw2_SetDisplayMode( lpDD, xRes, yRes, bpp, refresh, 0);
    if (hResult != DD_OK) {
      GDBG_INFO(80, "Couldn't set display mode\n" );
      msgModeSetFailure(hResult);
      GDBG_INFO(80, "Retrying at default resolution\n");
      hResult = IDirectDraw2_SetDisplayMode( lpDD, xRes, yRes, bpp, 0, 0 );
      
      if (hResult != DD_OK) {
        GDBG_INFO(80, "Setting video mode %dx%d@default refresh failed!\n",
                  xRes, yRes);
        msgModeSetFailure(hResult);        
        
        if (!_set_vidmode_relaxed) {
          GDBG_INFO(80, "Returning FXFALSE\n");
          return FXFALSE;
        } else {
          GDBG_INFO(80, "Continuing operation due to relaxation condition\n");
        }
      }
    }
    GDBG_INFO(80, "Display Mode Set\n" );
  } else {
    
    GDBG_INFO(80, "Did not find mode %dx%d@any refresh\n", xRes, yRes);
    GDBG_INFO(80, "Setting video mode %dx%d@default refresh\n", xRes, yRes);

#if 0 /* This only works for single display systems */
    /*
     ** This is really a hack here. We use the registry key entry to work
     ** around DDraw refresh rate problem.
     */
    {
      static char *regPath = NULL;
      char mpath[256], mpath_8[256], tmp[32];
      FxU32 reg_flag = 1;
      if (regPath == NULL) {
        regPath = getModesRegPath();
      }
      if ((regPath) && (bpp == 8)) {
        HKEY hKey, hKey8;
        FxU32 retVal;
        DWORD type, szData, type8, szData8;
        static char strval[255], strval8[255];
        
        /* get the current bpp path */
        szData = sizeof(strval);
        strcpy(mpath, regPath);
        sprintf(tmp, "%d\\%d,%d",devMode.dmBitsPerPel,xRes,yRes);
        strcat(mpath, tmp);
        /* get the 8 bpp path */
        szData8 = sizeof(strval8);
        strcpy(mpath_8, regPath);
        sprintf(tmp, "8\\%d,%d",xRes,yRes);
        strcat(mpath_8, tmp);
        /*
         ** Get the original refresh rate from registry key
         */
        if (RegOpenKey(HKEY_LOCAL_MACHINE, mpath, &hKey) == ERROR_SUCCESS)  {
          if (RegQueryValueEx(hKey, "RefreshRate", NULL, &type, strval,
                              &szData) == ERROR_SUCCESS)  {
            /*
             ** overwrite 8 bit registry with glide refresh rate and set
             ** the display mode. this is a workaround of the DDraw
             ** refresh rate problem
             */
            if (RegOpenKey(HKEY_LOCAL_MACHINE, mpath_8, &hKey8)
                == ERROR_SUCCESS) {
              if (RegQueryValueEx(hKey8, "RefreshRate", NULL, &type8,
                                  strval8, &szData8) == ERROR_SUCCESS) {
                retVal = 1;
              } else {
                retVal = 0;
              }
              RegSetValueEx(hKey8, "RefreshRate", 0, REG_SZ, strval,
                            strlen(strval));
              hResult = IDirectDraw2_SetDisplayMode(lpDD, xRes, yRes,
                                                    bpp, 0, 0);
              reg_flag = 0;
              /*
               ** reset the original refresh rate
               */
              if (retVal) {
                RegSetValueEx(hKey8, "RefreshRate", 0,
                              REG_SZ, strval8, strlen(strval8));
              }
              RegCloseKey(hKey8);
              /*
               ** delete the key if the refresh rate isn't there in the
               ** first place
               */
              if (!retVal) {
                RegDeleteValue(hKey8, "RefreshRate" );
                RegDeleteKey(hKey8, "RefreshRate" );
              }
              RegCloseKey(hKey);
            }
          } else {
            /*
             ** We did not find the "RefreshRate" key in the
             ** devMode.dmBitsPerPel node so let's see if it exists for
             ** this resolution under the 8 bpp node:
             */
            if (RegOpenKey(HKEY_LOCAL_MACHINE, mpath_8, &hKey8)
                == ERROR_SUCCESS) {
              if (RegQueryValueEx(hKey8, "RefreshRate", NULL, &type8, strval8,
                                  &szData8) == ERROR_SUCCESS) {
                retVal = 1;
              } else {
                retVal = 0;
              }
              RegSetValueEx(hKey8, "RefreshRate", 0, REG_SZ, strval,
                            strlen(strval));
              hResult = IDirectDraw2_SetDisplayMode(lpDD, xRes, yRes,
                                                    bpp, 0, 0);
              reg_flag = 0;
              /*
               ** reset the original refresh rate
               */
              if (retVal) {
                RegSetValueEx(hKey8, "RefreshRate", 0,
                              REG_SZ, strval8, strlen(strval8));
              }
              RegCloseKey(hKey8);
              /*
               ** delete the key if the refresh rate isn't there in the
               ** first place
               */
              if (!retVal) {
                RegDeleteValue(hKey8, "RefreshRate" );
                RegDeleteKey(hKey8, "RefreshRate" );
              }
              RegCloseKey(hKey);
            }
          }
        }
      }
      if (reg_flag)
        hResult = IDirectDraw2_SetDisplayMode( lpDD, xRes, yRes, bpp, 0, 0 );
    }
#else
    hResult = IDirectDraw2_SetDisplayMode( lpDD, xRes, yRes, bpp, 0, 0 );
#endif
    
    if (hResult != DD_OK) {
      GDBG_INFO(80, "Failed!\n",
                xRes, yRes);
      msgModeSetFailure(hResult);
      if (!_set_vidmode_relaxed) {
        GDBG_INFO(80, "Returning FXFALSE\n");
        return FXFALSE;
      } else {
        GDBG_INFO(80, "Continuing operation due to relaxation condition\n");
      }
    }
  }
#endif /* WINXP_FASTER_ALT_TAB_FIX || WINXP_ALT_TAB_FIX */
  
  return FXTRUE;
  
} /* setVideoMode */

void 
resetVideo( void ) 
{
#define FN_NAME "resetVideo"

#ifdef IS_ALT_TAB
  OSVERSIONINFO ovi;
#endif
  
  GDBG_INFO(80, "%s:  called!\n", FN_NAME);
  
  if ( lpDD != NULL ) {
    GDBG_INFO(80, "%s:  has lpDD!\n", FN_NAME);

#if (WINXP_FASTER_ALT_TAB_FIX || WINXP_ALT_TAB_FIX)
    IDirectDraw2_RestoreDisplayMode( lpDD );
    GDBG_INFO(80, "%s:  Restored Display Mode!\n", FN_NAME);
#endif
    
    IDirectDraw2_SetCooperativeLevel( lpDD, hwndApp, DDSCL_NORMAL );
    GDBG_INFO(80, "%s:  Set cooperative level!\n", FN_NAME);
    
    IDirectDraw2_Release( lpDD );
    GDBG_INFO(80, "%s:  Released lpDD!\n", FN_NAME);

#if WINXP_ALT_TAB_FIX
    if ( lpDD4 ) {
      IDirectDraw_Release( lpDD4 );
      GDBG_INFO(80, "%s:  Released lpDD4!\n", FN_NAME);
    }
#endif

    if ( lpDD1 != NULL ) {
      GDBG_INFO(80, "%s:  has lpDD1!\n", FN_NAME);
      IDirectDraw_Release( lpDD1 );
      GDBG_INFO(80, "%s:  Released lpDD1!\n", FN_NAME);
    }
  }
  
  lpDD = NULL;
  lpDD1 = NULL;
#if WINXP_ALT_TAB_FIX
  lpDD4 = NULL;
#endif

#ifdef IS_ALT_TAB
  /* get os version */
  ovi.dwOSVersionInfoSize = sizeof ( ovi );
  GetVersionEx ( &ovi );

  //ChangeDisplaySettings(NULL, 0);
  if(((ovi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) && (ovi.dwMinorVersion < 10)) ||
     ((ovi.dwPlatformId == VER_PLATFORM_WIN32_NT) && (ovi.dwMajorVersion < 5)))
  {
    // win95,nt4
    /* KoolSmoky - ripped from quake3 */
    ChangeDisplaySettings(&curDevMode, CDS_RESET);
  } else {
    // win98,win2k,winxp
    /* KoolSmoky - ripped from quake3 */
    ChangeDisplaySettingsEx(curDeviceName, &curDevMode, NULL, CDS_RESET, NULL);
  }
#endif

  return;
#undef FN_NAME
} /* resetVideo */

typedef struct WidthHeight_s {
  FxU32 width; 
  FxU32 height;
} WidthHeight_t;

static  WidthHeight_t widthHeightByResolution[] = {
  {320, 200},                 /* GR_RESOLUTION_320x200 */
  {320, 240},                 /* GR_RESOLUTION_320x240 */
  {400, 256},                 /* GR_RESOLUTION_400x256 */
  {512, 384},                 /* GR_RESOLUTION_512x384 */
  {640, 200},                 /* GR_RESOLUTION_640x200 */
  {640, 350},                 /* GR_RESOLUTION_640x350 */
  {640, 400},                 /* GR_RESOLUTION_640x400 */
  {640, 480},                 /* GR_RESOLUTION_640x480 */
  {800, 600},                 /* GR_RESOLUTION_800x600 */
  {960, 720},                 /* GR_RESOLUTION_960x720 */
  {856, 480},                 /* GR_RESOLUTION_856x480 */
  {512, 256},                 /* GR_RESOLUTION_512x256 */
  {1024, 768},                /* GR_RESOLUTION_1024x768 */
  {1280, 1024},               /* GR_RESOLUTION_1280x1024 */
  {1600, 1200},               /* GR_RESOLUTION_1600x1200 */
  {400, 300},                 /* GR_RESOLUTION_400x300 */
  {1152, 864},                /* GR_RESOLUTION_1152x864 */
  {1280, 960},                /* GR_RESOLUTION_1280x960 */
  {1600, 1024},               /* GR_RESOLUTION_1600x1024 */
  {1792, 1344},               /* GR_RESOLUTION_1792x1344 */
  {1856, 1392},               /* GR_RESOLUTION_1856x1392 */
  {1920, 1440},               /* GR_RESOLUTION_1920x1440 */
  {2048, 1536},               /* GR_RESOLUTION_2048x1536 */
  {2048, 2048}                /* GR_RESOLUTION_2048x2048 */
};

static char *resNames[] = {
  "GR_RESOLUTION_320x200",
  "GR_RESOLUTION_320x240",
  "GR_RESOLUTION_400x256",
  "GR_RESOLUTION_512x384",
  "GR_RESOLUTION_640x200",
  "GR_RESOLUTION_640x350",
  "GR_RESOLUTION_640x400",
  "GR_RESOLUTION_640x480",
  "GR_RESOLUTION_800x600",
  "GR_RESOLUTION_960x720",
  "GR_RESOLUTION_856x480",
  "GR_RESOLUTION_512x256",
  "GR_RESOLUTION_1024x768",
  "GR_RESOLUTION_1280x1024",
  "GR_RESOLUTION_1600x1200",
  "GR_RESOLUTION_400x300",
  "GR_RESOLUTION_1152x864",
  "GR_RESOLUTION_1280x960",
  "GR_RESOLUTION_1600x1024",
  "GR_RESOLUTION_1792x1344",
  "GR_RESOLUTION_1856x1392",
  "GR_RESOLUTION_1920x1440",
  "GR_RESOLUTION_2048x1536",
  "GR_RESOLUTION_2048x2048"
};

static FxU32 refresh[] = {
  60,  //"GR_REFRESH_60Hz",
  70,  //"GR_REFRESH_70Hz",
  72,  //"GR_REFRESH_72Hz",
  75,  //"GR_REFRESH_75Hz",
  80,  //"GR_REFRESH_80Hz",
  90,  //"GR_REFRESH_90Hz",
  100, //"GR_REFRESH_100Hz",
  85,  //"GR_REFRESH_85Hz",
  120  //"GR_REFRESH_120Hz"
};


#ifdef IS_ALT_TAB
/*
**  checkResolution - check to see if a given resolution is available
**
**  This routine may only be called once and IT MUST BE CALLED FROM
**  hwcInit().
**  Any other useage will be very very bad, as it will mess with the
**  app's DirectDraw context.
*/
/*  KoolSmoky - we won't mess with ddraw context anymore, we will be
**  very very good.
*/
FxBool
checkResolutions( FxBool *supportedByResolution, FxU32 stride, void *hmon, int h3pixelSize, char *devicename) 
{
#define FN_NAME "checkResolution"
  DEVMODE chkDevMode;
  DWORD chkRes, chkRefresh;
  OSVERSIONINFO ovi;

  GDBG_INFO(80, "%s:  called!\n", FN_NAME);

  /* get os version */
  ovi.dwOSVersionInfoSize = sizeof ( ovi );
  GetVersionEx ( &ovi );

  chkDevMode.dmSize   = sizeof(DEVMODE);
  chkDevMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;

  for(chkRes = 0x0;
      chkRes < (sizeof(widthHeightByResolution) / sizeof(WidthHeight_t));
      chkRes++) {
    for (chkRefresh = 0 ;
	 chkRefresh < (sizeof(refresh) / sizeof(FxU32)) ;
         chkRefresh++) {
      chkDevMode.dmPelsWidth         = widthHeightByResolution[chkRes].width;
      chkDevMode.dmPelsHeight        = widthHeightByResolution[chkRes].height;

      if(h3pixelSize == 4)
        chkDevMode.dmBitsPerPel      = 32UL;
      else
        chkDevMode.dmBitsPerPel      = 16UL;
      
      chkDevMode.dmDisplayFrequency  = refresh[chkRefresh];

      //ChangeDisplaySettings(NULL, 0);
      if(((ovi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) && (ovi.dwMinorVersion < 10)) ||
         ((ovi.dwPlatformId == VER_PLATFORM_WIN32_NT) && (ovi.dwMajorVersion < 5)))
      {
        // win95,nt4
        if(ChangeDisplaySettings(&chkDevMode, CDS_TEST) == DISP_CHANGE_SUCCESSFUL)
          supportedByResolution[chkRes*stride + chkRefresh] = FXTRUE;
        else
          supportedByResolution[chkRes*stride + chkRefresh] = FXFALSE;
      } else {
        // win98,win2k,winxp
        if(ChangeDisplaySettingsEx(devicename, &chkDevMode, NULL, CDS_TEST, NULL) == DISP_CHANGE_SUCCESSFUL)
          supportedByResolution[chkRes*stride + chkRefresh] = FXTRUE;
        else
          supportedByResolution[chkRes*stride + chkRefresh] = FXFALSE;
      }
      
    }
  }
  
  return FXTRUE;
#undef FN_NAME
} /* checkResolutions */
#endif /* IS_ALT_TAB */

#if (WINXP_FASTER_ALT_TAB_FIX || WINXP_ALT_TAB_FIX)

static FxU32 resStride = 0;

static HRESULT WINAPI
checkResEMCallback(LPDDSURFACEDESC surfaceDesc, LPVOID lpContext)
{
#define FN_NAME "checkResEMCallback"
  FxBool *resList = (FxBool *) lpContext;
  int res, ref;
  
  GDBG_INFO(80, FN_NAME ":\n");
  GDBG_INFO(80, FN_NAME "\tsurfaceDesc->dwWidth == %d\n",surfaceDesc->dwWidth);
  GDBG_INFO(80, FN_NAME 
            "\tsurfaceDesc->dwHeight == %d\n",surfaceDesc->dwHeight);
  GDBG_INFO(80, FN_NAME 
            "\tsurfaceDesc->dwRefreshRate == %d\n",surfaceDesc->dwRefreshRate);
  GDBG_INFO(80, FN_NAME 
            "\tsurfaceDesc->ddpfPixelFormat.dwRGBBitCount == %d\n", 
            surfaceDesc->ddpfPixelFormat.dwRGBBitCount);
  
  for (
    res = 0x0;
    res < (sizeof(widthHeightByResolution) / sizeof(WidthHeight_t));
    res++) {
    if (
      (surfaceDesc->dwWidth == widthHeightByResolution[res].width) &&
      (surfaceDesc->dwHeight == widthHeightByResolution[res].height)
      ) {
      for (ref = 0; ref < (sizeof(refresh) / sizeof(FxU32)); ref++) {
        if (surfaceDesc->dwRefreshRate == 0 || surfaceDesc->dwRefreshRate == refresh[ref])
          resList[res*resStride+ref] = FXTRUE;
      }
      return DDENUMRET_OK;
    }
  }
  
  return DDENUMRET_OK;
#undef FN_NAME
} /* checkResEMCallback */

/*
**  checkResolution - check to see if a given resolution is available
**
**  This routine may only be called once and IT MUST BE CALLED FROM
**  hwcInit().
**  Any other useage will be very very bad, as it will mess with the
**  app's DirectDraw context.
*/
FxBool
checkResolutions(FxBool *supportedByResolution, FxU32 stride, void *hmon) 
{
#define FN_NAME "checkResolution"  
  LPGUID          ddGuid = NULL;
  HMODULE         ddraw = NULL; 
  HRESULT hResult;
  
  hwndApp = GetActiveWindow();
  
  if ( hwndApp == NULL ) {
    GDBG_INFO( 80, "Couldn't get a valid window handle\n" );
  }
  
  resStride = stride;
  
  ddGuid = NULL;
  ddraw = GetModuleHandle( "ddraw.dll" );      
  if ( ddraw ) {
    LPDIRECTDRAWENUMERATEEXA ddEnumEx;
    ddEnumEx = (void*)GetProcAddress( ddraw, "DirectDrawEnumerateExA" );
    if ( ddEnumEx ) {
      DWORD   data[2];
      data[0] = (DWORD)hmon;
      data[1] = 0;
      ddEnumEx( ddEnumCbEx, data, DDENUM_ATTACHEDSECONDARYDEVICES );
      if ( data[1] ) {
        ddGuid = (LPGUID)data[1];
      }
    }
  }
  
  //checkSpecialList();
  if (lpDD == NULL) {
    /* only create directdraw object once */
    if ( DirectDrawCreate( ddGuid, &lpDD1, NULL ) != DD_OK) {
      GDBG_INFO(80, "DDraw Obj Create Failed!\n");
    }
    else GDBG_INFO(80, "DDraw Obj created!\n");
    if ( IDirectDraw_QueryInterface( lpDD1, &IID_IDirectDraw2, 
                                     (LPVOID*)&lpDD ) != DD_OK ) {
      IDirectDraw_Release( lpDD1 );
#if WINXP_ALT_TAB_FIX
      lpDD4 = NULL;
#endif
      lpDD1 = NULL;
      lpDD  = NULL;
      GDBG_INFO(80, "DDraw Obj Create Failed!\n");
      return FXFALSE;            
    } 
    else GDBG_INFO(80, "DDraw2 Obj created!\n");
#if WINXP_ALT_TAB_FIX
    if ( IDirectDraw_QueryInterface( lpDD1, &IID_IDirectDraw4, 
                                     (LPVOID*)&lpDD4 ) != DD_OK ) {
      IDirectDraw_Release( lpDD );
      IDirectDraw_Release( lpDD1 );
      lpDD4 = NULL;
      lpDD1 = NULL;
      lpDD  = NULL;
      GDBG_INFO(80, "DDraw Obj Create Failed!\n");
      return FXFALSE;            
    } 
    else GDBG_INFO(80, "DDraw4 Obj created!\n");
#endif
  }
  
  hResult = IDirectDraw2_EnumDisplayModes(lpDD, 0, NULL,
                                          (LPVOID) supportedByResolution,
                                          checkResEMCallback);
  
  resetVideo();
  return FXTRUE;
#undef FN_NAME
} /* checkResolutions */

#endif /* WINXP_FASTER_ALT_TAB_FIX || WINXP_ALT_TAB_FIX */


void EnableOpenGL( char *regpath )
{
  GDBG_INFO(80, "EnableOpenGL: called!\n");
  
  opengl_regpath = regpath;
  
#ifdef IS_ALT_TAB
  is_opengl = FXTRUE;
#endif
  
  GDBG_INFO(80, "opengl regpath: %s\n", opengl_regpath);
  
} /* EnableOpenGL */