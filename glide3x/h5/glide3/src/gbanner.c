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
** $Header$
** $Log: 
**  3    3dfx      1.0.1.0.1.0 10/11/00 Brent           Forced check in to enforce
**       branching.
**  2    3dfx      1.0.1.0     06/20/00 Joseph Kain     Changes to support the
**       Napalm Glide open source release.  Changes include cleaned up offensive
**       comments and new legal headers.
**  1    3dfx      1.0         09/11/99 StarTeam VTS Administrator 
** $
** 
** 8     2/19/99 8:03p Peter
** new splash
** 
** 7     11/21/98 10:19a Atai
** fixed test37 grChromaRangeModeExt error and rename functions
** 
** 6     7/16/98 8:17a Jdt
** fxcmd.h
** 
** 5     5/29/98 11:45a Atai
** 1.added _EXT for extension #defines. 
** 2. change GR_TEXBASE_* values
** 3. Remove GR_TEXCHROMA_ENABLE_SUBSTITUTE_RGB
** 
** 3     2/05/98 2:30p Atai
** fixed bug#1222.
** 
** 2     1/22/98 10:35a Atai
** 1. introduce GLIDE_VERSION, g3\glide.h, g3\glideutl.h, g2\glide.h,
** g2\glideutl.h
** 2. fixed grChromaRange, grSstOrigin, and grGetProcAddress
 * 
 * 1     1/16/98 4:29p Atai
 * create glide 3 src
 * 
 * 17    1/05/98 6:06p Atai
 * glide extension stuff
 * 
 * 16    12/15/97 5:52p Atai
 * disable obsolete glide2 api for glide3
 * 
 * 15    11/18/97 4:36p Peter
 * make thing more translucent
 * 
 * 14    9/15/97 7:31p Peter
 * more cmdfifo cleanup, fixed normal buffer clear, banner in the right
 * place, lfb's are on, Hmmmm.. probably more
 * 
 * 13    7/26/97 3:04p Peter
 * gratuitous plug is translucent
 * 
 * 12    7/08/97 2:48p Peter
 * shameless plug just uses lfb routines
 * 
 * 11    5/27/97 1:16p Peter
 * Basic cvg, w/o cmd fifo stuff. 
 * 
 * 10    5/21/97 6:05a Peter
 * 
 * 9     5/02/97 2:08p Pgj
 * screen_width/height now FxU32
 * 
 * 8     3/16/97 2:24a Jdt
 * Fixed bug.  Didn't initialize info.
 * 
 * 7     3/12/97 11:51p Jdt
 * Watcom warning.
 * 
 * 6     3/12/97 4:20p Jdt
 * Fixed for VG96 and optimized SST-1
 * 
 * 5     2/26/97 11:55a Jdt
 * Updated banner for new lfb api
 * 
 * 4     12/23/96 1:37p Dow
 * chagnes for multiplatform glide
**
*/
#include <3dfx.h>
#include <glidesys.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"
#include "fxcmd.h"

void
_grShamelessPlug(void)
{
  GR_BEGIN_NOFIFOCHECK("_grShamelessPlug", 80);
  GDBG_INFO_MORE(gc->myLevel, "()\n");

  if (gc->pluginInfo.plugProc != NULL) {
    FxU32
      plugWidth, plugHeight,
      plugStride;
    GrLfbWriteMode_t
      plugFormat;
    const void*
      plugData = (*gc->pluginInfo.plugProc)(&plugWidth, &plugHeight,
                                            &plugStride,
                                            &plugFormat);

    if (plugData != NULL) {
#ifdef GLIDE_PLUG
      GrState state;
      
      grGlideGetState(&state);
      grDisableAllEffects();
      
      grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                     GR_COMBINE_FACTOR_ONE,
                     GR_COMBINE_LOCAL_CONSTANT,
                     GR_COMBINE_OTHER_TEXTURE, FXFALSE);
      grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                     GR_COMBINE_FACTOR_ONE,
                     GR_COMBINE_LOCAL_NONE,
                     GR_COMBINE_OTHER_TEXTURE,
                     FXFALSE);
      grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,
                           GR_BLEND_ONE_MINUS_SRC_ALPHA,
                           GR_BLEND_ZERO, GR_BLEND_ZERO);
      grClipWindow(0, 0,
                   gc->state.screen_width - 1,
                   gc->state.screen_height - 1); 
      grDepthMask(FXFALSE);
      grDepthBufferFunction(GR_CMP_ALWAYS);
      grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);
      
      grChromakeyValue(0x0000);

      grChromakeyMode(GR_CHROMAKEY_ENABLE);
      grLfbConstantAlpha((FxU8) 0x40);
      grLfbWriteColorFormat(GR_COLORFORMAT_ARGB);
      
      _grLfbWriteRegion(FXTRUE, GR_BUFFER_BACKBUFFER,
                        gc->state.screen_width - (plugWidth + 1), 
                        gc->state.screen_height - (plugHeight + 1),
                        GR_LFBWRITEMODE_565,
                        plugWidth, plugHeight,
                        plugStride,
                        plugData);
      
      grGlideSetState(&state);
