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
** 19    9/29/98 2:32p Atai
** change color type if parameter is enabled
** 
** 18    9/09/98 11:57a Atai
** fix grVertexLayout dbg msg
** 
** 17    7/24/98 1:41p Hohn
** 
** 16    6/11/98 5:12p Atai
** added aa case for OGL
** 
** 15    6/08/98 7:28p Atai
** remove unused defines, add state selectors and update state routine
** prototype
** 
** 14    6/08/98 3:20p Atai
** fix tri stats
** 
** 13    5/29/98 11:30a Atai
** 1.added _EXT for extension #defines. 
** 2. change GR_TEXBASE_* values
** 
** 12    5/12/98 7:26p Atai
** added fog parameter offset and mode
** 
** 11    4/21/98 1:34p Atai
** make 32 bit clean
** 
** 10    4/17/98 10:59a Atai
** added grGlideGetVertexLayout and grGlideSetVertexLayout
** 
** 9     3/21/98 11:31a Atai
** added GR_TRIANGLE_STRIP_CONTINUE and GR_TRIANGLE_FAN_CONTINUE
** 
** 8     2/09/98 6:20p Atai
** remove aa strip and fan for grDrawVertexArrayContiguous
** 
** 7     2/06/98 6:57p Atai
** rename grVertexArrayLinear to grDrawVertexArrayContiguous
** 
** 6     2/05/98 6:19p Atai
** lazy evaluate for grVertexLayout
** 
** 5     2/03/98 3:40p Atai
** remove aa strip/fan and code clean up
** 
** 4     1/28/98 10:12a Atai
** update GrState size
** 
** 3     1/26/98 11:30a Atai
** update to new glide.h
** 
** 2     1/22/98 10:35a Atai
** 1. introduce GLIDE_VERSION, g3\glide.h, g3\glideutl.h, g2\glide.h,
** g2\glideutl.h
** 2. fixed grChromaRange, grSstOrigin, and grGetProcAddress
 * 
 * 1     1/16/98 4:29p Atai
 * create glide 3 src
 * 
 * 14    1/08/98 4:58p Atai
 * tex table broadcast, grVertexLayout enable/disable, stq, and some
 * defines
 * 
 * 13    12/12/97 10:59a Atai
 * clip space and viewport
 * 
 * 12    12/08/97 10:42a Atai
 * added grDrawVertexArrayLinear()
 * 
 * 11    11/21/97 6:05p Atai
 * use one datalist (tsuDataList) in glide3
 * 
 * 10    11/18/97 6:11p Peter
 * fixed glide3 muckage
 * 
 * 9     11/18/97 3:24p Atai
 * change grParameterData to grVertexLayout
 * define GR_PARAM_*
 * 
 * 8     11/07/97 11:22a Atai
 * remove GR_*_SMOOTH. use GR_SMOOTH
 * 
 * 7     11/06/97 6:10p Atai
 * update GrState size
 * rename grDrawArray to grDrawVertexArray
 * update _grDrawPoint and _grDrawVertexList
 * 
 * 6     10/21/97 8:36p Atai
 * added gr_lines routines
 * use dword offset
 * 
 * 5     10/17/97 2:11p Atai
 * added grContinueArray. We only support non aa mode for now.
 * 
 * 4     10/14/97 4:34p Atai
 * filled out the calls to different drawarray routines
 * 
 * 3     9/29/97 1:26p Dow
 * Fixed packed color strips/fans
 * 
 * 2     9/26/97 10:24a Dow
 * Fixed state muckage in Glide3 parameter data
 * 
 * 1     9/23/97 2:04p Dow
 * DI code for strips
**
*/

#include <3dfx.h>
#include <glidesys.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>

#include "fxglide.h"

