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
 ** $Log$
 ** Revision 1.1.1.1.8.2  2003/11/03 13:34:29  dborca
 ** Voodoo2 happiness (DJGPP & Linux)
 **
 ** Revision 1.1.1.1.8.1  2003/06/29 18:45:55  guillemj
 ** Fixed preprocessor invalid token errors.
 **
 ** Revision 1.1.1.1  1999/12/07 21:42:31  joseph
 ** Initial checkin into SourceForge.
 **
** 
** 1     10/08/98 11:30a Brent
** 
** 22    10/07/98 9:43p Peter
** triangle procs for 3DNow!(tm)
** 
** 21    10/06/98 8:23p Peter
** 3DNow!(tm) texture downloads
** 
** 20    10/06/98 7:18p Atai
** added triangle and drawlist asm routine for clip coords
** 
** 19    9/08/98 7:13p Atai
** same
** 
** 18    9/08/98 6:56p Atai
** fix debug info. added underline for internal routines and some \n
** 
** 17    7/01/98 11:44a Atai
** cast _grDepthBiasLevel call
** 
** 16    7/01/98 11:31a Atai
** make grDepthBiasLevel argument FxI32
** 
** 15    6/24/98 1:47p Atai
** code clean up; rename texute line routine
** 
** 14    6/21/98 11:58a Atai
** fixed fogcoord paramindex
** 
** 13    6/17/98 10:39a Atai
** fix grDepthBiasLevel
** 
** 12    6/11/98 5:12p Atai
** added aa case for OGL
** 
** 11    6/09/98 5:33p Atai
** replace grSstControl with grEnable/grDisable
** 
** 10    6/03/98 2:34p Atai
** fix chromarange
** 
** 9     5/29/98 6:39p Atai
** fix chromarange
** 
** 8     5/28/98 3:27p Atai
** check the invalid flag in grBufferClear
** 
** 7     4/24/98 2:21p Atai
** call _grUpdateParamIndex() if the state is dirty
** 
** 6     4/23/98 7:33p Atai
** grValidateState: send register in a group
** 
** 5     2/24/98 6:01p Atai
** modify validatestate
** 
** 4     1/30/98 1:19p Atai
** fixed chromarange
** 
** 3     1/28/98 2:26p Atai
** fix grDisable GR_SHAMELESS_PLUG
** 
** 2     1/22/98 10:35a Atai
** 1. introduce GLIDE_VERSION, g3\glide.h, g3\glideutl.h, g2\glide.h,
** g2\glideutl.h
** 2. fixed grChromaRange, grSstOrigin, and grGetProcAddress
 * 
 * 1     1/16/98 4:29p Atai
 * create glide 3 src
 * 
 * 17    1/13/98 12:42p Atai
 * fixed grtexinfo, grVertexLayout, and draw triangle
 * 
 * 16    1/10/98 4:01p Atai
 * inititialize vertex layout, viewport, added defines
 * 
 * 15    1/05/98 6:06p Atai
 * glide extension stuff
 * 
 * 14    12/17/97 4:05p Atai
 * added grChromaRange(), grGammaCorrecionRGB(), grRest(), and grGet()
 * functions
 * 
 * 13    12/16/97 11:38a Atai
 * added grChromaRange()
 * 
 * 12    12/15/97 5:52p Atai
 * disable obsolete glide2 api for glide3
 * 
 * 10    12/12/97 1:30p Atai
 * remove fp z  buffer
 * 
 * 8     12/08/97 10:44a Atai
 * added entry point for grCoordinateSpace(), grDepthRange(), and
 * grViewport()
 * 
 * 7     11/13/97 4:38p Atai
 * invalidate lfbMode and c0c1
 * 
 * 6     11/10/97 5:20p Atai
 * added factor for grAlphaCombine and remove extra  _grChromakeyMode
 * 
 * 5     11/07/97 11:22a Atai
 * remove GR_*_SMOOTH. use GR_SMOOTH
 * 
 * 4     10/15/97 7:33a Dow
 * Made _grValidateState use central routine for writing data
 * 
 * 3     10/14/97 4:18p Atai
 * added grEnable and grDisable
 * 
 * 2     10/10/97 2:57p Dow
 * Minor adjustments
 * 
 * 1     10/09/97 5:19p Dow
 * State Monster file
 */


#include <3dfx.h>
#include <glidesys.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"

/*-------------------------------------------------------------------
  Function: grStipplePattern
  Date: 23-Nov-2000
  Implementor(s): alanh
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grStipplePattern, void , (GrStipplePattern_t stipple))
{
 #define FN_NAME "grStipplePattern"

  /* [dBorca] TODO
   *
  GR_BEGIN_NOFIFOCHECK("grStipplePattern\n", 85);

  INVALIDATE(stipple);

  STOREARG(grStipplePattern, stipple);
   */

 #undef FN_NAME
} /* grStipplePattern */

/*-------------------------------------------------------------------
  Function: grStippleMode
  Date: 23-Nov-2000
  Implementor(s): alanh
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grStippleMode, void , (GrStippleMode_t mode) )
{
 #define FN_NAME "grStippleMode"

  /* [dBorca] TODO
   *
  GR_BEGIN_NOFIFOCHECK("grStippleMode\n", 85);

  INVALIDATE(fbzMode);

  STOREARG(grStippleMode, mode);
   */

 #undef FN_NAME
} /* grStippleMode */

