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
** 
** 31    6/02/98 8:03p Peter
** Mmmmm.... points
 * 
 * 30    9/07/97 1:52p Atai
 * performance tuning for previous check-in
 * 
 * 29    9/05/97 7:20p Atai
 * fixed bug 764 (grDrawPoint integer snapping).
 * 
 * 28    6/20/97 5:51p Dow
 * Moved grDebugGroupWriteHEader to sst96.c
 * 
 * 27    5/27/97 11:37p Pgj
 * Fix for Bug report 545
 * 
 * 26    5/27/97 2:00p Dow
 * Parenthesized a ternary op in GR_BEGIN to work around a Watcom compiler
 * bug.
 * 
 * 25    3/21/97 12:42p Dow
 * Made STWHints not send the Bend Over Baby Packet to FBI Jr.
 * 
 * 24    3/04/97 9:08p Dow
 * Neutered multiplatform multiheaded monster
 * 
 * 23    2/18/97 9:51a Jdt
 * Ifdefed out some if(0) code to supress watcom warnings
 * 
 * 22    12/23/96 1:37p Dow
 * chagnes for multiplatform glide
 * 
 * 21    11/18/96 12:13p Jdt
 * Made all debugging code go through the C-call to the assembly trisetup
 * code.
 * 
 * 20    11/14/96 11:52p Jdt
 * Fixed Watcom Assembly calling bug
**
*/
#include <3dfx.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

#include <memory.h>

#define SST_XY_HALF      ( 1 << ( SST_XY_FRACBITS - 1 ) )
#define SST_XY_ONE       ( 1 << SST_XY_FRACBITS )

#define OUTBOUNDSX(a) ((FARRAY(a, GR_VERTEX_X_OFFSET) < 0.f) || (FARRAY(a, GR_VERTEX_X_OFFSET) > gc->state.screen_width))
#define OUTBOUNDSY(a) ((FARRAY(a, GR_VERTEX_Y_OFFSET) < 0.f) || (FARRAY(a, GR_VERTEX_Y_OFFSET) > gc->state.screen_height))
#define OUTBOUNDS(a) (OUTBOUNDSX(a) || OUTBOUNDSY(a))

/* access a floating point array with a byte index */
#define FARRAY(p,i) (*(float *)((i)+(int)(p)))
/* access a byte array with a byte index and convert to float */
#define FbARRAY(p,i) (float)(((unsigned char *)p)[i])

/* X and Y have fixed position. These represent offsets when
 * the vertex structure is seen as an array of bytes
 */
#define GR_VERTEX_X_OFFSET 0
#define GR_VERTEX_Y_OFFSET 4

/*---------------------------------------------------------------------------
  NOTE: by Gary Tarolli
  
  The following code is very carefully scheduled for MSVC4.2 Version 10.20.6166
  The trick is to schedule operations between PCI writes (GR_SET*).
  This is because PCI writes cannot get off the CPU chip quickly and there
  are about 6-12 dead clocks between back-to-back writes (P-90 to P-166).
  So we try to do useful work during this time - this makes the code hard to
  read because simple operations like mem++ may be broken up into three lines
  of C code to perform, to allow the compiler to move the pieces around more.
  PLEASE PLEASE PLEASE don't touch this code without benchmarkng it before
  and after you touch it.  Moving a line of code up/down changes performance!
  ---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------
** grDrawPoint
*/

