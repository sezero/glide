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
** Revision 1.1.1.1  1999/12/07 21:48:52  joseph
** Initial checkin into SourceForge.
**
 * 
 * 13    8/21/97 2:26p Atai
 * fix FIFO size
 * 
 * 12    7/25/97 2:55p Atai
 * Remove slope calculation in edge sense code, use cross product instead.
 * Move grAADrawTriEdgeSense() 1/area calculation after pci write command.
 * 
 * 11    7/24/97 5:59p Atai
 * optimize grAADrawPoint, grAADrawLine and grAADrawTriangle for sst1
 * 
 * 10    3/04/97 9:08p Dow
 * Neutered multiplatform multiheaded monster
 * 
 * 9     12/23/96 1:37p Dow
 * chagnes for multiplatform glide
**
*/

#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

#define FARRAY(p,i) (*(float *)((i)+(int)(p)))
/* access a byte array with a byte index and convert to float */
#define FbARRAY(p,i) (float)(((unsigned char *)p)[i])

/* X and Y have fixed position. These represent offsets when
 * the vertex structure is seen as an array of bytes
 */
#define GR_VERTEX_X_OFFSET 0
#define GR_VERTEX_Y_OFFSET 4

/*
** -----------------------------------------------------------------------
**
** Antialiasing rendering functions
**
** -----------------------------------------------------------------------
*/

/*---------------------------------------------------------------------------
** grAADrawPoint
**
**   |-w-|
**   A---B -
**   |\ /| |
**   | E | h
**   |/ \| |
**   D --C -
**   w = h = 2pixels
**
** pci write command
** 1st triangle EDA
**     8 + 3 * n
** 2nd triangle BEC
**     8
** 3rd triangle AED
**     5
** 4th triangle ABE
**     8
** total = 29 + 3 * n
*/
void FX_CSTYLE
_grAADrawPoint ( const void *e )
{
  int i, j;
  float *fp;
  struct dataList_s *dlp;
  float dp;
  float PX, PY;

  GR_BEGIN("grAADrawPoint",94,((_GlideRoot.curTriSize - _GlideRoot.curTriSizeNoGradient) >> 3) * 12 + 116);

  PX = FARRAY(e, GR_VERTEX_X_OFFSET);
  PY = FARRAY(e, GR_VERTEX_Y_OFFSET);
  if (gc->state.vData.colorType == GR_FLOAT) {
     j = *(long *)((FxU32)e + gc->state.vData.aInfo.offset);
  } else {
     float aleph = FbARRAY(e, gc->state.vData.pargbInfo.offset + 3);
     j = *(long *)&aleph;
  }
  j ^= 0x80000000;

  /* 1st triangle EDA */
  GR_SETF(hw->FvA.x, PX);
  dp = PX - _GlideRoot.pool.f1;
  GR_SETF(hw->FvB.x, dp);
  dp = PX + _GlideRoot.pool.f1;
  GR_SETF(hw->FvC.x, dp);
  dlp = gc->dataList;
  GR_SETF(hw->FvA.y, PY);
  dp = PY + _GlideRoot.pool.f1;
  GR_SETF(hw->FvB.y, dp);
  i = dlp->i;
  GR_SETF(hw->FvC.y, dp);
  while (i) {
    fp = dlp->addr;
    if (i & 1) {     /* packer bug check */
      if (i & 2) P6FENCE;
      GR_SETF( fp[0], 0.0f);
      if (i & 2) P6FENCE;
      dlp++;
      i = dlp->i;
    }
    else if (i >= 0) {
      dp = FARRAY(e,i);
      GR_SETF( fp[0], dp );
      GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
      dlp++;
      i = dlp->i;
      GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
    }
    else {
      /* [dBorca] Packed Color Workaround (tm) */
      i = (i & 0xffffff) + ((i >> 24) & 3);
      dp = FbARRAY(e,i);
      GR_SETF( fp[0], dp );
      GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
      dlp++;
      i = dlp->i;
      GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
    }
  }
  GR_SET(hw->Fdady, j);
  P6FENCE_CMD( GR_SET(hw->triangleCMD, 0xffffffff) );

  /* 2nd triangle BEC */
  dp = PX + _GlideRoot.pool.f1;
  GR_SETF(hw->FvA.x, dp);
  GR_SETF(hw->FvB.x, PX);
  dp = PY - _GlideRoot.pool.f1;
  GR_SETF(hw->FvA.y, dp);
  GR_SETF(hw->FvB.y, PY);

  GR_SET(hw->Fa, 0);
  GR_SET(hw->Fdadx, j);
  GR_SET(hw->Fdady, 0);
  P6FENCE_CMD( GR_SET(hw->triangleCMD, 0xffffffff) );

  /* 3rd triangle AED */
  dp = PX - _GlideRoot.pool.f1;
  GR_SETF(hw->FvA.x, dp);
  GR_SETF(hw->FvC.x, dp);

  j ^= 0x80000000;
  GR_SET(hw->Fa, 0);
  GR_SET(hw->Fdadx, j);
  P6FENCE_CMD( GR_SET(hw->triangleCMD, 1) );

  /* 4th triangle ABE */
  dp = PX + _GlideRoot.pool.f1;
  GR_SETF(hw->FvB.x, dp);
  GR_SETF(hw->FvC.x, PX);
  dp = PY - _GlideRoot.pool.f1;
  GR_SETF(hw->FvB.y, dp);
  GR_SETF(hw->FvC.y, PY);

  GR_SET(hw->Fa, 0);
  GR_SET(hw->Fdadx, 0);
  GR_SET(hw->Fdady, j);
  P6FENCE_CMD( GR_SET(hw->triangleCMD, 1) );

  GR_END();
} /* grAADrawPoint */

