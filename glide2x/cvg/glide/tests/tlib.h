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


#ifndef _TLIB_H_
#define _TLIB_H_
#ifdef __cplusplus
extern "C" {
#endif


/* If not debugging, change the meaning of the ANSI assert
 * so that it is a harmless wrapper rather than ((void)0) as in <assert.h>
 */
#ifndef DEBUG
#ifdef assert
#undef assert
#endif
#define assert(exp) (exp)
#endif

int tlGetOpt( int argc, char *argv[], const char *tags, char *match, char **remArgs[] );

int tlGetResolutionConstant( const char *identifier, float *width, float *height );
void tlGetDimsByConst(const int res, float *w, float *h);
const char *tlGetResolutionString( int res );
const char *tlGetResolutionList( void );

void  tlSetScreen( float width, float height );
float tlScaleX( float coord );
float tlScaleY( float coord );

void tlConSet( float minX, float minY, 
               float maxX, float maxY,
               int columns, int rows,
               int color );
int  tlConOutput( const char *fmt, ... );
void tlConClear();
void tlConRender();

int  tlKbHit( void );
char tlGetCH( void );

void tlSleep( int seconds );

#define LFB_DATA_LITERAL       0
#define LFB_DATA_RLE           1
#define IMAGE_SRLE             (((FxU32)'S' << 24) | ((FxU32)'R' << 16) | ((FxU32)'L' << 8) | ((FxU32)'E'))

FxBool SimpleRleDecode(FxU16 width,FxU16 height,FxU8 pixelsize, FxU8 *mem,FxU8 *buff);
void tlGrabRect(void *memory, FxU32 minx, FxU32 miny, FxU32 maxx, FxU32 maxy); 
FxBool tlScreenDump(const char *filename, FxU16 width,  FxU16 height);

typedef struct {
    float x;
    float y;
    float z;
    float w;
    float s;
    float t;
    float r;
    float g;
    float b;
    float a;
} TlVertex3D;

typedef float TlMatrix[4][4];

const float *tlIdentity( void );
const float *tlZRotation( float degrees );
const float *tlYRotation( float degrees );
const float *tlXRotation( float degrees );
const float *tlTranslation( float x, float y, float z );
void tlSetMatrix( const float *m );
void tlMultMatrix( const float *m );
void tlTransformVertices( TlVertex3D *dstList, 
                          TlVertex3D *srcList, 
                          unsigned length );
void tlProjectVertices( TlVertex3D *dstList,
                        TlVertex3D *srcList, 
                        unsigned length );

FxBool tlOkToRender(void);

FxBool
tlErrorMessage(char *err);  

typedef FxU32 TlPalette[256];
typedef struct {
  FxU8  yRGB[16];
  FxI16 iRGB[4][3];
  FxI16 qRGB[4][3];
  FxU32 packed_data[12];
} TlNCCTable;

typedef union {
    TlPalette  palette;
    TlNCCTable nccTable;
} TlTextureTable;

typedef struct {
    GrTexInfo      info;
    GrTexTable_t   tableType;
    TlTextureTable tableData;
} TlTexture;

#define NO_TABLE ((GrTexTable_t)(~0))

int tlLoadTexture( const char *filename, 
                   GrTexInfo *info, 
                   GrTexTable_t *tableType,
                   void *table );

/* We are making function calls from within assert() */
#ifdef NDEBUG
#undef NDEBUG
#endif

#ifdef __cplusplus
}
#endif
#endif