/*-------------------------------------------------------------------
  Function: grEnable
  Date: 10-Oct-97
  Implementor(s): atai
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grEnable, void , (GrEnableMode_t mode) )
{
#define FN_NAME "grEnable"
  GR_BEGIN_NOFIFOCHECK("grEnable\n", 85);

  switch (mode) {
  case GR_AA_ORDERED:
    gc->state.grEnableArgs.primitive_smooth_mode = 
      GR_AA_ORDERED_POINTS_MASK |
      GR_AA_ORDERED_LINES_MASK |
      GR_AA_ORDERED_TRIANGLES_MASK;
    break;
  case GR_AA_ORDERED_POINTS_OGL:
    gc->state.grEnableArgs.primitive_smooth_mode |= GR_AA_ORDERED_POINTS_MASK;
    break;
  case GR_AA_ORDERED_LINES_OGL:
    gc->state.grEnableArgs.primitive_smooth_mode |= GR_AA_ORDERED_LINES_MASK;
    break;
  case GR_AA_ORDERED_TRIANGLES_OGL:
    gc->state.grEnableArgs.primitive_smooth_mode |= GR_AA_ORDERED_TRIANGLES_MASK;
    break;
  case GR_SHAMELESS_PLUG:
    /*ENABLEMODE(shameless_plug_mode);*/ /* [dBorca] futile */
    _GlideRoot.environment.shamelessPlug = GR_MODE_ENABLE;
    break;
  case GR_VIDEO_SMOOTHING:
    /*ENABLEMODE(video_smooth_mode);*/
    break;
  case GR_ALLOW_MIPMAP_DITHER:
    gc->state.allowLODdither = GR_MODE_ENABLE;
    break;
  case GR_PASSTHRU:
    _grSstControl(GR_CONTROL_ACTIVATE);
    break;
  }

#undef FN_NAME
} /* grEnable */

/*-------------------------------------------------------------------
  Function: grDisable
  Date: 10-Oct-97
  Implementor(s): atai
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grDisable, void , (GrEnableMode_t mode) )
{
#define FN_NAME "grDisable"
  GR_BEGIN_NOFIFOCHECK("grDisable\n", 85);

  switch (mode) {
  case GR_AA_ORDERED:
    gc->state.grEnableArgs.primitive_smooth_mode = 0;
    break;
  case GR_AA_ORDERED_POINTS_OGL:
    gc->state.grEnableArgs.primitive_smooth_mode &= ~GR_AA_ORDERED_POINTS_MASK;
    break;
  case GR_AA_ORDERED_LINES_OGL:
    gc->state.grEnableArgs.primitive_smooth_mode &= ~GR_AA_ORDERED_LINES_MASK;
    break;
  case GR_AA_ORDERED_TRIANGLES_OGL:
    gc->state.grEnableArgs.primitive_smooth_mode &= ~GR_AA_ORDERED_TRIANGLES_MASK;
    break;
  case GR_SHAMELESS_PLUG:
    /*DISABLEMODE(shameless_plug_mode);*/ /* [dBorca] futile */
    _GlideRoot.environment.shamelessPlug = GR_MODE_DISABLE;
    break;
  case GR_VIDEO_SMOOTHING:
    /*DISABLEMODE(video_smooth_mode);*/
    break;
  case GR_ALLOW_MIPMAP_DITHER:
    gc->state.allowLODdither = GR_MODE_DISABLE;
    break;
  case GR_PASSTHRU:
    _grSstControl(GR_CONTROL_DEACTIVATE);
    break;
  }

#undef FN_NAME
} /* grDisable */

/*-------------------------------------------------------------------
  Function: grCoordinateSpace
  Date: 01-Dec-97
  Implementor(s): atai
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grCoordinateSpace, void , (GrCoordinateSpaceMode_t mode) )
{
#define FN_NAME "grCoordinateSpace"
  GR_BEGIN_NOFIFOCHECK("grCoordinateSpace\n", 85);

  switch (mode) {
  case GR_WINDOW_COORDS:
    gc->state.grCoordinateSpaceArgs.coordinate_space_mode = GR_WINDOW_COORDS;
    break;
  case GR_CLIP_COORDS:
    gc->state.grCoordinateSpaceArgs.coordinate_space_mode = GR_CLIP_COORDS;
    /* [dBorca] not implemented yet!!! */
    exit(-1);
    break;
  }

#undef FN_NAME
} /* grCoordinateSpace */

/*-------------------------------------------------------------------
  Function: grDepthRange
  Date: 01-Dec-97
  Implementor(s): atai
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grDepthRange, void , (FxFloat n, FxFloat f) )
{
#define FN_NAME "grDepthRange"
  GR_BEGIN_NOFIFOCHECK("grDepthRange\n", 85);

  gc->state.Viewport.n = n;
  gc->state.Viewport.f  = f;
  gc->state.Viewport.hdepth = (f - n) * 0.5f * 65535.f;
  gc->state.Viewport.oz = (f + n) * 0.5f * 65535.f;

#undef FN_NAME
} /* grDepthRange */

/*-------------------------------------------------------------------
  Function: grViewport
  Date: 01-Dec-97
  Implementor(s): atai
  Description:
  
  Arguments:
  
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grViewport, void , (FxI32 x, FxI32 y, FxI32 width, FxI32 height) )
{
#define FN_NAME "grViewport"
  GR_BEGIN_NOFIFOCHECK("grViewport\n", 85);

  gc->state.Viewport.ox      = (FxFloat)(x + width * 0.5f);
  gc->state.Viewport.oy      = (FxFloat)(y + height *0.5f);
  gc->state.Viewport.hwidth  = width * 0.5f;
  gc->state.Viewport.hheight = height * 0.5f;

#undef FN_NAME
} /* grViewport */