/*-------------------------------------------------------------------
  Function: grVertexLayout
  Date: 17-Sep-97
  Implementor(s): dow
  Library: Init Code for 
  Description:
    This routine defines the format for vertex arrays.

  Arguments:
    param       -  Type of date-i.e. vertex, color, or texture info:
                   GR_COLOR, GR_VERTEX, GR_TEXTURE0, GR_TEXTURE1
    components  -  Which components are specified
                   Valid Components:


        So, this table summarizes the legal combinations:
        Param           Type            Size    Description 
        =======================================================================================================
        GR_PARAM_XY     FxFloat         8       X and Y coordinates.  Offset must be zero.
        GR_PARAM_Z      FxFloat         4       Z coordinate.
        GR_PARAM_A      FxFloat         4       Alpha value.
        GR_PARAM_RGB    FxFloat         12      RGB triplet.
        GR_PARAM_PARGB  FxU32           4       Packed ARGB.  High-order byte is A, followed by R, G, and B.
        GR_PARAM_STn    FxFloat         8       S and T coordinates for TMU , where n is in the range [0, TBD]
        GR_PARAM_Wn     FxFloat         4       

  Return:
  Nothing ever.
  -------------------------------------------------------------------*/
GR_DIENTRY(grVertexLayout, void , (FxU32 param, FxI32 offset, FxU32 mode) )
{
#define FN_NAME "grVertexLayout"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 85);

  GDBG_INFO_MORE((gc->myLevel, "(0x%x, 0x%x)\n", param, offset));

  switch (param) {
  case GR_PARAM_XY:
    GR_CHECK_F(myName,
               (offset != 0), 
               "Offset must be zero.");

    gc->state.vData.vertexInfo.offset = offset;
    gc->state.vData.vertexInfo.mode = mode;
    break;

  case GR_PARAM_Z:

    gc->state.vData.zInfo.offset = offset;
    gc->state.vData.zInfo.mode = mode;

    break;

  case GR_PARAM_W:

    gc->state.vData.wInfo.offset = offset;
    gc->state.vData.wInfo.mode = mode;

    break;

  case GR_PARAM_FOG_EXT:
    /*
    ** Fog coordinate is an extension in Glide3. It is supported in V2 and VB.
    ** If z-buffering, we use w iterator for fog coordinate.
    ** If w-buffering, we move the w iterator to floating point z and use w iterator for fog.
    */
    gc->state.vData.fogInfo.offset = offset;
    gc->state.vData.fogInfo.mode = mode;

    break;

  case GR_PARAM_A:

    gc->state.vData.aInfo.offset = offset;
    if (mode == GR_PARAM_ENABLE)
      gc->state.vData.colorType = GR_FLOAT;
    gc->state.vData.aInfo.mode = mode;

    break;
  case GR_PARAM_RGB:

    gc->state.vData.rgbInfo.offset = offset; 
    if (mode == GR_PARAM_ENABLE)
      gc->state.vData.colorType = GR_FLOAT;
    gc->state.vData.rgbInfo.mode = mode;

    break;
  case GR_PARAM_PARGB:

    gc->state.vData.pargbInfo.offset = offset;
    if (mode == GR_PARAM_ENABLE)
      gc->state.vData.colorType = GR_U8;
    gc->state.vData.pargbInfo.mode = mode;

    break;
  case GR_PARAM_ST0:

    gc->state.vData.st0Info.offset = offset;
    gc->state.vData.st0Info.mode = mode;

    break;
  case GR_PARAM_ST1:

    gc->state.vData.st1Info.offset = offset;
    gc->state.vData.st1Info.mode = mode;

    break;
  case GR_PARAM_Q:

    gc->state.vData.qInfo.offset = offset;
    gc->state.vData.qInfo.mode = mode;

    break;
  case GR_PARAM_Q0:

    gc->state.vData.q0Info.offset = offset;
    gc->state.vData.q0Info.mode = mode;

    break;
  case GR_PARAM_Q1:

    gc->state.vData.q1Info.offset = offset;
    gc->state.vData.q1Info.mode = mode;

    break;
  }

#if 0
  /* [dBorca] Should use delayed validation */
  gc->state.invalid |= vtxlayoutBIT;
#else
  _grUpdateParamIndex();
#endif

  GR_END();
#undef FN_NAME
} /* grParameterData */

/*-------------------------------------------------------------------
  Function: grGlideGetVertexLayout
  Date: 16-Apr-98
  Implementor(s): atai
  Library:  
  Description:
    This routine gets the vertex layout

  Arguments:
    layout  -  vertexlayout
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grGlideGetVertexLayout, void , (void *layout) )
{
#define FN_NAME "grGlideGetVertexLayout"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 87);
  GDBG_INFO_MORE((gc->myLevel,"(0x%x)\n",layout));
  GR_ASSERT(layout != NULL);

  *((GrVertexLayout *)layout) = gc->state.vData;

  GR_END();
#undef FN_NAME
} /* grGlideGetVertexLayout */

