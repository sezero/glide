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
*/

#include <stdio.h>
#include <3dfx.h>
#include <gdebug.h>
#define WIN32_LEAN_AND_MEAN 
#include <windows.h> 
#include <ddraw.h> 
#include <sst1vid.h>
#include "qmodes.h"
#define IS_32
#define Not_VxD
#include <minivdd.h>
#include <vmm.h>
#include <configmg.h>

/* XXXTACOHACK -- The required header file isn't shipping yet - decls will be removed at some 
   future time */

#ifndef HMONITOR_DECLARED // AJB- Make def compatible w/ vc6 headers
typedef void *HMONITOR;
#define HMONITOR_DECLARED
#endif
typedef BOOL (FAR PASCAL * LPDDENUMCALLBACKEXA)(GUID FAR *, LPSTR, LPSTR, LPVOID, HMONITOR); 
extern HRESULT WINAPI DirectDrawEnumerateExA( LPDDENUMCALLBACKEXA lpCallback, 
                                              LPVOID lpContext, DWORD dwFlags); 
typedef HRESULT (WINAPI * LPDIRECTDRAWENUMERATEEXA)( LPDDENUMCALLBACKEXA lpCallback, 
                                                     LPVOID lpContext, 
                                                     DWORD dwFlags); 
#define DDENUM_ATTACHEDSECONDARYDEVICES     0x00000001L 
/* XXXTACOHACK -- The required header file isn't shipping yet - decls will be removed at some 
   future time */

static GUID          fooGuid;
static LPDIRECTDRAW  lpDD1;
static LPDIRECTDRAW2 lpDD;
static HWND          hwndApp;
static DEVMODE       theDM ;
static WNDPROC       oldWindowProc ;

/* Windows */
#define SEPARATOR '\\'
/* UNIX */
#define SEPARATOR2 '/'

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


#define MAX_SPECIAL_LIST 8
static int _special_list[MAX_SPECIAL_LIST][50] = 
{{13, 7, 11, -8, -9, 3, 4, 12},  /* nhl98dem */
 {45, 39, 43, -8, -9, 35, 36, 44}, /* lower */
  {13, 7, 11, -8, -9}, /*NHL98 */
 {45, 39, 43, -8, -9}, /* lower */
 {30, 6, 11, 8, 3, 4}, /* _GLIDE excalibur*/
 {30, 38, 43, 40, 35, 36}, /*lower */
 {5, 11, 24}, /* fly */ 
 {37, 43, 56}, /*lower */
};

static int
_eadecrypt(char *name)
{
  int i, j;

  for (j = 0; j < MAX_SPECIAL_LIST; j++){
    int success = 1;
    for (i = 0; i < (int)strlen(name)-4; i++){
      if (name[i] != _special_list[j][i] + 65){
        success = 0;
        break;
      }
    }
    if (success) {
      return 1;
    }
  }
  return 0;
}

static int _set_exclusive_relaxed;
static int _set_vidmode_relaxed;

void
checkSpecialList()
{
#if defined( __WATCOMC__ )
  /* Nothing for DOS */
#else 
/* windows only */
  HMODULE module;
  module = GetModuleHandle(NULL);
  _set_exclusive_relaxed = 0;
  if (module) {
    char pathname[256];
    char *fname;
    GDBG_INFO(80,"Got module handle\n");
    strcpy(pathname, "deadbeef");
    GetModuleFileName(module, pathname, 256);
    fname = _parseFilename(pathname);
    if (fname != NULL){
      GDBG_INFO(80,"module name %s\n", fname);
      if (_eadecrypt(fname)){
        GDBG_INFO(80,"Special found\n");
        _set_exclusive_relaxed = 1;
      }
    }
  }
#endif
}

static BOOL FAR PASCAL
ddEnumCbEx( GUID FAR *guid, LPSTR desc, LPSTR name, LPVOID ctx, HMONITOR hmon ) 
{
  DWORD    *data  = (DWORD*)ctx;
  HMONITOR target = (HMONITOR)data[0];
  BOOL     rv     = DDENUMRET_OK;
  
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
  return rv;
}

