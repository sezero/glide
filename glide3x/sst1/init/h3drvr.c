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
**
** $Revision$ 
** $Date$ 
**
*/

#include <3dfx.h>
#include <sst1init.h>
#include "init.h"
#include "fxinit.h"

static FxBool setVideo( FxU32                   hWnd,
                        GrScreenResolution_t    sRes,
                        GrScreenRefresh_t       vRefresh,
                        InitColorFormat_t       cFormat,
                        InitOriginLocation_t    yOrigin,
                        int                     nColBuffers,
                        int                     nAuxBuffers,
                        int                     *xres,
                        int                     *yres,
                        int                     *fbStride,
                        sst1VideoTimingStruct   *vidTimings) {
    return FXFALSE;
}

static void restoreVideo( void ) {
}

static FxBool enableTransport( InitFIFOData *info ) {
    FxBool rv = FXFALSE;
    return rv;
}

static void disableTransport( void ) {
}

static void swapBuffers( FxU32 code ) {
} 

static FxU32 status( void ) {
  return 0;
} 

static FxBool busy(void) {
  return 0;
} 

static void idle( void ) {
}

static void *getBufferPtr( InitBuffer_t buffer, int *strideBytes ) {
    return 0;
}

static void renderBuffer( InitBuffer_t buffer ) {
    return;
}

static void origin( InitOriginLocation_t origin ) {
    return;
}

static void ioCtl( FxU32 token, void *argument ) {
    return;
}

static FxBool control( FxU32 code ) {
    return FXFALSE;
}

static FxBool wrapFIFO(InitFIFOData *fd) {
    return FXTRUE;
}

static void h3gamma( double gamma ) {
}

static void sliPciOwner( FxU32 *regbase, FxU32 owner ) {
}

void h3DriverInit( InitContext *context ) {
    context->setVideo         = setVideo;
    context->restoreVideo     = restoreVideo;
    context->enableTransport  = enableTransport;
    context->disableTransport = disableTransport;
    context->swapBuffers      = swapBuffers;
    context->status           = status;
    context->busy             = busy;
    context->idle             = idle;
    context->getBufferPtr     = getBufferPtr;
    context->renderBuffer     = renderBuffer;
    context->origin           = origin;
    context->ioCtl            = ioCtl;
    context->control          = control;
    context->wrapFIFO         = wrapFIFO;


    context->gamma            = h3gamma;
    context->sliPciOwner      = sliPciOwner;
}