/*-------------------------------------------------------------------
  Function: grGlideSetVertexLayout
  Date: 16-Apr-98
  Implementor(s): atai
  Library:  
  Description:
    This routine sets the vertex layout

  Arguments:
    layout  -  vertexlayout
  Return:
  -------------------------------------------------------------------*/
GR_DIENTRY(grGlideSetVertexLayout, void , (const void *layout) )
{
#define FN_NAME "grGlideSetVertexLayout"
  GR_BEGIN_NOFIFOCHECK(FN_NAME, 87);
  GDBG_INFO_MORE((gc->myLevel,"(0x%x)\n",layout));
  GR_ASSERT(layout != NULL);

  gc->state.vData = *((GrVertexLayout *)layout);
#if 0
  /* [dBorca] Should use delayed validation */
  gc->state.invalid |= vtxlayoutBIT;
#else
  _grUpdateParamIndex();
#endif

  GR_END();
#undef FN_NAME
} /* grGlideSetVertexLayout */

/*-------------------------------------------------------------------
  Function: grDrawVertexArray
  Date: 18-Sep-97
  Implementor(s): dow
  Description:
  
  Arguments:
        mode:   GR_POINTS, GR_LINE_STRIP, GR_POLYGON, GR_TRIANLGE_STRIP,
                GR_TRIANGLE_FAN, GR_TRIANGLES
  
  Return:
        Nothing ever
  -------------------------------------------------------------------*/
GR_DIENTRY(grDrawVertexArray, void , (FxU32 mode, FxU32 Count, void *pointers) )
{
#define FN_NAME "grDrawVertexArray"
  FxU32 i;
  float **vPtr = pointers;

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 90);

  GDBG_INFO_MORE((gc->myLevel, "(0x%x, 0x%x, 0x%x)\n",
                 mode, Count, pointers));

#ifdef GDBG_INFO_ON
  {
    FxU32 i;
    for (i = 0; i < Count; i++)
      GDBG_INFO((110, "%s:  pointers[%d] = 0x%x\n",
                FN_NAME, i, ((float **)pointers)[i]));
  }
#endif

  switch (mode) {
  case GR_POINTS:
    /* [dBorca] "AA" is checked inside grDrawPoint */
    for (i = 0; i < Count; i++) {
        grDrawPoint(vPtr[i]);
    }
    break;
  case GR_LINE_STRIP:
    /* [dBorca] "AA" is checked inside grDrawLine */
    for (i = 1; i < Count; i++) {
        grDrawLine(vPtr[i-1], vPtr[i]);
    }
    break;
  case GR_LINES:
    /* [dBorca] "AA" is checked inside grDrawLine */
    for (i = 2; i <= (FxI32)Count; i += 2) {
        grDrawLine(vPtr[i-2], vPtr[i-1]);
    }
    break;
    
    /*
    ** anti-alias does not apply to strip and fan
    */
  case GR_TRIANGLE_STRIP:
    /* [dBorca] we need to cache the last TWO vertices */
    for (i = 3; i <= Count; i++) {
        /* CullFlip */
        if (i & 1) {
           grDrawTriangle(vPtr[i-3], vPtr[i-2], vPtr[i-1]);
        } else {
           grDrawTriangle(vPtr[i-3], vPtr[i-1], vPtr[i-2]);
        }
    }
    break;

  case GR_POLYGON:
  case GR_TRIANGLE_FAN:
    /* [dBorca] we need to cache the first & last vertices */
    for (i = 1; i < Count - 1; i++) {
        grDrawTriangle(vPtr[0], vPtr[i], vPtr[i+1]);
    }
    break;

/* [dBorca] not implemented yet!!!
  case GR_TRIANGLE_STRIP_CONTINUE:
    break;

  case GR_TRIANGLE_FAN_CONTINUE:
    break;
*/
      
  case GR_TRIANGLES:
    /* [dBorca] jump to anti-aliased function if GR_AA_ORDERED */
    if (gc->state.grEnableArgs.primitive_smooth_mode & GR_AA_ORDERED_TRIANGLES_MASK) {
       for (i = 3; i <= Count; i += 3) {
           grAADrawTriangle(vPtr[i-3], vPtr[i-2], vPtr[i-1], FXTRUE, FXTRUE, FXTRUE);
       }
    } else {
       for (i = 3; i <= Count; i += 3) {
           grDrawTriangle(vPtr[i-3], vPtr[i-2], vPtr[i-1]);
       }
    }
    break;

  }