typedef struct _emcStruct {
  FxU32 xRes, yRes, Refresh;
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
      (surfaceDesc->ddpfPixelFormat.dwRGBBitCount == 16) &&
      (surfaceDesc->dwRefreshRate == emcData->Refresh)) {
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
    GDBG_INFO(80, "OS == WNT\n");
    /* It is hardcoded on NT via Display Control code. see:
     * $/devel/swtools/bansheecp2 */
    retVal = "SYSTEM\\CurrentControlSet\\Services\\3Dfx\\Device0\\modes\\";
  } else {
    QDEVNODE QDevNode;
    QIN Qin;
    int status;

    GDBG_INFO(80, "OS == W9X\n");

    Qin.dwSubFunc = QUERYDEVNODE;
    {
      HDC curDC = GetDC(NULL);

      status = ExtEscape ( curDC, QUERYESCMODE, 
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

  return retVal;
} /* getRegPath */

static char *
getDefaultRegPath() 
{
  char *retVal = NULL;
  OSVERSIONINFO ovi;
  
  ovi.dwOSVersionInfoSize = sizeof ( ovi );
  GetVersionEx ( &ovi );
  if (ovi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
    GDBG_INFO(80, "OS == WNT\n");
    /* It is hardcoded on NT via Display Control code. see:
     * $/devel/swtools/bansheecp2 */
    retVal = "SYSTEM\\CurrentControlSet\\Services\\3Dfx\\Device0\\modes\\";
  } else {
    QDEVNODE QDevNode;
    QIN Qin;
    int status;

    GDBG_INFO(80, "OS == W9X\n");

    Qin.dwSubFunc = QUERYDEVNODE;
    {
      HDC curDC = GetDC(NULL);

      status = ExtEscape ( curDC, QUERYESCMODE, 
                           sizeof(Qin), (LPCSTR)&Qin, 
                           sizeof(QDevNode), (LPSTR)&QDevNode );
      ReleaseDC(NULL, curDC);
    }

    if ( status > 0 ) {
      static char regPath[255];

      CM_Get_DevNode_Key( QDevNode.dwDevNode, NULL, 
                          &regPath, sizeof(regPath), 
                          CM_REGISTRY_SOFTWARE );
      strcat(regPath, "\\Default\\");

      retVal = regPath;
    }
  }

  return retVal;
} /* getDefaultRegPath */

static FxBool
getRefreshRateAndSetMode (int xRes, int yRes, int bpp, DEVMODE devMode)
{
  /* MDM-Richardson
   * Sort of new way for refresh rate handling 
   *
   * Get the Default RefreshRate
   * Get the User Selected RefreshRate for the given mode
   * Get the 8 BPP RefreshRate for the given mode
   * Delete the 8 BPP RefreshRate value so the Defaul key will be taken
   * If we have a User Selected RefreshRate, then lets use it
   * If we don't have a User Selected RefreshRate, then lets use Default
   * Set the Mode
   * Restore the Default RefreshRate value
   * Restore the 8 BPP RefreshRate value if there was one
   *
   * Note: Separated the Registry accesses to prevent registry overload
   *       or registry corruption.  Only one HKEY is open at any given 
   *       time.
   */
  static char *RegPath = NULL;
  static char *DefaultRegPath = NULL;
  HRESULT hResult;

  /* Get the devnode mode registry path */
  if (RegPath == NULL) {
    RegPath = getModesRegPath();
  }

  /* Get the devnode default registry path */
  if (DefaultRegPath == NULL) {
    DefaultRegPath = getDefaultRegPath();
  }

  /* If we have a RegPath and a DefaultRegPath and our bpp is 8, continue */
  if ((RegPath) && (DefaultRegPath) && (bpp == 8)) {
    HKEY hKey, hKey8bpp, hKeyDefault;
    static char sz8BppRegPath [255], szCurBppRegPath[255];
    static char sz8BppRefresh [10], szCurBppRefresh [10];
    static char szDefaultRefresh [10], szNewDefault [10];
    DWORD dw8BppRefreshSize, dwCurBppRefreshSize;
    DWORD dwDefaultRefreshSize;
    DWORD type;

    /* Setup all our variables */
    dw8BppRefreshSize = sizeof(sz8BppRefresh);
    dwCurBppRefreshSize = sizeof(szCurBppRefresh);
    dwDefaultRefreshSize = sizeof(szDefaultRefresh);

    sprintf(szCurBppRegPath, "%s%d\\%d,%d", RegPath, 
            devMode.dmBitsPerPel, xRes, yRes);

    sprintf(sz8BppRegPath, "%s%d\\%d,%d", RegPath, 
            bpp, xRes, yRes);

    /* Just as a precautionary measure, clear the memory */
    memset(szCurBppRefresh, '\0', sizeof(szCurBppRefresh));
    memset(sz8BppRefresh, '\0', sizeof(sz8BppRefresh));
    memset(szDefaultRefresh, '\0', sizeof(szDefaultRefresh));

    /* Open the Default Reg Key and query for the RefreshRate */
    if (RegOpenKey(HKEY_LOCAL_MACHINE, DefaultRegPath, &hKeyDefault) 
                   == ERROR_SUCCESS) {
      if (RegQueryValueEx(hKeyDefault, "RefreshRate", 0, &type, 
                          szDefaultRefresh, &dwDefaultRefreshSize)
                          == ERROR_SUCCESS) {
        /* Do nothing, we have the string now */
      }
      else {
        dwDefaultRefreshSize = 0;
      }
      RegCloseKey(hKeyDefault);
    }

    /* Open the current bpp registry key and query for its RefreshRate */
    if (RegOpenKey(HKEY_LOCAL_MACHINE, szCurBppRegPath, &hKey) 
                   == ERROR_SUCCESS) {
      if (RegQueryValueEx(hKey, "RefreshRate", NULL, &type, 
                          szCurBppRefresh, &dwCurBppRefreshSize) 
                          == ERROR_SUCCESS)  {
        /* Do nothing, we have the string now */
      }
      else {
        dwCurBppRefreshSize = 0;
      }
      RegCloseKey(hKey);
    }

    /* Open the 8bpp registry key, save the RefreshRate and delete 
     * the value so the Default refresh rate will take priority
     */
    if (RegOpenKey(HKEY_LOCAL_MACHINE, sz8BppRegPath, &hKey8bpp )
                   == ERROR_SUCCESS) {
      if (RegQueryValueEx(hKey8bpp, "RefreshRate", NULL, &type, 
                          sz8BppRefresh, &dw8BppRefreshSize) 
                          == ERROR_SUCCESS)  {
        /* Delete the Refresh Rate in 8bpp so DDraw will use the 
         * default setting
         */
        RegDeleteValue(hKey8bpp, "RefreshRate" );
        RegDeleteKey(hKey8bpp, "RefreshRate" );
      }
      else {
        dw8BppRefreshSize = 0;
      }
      RegCloseKey(hKey8bpp);
    }

    /* If we have an existing refresh rate for this mode
     * set it
     */
    if (dwCurBppRefreshSize > 0) {
      sprintf(szNewDefault, "%s", szCurBppRefresh );
    }
    /* Else, lets use the default refresh rate */
    else if (dwDefaultRefreshSize >0) {
      sprintf(szNewDefault, "%s", szDefaultRefresh );
    }
    /* Else, end all be all of failsafe, adapter default */
    else {
      sprintf(szNewDefault, "0");
    }

    /* Open the Default registry path and write the new default
     * RefreshRate
     */
    if (RegOpenKey(HKEY_LOCAL_MACHINE, DefaultRegPath, &hKeyDefault) 
                   == ERROR_SUCCESS) {
      RegSetValueEx(hKeyDefault, "RefreshRate", 0, REG_SZ,
                    szNewDefault, strlen(szNewDefault));
      RegCloseKey(hKeyDefault);
    }
    
    /* Tell DirectDraw to set the mode */
    hResult = IDirectDraw2_SetDisplayMode(lpDD, xRes, yRes,
                                          bpp, 0, 0);

    /* Restore the old Default RefreshRate */
    if (RegOpenKey(HKEY_LOCAL_MACHINE, DefaultRegPath, &hKeyDefault) 
                   == ERROR_SUCCESS) {
      RegSetValueEx(hKeyDefault, "RefreshRate", 0, REG_SZ,
                    szDefaultRefresh, strlen(szDefaultRefresh));
      RegCloseKey(hKeyDefault);
    }

    /* If we did have an 8bpp RefreshRate value, restore it */
    if (dw8BppRefreshSize > 0) {
      if (RegOpenKey(HKEY_LOCAL_MACHINE, sz8BppRegPath, &hKey8bpp )
                     == ERROR_SUCCESS) {
        RegSetValueEx(hKey8bpp, "RefreshRate", 0, REG_SZ,
                      sz8BppRefresh, strlen(sz8BppRefresh));
        RegCloseKey(hKey8bpp);
      }
    }

    /* After cleaning up our registry tampering, lets make sure 
     * we are okay
     */
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
   else
  {
    /*
     * AJB- Make this hodaddy pay attention to return values
     */
    hResult = IDirectDraw2_SetDisplayMode(lpDD, xRes, yRes, bpp, 0, 0);
    if (hResult != DD_OK) 
    {
      GDBG_INFO(80, "Failed!\n", xRes, yRes) ;
      msgModeSetFailure(hResult) ;
      if (!_set_vidmode_relaxed)
      {
        GDBG_INFO(80, "Returning FXFALSE\n");
        return FXFALSE;
      }
       else
      {
        GDBG_INFO(80, "Continuing operation due to relaxation condition\n");
      }
    }
  }
  
  return FXTRUE;
} /* getRefreshRateAndSetMode */
#endif



/*
 * To answer your question:
 *
 * This function is patched into the windows message handler of the
 * calling app when the app requests a fullscreen mode that is the
 * same as the desktop mode.  When this handler gets called with
 * WM_ACTIVATEAPP (FALSE) then our window is being alt-tab'd away
 * from and we need to bail out & make things safe for the display
 * driver.  The easiest way to do this is to call our own resetVideo
 * function.  The resetVideo function will give up its DDraw exclusive
 * lock, close DDraw, ChangeDisplaySettings back to the original desktop
 * mode and, most importantly, unhook this message handler from the call
 * chain.
 */
void resetVideo(void) ;

LRESULT CALLBACK MyWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_ACTIVATEAPP:
      if (wParam == 0)
        resetVideo() ;
      break ;
    default:
      break ;
  } ;

  return CallWindowProc(oldWindowProc, hwnd, uMsg, wParam, lParam) ;
}

FxBool 
setVideoMode( HWND hwnd, int xRes, int yRes, int pixelSize, int refresh, void *hmon ) 
{
  LPGUID          ddGuid = NULL;
  HMODULE         ddraw = NULL; 
  DDSURFACEDESC   ddsd;
  HRESULT hResult;
  DEVMODE devMode;
  FxU32 bpp = pixelSize << 3;
  EMCData emcData;               /* Enum Modes Callbac Data */

  GDBG_INFO( 80, "setVideoMode sees hwnd %x\n", hwnd);
  hwndApp = ( hwnd == NULL ) ? GetActiveWindow() : hwnd;
  

  if ( hwndApp == NULL ) {
    GDBG_INFO( 80, "Couldn't get a valid window handle\n" );
  }
  
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
  

  /*
  **  Oh, this is lovely.  What we have here is a failure to
  **  communicate.  If the current mode is exactly the same as the
  **  mode in which Glide wishes to run, then the DirecDraw driver
  **  doesn't get called when we restore the video (or on an alt-tab)
  **  this would be fine, except Glide has its way with the video
  **  overlay registers.  So, since Glide *never* wants 8 bits per
  **  pixel, we just force it here, and our DirectDraw driver always
  **  be called when Glide gives up video exclusivity. -CHD
  */
  /* Lovely my foot. What about when we run from an 8bpp desktop?
   * Let's only be stupid when we absolutely have to be. -AJB
   */
  /* The latest stupidity: When the desktop is in the same video mode
   * as the app has requested we can infer that an alt-tab is happening
   * by patching the message handler for the app and looking for the
   * WM_ACTIVATEAPP message with a FALSE parameter (which is Windows-ese
   * for "you are being alt-tab'd away from).  This sounds scary but
   * it is actually exactly what goes on under the hood of direct draw.
   */
  {
    HDC hdc = GetDC(NULL) ;

    oldWindowProc = NULL ;

    if ((xRes == GetSystemMetrics(SM_CXSCREEN)) &&
        (yRes == GetSystemMetrics(SM_CYSCREEN)))
    {
      if (bpp == (DWORD)GetDeviceCaps(hdc, BITSPIXEL))
      {
        oldWindowProc = (WNDPROC)GetWindowLong(hwndApp, GWL_WNDPROC) ;
        SetWindowLong(hwndApp, GWL_WNDPROC, (DWORD)&MyWindowProc) ;
      }
    }
    ReleaseDC(NULL, hdc) ;

    EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &theDM) ;

    /* EnumDisplaySettings doesn't enum current or registry
     * settings in win95. If we get wacky data back, make
     * a struct manually.
     */
    if ((theDM.dmBitsPerPel < 8) ||
        (theDM.dmBitsPerPel > 32)) 
    {
      /*
       * AJB- copy the current device mode so we can restore it upon
       * exit.  It seems that in win95 after an OpenGL app (Quake3)
       * has set the mode w/ ChangeDisplaySettings, the DirectX call
       * to restore the video mode doesn't.  Doing it manually seems
       * to work, tho.
       */
      theDM.dmSize = sizeof(DEVMODE) ;
      theDM.dmPelsWidth = GetSystemMetrics(SM_CXSCREEN) ;
      theDM.dmPelsHeight = GetSystemMetrics(SM_CYSCREEN) ;
      theDM.dmBitsPerPel = (DWORD)GetDeviceCaps(hdc, BITSPIXEL) ;
      theDM.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT ;
    }
  }

  checkSpecialList();
  if (lpDD == NULL) {
    /* only create directdraw object once */
    if ( DirectDrawCreate( ddGuid, &lpDD1, NULL ) != DD_OK) {
      GDBG_INFO(80, "DDraw Obj Create Failed!\n");
    }
    else GDBG_INFO(80, "DDraw Obj created!\n");
    if ( IDirectDraw_QueryInterface( lpDD1, &IID_IDirectDraw2, 
                                    (LPVOID*)&lpDD ) != DD_OK ) {
      IDirectDraw_Release( lpDD1 );
      lpDD1 = NULL;
      lpDD  = NULL;
      GDBG_INFO(80, "DDraw Obj Create Failed!\n");
      return FXFALSE;            
    } 
    else GDBG_INFO(80, "DDraw2 Obj created!\n");
  }
  
  /* Set Exclusive Mode, change resolution,  */
  GDBG_INFO(80, "Setting Full screen exclusive mode!\n");
  GDBG_INFO(80, "Calling IDD2_SetCoop: 0x%x, 0x%x, 0x%x\n", lpDD, hwndApp, 
            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN );
  
  if (
      (hResult =
       IDirectDraw2_SetCooperativeLevel(lpDD, hwndApp, 
                                        DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN ))
      != DD_OK) {
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
      GDBG_INFO(80, "Retrying at default refresh\n");
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

    /*
    ** This is really a hack here. We use the registry key entry to work
    ** around DDraw refresh rate problem.
    */
    {
      EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode) ;
      if (getRefreshRateAndSetMode (xRes, yRes, bpp, devMode) == FXFALSE)
        return FXFALSE;
    }
  }
  
  return FXTRUE;
  
} /* setVideoMode */