#endif /* GLIDE_PLUG */
    }
  }

  GR_END();
} /* _grShamelessPlug */



#if !(GLIDE_PLATFORM & GLIDE_OS_WIN32)

#if GLIDE_PLUG
#if GLIDE_PLUG_EXT
static FxU32 fxPlugWidth;
static FxU32 fxPlugHeight;
static FxU32 fxPlugStride;
#else
#include "banner.inc"
#endif
static GrLfbWriteMode_t fxPlugFormat = GR_LFBWRITEMODE_565;
static FxU16 *fxPlugData;
#endif /* GLIDE_PLUG */



void fxSplashShutdown (void)
{
#if GLIDE_PLUG
 if (fxPlugData != NULL) {
    free(fxPlugData);
    fxPlugData = NULL;
 }
#endif
}



FxBool fxSplashInit (FxU32 hWnd,
                     FxU32 screenWidth, FxU32 screenHeight,
                     FxU32 numColBuf, FxU32 numAuxBuf,
                     GrColorFormat_t colorFormat)
{
#if GLIDE_PLUG
 if (fxPlugData == NULL) {
#if GLIDE_PLUG_EXT
    /* [dBorca]
     * we should try to extract TGA resource from 3dfxspl3.dll
     */
    FILE *f;
    if ((f = fopen("3dfxplug.tga", "rb")) != NULL) {
       int bpp, skip;
       int i, j, decoded;
       unsigned char header[18], b1[4], b0;

       /* read TGA header */
       if (!fread(header, 18, 1, f)) {
          fclose(f);
          return FXFALSE;
       }

       /* fill in values */
       fxPlugWidth = ((unsigned short *)header)[6];
       fxPlugHeight = ((unsigned short *)header)[7];
       fxPlugStride = fxPlugWidth * 2;

       /* compute bits/pixel, then bytes/pixel; also check TGA type */
       bpp = header[16];
       if (((bpp != 16) && (bpp != 24) && (bpp != 32)) || ((header[2] & ~0x8) != 2)) {
          fclose(f);
          return FXFALSE;
       }
       bpp >>= 3;

       /* skip colormap + junk */
       skip = header[0];
       if (header[1]) {
          skip += *(unsigned short *)&header[5] * header[7] >> 3;
       }
       fseek(f, skip, SEEK_CUR);

       /* allocate datablock */
       if ((fxPlugData = malloc(fxPlugStride * fxPlugHeight)) == NULL) {
          fclose(f);
          return FXFALSE;
       }

       if (header[2] == 10) {
          /* RLE */
          j = 0;
          while (j < (fxPlugWidth * fxPlugHeight)) {
                /* packet header */
                if (!fread(&b0, 1, 1, f)) {
                   fxSplashShutdown();
                   fclose(f);
                   return FXFALSE;
                }
                if (b0 & 0x80) {
                   /* replicate pixels */
                   b0 &= 0x7f;
                   if (!fread(b1, bpp, 1, f)) {
                      fxSplashShutdown();
                      fclose(f);
                      return FXFALSE;
                   }
                   switch (bpp) {
                          case 2:
                               decoded = *(unsigned short *)&b1[0];
                               decoded = ((decoded & 0x7fe0) << 1)
                                       | ((decoded & 0x0200) >> 4)
                                       |  (decoded & 0x001f);
                               break;
                          case 3:
                          case 4:
                               decoded = (b1[0]>>3) + ((b1[1]>>2)<<5) + ((b1[2]>>3)<<11);
                               break;
                   }
                   for (i = 0; i <= b0; i++) {
                       fxPlugData[j++] = decoded;
                   }
                } else {
                   /* read pixels */
                   for (i = 0; i <= b0; i++) {
                       if (!fread(b1, bpp, 1, f)) {
                          fxSplashShutdown();
                          fclose(f);
                          return FXFALSE;
                       }
                       switch (bpp) {
                              case 2:
                                   decoded = *(unsigned short *)&b1[0];
                                   decoded = ((decoded & 0x7fe0) << 1)
                                           | ((decoded & 0x0200) >> 4)
                                           |  (decoded & 0x001f);
                                   break;
                              case 3:
                              case 4:
                                   decoded = (b1[0]>>3) + ((b1[1]>>2)<<5) + ((b1[2]>>3)<<11);
                                   break;
                       }
                       fxPlugData[j++] = decoded;
                   }
                }
          }
          /* flip (RLE can cross scanlines, thus we can't use tricks) */
          if (!(header[17] & 0x20)) {
             for (i = 0; i < fxPlugHeight / 2; i++) {
                 unsigned short *src = &fxPlugData[fxPlugWidth * i];
                 unsigned short *dst = &fxPlugData[fxPlugWidth * (fxPlugHeight - i - 1)];
                 for (j = 0; j < fxPlugWidth; j++) {
                     decoded = dst[j];
                     dst[j] = src[j];
                     src[j] = decoded;
                 }
             }
          }
       } else if (header[2] == 2) {
          /* normal (flip on-the-fly) */
          for (i = 0; i < fxPlugHeight; i++) {
              int l = (header[17] & 0x20) ? i : (fxPlugHeight-i-1);
              unsigned short *line = &fxPlugData[fxPlugWidth * l];
              for (j = 0; j < fxPlugWidth; j++) {
                  if (!fread(b1, bpp, 1, f)) {
                     fxSplashShutdown();
                     fclose(f);
                     return FXFALSE;
                  }
                  switch (bpp) {
                         case 2:
                              decoded = *(unsigned short *)&b1[0];
                              decoded = ((decoded & 0x7fe0) << 1)
                                      | ((decoded & 0x0200) >> 4)
                                      |  (decoded & 0x001f);
                              break;
                         case 3:
                         case 4:
                              decoded = (b1[0]>>3) + ((b1[1]>>2)<<5) + ((b1[2]>>3)<<11);
                              break;
                  }
                  line[j] = decoded;
              }
          }
       }

       fclose(f);
    }

#else  /* GLIDE_PLUG_EXT */

    /* [dBorca]
     * embedded image is always 16bit RLE and does not need to be flipped
     */
    int k = 0;
    int i, j, decoded;
    unsigned char b0;

    /* allocate datablock */
    if ((fxPlugData = malloc(fxPlugStride * fxPlugHeight)) == NULL) {
       return FXFALSE;
    }

    /* RLE */
    j = 0;
    while (j < (fxPlugWidth * fxPlugHeight)) {
          /* packet header */
          b0 = tga_16rle[k++];
          if (b0 & 0x80) {
             /* replicate pixels */
             b0 &= 0x7f;
             decoded = *(unsigned short *)&tga_16rle[k];
             k += 2;
             for (i = 0; i <= b0; i++) {
                 fxPlugData[j++] = decoded;
             }
          } else {
             /* read pixels */
             for (i = 0; i <= b0; i++) {
                 decoded = *(unsigned short *)&tga_16rle[k];
                 k += 2;
                 fxPlugData[j++] = decoded;
             }
          }
    }
#endif /* GLIDE_PLUG_EXT */
 }
 return FXTRUE;

#else  /* GLIDE_PLUG */

 return FXFALSE;
#endif /* GLIDE_PLUG */
}



const void *fxSplashPlug (FxU32* w, FxU32* h,
                          FxI32* strideInBytes,
                          GrLfbWriteMode_t* format)
{
#if GLIDE_PLUG
 *w = fxPlugWidth;
 *h = fxPlugHeight;
 *strideInBytes = fxPlugStride;
 *format = fxPlugFormat;
 return fxPlugData;
#else
 return NULL;
#endif
}



void fxSplash (float x, float y, float w, float h, FxU32 frameNumber)
{
}

#endif /* (GLIDE_PLATFORM & GLIDE_OS_WIN32) */