static void grSetVertexParameter(const void *v1)
{
  GR_DCL_GC;
  GR_DCL_HW;
  struct dataList_s *dlp;
  int i;
  float *fp, dp;

  dlp = gc->dataList;
  i = dlp->i;

  while (i) {
    fp = dlp->addr;
    if (i & 1) {     /* packer bug check */
      if (i & 2) P6FENCE;
      GR_SETF( fp[0], 0.0f);
      if (i & 2) P6FENCE;
      dlp++;
      i = dlp->i;
    }
    else if (i >= 0) {
      dp = FARRAY(v1,i);
      GR_SETF( fp[0], dp );
      dlp++;
      i = dlp->i;
    }
    else {
      /* [dBorca] Packed Color Workaround (tm) */
      i = (i & 0xffffff) + ((i >> 24) & 3);
      dp = FbARRAY(v1,i);
      GR_SETF( fp[0], dp );
      dlp++;
      i = dlp->i;
    }
  }
}

/*---------------------------------------------------------------------------
** grAADrawLine
**  A(x1,y1-1.0)+
**              |  \
**              | \   \
**              |    \   \
**     V1(x1,y1)*           \
**              |  *     \     \
**              |  \  *           \
**              |             \      \
**  E(x1,y1+1.0)+       \  *      \     \
**                 \          *            +B(x2,y2-1.0)
**                    \      \    *   \    |
**                       \                 |
**                          \     \    *  \|
**                             \           +V2(x2,y2)
**                                \   \    |
**                                   \     |
**                                      \ \|
**                                         +F(x2,y2+1.0)
**                                         
** pci write command (x major line )
** 1st triangle A-B-V2
**     10 + 3 * n
** 2nd triangle A-V1-V2
**     6 + n
** 3rd triangle V1-E-F
**     6 + n
** 4th triangle V1-V2-F
**     5 + n
** total = 
*/
void FX_CSTYLE
_grAADrawLine ( const void *v1, const void *v2 )
{
  float adx;         /* |dX| */
  const void *tv;
  struct dataList_s *dlp;
  int i;
  float *fp, dp;
  float tmp1, tmp2;
  float dx, dy;           /* delta X and Y */

  float v1_x, v1_y, v2_x, v2_y, v1_a, v2_a;

  GR_BEGIN("grAADrawLine",95, ((_GlideRoot.curTriSize - _GlideRoot.curTriSizeNoGradient) >> 3) * 24 + 108);

  GDBG_INFO((95,"grAADrawLine(0x%x,0x%x)\n",v1,v2));

  /* turn off culling so triangles unconditionally draw */

  /* draw from low Y to high Y */
  if ( FARRAY(v2, GR_VERTEX_Y_OFFSET) < FARRAY(v1, GR_VERTEX_Y_OFFSET) ) {
    tv = v1; v1 = v2; v2 = tv;
  }

  v1_x = FARRAY(v1, GR_VERTEX_X_OFFSET);
  v1_y = FARRAY(v1, GR_VERTEX_Y_OFFSET);
  v2_x = FARRAY(v2, GR_VERTEX_X_OFFSET);
  v2_y = FARRAY(v2, GR_VERTEX_Y_OFFSET);
  if (gc->state.vData.colorType == GR_FLOAT) {
     v1_a = FARRAY(v1, gc->state.vData.aInfo.offset);
     v2_a = FARRAY(v2, gc->state.vData.aInfo.offset);
  } else {
     v1_a = FbARRAY(v1, gc->state.vData.pargbInfo.offset + 3);
     v2_a = FbARRAY(v2, gc->state.vData.pargbInfo.offset + 3);
  }
  
  /* compute deltas and absolute deltas */
  dx = adx = v1_x - v2_x;
  dy = v2_y - v1_y;
  if ( adx < 0 )
    adx = -adx;
  
  if ( adx >= dy ) {           /* X major line */

    if (dx == 0.0f)
      return;

    /* 1st triangle A-B-V2*/

    GR_SETF( hw->FvA.x, v1_x);
    dx = _GlideRoot.pool.f1 / dx;
    GR_SETF( hw->FvA.y, v1_y - _GlideRoot.pool.f1);
    tmp1 = -v1_a * dy;
    GR_SETF( hw->FvB.x, v2_x);
    GR_SETF( hw->FvB.y, v2_y - _GlideRoot.pool.f1);
    tmp2 = -v2_a * dy;
    GR_SETF( hw->FvC.x, v2_x);
    dlp = gc->dataList;
    GR_SETF( hw->FvC.y, v2_y);
    i = dlp->i;
    while (i) {
      fp = dlp->addr;
      if (i & 1) {     /* packer bug check */
        if (i & 2) P6FENCE;
        GR_SETF( fp[0], 0.0f);
        if (i & 2) P6FENCE;
        dlp++;
        i = dlp->i;
      }
      else if (i >= 0) {
        dp = FARRAY(v1,i);
	GR_SETF( fp[0], dp );
	dp -= FARRAY(v2, i);
	GR_SETF( fp[DPDX_OFFSET>>2] , dp * dx);
	dlp++;
	i = dlp->i;
	GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
      }
      else {
        /* [dBorca] Packed Color Workaround (tm) */
        i = (i & 0xffffff) + ((i >> 24) & 3);
        dp = FbARRAY(v1,i);
	GR_SETF( fp[0], dp );
	dp -= FbARRAY(v2, i);
	GR_SETF( fp[DPDX_OFFSET>>2] , dp * dx);
	dlp++;
	i = dlp->i;
	GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
      }
    }
    GR_SET( hw->Fa , 0 );
    dp = - tmp2 * dx;
    GR_SETF( hw->Fdadx , dp );
    GR_SETF( hw->Fdady , v2_a );
    P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, -dx ) );

    /* 2nd triangle A-V1-V2*/
    GR_SETF( hw->FvB.x, v1_x);
    grSetVertexParameter(v1);
    GR_SET( hw->Fa , 0 );
    dp = v1_a - v2_a;
    GR_SETF( hw->FvB.y, v1_y);
    dp -= tmp1;
    GR_SETF( hw->Fdady , v1_a );
    dp *= dx;
    GR_SETF( hw->Fdadx , dp );
    P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, dx ) );

    /* 3rd triangle v1-E-F */
    GR_SETF( hw->FvA.y, v1_y);
    GR_SETF( hw->FvB.y, v1_y + _GlideRoot.pool.f1);
    GR_SETF( hw->FvC.y, v2_y + _GlideRoot.pool.f1);

    grSetVertexParameter(v1);

    GR_SETF( hw->Fdady , -v1_a );
    dp = tmp1 * dx;
    GR_SETF( hw->Fdadx , dp );
    P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, dx ) );

    /* 4th triangle V1-V2-F*/
    GR_SETF( hw->FvB.x, v2_x);
    dp = v1_a - v2_a + tmp2;
    GR_SETF( hw->FvB.y, v2_y);
    dp *= dx;

    grSetVertexParameter(v1);

    GR_SETF( hw->Fdadx , dp ); 
    GR_SETF( hw->Fdady , -v2_a );
    P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, -dx ) );

  } else {                      /* Y major line */

    if (dy == 0.0f)
      return;

    /* 1st triangle v1-v2-b */
    GR_SETF( hw->FvA.x, v1_x);
    tmp1 = v1_a * dx;
    GR_SETF( hw->FvA.y, v1_y);
    tmp2 = v2_a * dx;
    GR_SETF( hw->FvB.x, v2_x);
    dy = - 1.0f / dy;
    GR_SETF( hw->FvB.y, v2_y);
    GR_SETF( hw->FvC.x, v2_x - _GlideRoot.pool.f1);
    dlp = gc->dataList;
    GR_SETF( hw->FvC.y, v2_y);
    i = dlp->i;
    while (i) {
      fp = dlp->addr;
      if (i & 1) {     /* packer bug check */
	if (i & 2) P6FENCE;
	GR_SETF( fp[0], 0.0f);
	if (i & 2) P6FENCE;
	dlp++;
	i = dlp->i;
      }
      else if (i >= 0) {
	dp = FARRAY(v1,i);
	GR_SETF( fp[0], dp );
        dp -= FARRAY(v2, i);
	GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
	dlp++;
	i = dlp->i;
        GR_SETF( fp[DPDY_OFFSET>>2] , dp * dy );
      }
      else {
        /* [dBorca] Packed Color Workaround (tm) */
        i = (i & 0xffffff) + ((i >> 24) & 3);
	dp = FbARRAY(v1,i);
	GR_SETF( fp[0], dp );
        dp -= FbARRAY(v2, i);
	GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0);
	dlp++;
	i = dlp->i;
        GR_SETF( fp[DPDY_OFFSET>>2] , dp * dy );
      }
    }
    dp = (v1_a - tmp2 - v2_a);
    GR_SETF( hw->Fdadx , v2_a );
    dp *= dy;
    GR_SETF( hw->Fdady , dp );
    P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, -dy ) );

    /* 2nd triangle v1-a-b */

    GR_SETF( hw->FvB.x, v1_x - _GlideRoot.pool.f1);
    GR_SETF( hw->FvB.y, v1_y);
    grSetVertexParameter(v1);
    GR_SETF( hw->Fdadx , v1_a );
    dp = - tmp1 * dy;
    GR_SETF( hw->Fdady , dp );
    P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, dy ) );

    /* 3rd triangle e-v1-v2 */
    GR_SETF( hw->FvA.x, v1_x + _GlideRoot.pool.f1);
    dp = (v1_a - v2_a + tmp1);
    GR_SETF( hw->FvB.x, v1_x);
    dp *= dy;
    GR_SETF( hw->FvC.x, v2_x);

    grSetVertexParameter(v1);
    GR_SET( hw->Fa , 0 );
    GR_SETF( hw->Fdadx , -v1_a );
    GR_SETF( hw->Fdady , dp );
    P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, dy ) );

    /* 4th triangle e-f-v2 */
    GR_SETF( hw->FvB.x, v2_x + _GlideRoot.pool.f1);
    GR_SETF( hw->FvB.y, v2_y);

    grSetVertexParameter(v1);
    GR_SET( hw->Fa , 0 );

    GR_SETF( hw->Fdadx , -v2_a );
    dp = tmp2 * dy;
    GR_SETF( hw->Fdady , dp );
    P6FENCE_CMD( GR_SETF( hw->FtriangleCMD, -dy ) );

  }
  GR_END();
} /* grAADrawLine */

