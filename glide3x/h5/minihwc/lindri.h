/*
** THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
** PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
** TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
** INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE
** DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com).
** THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
** EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR 
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
*/
#ifndef _LINDRI_H_
#define _LINDRI_H_

typedef struct _XF86DRIClipRect {
    unsigned short	x1; /* Upper left: inclusive */
    unsigned short	y1;
    unsigned short	x2; /* Lower right: exclusive */
    unsigned short	y2;
} XF86DRIClipRectRec, *XF86DRIClipRectPtr;

#define MAX_CHIPS 4

typedef unsigned long drmHandle,   *drmHandlePtr;   /* To mapped regions    */
typedef unsigned int  drmSize,     *drmSizePtr;	    /* For mapped regions   */
typedef void          *drmAddress, **drmAddressPtr; /* For mapped regions   */

typedef struct {
  drmHandle handle;
  drmSize size;
  drmAddress map;
} tdfxRegion, *tdfxRegionPtr;

typedef struct {
  tdfxRegion regs[MAX_CHIPS];
  int deviceID;
  int width;
  int height;
  int mem;
  int cpp;
  int stride;
  int fifoOffset;
  int fifoSize;
  int fbOffset;
  int backOffset;
  int depthOffset;
  int textureOffset;
  int textureSize;
  int numChips;
  int numSamples;
  void *driScrnPriv;
} tdfxScreenPrivate;

typedef struct dri_t {
  char *pFB;
  tdfxScreenPrivate *sPriv;
  int screenWidth;
  int screenHeight;
  int cpp;
  int x;
  int y;
  int w;
  int h;
  int stride;
  int windowedStride;
  int fullScreenStride;
  int numClip;
  XF86DRIClipRectPtr pClip;
  volatile int *fifoPtr;
  volatile int *fifoRead;
  int fullScreenPixFmt;
  int windowedPixFmt; 
  int sliCount;
  int isFullScreen;
} DRIDef;

extern DRIDef driInfo;

#endif