GR_ENTRY(grDrawPoint, void, ( const void *p ))
{
  int i, x,y;
  struct dataList_s *dlp;

  /* GMT: gross overestimate of fifo requirements */
  GR_BEGIN("grDrawPoint",90,_GlideRoot.curTriSize);
  GDBG_INFO_MORE((gc->myLevel,"(0x%x)\n",p));\

  /* [dBorca] jump to anti-aliased function if GR_AA_ORDERED */
  if (gc->state.grEnableArgs.primitive_smooth_mode & GR_AA_ORDERED_POINTS_MASK) {
     _grAADrawPoint(p);
     return;
  }

  /* we snap to an integer by adding a large enough number that it
   * shoves all fraction bits off the right side of the mantissa.
   *
   * NB: IEEE rounds to nearest integer by default, but applications
   * can change the rounding mode so that it is difficult to get the
   * correct truncation/ceiling operation w/ a simple adjustment to
   * the bias. 
   *
   * NB: The constant kNumMantissaBits defines how many bits of
   * integer precision a coordinate can have. This needs to be atleast
   * as large as the maximum hw screen resolution. We later use this
   * to compute a logical 1/2 value to fill an entire pixel.
   */
#define kNumMantissaBits 18UL
  {
    const float bias  = (const float)(3UL << kNumMantissaBits);

    /* Convert to 32-bit representation */
#define FP_TRUNC_BIAS(__fpVal, __fpBias) \
    ((__fpVal) < (__fpBias) ? (float)((__fpVal) + (__fpBias)) : (__fpVal))
    _GlideRoot.pool.ftemp1 = FP_TRUNC_BIAS(FARRAY(p, GR_VERTEX_X_OFFSET), bias);
    _GlideRoot.pool.ftemp2 = FP_TRUNC_BIAS(FARRAY(p, GR_VERTEX_Y_OFFSET), bias);

    /* Mask off the real fractional bits from the mantissa */
    x = ((*(FxU32*)&_GlideRoot.pool.ftemp1 & (0xFFFFFFFFUL << (22UL - kNumMantissaBits))) +
         (0x01UL << (21UL - kNumMantissaBits)));
    y = ((*(FxU32*)&_GlideRoot.pool.ftemp2 & (0xFFFFFFFFUL << (22UL - kNumMantissaBits))) +
         (0x01UL << (21UL - kNumMantissaBits)));
  }

#ifdef GLIDE_USE_ALT_REGMAP
  hw = SST_WRAP(hw,128);                /* use alternate register mapping */
#endif

  _GlideRoot.stats.pointsDrawn++;
  
  /* draw a little triangle, with the lower left corner at pixel center */
  GR_SET( hw->vA.x, x );
  GR_SET( hw->vA.y, y );
  
  x += (0x01UL << (21UL - kNumMantissaBits));
  GR_SET( hw->vB.x, x );
  GR_SET( hw->vB.y, y );

  y += (0x01UL << (21UL - kNumMantissaBits));
  GR_SET( hw->vC.y, y );
  GR_SET( hw->vC.x, x );

  /* we don't care what the slopes are because the pixel center that is drawn */
  /* is exactly at vertex A - isn't that wonderful */
  dlp = gc->dataList;
  i = dlp->i;
  while (i) {
    if (i & 1) {        /* packer bug check */
      if (i & 2) P6FENCE;
      GR_SETF( *dlp->addr, 0.0F );
      if (i & 2) P6FENCE;
    }
    else if (i >= 0) {
      GR_SETF( *dlp->addr, FARRAY(p,i) );
    } else {
      /* [dBorca] Packed Color Workaround (tm) */
      i = (i & 0xffffff) + ((i >> 24) & 3);
      GR_SETF( *dlp->addr, FbARRAY(p,i) );
    }
    dlp++;
    i = dlp->i;
  }
  P6FENCE_CMD( GR_SET( hw->triangleCMD, 0x0000001UL) );

  GR_END_SLOPPY();
} /* grDrawPoint */

/*---------------------------------------------------------------------------
** grDrawLine
**
** NOTE: 1. this will not fill the last pixel in line because
**          B2 or C is on the right edge and the right edge is not
**          drawn.
** (0,0)
**
**  A(x1,y1-0.5)+
**              |  \
**              |  \  \
**       (x1,y1)*         \
**              |  *           \
**              |     *\           \
** B1(x1,y1+0.5)+         *             \
**                 \        \ *            +B2(x2,y2-0.5)
**                     \          *        |
**                        \       \    *   |
**                             \     \     +  (x2,y2)
**                                 \       |
**                                      \  |
**                                         +C(x2,y2+0.5)
*/