typedef enum {
  aaEdgeSenseTop,
  aaEdgeSenseLeft,
  aaEdgeSenseBottom,
  aaEdgeSenseRight
} aaEdgeSense;

/*
**  grAADrawTriEdgeSense
**
**  This routine combines aaCalcEdgeSense(), grAADrawTriEdge(), and
**  grAADrawEdgeTriangle(). The goal is to
**  1. reduce the number of divide from 3 to 1
**  2. minimize pci write command.
*/

static void
grAADrawTriEdgeSense(const void *a,const void *b,const void *c)
{
  const void *tv;
  float dx, dy, m, dp;
  aaEdgeSense sense;

  float a_a, b_a;

  GR_BEGIN_NOFIFOCHECK("grAADrawTriEdgeSense",gc->myLevel);


  if (FARRAY(a, GR_VERTEX_Y_OFFSET) >= FARRAY(b, GR_VERTEX_Y_OFFSET)) {
    tv = a; a = b; b = tv;
  }
  dx = FARRAY(a, GR_VERTEX_X_OFFSET) - FARRAY(b, GR_VERTEX_X_OFFSET);
  dy = FARRAY(a, GR_VERTEX_Y_OFFSET) - FARRAY(b, GR_VERTEX_Y_OFFSET);

  if (gc->state.vData.colorType == GR_FLOAT) {
     a_a = FARRAY(a, gc->state.vData.aInfo.offset);
     b_a = FARRAY(b, gc->state.vData.aInfo.offset);
  } else {
     a_a = FbARRAY(a, gc->state.vData.pargbInfo.offset + 3);
     b_a = FbARRAY(b, gc->state.vData.pargbInfo.offset + 3);
  }

  if (dx == 0.f) {
    if (FARRAY(c, GR_VERTEX_X_OFFSET) >  FARRAY(a, GR_VERTEX_X_OFFSET))
      sense = aaEdgeSenseLeft;
    else
      sense =  aaEdgeSenseRight;
  }
  else if (dy == 0.0f) {
    if (FARRAY(c, GR_VERTEX_Y_OFFSET) > FARRAY(a, GR_VERTEX_Y_OFFSET))
      sense = aaEdgeSenseBottom;
    else
      sense = aaEdgeSenseTop;
  }
  else {
    float dcax, dcay, dcbx, dcby, cp;
    dcax = FARRAY(a, GR_VERTEX_X_OFFSET) - FARRAY(c, GR_VERTEX_X_OFFSET);
    dcay = FARRAY(a, GR_VERTEX_Y_OFFSET) - FARRAY(c, GR_VERTEX_Y_OFFSET);
    dcbx = FARRAY(b, GR_VERTEX_X_OFFSET) - FARRAY(c, GR_VERTEX_X_OFFSET);
    dcby = FARRAY(b, GR_VERTEX_Y_OFFSET) - FARRAY(c, GR_VERTEX_Y_OFFSET);
    cp = dcax * dcby - dcay * dcbx;

    if (dx > 0.0f) {
      if (dx >= -dy)    /* X-major line */
	sense = (cp > 0) ? aaEdgeSenseTop : aaEdgeSenseBottom;
      else              /* Y-major line */
	sense = (cp > 0) ? aaEdgeSenseRight : aaEdgeSenseLeft;
    }
    else {
      if (dx <= dy)     /* X-major line */
	sense = (cp < 0) ? aaEdgeSenseTop : aaEdgeSenseBottom;
      else              /* Y-major line */
	sense = (cp < 0) ? aaEdgeSenseLeft : aaEdgeSenseRight;
    }
  }

  switch (sense) {
  case aaEdgeSenseTop:
    /* pci comamnd 14 + 2 * n */
    GR_SET_EXPECTED_SIZE(_GlideRoot.curTriSizeNoGradient << 1);
    GR_SETF( hw->FvA.x, FARRAY(a, GR_VERTEX_X_OFFSET));
    GR_SETF( hw->FvA.y, FARRAY(a, GR_VERTEX_Y_OFFSET));
    GR_SETF( hw->FvB.x, FARRAY(a, GR_VERTEX_X_OFFSET));
    GR_SETF( hw->FvC.x, FARRAY(b, GR_VERTEX_X_OFFSET));
    m = 1.0f / dx;
    GR_SETF( hw->FvB.y, FARRAY(a, GR_VERTEX_Y_OFFSET) + _GlideRoot.pool.f1);
    dp = a_a * dy;
    GR_SETF( hw->FvC.y, FARRAY(b, GR_VERTEX_Y_OFFSET) + _GlideRoot.pool.f1);
    
    grSetVertexParameter(a);
    
    GR_SETF( hw->Fdady, -a_a);
    dp *= m;
    GR_SETF( hw->Fdadx, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, dx ) );
      
    dp = b_a * dy;
    GR_SETF( hw->FvB.x, FARRAY(b, GR_VERTEX_X_OFFSET));
    dp = a_a - b_a + dp;
    GR_SETF( hw->FvB.y, FARRAY(b, GR_VERTEX_Y_OFFSET));

    grSetVertexParameter(a);
    
    GR_SETF( hw->Fdady, -b_a);
    dp *= m;
    GR_SETF( hw->Fdadx, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dx ) );
    
    break;
    
  case aaEdgeSenseLeft:
    /* pci comamnd 16 + 2 * n */
    GR_SET_EXPECTED_SIZE(8+(_GlideRoot.curTriSizeNoGradient << 1));
    GR_SETF( hw->FvA.y, FARRAY(a, GR_VERTEX_Y_OFFSET));
    GR_SETF( hw->FvB.x, FARRAY(a, GR_VERTEX_X_OFFSET));
    GR_SETF( hw->FvB.y, FARRAY(a, GR_VERTEX_Y_OFFSET));
    GR_SETF( hw->FvC.y, FARRAY(b, GR_VERTEX_Y_OFFSET));    
    m = 1.0f / dy;
    GR_SETF( hw->FvA.x, FARRAY(a, GR_VERTEX_X_OFFSET) - _GlideRoot.pool.f1);
    GR_SETF( hw->FvC.x, FARRAY(b, GR_VERTEX_X_OFFSET) - _GlideRoot.pool.f1);

    grSetVertexParameter(a);
    GR_SET(hw->Fa, 0);

    GR_SETF( hw->Fdadx, a_a);
    dp = - a_a * dx * m;
    GR_SETF( hw->Fdady, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dy ) );
    
    GR_SETF( hw->FvA.x, FARRAY(a, GR_VERTEX_X_OFFSET));
    dp = b_a * dx;
    GR_SETF( hw->FvB.x, FARRAY(b, GR_VERTEX_X_OFFSET));
    dp = a_a - dp - b_a;
    GR_SETF( hw->FvB.y, FARRAY(b, GR_VERTEX_Y_OFFSET));

    grSetVertexParameter(a);

    GR_SETF( hw->Fdadx, b_a);
    dp *= m;
    GR_SETF( hw->Fdady, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dy ) );
    
    break;
    
  case aaEdgeSenseBottom:
    
    if (dy < -1.0f) {
      /* pci comamnd 17 + 2 * n */
      GR_SET_EXPECTED_SIZE(12+(_GlideRoot.curTriSizeNoGradient << 1));
      GR_SETF( hw->FvA.x, FARRAY(a, GR_VERTEX_X_OFFSET));
      GR_SETF( hw->FvB.x, FARRAY(a, GR_VERTEX_X_OFFSET));
      GR_SETF( hw->FvB.y, FARRAY(a, GR_VERTEX_Y_OFFSET));
      GR_SETF( hw->FvC.x, FARRAY(b, GR_VERTEX_X_OFFSET));      
      m = 1.0f / dx;
      GR_SETF( hw->FvA.y, FARRAY(a, GR_VERTEX_Y_OFFSET) - _GlideRoot.pool.f1);
      GR_SETF( hw->FvC.y, FARRAY(b, GR_VERTEX_Y_OFFSET) - _GlideRoot.pool.f1);

      grSetVertexParameter(a);
      GR_SET(hw->Fa, 0);
      
      dp = a_a * dy;
      dp *= m;
      GR_SETF( hw->Fdadx, -dp);
      GR_SETF( hw->Fdady, a_a);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, dx ) );
    
      GR_SETF( hw->FvA.y, FARRAY(a, GR_VERTEX_Y_OFFSET));
      dp = b_a * dy;
      GR_SETF( hw->FvB.x, FARRAY(b, GR_VERTEX_X_OFFSET));
      GR_SETF( hw->FvB.y, FARRAY(b, GR_VERTEX_Y_OFFSET) - _GlideRoot.pool.f1);
      dp = a_a - b_a - dp;
      GR_SETF( hw->FvC.y, FARRAY(b, GR_VERTEX_Y_OFFSET));
      grSetVertexParameter(a);
      
      GR_SETF( hw->Fdady, b_a);
      dp *= m;
      GR_SETF( hw->Fdadx, dp);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dx ) );
    }
    else if (dy == 0.0f) {

      /* pci comamnd 16 + 2 * n */
      GR_SET_EXPECTED_SIZE(8+(_GlideRoot.curTriSizeNoGradient << 1));
      GR_SETF( hw->FvA.x, FARRAY(a, GR_VERTEX_X_OFFSET));
      GR_SETF( hw->FvC.x, FARRAY(b, GR_VERTEX_X_OFFSET));      
      GR_SETF( hw->FvC.y, FARRAY(b, GR_VERTEX_Y_OFFSET));
      GR_SETF( hw->FvB.x, FARRAY(b, GR_VERTEX_X_OFFSET));
      m = 1.0f / dx;
      GR_SETF( hw->FvA.y, FARRAY(a, GR_VERTEX_Y_OFFSET) - _GlideRoot.pool.f1);
      GR_SETF( hw->FvB.y, FARRAY(b, GR_VERTEX_Y_OFFSET) - _GlideRoot.pool.f1);

      grSetVertexParameter(a);
      GR_SET(hw->Fa, 0);

      GR_SET( hw->Fdadx, 0);
      GR_SETF( hw->Fdady, b_a);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dx ) );
    
      GR_SETF( hw->FvB.x, FARRAY(a, GR_VERTEX_X_OFFSET));
      GR_SETF( hw->FvB.y, FARRAY(a, GR_VERTEX_Y_OFFSET));
      grSetVertexParameter(a);
      GR_SET(hw->Fa, 0);
    
      dp = a_a - b_a;
      GR_SETF( hw->Fdady, a_a);
      dp *= m;
      GR_SETF( hw->Fdadx, dp);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, dx ) );
    }
    else {

      /* pci comamnd 20 + 2 * n */
      GR_SET_EXPECTED_SIZE(24+(_GlideRoot.curTriSizeNoGradient << 1));
      GR_SETF( hw->FvA.x, FARRAY(a, GR_VERTEX_X_OFFSET));
      GR_SETF( hw->FvB.x, FARRAY(b, GR_VERTEX_X_OFFSET));
      GR_SETF( hw->FvC.x, FARRAY(a, GR_VERTEX_X_OFFSET));      
      GR_SETF( hw->FvC.y, FARRAY(a, GR_VERTEX_Y_OFFSET));
      m = 1.0f / dx;
      GR_SETF( hw->FvA.y, FARRAY(a, GR_VERTEX_Y_OFFSET) - _GlideRoot.pool.f1);
      GR_SETF( hw->FvB.y, FARRAY(b, GR_VERTEX_Y_OFFSET) - _GlideRoot.pool.f1);

      grSetVertexParameter(a);
      GR_SET(hw->Fa, 0);
      
      dp = -a_a * dy * m;
      GR_SETF( hw->Fdadx, dp);
      GR_SETF( hw->Fdady, a_a);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, -dx ) );

      GR_SETF( hw->FvA.x, FARRAY(b, GR_VERTEX_X_OFFSET));
      GR_SETF( hw->FvA.y, FARRAY(b, GR_VERTEX_Y_OFFSET) - _GlideRoot.pool.f1);
      GR_SETF( hw->FvB.x, FARRAY(a, GR_VERTEX_X_OFFSET));
      GR_SETF( hw->FvB.y, FARRAY(a, GR_VERTEX_Y_OFFSET));
      dp = b_a * dy;
      GR_SETF( hw->FvC.x, FARRAY(b, GR_VERTEX_X_OFFSET));      
      GR_SETF( hw->FvC.y, FARRAY(b, GR_VERTEX_Y_OFFSET));

      grSetVertexParameter(b);
      GR_SET(hw->Fa, 0);
      dp = a_a - dp - b_a;
      GR_SETF( hw->Fdady, b_a);
      dp *= m;
      GR_SETF( hw->Fdadx, dp);
      P6FENCE_CMD( GR_SETF( hw->triangleCMD, dx ) );
    }
    
    break;
  case aaEdgeSenseRight:
    /* pci comamnd 16 + 2 * n */
    GR_SET_EXPECTED_SIZE(8+(_GlideRoot.curTriSizeNoGradient << 1));
    GR_SETF( hw->FvA.y, FARRAY(a, GR_VERTEX_Y_OFFSET));
    GR_SETF( hw->FvB.x, FARRAY(a, GR_VERTEX_X_OFFSET));
    GR_SETF( hw->FvB.y, FARRAY(a, GR_VERTEX_Y_OFFSET));
    GR_SETF( hw->FvC.y, FARRAY(b, GR_VERTEX_Y_OFFSET));
    m = 1.0f / dy;
    GR_SETF( hw->FvA.x, FARRAY(a, GR_VERTEX_X_OFFSET) + _GlideRoot.pool.f1);
    GR_SETF( hw->FvC.x, FARRAY(b, GR_VERTEX_X_OFFSET) + _GlideRoot.pool.f1);

    grSetVertexParameter(a);
    GR_SET(hw->Fa, 0);

    dp = a_a * dx;
    GR_SETF( hw->Fdadx, -a_a);
    dp *= m;
    GR_SETF( hw->Fdady, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, dy ) );
    
    GR_SETF( hw->FvA.x, FARRAY(a, GR_VERTEX_X_OFFSET));
    dp = b_a * dx;
    GR_SETF( hw->FvB.x, FARRAY(b, GR_VERTEX_X_OFFSET));
    dp = dp + a_a - b_a;
    GR_SETF( hw->FvB.y, FARRAY(b, GR_VERTEX_Y_OFFSET));
    grSetVertexParameter(a);
    
    GR_SETF( hw->Fdadx, -b_a);
    dp *= m;
    GR_SETF( hw->Fdady, dp);
    P6FENCE_CMD( GR_SETF( hw->triangleCMD, dy ) );   
    break;
  }
  GR_END_SLOPPY();
} /* grAADrawTriEdge */