#undef FN_NAME
} /* grDrawVertexArray */


/*-------------------------------------------------------------------
  Function: grDrawVertexArrayContiguous
  Date: 04-Dec-97
  Implementor(s): atai
  Description:
  
  Arguments:
        mode:   GR_POINTS, GR_LINE_STRIP, GR_POLYGON, GR_TRIANLGE_STRIP,
                GR_TRIANGLE_FAN, GR_TRIANGLES
  
  Return:
        Nothing ever
  -------------------------------------------------------------------*/
GR_DIENTRY(grDrawVertexArrayContiguous, void , (FxU32 mode, FxU32 Count, void *pointers, FxU32 stride) )
{
#define FN_NAME "grDrawVertexArrayContiguous"
  FxU32 i;
  char *vPtr = pointers;

  GR_BEGIN_NOFIFOCHECK(FN_NAME, 90);

  GDBG_INFO_MORE((gc->myLevel, "(0x%x, 0x%x, 0x%x)\n",
                 mode, Count, pointers));

#ifdef GDBG_INFO_ON
  {
    FxU32 i;
    for (i = 0; i < Count; i++)
      GDBG_INFO((110, "%s:  pointers[%d] = 0x%x\n",
                FN_NAME, i, (int)pointers + gc->state.vData.vStride * i));
  }
#endif

  gc->state.vData.vStride = stride >> 2;
  switch (mode) {
  case GR_POINTS:
    /* [dBorca] "AA" is checked inside grDrawPoint */
    for (i = 0; i < Count; i++) {
        grDrawPoint(vPtr);
        vPtr += stride;
    }
    break;
  case GR_LINE_STRIP:
    /* [dBorca] "AA" is checked inside grDrawLine */
    for (i = 1; i < Count; i++) {
        grDrawLine(vPtr, vPtr + stride);
        vPtr += stride;
    }
    break;
  case GR_LINES:
    /* [dBorca] "AA" is checked inside grDrawLine */
    for (i = 2; i <= (FxI32)Count; i += 2) {
        grDrawLine(vPtr, vPtr + stride);
        vPtr += stride * 2;
    }
    break;
    
  case GR_TRIANGLE_STRIP:
    /* [dBorca] we need to cache the last TWO vertices */
    for (i = 3; i <= Count; i++) {
        /* CullFlip */
        if (i & 1) {
           grDrawTriangle(vPtr, vPtr + stride, vPtr + stride * 2);
        } else {
           grDrawTriangle(vPtr, vPtr + stride * 2, vPtr + stride);
        }
        vPtr += stride;
    }
    break;

  case GR_POLYGON:
  case GR_TRIANGLE_FAN:
    /* [dBorca] we need to cache the first & last vertices */
    for (i = 1; i < Count - 1; i++) {
        grDrawTriangle(vPtr, vPtr + i * stride, vPtr + (i + 1) * stride);
    }
    break;
      
/* [dBorca] not implemented yet!!!
  case GR_TRIANGLE_STRIP_CONTINUE:
    break;

  case GR_TRIANGLE_FAN_CONTINUE:
    break;
*/
      
  case GR_TRIANGLES:
    /* [dBorca] jump to anti-aliased function if GR_AA_ORDERED */
    if (gc->state.grEnableArgs.primitive_smooth_mode & GR_AA_ORDERED_TRIANGLES_MASK) {
       for (i = 3; i <= Count; i += 3) {
           grAADrawTriangle(vPtr, vPtr + stride, vPtr + stride * 2, FXTRUE, FXTRUE, FXTRUE);
           vPtr += stride * 3;
       }
    } else {
       for (i = 3; i <= Count; i += 3) {
           grDrawTriangle(vPtr, vPtr + stride, vPtr + stride * 2);
           vPtr += stride * 3;
       }
    }
    break;

  }
#undef FN_NAME
} /* grDrawVertexArrayContiguous */
