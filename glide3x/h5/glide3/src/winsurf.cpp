//
// This sets up Windowed rendering
//


#include <math.h>

extern "C" {

	// All this is C Linked

	#include <3dfx.h>

	#define FX_DLL_DEFINITION
	#include <fxdll.h>
	#include <glide.h>
	#include "fxglide.h"
	#include "fxcmd.h"
	#include "gsfc.h"
};

/* Begin hacky stuff to enable windowed Glide rendering in Windows */

#include <ddraw.h>

static LPDIRECTDRAW			lpDDraw1 = 0;
static LPDIRECTDRAW2		lpDDraw2 = 0;
static LPDIRECTDRAWSURFACE	lpPrimSurf = 0;
static LPDIRECTDRAWSURFACE	lpColSurf = 0;
static LPDIRECTDRAWSURFACE	lpAuxSurf = 0;
static LPDIRECTDRAWSURFACE	lpTexSurf0 = 0;
static LPDIRECTDRAWSURFACE	lpTexSurf1 = 0;
static HWND					thehWnd = 0;
static DWORD				surfWidth = 0;
static DWORD				surfHeight = 0;
static DWORD				screenWidth = 0;
static DWORD				screenHeight = 0;
static DWORD				bpp = 0;
static BOOL					useDDOverlay = FALSE;
static INT					WindowPosX = 0;
static INT					WindowPosY = 0;

extern "C" void doSplash( void );
extern "C" void _grReleaseWindowSurface(GrContext_t ctx);

#define OVERLAY_COLOUR	0x0D0E0F