#if 0
/*
** aaCalculateSignOfSinTheta
**  
*/
float
aaCalculateSignOfSinTheta(GrVertex *a, GrVertex *b, GrVertex *c)
{
  float
    x1, y1, x2, y2;

  /*
     Now, we make two vectors: ab [x1 y1] and ac [x2 y2]
     Fromt the cross product, we know that
     
     x1*y2 - x2*y1 = |ab|*|ac|*sin(theta), where theta is the angle
     between the two lines.  If sin(theta) > 0, c is above the line ab,
     and if it's negative, it's below ab.
     */
  x1 = b->x - a->x;
  y1 = b->y - a->y;
  
  x2 = c->x - a->x;
  y2 = c->y - a->y;

  return ((x1 * y2) - (x2 * y1));
  
} /* aaCalculateSignOfSinTheta */
#endif


/*---------------------------------------------------------------------------
** grAADrawTriangle
**
**  NOTE:  This doesn't quite work yet
*/
GR_ENTRY(grAADrawTriangle, void, (const void *a, const void *b, const void *c, FxBool ab_antialias, FxBool bc_antialias, FxBool ca_antialias ) )
{
  FxU32
    fbzMode,                    /* What we write to fbzMode */
    fbzModeOld;                 /* Squirrel away current fbzMode */

  GR_BEGIN_NOFIFOCHECK("grAADrawTriangle",96);
  GDBG_INFO_MORE((gc->myLevel,"(0x%x,0x%x,0x%x,%d,%d,%d)\n",
                  a,b,c,ab_antialias,bc_antialias,ca_antialias));

  fbzModeOld = gc->state.fbi_config.fbzMode;
  if ( TRISETUP ( a, b, c ) <= 0) {/* backfaced or zero area */
    GR_END();
    return;
  }
  /* Disable depth buffer writes for edge triangles */
  fbzMode = fbzModeOld;
  fbzMode &= ~(SST_ZAWRMASK);
  GR_SET_EXPECTED_SIZE(4);
  GR_SET( hw->fbzMode, fbzMode );
  GR_CHECK_SIZE();

  if ( ab_antialias ) {
    grAADrawTriEdgeSense(a, b, c);
  }
  
  if ( bc_antialias ) {
    grAADrawTriEdgeSense(b, c, a);
  }
  
  if ( ca_antialias ) {
    grAADrawTriEdgeSense(c, a, b);
  }

  /* Restore the old fbzMode */
  GR_SET_EXPECTED_SIZE(4);
  GR_SET( hw->fbzMode, fbzModeOld );
  GR_END();
} /* grAADrawTriangle */
