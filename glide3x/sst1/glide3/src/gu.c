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
** Revision 1.1.1.1  1999/12/07 21:48:53  joseph
** Initial checkin into SourceForge.
**
 * 
 * 12    12/19/97 8:09a Peter
 * fog table propogation
 * 
 * 11    3/09/97 10:31a Dow
 * Added GR_DIENTRY for di glide functions
 * 
 * 10    3/05/97 9:36p Jdt
 * Added guEncodeRLE16
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
#include "fxinline.h"

#if ( (GLIDE_PLATFORM & GLIDE_SST_HW) && (GLIDE_PLATFORM & GLIDE_HW_SST1) )
#include <sst1init.h>
#endif

/*---------------------------------------------------------------------------
** guFogTableIndexToW
*/
GR_DIENTRY(guFogTableIndexToW, float, ( int i ))
{
  return (float)pow(2.0,3.0+(double)(i>>2)) / (8-(i&3));
}

/*---------------------------------------------------------------------------
** guFogGenerateExp
*/
GR_DIENTRY(guFogGenerateExp, void,
           ( GrFog_t fogtable[kInternalFogTableEntryCount], float density )) 
{
  int   i;
  float f;
  float scale;
  float dp;

  GDBG_INFO((99,"guFogGenerateExp(0x%x,%g)\n",fogtable,density));
  dp = density * guFogTableIndexToW( kInternalFogTableEntryCount - 1 );
  scale = 1.0F / ( 1.0F - ( float ) exp( -dp ) );

  for ( i = 0; i < kInternalFogTableEntryCount; i++ )
  {
     dp = density * guFogTableIndexToW( i );
     f = ( 1.0F - ( float ) exp( -dp ) ) * scale;

     if ( f > 1.0F )
        f = 1.0F;
     else if ( f < 0.0F )
        f = 0.0F;

     f *= 255.0F;
     fogtable[i] = ( GrFog_t ) f;
  }
} /* guFogGenerateExp */

/*---------------------------------------------------------------------------
** guFogGenerateExp2
*/
GR_DIENTRY(guFogGenerateExp2, void,
           ( GrFog_t fogtable[kInternalFogTableEntryCount], float density ))
{
  int   i;
  float f;
  float scale;
  float dp;

  GDBG_INFO((99,"guFogGenerateExp2(0x%x,%g)\n",fogtable,density));
  dp = density * guFogTableIndexToW( kInternalFogTableEntryCount - 1 );
  scale = 1.0F / ( 1.0F - ( float ) exp( -( dp * dp ) ) );

  for ( i = 0; i < kInternalFogTableEntryCount; i++ )
  {
     dp = density * guFogTableIndexToW( i );
     f = ( 1.0F - ( float ) exp( -( dp * dp ) ) ) * scale;

     if ( f > 1.0F )
        f = 1.0F;
     else if ( f < 0.0F )
        f = 0.0F;

     f *= 255.0F;
     fogtable[i] = ( GrFog_t ) f;
  }
} /* guFogGenerateExp2 */

/*---------------------------------------------------------------------------
** guFogGenerateLinear
*/
GR_DIENTRY(guFogGenerateLinear, void, 
           ( GrFog_t fogtable[kInternalFogTableEntryCount],
            float nearZ, float farZ ))
{
   int i;
   float world_w;
   float f;

  GDBG_INFO((99,"guFogGenerateLinear(0x%x,%g,%g)\n",fogtable,nearZ,farZ));
   for ( i = 0; i < kInternalFogTableEntryCount; i++ )
   {
      world_w = guFogTableIndexToW( i );
      if ( world_w > 65535.0F )
         world_w = 65535.0F;

      f = ( world_w - nearZ ) / ( farZ - nearZ );
      if ( f > 1.0F )
         f = 1.0F;
      else if ( f < 0.0F )
         f = 0.0F;
      f *= 255.0F;
      fogtable[i] = ( GrFog_t ) f;
   }
} /* guFogGenerateLinear */