GR_ENTRY(grDrawLine, void, ( const void *a, const void *b ))
{
  float    m, dp;
  #define  DX _GlideRoot.pool.ftemp1
  #define ADY _GlideRoot.pool.ftemp2

  int i,j;
  float *fp;
  struct dataList_s *dlp;


  GR_BEGIN("grDrawLine",91,12+ _GlideRoot.curTriSize);
  GDBG_INFO_MORE((gc->myLevel,"(0x%x,0x%x)\n",a,b));

  /* [dBorca] jump to anti-aliased function if GR_AA_ORDERED */
  if (gc->state.grEnableArgs.primitive_smooth_mode & GR_AA_ORDERED_LINES_MASK) {
     _grAADrawLine(a, b);
     return;
  }

#ifdef GLIDE_USE_ALT_REGMAP
  hw = SST_WRAP(hw,128);                /* use alternate register mapping */
#endif
   /*
   ** compute absolute deltas and draw from low Y to high Y
   */
   ADY = FARRAY(b, GR_VERTEX_Y_OFFSET) - FARRAY(a, GR_VERTEX_Y_OFFSET);
   i = *(long *)&ADY;
   if ( i < 0 ) {
     const void *tv;
     tv = a; a = b; b = tv;
     i ^= 0x80000000;        /* ady = -ady; */
     (*(long *)&ADY) = i;
   }

   DX = FARRAY(b, GR_VERTEX_X_OFFSET) - FARRAY(a, GR_VERTEX_X_OFFSET);
   j = *(long *)&DX;
   if (j < 0 ) {
     j ^= 0x80000000;        /* adx = -adx; */
   }

   /*
   ** X major line
   */
   if (j >= i ) {                       /* if (adx > ady) */
     if (j == 0) goto all_done;         /* check for zero-length lines */
     /* start up divide and overlap with as much integer stuff as possible*/
     m = _GlideRoot.pool.f1 / DX;
       dlp = gc->dataList;
      GR_SETF(hw->FvA.x,FARRAY(a, GR_VERTEX_X_OFFSET));
       dp = FARRAY(b, GR_VERTEX_X_OFFSET);
      GR_SETF(hw->FvB.x,dp);
      GR_SETF(hw->FvC.x,dp)
       _GlideRoot.stats.linesDrawn++;
      
     GR_SETF(hw->FvA.y,FARRAY(a, GR_VERTEX_Y_OFFSET) - _GlideRoot.pool.fHalf);

     dp = FARRAY(b, GR_VERTEX_Y_OFFSET);
     GR_SETF(hw->FvB.y,dp - _GlideRoot.pool.fHalf);

      i = dlp->i;
     GR_SETF(hw->FvC.y,dp + _GlideRoot.pool.fHalf);

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
         dp = FARRAY(a,i);
         GR_SETF( fp[0], dp );
         dp = FARRAY(b,i) - dp;
         GR_SETF( fp[DPDX_OFFSET>>2] , dp * m );
         dlp++;
         i = dlp->i;
         GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
       }
       else {
         /* [dBorca] Packed Color Workaround (tm) */
         i = (i & 0xffffff) + ((i >> 24) & 3);
         dp = FbARRAY(a,i);
         GR_SETF( fp[0], dp );
         dp = FbARRAY(b,i) - dp;
         GR_SETF( fp[DPDX_OFFSET>>2] , dp * m );
         dlp++;
         i = dlp->i;
         GR_SETF( fp[DPDY_OFFSET>>2] , _GlideRoot.pool.f0 );
       }
     }
     P6FENCE_CMD( GR_SETF(hw->FtriangleCMD,_GlideRoot.pool.ftemp1) );

     GR_SETF(hw->FvB.x,FARRAY(a, GR_VERTEX_X_OFFSET));
     GR_SETF(hw->FvB.y,FARRAY(a, GR_VERTEX_Y_OFFSET) + _GlideRoot.pool.fHalf);
     P6FENCE_CMD( GR_SETF(hw->FtriangleCMD,-_GlideRoot.pool.ftemp1));
   }

   /*
   ** Y major line
   */
   else {
     m = _GlideRoot.pool.f1 / ADY;
     dlp = gc->dataList;
     GR_SETF(hw->FvA.y,FARRAY(a, GR_VERTEX_Y_OFFSET));
     dp = FARRAY(b, GR_VERTEX_Y_OFFSET);
     GR_SETF(hw->FvB.y,dp);
     _GlideRoot.stats.linesDrawn++;
     GR_SETF(hw->FvC.y,dp);
     
     GR_SETF(hw->FvA.x,FARRAY(a, GR_VERTEX_X_OFFSET) - _GlideRoot.pool.fHalf);

     dp = FARRAY(b, GR_VERTEX_X_OFFSET);
     GR_SETF(hw->FvB.x,dp - _GlideRoot.pool.fHalf);

     i = dlp->i;
     GR_SETF(hw->FvC.x,dp + _GlideRoot.pool.fHalf);
       
     while (i) {
       fp = dlp->addr;
       if (i & 1) {     /* packer bug check */
         if (i & 2) P6FENCE;
         GR_SETF( fp[0], 0.0f );
         if (i & 2) P6FENCE;
         dlp++;
         i = dlp->i;
       }
       else if (i >= 0) {
         dp = FARRAY(a,i);
         GR_SETF( fp[0], dp );
         dp = FARRAY(b,i) - dp;
         GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0 );
         dlp++;
         i = dlp->i;
         GR_SETF( fp[DPDY_OFFSET>>2] , dp * m );
       }
       else {
         /* [dBorca] Packed Color Workaround (tm) */
         i = (i & 0xffffff) + ((i >> 24) & 3);
         dp = FbARRAY(a,i);
         GR_SETF( fp[0], dp );
         dp = FbARRAY(b,i) - dp;
         GR_SETF( fp[DPDX_OFFSET>>2] , _GlideRoot.pool.f0 );
         dlp++;
         i = dlp->i;
         GR_SETF( fp[DPDY_OFFSET>>2] , dp * m );
       }
     }
     P6FENCE_CMD( GR_SET( hw->triangleCMD, 0xFFFFFFFF) );
     
     GR_SETF(hw->FvB.x,FARRAY(a, GR_VERTEX_X_OFFSET) + _GlideRoot.pool.fHalf);
     GR_SETF(hw->FvB.y,FARRAY(a, GR_VERTEX_Y_OFFSET));
     P6FENCE_CMD( GR_SET( hw->triangleCMD, 1) );
   }

  GR_END_SLOPPY();
  return;

