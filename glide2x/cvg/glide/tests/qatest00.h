/*
** Copyright (c) 1997, 3Dfx Interactive, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of 3Dfx Interactive, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of 3Dfx Interactive, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
**
*/

#ifndef _QATEST00_H_
#define _QATEST00_H_

#ifdef __cplusplus
extern "C" {
#endif

#define RED_565   0xF800
#define GRN_565   0x07E0
#define BLU_565   0x001F
#define RED_1555  0x7C00
#define GRN_1555  0x03E0
#define BLU_1555  0x001F
#define RED_4444  0x0F00
#define GRN_4444  0x00F0
#define BLU_4444  0x000F
#define RED_332   0x00E0
#define GRN_332   0x001C
#define BLU_332   0x0003
#define RED_8332  0x00E0
#define GRN_8332  0x001C
#define BLU_8332  0x0003

#define RED   0x000000ff
#define BLUE  0x00ff0000

typedef enum { DISABLE, NEAREST, TRILINEAR, DITHRD_NREST, DITHRD_TRILIN } MipMapMode_t;
typedef enum { SCALE_IN=1, SCALE_OUT=-1 } ScaleDir_t; //sign
typedef enum { SCALE_1=0, SCALE_2, SCALE_4, SCALE_8, SCALE_16, SCALE_32 } ScaleAmt_t;//bitwise shift factor
typedef enum { BOTH_POINT, MIN_BILNEAR, MAG_BILNEAR, BOTH_BILNEAR } FilterMode_t;

typedef struct {
  float    fStartX, fEndX;
  float    rStartX, rEndX;
  float    fStartY, fEndY;
  float    rStartY, rEndY;
  GrLOD_t  currLod;
} surfInfo_t;

typedef struct {
  const char	*lpzTexFileName;
  TlTexture   *lpTlTexture;
  hTexId_t    hTexture;
  surfInfo_t 	surfInfo;
}  myTexture_t;

typedef struct
{
  float sMult;
  float tMult;
} TexCoordFactors;


static void doHelp( void );
static hTexId_t doLoadTexture( const char *lpzFileName, GrTexInfo *tEtTexInfo, 
															 															FxU16 *yColors, FxBool isDwnldble );
static void doUnloadTextures( );
static void doScaleScreenSurf( hTexId_t deTexId, ScaleDir_t deDir, ScaleAmt_t deAmt );
static void getFgVerts( hTexId_t deTexId, GrVertex *deFgVerts );
static int getLodWidth( GrLOD_t lod, GrAspectRatio_t aspect );
static int getLodHeight( GrLOD_t lod, GrAspectRatio_t aspect );
static void getLodToDwnld( );
static void getLodPrtlToDwnld( );
static void doResetFgVerts( hTexId_t deTexId );

#ifdef NDEBUG
#undef NDEBUG
#endif

#ifdef __cplusplus
}
#endif
#endif
