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
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished  -
** rights reserved under the Copyright Laws of the United States.
**
**
** $Revision$ 
** $Date$ 
**
*/

#include <stdlib.h>
#include <stdio.h>
#ifndef __linux__
#include <conio.h>
#endif
#include <assert.h>
#include <string.h>

#include <glide.h>
#include "tlib.h"

#define RED_565    0xF800
#define GRN_565    0x07E0
#define BLU_565    0x001F
#define INVBLU_565 0xFFE0
#define INVGRN_565 0xF81F
#define INVRED_565 0x07FF
#define WHT_565    0xFFFF
#define BLK_565    0x0000

#define NUM_X_BLCKS 7
#define NUM_Y_BLCKS 7

GrHwConfiguration hwconfig;
static char version[80];

static const char name[]    = "QATEST01";
static const char purpose[] = "Buffer Swap Alliance bug";
static const char usage[]   = "-n <frames> -r <res> -d <filename>";


int main( int argc, char **argv) 
{
  /* Definitions */
  char match; 
  char **remArgs;
  int  rv;
  
  GrScreenResolution_t resolution = GR_RESOLUTION_640x480;
  float                scrWidth   = 640.0f;
  float                scrHeight  = 480.0f;
  int frames                      = -1;

  GrLfbInfo_t          myLfbInfo;
  int                  nBlckWdth;
  int                  nBlckHght;

  FxU16  yBlckClrs[NUM_Y_BLCKS][NUM_X_BLCKS] = 
    {{RED_565, GRN_565, BLU_565, INVRED_565, INVGRN_565, INVBLU_565, WHT_565},
     {RED_565, GRN_565, BLU_565, INVRED_565, INVGRN_565, INVBLU_565, WHT_565},
     {RED_565, GRN_565, BLU_565, INVRED_565, INVGRN_565, INVBLU_565, WHT_565},
     {RED_565, GRN_565, BLU_565, INVRED_565, INVGRN_565, INVBLU_565, WHT_565},
     {RED_565, GRN_565, BLU_565, INVRED_565, INVGRN_565, INVBLU_565, WHT_565},
     {RED_565, GRN_565, BLU_565, INVRED_565, INVGRN_565, INVBLU_565, WHT_565},
     {BLK_565, BLK_565, BLK_565, BLK_565,    BLK_565,    BLK_565,    BLK_565}};


  /* Initializations */
  nBlckWdth = ((int) scrWidth / NUM_X_BLCKS);
  nBlckHght = ((int) scrHeight / NUM_Y_BLCKS);
  
  /* Code */
    /* Process Command Line Arguments */
  while( (rv = tlGetOpt( argc, argv, "nrd", &match, &remArgs )) ) {
    if ( rv == -1 ) {
      printf( "Unrecognized command line argument\n" );
      printf( "%s %s\n", name, usage );
      printf( "Available resolutions:\n%s\n",
             tlGetResolutionList() );
      exit(1);
    }
    switch( match ) {
    case 'n':
      frames = atoi( remArgs[0] );
      break;
    case 'r':
      resolution = tlGetResolutionConstant( remArgs[0], 
                                           &scrWidth, 
                                           &scrHeight );
      break;
    }
  }
  
  tlSetScreen( scrWidth, scrHeight );
  
  grGlideGetVersion( version );
  
  printf( "%s:\n%s\n", name, purpose );
  printf( "%s\n", version );
  printf( "Resolution: %s\n", tlGetResolutionString( resolution ) );
  if ( frames == -1 ) {
    printf( "Press A Key To Begin Test.\n" );
    tlGetCH();
  }
  
  /* Initialize Glide */
  grGlideInit();
  assert( grSstQueryHardware( &hwconfig ) );
  grSstSelect( 0 );
  assert( grSstWinOpen( 0, resolution, GR_REFRESH_60Hz, GR_COLORFORMAT_ABGR,
                        GR_ORIGIN_UPPER_LEFT, 2, 1 ) );
  
  myLfbInfo.size = sizeof(GrLfbInfo_t);
  if ( !grLfbLock(GR_LFB_WRITE_ONLY, GR_BUFFER_BACKBUFFER, GR_LFBWRITEMODE_565, 
                  GR_ORIGIN_UPPER_LEFT, FXFALSE, &myLfbInfo))
    frames=0;
  
  while ( frames-- )
  {
    int i, j, k, l;
    FxU16 *lpPxl;
    FxU32 stride = myLfbInfo.strideInBytes;
    void  *tmpLfb;
    
    tmpLfb = (void*) ((FxU16*)myLfbInfo.lfbPtr + 1 + 2*stride/2);
    grBufferClear( 0xffffff, 0, GR_WDEPTHVALUE_FARTHEST );
    
    for (i=0; i < NUM_Y_BLCKS; ++i) {
      for (j=0; j < NUM_X_BLCKS; ++j) {
        lpPxl = ((FxU16 *) tmpLfb + j*nBlckWdth) + 
                  (i*nBlckHght*stride/2);
        for(k=0; k < nBlckHght; ++k) {
          for (l=0; l < nBlckWdth; ++l) {
            *lpPxl = yBlckClrs[i][j];
            ++lpPxl;
          }
          lpPxl += (stride/2 - nBlckWdth);  /* inc in pixel space */
        }
      }
    }
   
    grBufferSwap( 1 );
    if (hwconfig.SSTs[0].type == GR_SSTTYPE_SST96) {
      tlGetDimsByConst(resolution,
                       &scrWidth, 
                       &scrHeight );
        
      grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);
    } 

    if ( tlKbHit() ) frames = 0;
  }
  grLfbUnlock( GR_LFB_WRITE_ONLY, GR_BUFFER_BACKBUFFER ); 
  grGlideShutdown();
  exit(0);
}