/* This will create our DirectDraw surfaces and create the context */
extern "C" GrContext_t _grCreateWindowSurface(FxU32 hWnd,
										GrColorFormat_t		format, 
										GrOriginLocation_t	origin, 
										GrPixelFormat_t		pixelformat,
										int					nAuxBuffer)
{
	/* Disable this for now */
//	return 0;

	/* Allocate a context */
	GrContext_t ctx = grSurfaceCreateContext(GR_SURFACECONTEXT_WINDOWED);
	GrGC *gc = (GrGC *)ctx;

	thehWnd = (HWND) hWnd;

	screenWidth = GetSystemMetrics(SM_CXSCREEN);
	screenHeight = GetSystemMetrics(SM_CYSCREEN);

	RECT clientRect;
	GetClientRect(thehWnd, &clientRect);
	surfWidth = clientRect.right - clientRect.left;
	surfHeight = clientRect.bottom - clientRect.top;

	switch(pixelformat)
	{
		case GR_PIXFMT_ARGB_8888:
		case GR_PIXFMT_AA_2_ARGB_8888:
		case GR_PIXFMT_AA_4_ARGB_8888:
		case GR_PIXFMT_AA_8_ARGB_8888:
			bpp = 32;
			break;

		default:
			if (0 && _GlideRoot.environment.outputBpp == 32)
			{
				gc->state.forced32BPP = 16;
				bpp = 32;
			}
			else
				bpp = 16;
		break;
	}
			bpp = 16;

	/* Now, create DirectDraw */
	if ( DirectDrawCreate( NULL, &lpDDraw1, NULL ) != DD_OK)
	{
		GDBG_INFO(80, "DDraw Obj Create Failed!\n");
		_grReleaseWindowSurface(ctx);
		return 0;
	}
	GDBG_INFO(80, "DDraw Obj created!\n");

	/* Get us a DirectDraw2 interface */
	if ( lpDDraw1->QueryInterface (IID_IDirectDraw2, (LPVOID*)&lpDDraw2 ) != DD_OK ) 
	{
		GDBG_INFO(80, "DDraw2 Obj Create Failed!\n");
		_grReleaseWindowSurface(ctx);
		return 0;
	} 
	GDBG_INFO(80, "DDraw2 Obj created!\n");

	/* Set the Coop level to normal */
	lpDDraw2->SetCooperativeLevel((HWND) hWnd, DDSCL_NORMAL);

	/* Now create us a Primary surface */
	DDSURFACEDESC ddsd;
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd); 
	ddsd.dwFlags = DDSD_CAPS; 
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE|DDSCAPS_VIDEOMEMORY; 
 
	if (lpDDraw2->CreateSurface(&ddsd, &lpPrimSurf, NULL) != DD_OK)
	{
		GDBG_INFO(80, "Primary Surface Create Failed!\n");
		_grReleaseWindowSurface(ctx);
		return 0;
	}

	/* Now create us a Colour surface */
	if (useDDOverlay) do
	{
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd); 
		ddsd.dwFlags = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT; 
		ddsd.ddsCaps.dwCaps = DDSCAPS_OVERLAY|DDSCAPS_VIDEOMEMORY; 
		ddsd.dwWidth = surfWidth;
		ddsd.dwHeight = screenHeight;

		ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
		ddsd.ddpfPixelFormat.dwRGBBitCount = bpp;
		ddsd.ddpfPixelFormat.dwRGBBitCount = 16;
		ddsd.ddpfPixelFormat.dwRBitMask    = 0xF800; 
		ddsd.ddpfPixelFormat.dwGBitMask    = 0x07E0;
		ddsd.ddpfPixelFormat.dwBBitMask    = 0x001F; 

 
		if (lpDDraw2->CreateSurface(&ddsd, &lpColSurf, NULL) != DD_OK)
		{
			GDBG_INFO(80, "Overlay Colour Surface Create Failed!\n");
			lpColSurf = 0;
			break;
		}
		RECT rectOverlay = {0,0,surfWidth,surfHeight};
		RECT rectClient;
		DDOVERLAYFX  ddofx;
	    POINT p = {0, 0};
		GetClientRect(thehWnd, &rectClient);
		ClientToScreen(thehWnd, &p);
		rectClient.bottom += p.y;
		rectClient.top = p.y;
		rectClient.left = p.x;
		rectClient.right += p.x;
		WindowPosX = p.x;
		WindowPosY = p.y;

		memset(&ddofx, 0, sizeof(ddofx));
		ddofx.dwSize = sizeof(ddofx);
		ddofx.dckDestColorkey.dwColorSpaceLowValue = OVERLAY_COLOUR;
		ddofx.dckDestColorkey.dwColorSpaceHighValue = OVERLAY_COLOUR;

		HRESULT ddrval = lpColSurf->UpdateOverlay(&rectOverlay, lpPrimSurf,
												  &rectClient,
												  DDOVER_SHOW|DDOVER_DDFX | DDOVER_KEYDESTOVERRIDE,
												  &ddofx);

		if(ddrval != DD_OK)
		{
			GDBG_INFO(80, "Unable to update overlay!\n");
			lpColSurf->Release();
			lpColSurf = 0;
			break;
		}
		
		/* Now Fill Primary with the col */
		DDBLTFX ddbfx;
		memset(&ddbfx, 0, sizeof(ddbfx));
		ddbfx.dwSize = sizeof(DDBLTFX);
		ddbfx.dwFillColor = OVERLAY_COLOUR;

		lpPrimSurf->Blt(&rectClient, NULL, &rectClient, DDBLT_COLORFILL|DDBLT_WAIT, &ddbfx);

		//lpColSurf->Release();
		//lpColSurf = 0;
	}
	while(0);

	if (!lpColSurf)
	{
		useDDOverlay = FALSE;
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd); 
		ddsd.dwFlags = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT; 
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN|DDSCAPS_VIDEOMEMORY|DDSCAPS_LOCALVIDMEM; 
		ddsd.dwWidth = surfWidth;
		ddsd.dwHeight = screenHeight;
		ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
		ddsd.ddpfPixelFormat.dwRGBBitCount = bpp;
 
		if (lpDDraw2->CreateSurface(&ddsd, &lpColSurf, NULL) != DD_OK)
		{
			GDBG_INFO(80, "Colour Surface Create Failed!\n");
			_grReleaseWindowSurface(ctx);
			return 0;
		}
	}

	/* Now create us a Aux surface */
	if (nAuxBuffer)
	{
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd); 
		ddsd.dwFlags = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT; 
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN|DDSCAPS_VIDEOMEMORY|DDSCAPS_LOCALVIDMEM; 
		ddsd.dwWidth = surfWidth;
		ddsd.dwHeight = screenHeight;
		ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
		ddsd.ddpfPixelFormat.dwRGBBitCount = bpp;
 
		if (lpDDraw2->CreateSurface(&ddsd, &lpAuxSurf, NULL) != DD_OK)
		{
			GDBG_INFO(80, "Aux Surface Create Failed!\n");
			_grReleaseWindowSurface(ctx);
			return 0;
		}
	}

	/* Now create us a Tex surface */
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd); 
	ddsd.dwFlags = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT; 
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN|DDSCAPS_VIDEOMEMORY|DDSCAPS_LOCALVIDMEM; 
	ddsd.dwWidth = 2048;
	ddsd.dwHeight = 2048;
	ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
	ddsd.ddpfPixelFormat.dwRGBBitCount = 16;

	if (lpDDraw2->CreateSurface(&ddsd, &lpTexSurf0, NULL) != DD_OK)
	{
		GDBG_INFO(80, "Tex Surface Create Failed!\n");
		_grReleaseWindowSurface(ctx);
		return 0;
	}
	if (gc->num_tmu == 2)
	{
		if (lpDDraw2->CreateSurface(&ddsd, &lpTexSurf1, NULL) != DD_OK)
		{
			GDBG_INFO(80, "Tex Surface Create Failed!\n");
			_grReleaseWindowSurface(ctx);
			return 0;
		}
	}

	/* Select the Context */
	grSelectContext(ctx);

	/* Set the Render Buffer */
	grSurfaceSetRenderingSurface((GrSurface_t*) lpColSurf, FXFALSE);

	/* Set the Aux Buffer */
	if (nAuxBuffer) grSurfaceSetAuxSurface((GrSurface_t*) lpAuxSurf);

	/* Now create the texture buffer0 and 1 */
	grSurfaceSetTextureSurface(GR_TMU0, (GrSurface_t*) lpTexSurf0);
	if (gc->num_tmu == 2) grSurfaceSetTextureSurface(GR_TMU1, (GrSurface_t*) lpTexSurf1);

	/* Set some 'crap' */
	gc->grHwnd = hWnd;
    gc->grColBuf                 = gc->state.num_buffers = 2;
    gc->grAuxBuf                 = nAuxBuffer;
    gc->grPixelFormat            = (int) pixelformat;
    gc->chipmask                 = SST_CHIP_MASK_ALL_CHIPS;
    gc->state.color_format = format;
	gc->sliCount = 1;
	grSstOrigin(origin);

	/* Clear the buffers */
    grBufferClear( 0x0, 0, 0 );

	return ctx;
}

