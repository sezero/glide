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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef __linux__
#include <conio.h>
#endif
#include <assert.h>
#include <string.h>

#include <glide.h>
#include "tlib.h"


GrHwConfiguration hwconfig;
static char version[80];

static const char name[]    = "test16";
static const char purpose[] = "test grShamelessPlug and grSplash";
static const char usage[]   = "-n <frames> -r <res> -d <filename>";

void main( int argc, char **argv) {
    char match; 
    char **remArgs;
    int  rv;

    GrScreenResolution_t resolution = GR_RESOLUTION_640x480;
    float                scrWidth   = 640.0f;
    float                scrHeight  = 480.0f;
    int frames                      = -1;
    FxBool               scrgrab = FXFALSE;
    char                 filename[256];

    /* Process Command Line Arguments */
    while( rv = tlGetOpt( argc, argv, "nrd", &match, &remArgs ) ) {
        if ( rv == -1 ) {
            printf( "Unrecognized command line argument\n" );
            printf( "%s %s\n", name, usage );
            printf( "Available resolutions:\n%s\n",
                    tlGetResolutionList() );
            return;
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
        case 'd':
            scrgrab = FXTRUE;
            frames = 1;
            strcpy(filename, remArgs[0]);
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
    assert( grSstWinOpen( 0,
                      resolution,
                      GR_REFRESH_60Hz,
                      GR_COLORFORMAT_ABGR,
                      GR_ORIGIN_UPPER_LEFT,
                      2, 1 ) );
    
    tlConSet( 0.0f, 0.0f, 1.0f, 0.5f, 
              60, 15, 0xffffff );

    /* Set up Render State - flat shading  */
    grColorCombine( GR_COMBINE_FUNCTION_LOCAL,
                    GR_COMBINE_FACTOR_NONE,
                    GR_COMBINE_LOCAL_CONSTANT,
                    GR_COMBINE_OTHER_NONE,
                    FXFALSE );
    
#define BLUE  0x00ff0000
#define RED   0x000000ff
#define BLACK 0x00000000
    
    tlConOutput( "p - toggle shameless plug on/off\n" );
    tlConOutput( "s - run the splash screen\n" );
    tlConOutput( "r - render splash continuously\n" );
    tlConOutput( "Press any other key to quit\n" );
    while( frames-- && tlOkToRender()) {
        int x, y;
        GrVertex vtxA, vtxB, vtxC, vtxD;
        static int plug;
        static int render;
        static int frame = 1;

        if (hwconfig.SSTs[0].type == GR_SSTTYPE_SST96) {
          tlGetDimsByConst(resolution,
                           &scrWidth, 
                           &scrHeight );
        
          grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);
        }

        grBufferClear( BLACK, 0, GR_ZDEPTHVALUE_FARTHEST );

        /* Draw 10x10 grid of triangles */
        for( y = 0; y < 10; y++ ) {
            for( x = 0; x < 10; x++ ) {
                /* 
                   A-D
                   |\|
                   B-C
                 */
                vtxA.x = vtxB.x = tlScaleX( ((float)x)/10.0f );
                vtxA.y = vtxD.y = tlScaleY( ((float)y)/10.0f );
                vtxB.y = vtxC.y = tlScaleY( (((float)y)/10.0f) + 0.1f );
                vtxC.x = vtxD.x = tlScaleX( (((float)x)/10.0f) + 0.1f );
                
                grConstantColorValue( RED );
                grDrawTriangle( &vtxA, &vtxB, &vtxC );
                grConstantColorValue( BLUE );
                grDrawTriangle( &vtxA, &vtxC, &vtxD );
            }
        }

        
        if ( render ) {
            grSplash( tlScaleX( 0.0f ), tlScaleY( 0.79f ), 
                      tlScaleX( 0.2f ), tlScaleY( 0.2f ),
                      frame );
            frame++;
        }
        tlConRender();
        grBufferSwap( 1 );
        grSstIdle();

        /* grab the frame buffer */
        if (scrgrab) {
          if (!tlScreenDump(filename, (FxU16)scrWidth, (FxU16)scrHeight))
            printf( "Cannot open %s\n", filename);
          scrgrab = FXFALSE;
        }

        
        while( tlKbHit() ) {
            switch( tlGetCH() ) {
            case 'p':
            case 'P':
                plug = !plug;
                grGlideShamelessPlug( plug );
                break;
            case 's':
            case 'S':
                grSplash( 0.0f, 0.0f, scrWidth, scrHeight, 0 );
                break;
            case 'r':
            case 'R':
                render = !render;
                break;
            default:
                frames = 0;
                break;
            }
        }
    }
    
    grGlideShutdown();
    return;
}