void 
resetVideo( void ) 
{
#define FN_NAME "resetVideo"
  if ( lpDD ) {
    IDirectDraw2_RestoreDisplayMode( lpDD );
    GDBG_INFO(80, "%s:  Restored Display Mode!\n", FN_NAME);
    IDirectDraw2_SetCooperativeLevel( lpDD, hwndApp, DDSCL_NORMAL );
    GDBG_INFO(80, "%s:  Set cooperative level!\n", FN_NAME);
    IDirectDraw2_Release( lpDD );
    GDBG_INFO(80, "%s:  Released lpDD!\n", FN_NAME);    

    if ( lpDD1 ) {
      IDirectDraw_Release( lpDD1 );
      GDBG_INFO(80, "%s:  Released lpDD1!\n", FN_NAME);
    }
  }
  lpDD = NULL;
  lpDD1 = NULL;

  /*
   * If we patched the windows message handler, restore it now.
   */
  if (oldWindowProc)
  {
    FxU32 desktopBitsPerPel ;

    SetWindowLong(hwndApp, GWL_WNDPROC, (DWORD)oldWindowProc) ;

    /*
     * This is a little sucky: to revive the display driver
     * we have to make sure that a mode change happens here.
     */
    desktopBitsPerPel = theDM.dmBitsPerPel ;

    if (theDM.dmBitsPerPel == 16)
      theDM.dmBitsPerPel = 32 ;
    else
      theDM.dmBitsPerPel = 16 ;

    ChangeDisplaySettings(&theDM, 0) ;

    theDM.dmBitsPerPel = desktopBitsPerPel ;
  }

  /*
   * Restore the video mode that was set before we did our
   * setVideoMode.  This is especially important in win95
   * where ChangeDisplaySettings behaves differently from
   * win98 and win2k.
   */
  ChangeDisplaySettings(&theDM, 0) ;
  
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


static HRESULT WINAPI
checkResEMCallback(LPDDSURFACEDESC surfaceDesc, LPVOID lpContext)
{
#define FN_NAME "checkResEMCallback"
  FxBool *resList = (FxBool *) lpContext;
  int res;

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
      resList[res] = FXTRUE;
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
checkResolutions(FxBool *supportedByResolution, void *hmon) 
{
#define FN_NAME "checkResolution"  
  LPGUID          ddGuid = NULL;
  HMODULE         ddraw = NULL; 
  HRESULT hResult;
  
  hwndApp = GetActiveWindow();
  
  if ( hwndApp == NULL ) {
    GDBG_INFO( 80, "Couldn't get a valid window handle\n" );
  }
  
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
  
  checkSpecialList();
  if (lpDD == NULL) {
    /* only create directdraw object once */
    if ( DirectDrawCreate( ddGuid, &lpDD1, NULL ) != DD_OK) {
      GDBG_INFO(80, "DDraw Obj Create Failed!\n");
    }
    else GDBG_INFO(80, "DDraw Obj created!\n");
    if ( IDirectDraw_QueryInterface( lpDD1, &IID_IDirectDraw2, 
                                    (LPVOID*)&lpDD ) != DD_OK ) {
      IDirectDraw_Release( lpDD1 );
      lpDD1 = NULL;
      lpDD  = NULL;
      GDBG_INFO(80, "DDraw Obj Create Failed!\n");
      return FXFALSE;            
    } 
    else GDBG_INFO(80, "DDraw2 Obj created!\n");
  }
  
  hResult = IDirectDraw2_EnumDisplayModes(lpDD, 0, NULL,
                                (LPVOID) supportedByResolution,
                                          checkResEMCallback);

  resetVideo();
  return FXTRUE;
#undef FN_NAME
} /* checkResolutions */