/* This will blit/flip our surfaces */
extern "C" void _grFlipWindowSurface()
{
	// Blit from col to prim
	if (lpPrimSurf && lpColSurf)
	{
		POINT ptClient;
		ptClient.x = 0;
		ptClient.y = 0;
		ClientToScreen(thehWnd, &ptClient);

		if (!useDDOverlay)
		{
			RECT region;
			region.bottom = min(surfHeight,screenHeight-ptClient.y);
			region.right = min(surfWidth,screenWidth-ptClient.x);
			region.top = 0;
			region.left = 0;

			if (ptClient.y < 0) 
			{
				region.top = -ptClient.y;
				ptClient.y = 0;
			}

			if (ptClient.x < 0) 
			{
				region.left = -ptClient.x;
				ptClient.x = 0;
			}

			if (region.bottom != region.top && region.left != region.right)
				lpPrimSurf->BltFast(ptClient.x, ptClient.y, lpColSurf, &region, 0);
		}
		else
		{
			/* Need to update the overlay */
			if (ptClient.x != WindowPosX || ptClient.y != WindowPosY)
			{
				WindowPosX = ptClient.x;
				WindowPosY = ptClient.y;

				RECT region;
				RECT rectClient;

				region.bottom = min(surfHeight,screenHeight-ptClient.y);
				region.right = min(surfWidth,screenWidth-ptClient.x);
				region.top = 0;
				region.left = 0;

				if (ptClient.y < 0) 
				{
					region.top = -ptClient.y;
					rectClient.top = 0;
				}
				else
				{
					rectClient.top = ptClient.y;
				}

				if (ptClient.x < 0) 
				{
					region.left = -ptClient.x;
					rectClient.left = 0;
				}
				else
				{
					rectClient.left = ptClient.x;
				}

				// Now we need to work out dest width and height
				rectClient.right = (region.right-region.left)+rectClient.left;
				rectClient.bottom = (region.bottom-region.top)+rectClient.top;

				DDOVERLAYFX  ddofx;
				memset(&ddofx, 0, sizeof(ddofx));
				ddofx.dwSize = sizeof(ddofx);
				ddofx.dckDestColorkey.dwColorSpaceLowValue = OVERLAY_COLOUR;
				ddofx.dckDestColorkey.dwColorSpaceHighValue = OVERLAY_COLOUR;

				HRESULT ddrval = lpColSurf->UpdateOverlay(&region, lpPrimSurf,
														  &rectClient,
														  DDOVER_SHOW|DDOVER_DDFX|DDOVER_KEYDESTOVERRIDE,
														  &ddofx);

				if(ddrval != DD_OK)
				{
					GDBG_INFO(80, "Unable to update overlay!\n");
					lpColSurf->Release();
					lpColSurf = 0;
				}
				else
				{
					/* Now Fill Primary with the col */
					DDBLTFX ddbfx;
					memset(&ddbfx, 0, sizeof(ddbfx));
					ddbfx.dwSize = sizeof(DDBLTFX);
					ddbfx.dwFillColor = OVERLAY_COLOUR;

					lpPrimSurf->Blt(&rectClient, NULL, &rectClient, DDBLT_COLORFILL|DDBLT_WAIT, &ddbfx);
				}
			}
		}
	}
}

/* This will release the context, and release the surfaces, and destroy the DDRAW objects */
extern "C" void _grReleaseWindowSurface(GrContext_t ctx)
{
	// Release Context
	grSurfaceReleaseContext(ctx);
	GDBG_INFO(80, "Released context!\n");


	// Now release all the surfaces
	if (lpTexSurf1) 
	{
		lpTexSurf1->Release();
		lpTexSurf1 = 0;
	}
	if (lpTexSurf0) 
	{
		lpTexSurf0->Release();
		lpTexSurf0 = 0;
	}
	if (lpAuxSurf) 
	{
		lpAuxSurf->Release();
		lpAuxSurf = 0;
	}
	if (lpColSurf) 
	{
		lpColSurf->Release();
		lpColSurf = 0;
	}
	if (lpPrimSurf) 
	{
		lpPrimSurf->Release();
		lpPrimSurf = 0;
	}
	GDBG_INFO(80, "All surfaces released\n");

	// And the DirectDraw objects
	if (lpDDraw2) 
	{
		lpDDraw2->Release();
		lpDDraw2 = NULL;
	}
	if (lpDDraw1) 
	{
		lpDDraw1->Release();
		lpDDraw1 = NULL;
	}
	GDBG_INFO(80, "Direct Draw released\n");

}