all_done:  /* come here on degenerate lines */
  _GlideRoot.stats.linesDrawn++;
  GR_END_SLOPPY();
} /* grDrawLine */

/*---------------------------------------------------------------------------
** grDrawTriangle
*/

#if defined(GLIDE_USE_C_TRISETUP)
GR_ENTRY(grDrawTriangle, void, ( const void *a, const void *b, const void *c ))
{
  GR_BEGIN_NOFIFOCHECK("grDrawTriangle",92);
  GDBG_INFO_MORE((gc->myLevel,"(0x%x,0x%x,0x%x)\n",a,b,c));
  GR_CHECK_F(myName, !a || !b || !c, "NULL pointer passed");

  /* Silly warning killer */
  if ( 0 ) goto all_done;

#ifdef GLIDE_DEBUG
#if 0
if (0) {                        /* GMT: only use this if needed */
  FxU32 statBits;               /* bits we care about in status register */
  FxU32 status;

  status = GR_GET(hw->status);
  if ((statBits = status & SST_FIFOLEVEL) < _GlideRoot.stats.minPciFIFOFree)
    _GlideRoot.stats.minPciFIFOFree = statBits;

  statBits = (status >> SST_MEMFIFOLEVEL_SHIFT) & 0xffff;
  if (statBits < _GlideRoot.stats.minMemFIFOFree)
    _GlideRoot.stats.minMemFIFOFree = statBits;
}
#endif

  if (_GlideRoot.environment.triBoundsCheck) {
    if (OUTBOUNDS(a) || OUTBOUNDS(b) || OUTBOUNDS(c)) {
      GDBG_PRINTF(("Triangle out of bounds:\n"));
      GDBG_PRINTF(("a->x = %3.2f, a->y = %3.2f\n", FARRAY(a, GR_VERTEX_X_OFFSET), FARRAY(a, GR_VERTEX_Y_OFFSET)));
      GDBG_PRINTF(("b->x = %3.2f, b->y = %3.2f\n", FARRAY(b, GR_VERTEX_X_OFFSET), FARRAY(b, GR_VERTEX_Y_OFFSET)));
      GDBG_PRINTF(("c->x = %3.2f, c->y = %3.2f\n", FARRAY(c, GR_VERTEX_X_OFFSET), FARRAY(c, GR_VERTEX_Y_OFFSET)));
      GDBG_PRINTF(("Culling triangle based on these bogus values.\n"));
      goto all_done;
    }
  }
#endif /* GLIDE_DEBUG */

  /* _trisetup and _trisetup_asm return 0 if culled, 1 if drawn */ 
  TRISETUP( a, b, c );

all_done:
  GR_END();
} /* grDrawTriangle */
#endif

/*---------------------------------------------------------------------------
** _grColorCombineDelta0Mode
**
** GMT: when we are in delta0 mode, color comes from the RGB iterators
**      but the slopes are 0.0.  So when we enter delta0 mode we set
**      the iterators up and then we leave them alone during primitive
**      rendering
*/

GR_DDFUNC(_grColorCombineDelta0Mode, void, ( FxBool delta0mode ))
{
  GR_BEGIN("_grColorCombineDelta0Mode",85,(delta0mode ? 9*4 : 0));
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",delta0mode));

  if ( delta0mode ) {
    GR_SETF( hw->Fr, gc->state.r );
    GR_SETF( hw->Fg, gc->state.g );
    GR_SETF( hw->Fb, gc->state.b );
    GR_SET( hw->drdx, 0);
    GR_SET( hw->drdy, 0);
    GR_SET( hw->dgdx, 0);
    GR_SET( hw->dgdy, 0);
    GR_SET( hw->dbdx, 0);
    GR_SET( hw->dbdy, 0);
  }

  gc->state.cc_delta0mode = delta0mode;
  GR_END();

} /* _grColorCombineDeltaMode */
