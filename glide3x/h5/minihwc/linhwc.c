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


/*

  We need to provide the following functions:

char *
hwcGetErrorString(void);

hwcInfo *
hwcInit(FxU32 vID, FxU32 dID);

FxBool
hwcMapBoard(hwcBoardInfo *bInfo, FxU32 bAddrMask);

FxBool
hwcInitRegisters(hwcBoardInfo *bInfo);

FxBool
hwcAllocBuffers(hwcBoardInfo *bInfo, FxU32 nColBuffers, FxU32 nAuxBuffers); 

FxBool
hwcInitFifo(hwcBoardInfo *bInfo);

FxU32 
hwcInitAGPFifo(hwcBoardInfo *bInfo); 

FxBool
hwcInitVideo(hwcBoardInfo *bInfo, FxBool tiled, FxVideoTimingInfo
             *vidTiming, FxBool overlay);

FxBool
hwcRestoreVideo(hwcBoardInfo *bInfo);

FxBool
hwcCheckMemSize(hwcBoardInfo *bInfo, FxU32 xres, FxU32 yres, FxU32 nColBuffers,
                FxU32 nAuxBuffers, FxBool tiled);
*/

#include <minihwc.h>
#include <hwcio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/extensions/xf86dga.h>
#include <X11/extensions/xf86vmode.h>
#include "lindri.h"

static FxU32 fenceVar;
#define P6FENCE asm("xchg %%eax, %0" : : "m" (fenceVar) : "eax");

#define MAXFIFOSIZE     0x40000
#define FIFOPAD         0x0000
#define AUXPAD          0x1000
#define HWC_LFB_STRIDE     0x2000UL
#define HWC_TILED_BUFFER_BYTES    0x1000UL  /* 128 Bytes x 32 lines */
#define HWC_TILED_BUFFER_Y_ALIGN  0x20000UL
#define HWC_TILED_BUFFER_X_ADJUST 0x80UL

static hwcInfo hInfo;
static char errorString[1024];
static int finalVidDesktopOverlayStride;

static FxU32 calcBufferStride(hwcBoardInfo *bInfo, FxU32 xres, FxBool tiled);
static FxU32 calcBufferSize(hwcBoardInfo *bInfo, FxU32 xres, FxU32 yres, 
			    FxBool tiled);
static FxU32 calcBufferSizeInTiles(hwcBoardInfo *bInfo, FxU32 xres, 
				   FxU32 yres);
static FxU32 calcBufferHeightInTiles(hwcBoardInfo *bInfo, FxU32 yres);
static FxU32 hwcBufferLfbAddr(const hwcBoardInfo* bInfo, FxU32 physAddress);

typedef struct envitem_t {
  char *env;
  char *val;
  struct envitem_t *next;
} envitem;

static envitem *first=0;
static int envinit=0;

DRIDef driInfo={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

void grDRIOpen(char *pFB, tdfxScreenPrivate *sPriv,
	       volatile int *fifoPtr, volatile int *fifoRead) {
  driInfo.pFB=pFB;
  driInfo.sPriv=(tdfxScreenPrivate*)malloc(sizeof(tdfxScreenPrivate));
  memcpy(driInfo.sPriv, sPriv, sizeof(tdfxScreenPrivate));
  driInfo.fifoPtr=(volatile int **)fifoPtr;
  driInfo.fifoRead=(volatile int **)fifoRead;
  /* These are the fields used in glide src */
  driInfo.cpp=sPriv->cpp;
  driInfo.screenWidth=sPriv->width;
  driInfo.screenHeight=sPriv->height;
  driInfo.stride=driInfo.windowedStride=sPriv->stride;
  driInfo.isFullScreen=0;
}

void grDRIPosition(int x, int y, int w, int h, 
		   int numClip, XF86DRIClipRectPtr pClip) {
  driInfo.x=x;
  driInfo.y=y;
  driInfo.w=w;
  driInfo.h=h;
  driInfo.numClip=numClip;
  driInfo.pClip=pClip;
}

static void loadEnvFile() {
  FILE *file;
  char data[128];
  char *env, *val;
  envitem *item;
  unsigned int sawError=0;
  envitem *first=(envitem *)0;

  if (envinit) return;
  envinit=1;
  file=fopen("/etc/conf.3dfx/voodoo3", "r");
  if (!file) return;
  while (1) {
    if (!fgets(data, 128, file)) return;
    if (*data=='#') continue;
    if (*data=='\n') continue;
    val=strchr(data, '=');
    if (!val) {
      if (!sawError) {
	fprintf(stderr, "In config file /etc/conf.3dfx/voodoo3:\n");
	sawError=1;
      }
      fprintf(stderr, "Malformed line: %s\n", data);
      continue;
    }
    *val=0;
    env=data;
    val++;
    item=malloc(sizeof(envitem));
    item->env=malloc(strlen(env)+1);
    strcpy(item->env, env);
    item->val=malloc(strlen(val)+1);
    strcpy(item->val, val);
    item->next=first;
    first=item;
  }
}

static void deleteEnvData() {
  envitem *ptr, *next;

  ptr=first;
  while (ptr) {
    next=ptr->next;
    if (ptr->env) free(ptr->env);
    if (ptr->val) free(ptr->val);
    free(ptr);
    ptr=next;
  }
  first=0;
  envinit=0;
}

char *
hwcGetErrorString()
{
#define FN_NAME "hwcGetErrorString"
  return errorString;
#undef FN_NAME
} /* hwcGetErrorString */

hwcInfo *
hwcInit(FxU32 vID, FxU32 dID) {
  hInfo.nBoards = 0;

  errorString[0] = '\0';

  if (!driInfo.pFB) return 0;
  if (dID!=driInfo.sPriv->deviceID) return 0;
  hInfo.boardInfo[0].pciInfo.initialized = FXFALSE;
  hInfo.nBoards++;
  hInfo.boardInfo[0].boardNum = 0;

  hInfo.boardInfo[0].pciInfo.initialized = FXTRUE;;
  hInfo.boardInfo[0].pciInfo.vendorID = vID;
  hInfo.boardInfo[0].pciInfo.deviceID = dID;
  hInfo.boardInfo[0].h3Mem = driInfo.sPriv->mem>>20;
  if (driInfo.sPriv->cpp==3 || driInfo.sPriv->cpp==4) { /* 24 or 32 bpp modes */
    hInfo.boardInfo[0].h3pixelSize=4;
  } else {
    hInfo.boardInfo[0].h3pixelSize=2;
  }
  hInfo.boardInfo[0].h3nwaySli=driInfo.sPriv->numChips;
  hInfo.boardInfo[0].h3pixelSample=driInfo.sPriv->numSamples;

  if (hInfo.nBoards) {
    return &hInfo;
  } else {
    sprintf(errorString, "Can't find or access Banshee/V3 board\n");
    return 0;
  }
}

FxBool
hwcMapBoard(hwcBoardInfo *bInfo, FxU32 bAddrMask) {
  extern int getpid();
  int i;

  if (bInfo->pciInfo.initialized == FXFALSE) {
    sprintf(errorString, "hwcMapBoard: Called before hwcInit\n");
    return FXFALSE;
  }

  bInfo->linearInfo.initialized = FXTRUE;
  bInfo->osNT = FXFALSE;
  bInfo->procHandle = getpid();
  for (i=0; i<driInfo.sPriv->numChips; i++) {
    bInfo->linearInfo.linearAddress[(i<<2)+0]=(FxU32)driInfo.sPriv->regs[i].map;
    bInfo->linearInfo.linearAddress[(i<<2)+1]=(FxU32)driInfo.pFB;
  }
  return FXTRUE;
}

FxBool
hwcUnmapBoard(hwcBoardInfo *bInfo) {
  return FXTRUE;
}

FxBool
hwcInitRegisters(hwcBoardInfo *bInfo) {
  int dramInit1;
  int i;

  if (bInfo->linearInfo.initialized == FXFALSE) {
    printf(errorString, "hwcInitRegisters Called before hwcMapBoard\n");
    return FXFALSE;
  }
      
  bInfo->regInfo.initialized = FXTRUE;

  bInfo->regInfo.ioMemBase =
    bInfo->linearInfo.linearAddress[0] + SST_IO_OFFSET;
  bInfo->regInfo.cmdAGPBase =
    bInfo->linearInfo.linearAddress[0] + SST_CMDAGP_OFFSET;
  bInfo->regInfo.waxBase =
    bInfo->linearInfo.linearAddress[0] + SST_2D_OFFSET;
  bInfo->regInfo.sstBase =
    bInfo->linearInfo.linearAddress[0] + SST_3D_OFFSET;
  bInfo->regInfo.lfbBase =
    bInfo->linearInfo.linearAddress[0] + SST_LFB_OFFSET;
  bInfo->regInfo.rawLfbBase =
    bInfo->linearInfo.linearAddress[1];
  bInfo->regInfo.ioPortBase = (FxU16) bInfo->pciInfo.pciBaseAddr[2] & ~0x1;

  /* Figure out if it's SDRAM */
  HWC_IO_LOAD(bInfo->regInfo, dramInit1, dramInit1);
  dramInit1 &= SST_MCTL_TYPE_SDRAM;
  if ( dramInit1 & SST_MCTL_TYPE_SDRAM )
    bInfo->sdRAM = FXTRUE;
  else
    bInfo->sdRAM = FXFALSE;

  /* Determine the number of chips on the board XXX */
  bInfo->pciInfo.numChips=driInfo.sPriv->numChips;
  if (driInfo.sPriv->numChips>1) {
    for (i=1; i<driInfo.sPriv->numChips; i++) {
      bInfo->regInfo.slaveSstBase[i-1] =
	bInfo->linearInfo.linearAddress[(i<<2)+0] + SST_3D_OFFSET;
      bInfo->regInfo.slaveCmdBase[i-1] =
	bInfo->linearInfo.linearAddress[(i<<2)+0] + SST_CMDAGP_OFFSET;
      bInfo->regInfo.slaveIOBase[i-1] =
	bInfo->linearInfo.linearAddress[(i<<2)+0];
    }
  }
  return FXTRUE;
}

/* How the hw treats lfb accesses are dependent on the 'type' of
 * memory (tiled/linear) that the color/aux buffers are in. We
 * pre-compute the actual lfb address here while we know about the
 * memory space and if we adjusted the page alignment above.
 *
 * NB: If we are in tiled mode then the fact that we align the color
 * buffers on page boundaries means that the y offset of the buffers
 * may not actually be on a boundary for the tile addressing scheme.
 * The 'rounding' done to HWC_TILED_BUFFER_Y_ALIGN adjust for this.
 *
 * NB: The memory optimization of aligning color buffers on even page
 * boundaries will cause the tiled lfb access to be off by a page so
 * we add in the width of a page (HWC_TILED_BUFFER_X_ADJUST) here.
 */
static FxU32
hwcBufferLfbAddr(const hwcBoardInfo *bInfo, FxU32 physAddress)
{
  FxU32 retVal = 0x00UL;

  FxU32 tileAddress;
  FxU32 tileNumber;
  FxU32 tileOffset;
  FxU32 tileXOffset;
  FxU32 tileScanline;
  FxU32 tileRow;
  FxU32 lfbAddress;
  FxU32 lfbYOffset;
  FxU32 lfbBufferStride = bInfo->buffInfo.bufLfbStride;

  if (bInfo->vidInfo.tiled) {    
    GDBG_INFO(80, "\tphysAddress: 0x%08lx\n",physAddress);

    /* Compute address in tile space */
    tileAddress = physAddress - bInfo->primaryOffset;
    GDBG_INFO(80, "\ttileAddress: 0x%08lx\n",tileAddress);

    /* Compute tile number we're in (each tile is 4K bytes) */
    tileNumber = tileAddress >> 12;
    GDBG_INFO(80, "\ttileNumber: 0x%08lx (%d)\n",tileNumber,tileNumber);

    /* Compute base tile row we're in */
    tileRow = tileNumber / bInfo->buffInfo.bufStrideInTiles;
    GDBG_INFO(80, "\ttileRow: %d  (stride = %d)\n",tileNumber,bInfo->buffInfo.bufStrideInTiles);

    /* Compute offset within the tile */
    tileOffset = tileAddress - (tileNumber << 12);
    GDBG_INFO(80, "\ttileOffset: 0x%08lx\n",tileOffset);

    /* Compute scanline within the tile */
    tileScanline = tileOffset >> 7;
    GDBG_INFO(80, "\ttileScanline: 0x%08lx\n",tileScanline);

    /* Compute tile X offset within the row */
    tileXOffset = tileNumber - (tileRow * bInfo->buffInfo.bufStrideInTiles);
    GDBG_INFO(80, "\ttileXOffset: %d\n",tileXOffset);

    /* Compute Y offset in LFB space */
    lfbYOffset = ((tileRow * 32 + tileScanline) << (bInfo->h3nwaySli >> 1));

    /* Compute LFB address of tile start */
    lfbAddress =  bInfo->primaryOffset
                  + lfbYOffset * lfbBufferStride 
                  + tileXOffset * 128;

    GDBG_INFO(80, "\tlfbAddress: %08lx\n", lfbAddress);
    retVal = lfbAddress;
  } else {
    retVal = physAddress;
  }
  return retVal;
}

static FxU32
calculateLfbStride(FxU32 screenWidth)
{
#if	1
    int TileAperturePitch;
    for (TileAperturePitch = 1024;
         (TileAperturePitch < (16 << 10)) && (TileAperturePitch < screenWidth);
         TileAperturePitch <<= 1);
    return(TileAperturePitch);
#else
    return(0x1000);
#endif
}


#if	0
/*
 * 3Dfx WORKHERE:
 *     This needs to be revisited.  Why is it here, and
 *     why does it not seem to be used.
 */
FxBool hwcSetupBufferFullscreen(hwcBoardInfo *bInfo, FxU32 nColBuffers,
				FxU32 nAuxBuffers) 
{
#define FN_NAME "hwcSetupBufferFullscreen"
  FxU32 bufStride, bufSize;

  if (bInfo->vidInfo.initialized == FXFALSE) {
    sprintf(errorString, "%s:  Called before video initialization\n", FN_NAME);
    return FXFALSE;
  }

  GDBG_INFO(80, "%s(0x%x, 0x%x, 0x%x)\n", FN_NAME, bInfo, nColBuffers, nAuxBuffers);

  /* I've decided on > 2 instead of == 3 because we may support more
    than 3 buffers in the future, and want 4 to set the
    triple-buffering bit in dramInit1, also */
  bInfo->vidInfo.tripleBuffering = (nColBuffers > 2);

  bInfo->vidInfo.stride = bufStride = 
    calcBufferStride(bInfo, bInfo->vidInfo.xRes, bInfo->vidInfo.tiled);   

  /* We want to place the FIFO after the tram but before the color
     buffers with some pad */
  bufSize = calcBufferSize(bInfo, bInfo->vidInfo.xRes, bInfo->vidInfo.yRes, 
			   bInfo->vidInfo.tiled);

  bInfo->buffInfo.bufStride = bufStride;
  bInfo->buffInfo.bufSize = bufSize;

  if (bInfo->vidInfo.tiled) {
    driInfo.fullScreenStride=bInfo->buffInfo.bufStrideInTiles = (bufStride >> 7);
    bInfo->buffInfo.bufSizeInTiles =
      calcBufferSizeInTiles(bInfo, bInfo->vidInfo.xRes, bInfo->vidInfo.yRes);
    bInfo->buffInfo.bufHeightInTiles =
      calcBufferHeightInTiles(bInfo, bInfo->vidInfo.yRes);
  }

  bInfo->buffInfo.initialized = FXTRUE;
  bInfo->buffInfo.nColBuffers = nColBuffers;
  bInfo->buffInfo.nAuxBuffers = nAuxBuffers;

  bInfo->fbOffset =  driInfo.sPriv->fbOffset;

  bInfo->fifoInfo.fifoStart = driInfo.sPriv->fifoOffset;
  bInfo->fifoInfo.fifoLength = driInfo.sPriv->fifoSize;

  bInfo->tramOffset = driInfo.sPriv->textureOffset;

  /* !!! This needs to be reworked to support second col and aux buffers */

  bInfo->primaryOffset = (bInfo->h3Mem<<20) - (bufSize*3+0x1000);
  bInfo->tramSize = bInfo->primaryOffset-bInfo->tramOffset;

  bInfo->buffInfo.colBuffStart0[0] = bInfo->primaryOffset;
  bInfo->buffInfo.colBuffEnd0[0] = bInfo->buffInfo.colBuffStart0[0]+bufSize;

  bInfo->buffInfo.colBuffStart0[1] = bInfo->buffInfo.colBuffEnd0[0];
  bInfo->buffInfo.colBuffEnd0[1] = bInfo->buffInfo.colBuffStart0[1]+bufSize;

  bInfo->buffInfo.auxBuffStart0 = bInfo->buffInfo.colBuffEnd0[1];
  if (!(bInfo->buffInfo.auxBuffStart0&0x1000)) 
    bInfo->buffInfo.auxBuffStart0+=0x1000;
  bInfo->buffInfo.auxBuffEnd0 = bInfo->buffInfo.auxBuffStart0+bufSize;

  bInfo->buffInfo.lfbBuffAddr0[0] = bInfo->buffInfo.colBuffStart0[0];
  bInfo->buffInfo.lfbBuffAddr0[1] = 
    hwcBufferLfbAddr(bInfo, bInfo->buffInfo.colBuffStart0[1]);
  bInfo->buffInfo.lfbBuffAddr0[2] = 
    hwcBufferLfbAddr(bInfo, bInfo->buffInfo.auxBuffStart0);

  return FXTRUE;

#undef FN_NAME
}
#endif	/* 0 */
FxBool
hwcSetupBuffersWindowed(hwcBoardInfo *bInfo, FxU32 nColBuffers, FxU32 nAuxBuffers) 
{
#define FN_NAME "hwcSetupBuffersWindowed"
  FxU32 bufStride, bufSize;

  if (bInfo->vidInfo.initialized == FXFALSE) {
    sprintf(errorString, "%s:  Called before video initialization\n", FN_NAME);
    return FXFALSE;
  }

  GDBG_INFO(80, "%s(0x%x, 0x%x, 0x%x)\n", FN_NAME, bInfo, nColBuffers, nAuxBuffers);

  /* I've decided on > 2 instead of == 3 because we may support more
    than 3 buffers in the future, and want 4 to set the
    triple-buffering bit in dramInit1, also */
  bInfo->vidInfo.tripleBuffering = (nColBuffers > 2);

  bInfo->vidInfo.stride = bufStride = 
    calcBufferStride(bInfo, driInfo.sPriv->width, bInfo->vidInfo.tiled);   

  /* We want to place the FIFO after the tram but before the color
     buffers with some pad */
  bufSize = calcBufferSize(bInfo, driInfo.sPriv->width, 
			   driInfo.sPriv->height, bInfo->vidInfo.tiled); 

  bInfo->buffInfo.bufStride = bufStride;
  bInfo->buffInfo.bufSize = bufSize;
  bInfo->buffInfo.bufLfbStride = calculateLfbStride(bufStride);

  if (bInfo->vidInfo.tiled) {
    driInfo.fullScreenStride=bInfo->buffInfo.bufStrideInTiles = (bufStride >> 7);
    bInfo->buffInfo.bufSizeInTiles =
      calcBufferSizeInTiles(bInfo, driInfo.sPriv->width, driInfo.sPriv->height);
    bInfo->buffInfo.bufHeightInTiles =
      calcBufferHeightInTiles(bInfo, driInfo.sPriv->height);
  }

  bInfo->buffInfo.initialized = FXTRUE;
  bInfo->buffInfo.nColBuffers = nColBuffers;
  bInfo->buffInfo.nAuxBuffers = nAuxBuffers;

  bInfo->fbOffset =  driInfo.sPriv->fbOffset;

  bInfo->fifoInfo.fifoStart = driInfo.sPriv->fifoOffset;
  bInfo->fifoInfo.fifoLength = driInfo.sPriv->fifoSize;

  bInfo->tramOffset = driInfo.sPriv->textureOffset;
  bInfo->tramSize = driInfo.sPriv->textureSize;

  /* !!! This needs to be reworked to support second col and aux buffers */

  bInfo->primaryOffset = driInfo.sPriv->backOffset;

  bInfo->buffInfo.colBuffStart0[0] = driInfo.sPriv->fbOffset;
  bInfo->buffInfo.colBuffEnd0[0] = driInfo.sPriv->fbOffset + bufSize;

  bInfo->buffInfo.colBuffStart0[1] = driInfo.sPriv->backOffset;
  bInfo->buffInfo.colBuffEnd0[1] = driInfo.sPriv->backOffset+bufSize;

  bInfo->buffInfo.auxBuffStart0 = driInfo.sPriv->depthOffset;
  bInfo->buffInfo.auxBuffEnd0 = driInfo.sPriv->depthOffset+bufSize;

  bInfo->buffInfo.lfbBuffAddr0[0] = bInfo->buffInfo.colBuffStart0[0];
  bInfo->buffInfo.lfbBuffAddr0[1] =
      hwcBufferLfbAddr(bInfo, bInfo->buffInfo.colBuffStart0[1]);
  bInfo->buffInfo.lfbBuffAddr0[2] = 
      hwcBufferLfbAddr(bInfo, bInfo->buffInfo.auxBuffStart0);

  return FXTRUE;

#undef FN_NAME
} /* hwcSetupBuffersWindowed */

FxBool
hwcAllocBuffers(hwcBoardInfo *bInfo, FxU32 nColBuffers, FxU32 nAuxBuffers) 
{
  FxBool result;

  result=hwcSetupBuffersWindowed(bInfo, nColBuffers, nAuxBuffers);
  GDBG_INFO(80, "%s:  Board Info:\n", "hwcAllocBuffers");
  GDBG_INFO(80, "\thdc:             0x%x\n", bInfo->hdc);
  GDBG_INFO(80, "\textContextID:    0x%x\n", bInfo->extContextID);
  GDBG_INFO(80, "\tdevRev:          0x%x\n", bInfo->devRev);
  GDBG_INFO(80, "\tfbOffset:        0x%x\n", bInfo->fbOffset);
  GDBG_INFO(80, "\th3Rev:           0x%x\n", bInfo->h3Rev);
  GDBG_INFO(80, "\th3Mem:           0x%x\n", bInfo->h3Mem);
  GDBG_INFO(80, "\tboardNum:        0x%x\n", bInfo->boardNum);
  GDBG_INFO(80, "\tdeviceNum:       0x%x\n", bInfo->deviceNum);

  GDBG_INFO(80, "%s:  Buffer Info:\n", "hwcAllocBuffers");
  GDBG_INFO(80, "\tbufSize:         0x%x\n", bInfo->buffInfo.bufSize);
  GDBG_INFO(80, "\tbufSizeInTiles:  0x%x\n", bInfo->buffInfo.bufSizeInTiles);
  GDBG_INFO(80, "\tbufStride:       0x%x\n", bInfo->buffInfo.bufStride);
  GDBG_INFO(80, "\tbufStrideInTiles:0x%x\n", bInfo->buffInfo.bufStrideInTiles);
  GDBG_INFO(80, "\tbufHeightInTiles:0x%x\n", bInfo->buffInfo.bufHeightInTiles);
  GDBG_INFO(80, "\tbufLfbStride    :0x%x\n", bInfo->buffInfo.bufLfbStride);
  GDBG_INFO(80, "\tnColBuffers:     0x%x\n", bInfo->buffInfo.nColBuffers);
  GDBG_INFO(80, "\tcolBuffStart0[0]:    0x%x\n", bInfo->buffInfo.colBuffStart0[0]);
  GDBG_INFO(80, "\tcolBuffEnd0[0]:      0x%x\n", bInfo->buffInfo.colBuffEnd0[0]);
  GDBG_INFO(80, "\tcolBuffStart0[1]:    0x%x\n", bInfo->buffInfo.colBuffStart0[1]);
  GDBG_INFO(80, "\tcolBuffEnd0[1]:      0x%x\n", bInfo->buffInfo.colBuffEnd0[1]);
  GDBG_INFO(80, "\tnAuxBuffers:     0x%x\n", bInfo->buffInfo.nAuxBuffers);
  GDBG_INFO(80, "\tauxBuffStart0:    0x%x\n", bInfo->buffInfo.auxBuffStart0);
  GDBG_INFO(80, "\tauxBuffEnd0:      0x%x\n", bInfo->buffInfo.auxBuffEnd0);
  GDBG_INFO(80, "\ttramOffset:      0x%x\n", bInfo->tramOffset);
  GDBG_INFO(80, "\ttramSize:        0x%x\n", bInfo->tramSize);
  GDBG_INFO(80, "\tlfbBuffAddr0[0]   0x%x\n", bInfo->buffInfo.lfbBuffAddr0[0]);
  GDBG_INFO(80, "\tlfbBuffAddr0[1]   0x%x\n", bInfo->buffInfo.lfbBuffAddr0[1]);
  GDBG_INFO(80, "\tlfbBuffAddr0[2]   0x%x\n", bInfo->buffInfo.lfbBuffAddr0[2]);

  GDBG_INFO(80, "%s:  FIFO Info:\n", "hwcAllocBuffers");
  GDBG_INFO(80, "\tfifoStart:       0x%x\n", bInfo->fifoInfo.fifoStart);
  GDBG_INFO(80, "\tfifoLength:      0x%x\n", bInfo->fifoInfo.fifoLength);
  return result;
}

void hwcIdleHardwareWithTimeout(hwcBoardInfo *bInfo)
{
  FxU32 
    miscInit0, miscInit1, status, statusSlave, idle, timeout, i;

  /* Wait for hardware to idle. */
  idle = 0;
  timeout = 0;

checkforidle:
  do {
    if(idle > 0) {
      GDBG_INFO(80,"waiting for idle...\n");
    }
    HWC_IO_LOAD(bInfo->regInfo, status, status);
    for(i = 1; i < bInfo->pciInfo.numChips; i++) {
      HWC_IO_LOAD_SLAVE(i, bInfo->regInfo, status, statusSlave);
      status |= statusSlave;
    }
    /* Make sure we see an idle 3 times in a row from all chips. */
    if(status & SST_BUSY) {
     idle = 0;
    } else {
     idle++;
    }
    timeout++;
    /* Nothing the hardware does should take as long as reading the
     * status registers a billion times... */
    if(timeout >= 1000000000) {
      break;
    }        
  } while(idle < 3);  

  if(timeout >= 1000000000) {
    GDBG_INFO(80,"Hardware timeout on idle, resetting...\n");
    /* Reset FBI, 2D, and command streams. */
    HWC_IO_LOAD(bInfo->regInfo, miscInit0, miscInit0);
    /* Also be sure to make sure miscInit1's addressing is correct on Napalm */
    HWC_IO_STORE(bInfo->regInfo, miscInit0, (miscInit0 & ~BIT(30)) | SST_GRX_RESET | SST_2D_RESET);
    HWC_IO_LOAD(bInfo->regInfo, miscInit1, miscInit1);
    HWC_IO_STORE(bInfo->regInfo, miscInit1, miscInit1 | SST_CMDSTREAM_RESET);

    /* Give it a little time to propagate */
    for(idle = 0; idle < 100; idle++) {
      HWC_IO_LOAD(bInfo->regInfo, status, status);    
    }  
    /* Let hardware out of reset */
    HWC_IO_STORE(bInfo->regInfo, miscInit1, miscInit1);
    HWC_IO_STORE(bInfo->regInfo, miscInit0, miscInit0);
    
    /* Give it a little time to propagate */
    for(idle = 0; idle < 100; idle++) {
      HWC_IO_LOAD(bInfo->regInfo, status, status);    
    }  

    /* And make sure it's really idle... */
    goto checkforidle;
  }
}

FxBool
hwcInitFifo(hwcBoardInfo *bInfo, FxBool enableHoleCounting)
{
#define FN_NAME "hwcInitFifo"
  if (bInfo->regInfo.initialized == FXFALSE) {
    sprintf(errorString, "%s:  Called before hwcMapBoard\n", FN_NAME);
    return FXFALSE;
  }

  if (bInfo->buffInfo.initialized == FXFALSE) {
    sprintf(errorString, "%s:  Called before hwcInitBuffers\n", FN_NAME);
    return FXFALSE;
  }

#if 0
  driInfo.holeCounting=enableHoleCounting;
  hwcInitFifoRegs(enableHoleCounting);
#endif

  hwcIdleHardwareWithTimeout(bInfo);

  /* disable the CMD fifo */
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.baseSize, 0);

  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.baseAddrL, bInfo->fifoInfo.fifoStart>>12);
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.readPtrL, bInfo->fifoInfo.fifoStart);
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.readPtrH, 0);
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.aMin, bInfo->fifoInfo.fifoStart-4);
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.aMax, bInfo->fifoInfo.fifoStart-4);
  
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.depth, 0);
  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.holeCount, 0);
  /* Fifo LWM /HWM/ THRESHOLD */
  if (bInfo->pciInfo.deviceID == SST_DEVICE_ID_H3) { /* banshee */
    HWC_CAGP_STORE(bInfo->regInfo, cmdFifoThresh,
                   (0x09 << SST_HIGHWATER_SHIFT) | 0x2);
  } else if (IS_NAPALM(bInfo->pciInfo.deviceID)) {
    HWC_CAGP_STORE(bInfo->regInfo, cmdFifoThresh,
		   (0x20 << SST_HIGHWATER_SHIFT) | 0x8);
  } else {
    HWC_CAGP_STORE(bInfo->regInfo, cmdFifoThresh,
                   (0xf << SST_HIGHWATER_SHIFT) | 0x8);
  }

  HWC_CAGP_STORE(bInfo->regInfo, cmdFifo0.baseSize, 
		 (((bInfo->fifoInfo.fifoLength >> 12) - 1) | 
		  SST_EN_CMDFIFO |
		  (enableHoleCounting ? 0 : SST_CMDFIFO_DISABLE_HOLES)));

  GDBG_INFO(80 ,"%s:  CMD FIFO placed at physical addr 0x%x size 0x%0x\n", 
	    FN_NAME, driInfo.sPriv->fifoOffset, driInfo.sPriv->fifoSize);

  return FXTRUE;

#undef FN_NAME
} /* hwcInitFifo */

static FxBool
hwcInitVideoDesktopAndOverlaySurface(hwcRegInfo *rInfo,
                                     FxU32 *vidProcCfgAddr,
                                     FxU32 pixFmt,
                                     FxU32 desktopStartAddress,
                                     FxU32 desktopTiledStride,
                                     FxU32 overlayStartAddress,
                                     FxU32 overlayTiledStride)
{
  FxU32 doStride;
  FxU32 vidProcCfg = *vidProcCfgAddr;
  FxU32 vidScreenSize = 0;
  int desktopBpp = driInfo.cpp;
  int lg2desktopBpp;
  switch (desktopBpp) {
  case 2:
      lg2desktopBpp = 1;
      break;
  case 3:
      desktopBpp = 4;
     /*
      * Allez Oop.
      */
  case 4:
      lg2desktopBpp = 2;
      break;
  default:
     /*
      * This cannot happen.
      */
      fprintf(stderr,
              "Bad desktop BPP value %d in hwcSetupFullScreen\n",
              desktopBpp);
      return FXFALSE;
  }
  HWC_IO_STORE((*rInfo), vidProcCfg, vidProcCfg);
 /*
  * Set up the desktop.
  */
  HWC_IO_STORE((*rInfo), vidDesktopStartAddr,
               desktopStartAddress & SST_VIDEO_START_ADDR);
 /*
  * Set up the overlay.
  */
  vidProcCfg |= pixFmt;
  vidProcCfg |= (pixFmt >> 21) << 18;
  vidProcCfg &= ~SST_CURSOR_EN; /* Turn off HW Cursor */
 /*
  * These are kind of guesswork.
  *
  * Bypass both cluts.
  * Select the upper CLUT for the overlay, and the lower CLUT
  * for the desktop.
  */
  vidProcCfg |= SST_OVERLAY_CLUT_BYPASS|SST_DESKTOP_CLUT_BYPASS;
  vidProcCfg |= SST_OVERLAY_CLUT_SELECT;
  vidProcCfg |= SST_OVERLAY_EN | SST_DESKTOP_EN;
  vidProcCfg |= SST_DESKTOP_TILED_EN | SST_OVERLAY_TILED_EN;

  /* */
  doStride = SST_OVERLAY_TILE_STRIDE
             | SST_DESKTOP_TILE_STRIDE
             | (desktopTiledStride << SST_DESKTOP_STRIDE_SHIFT)
             | (overlayTiledStride << SST_OVERLAY_STRIDE_SHIFT);
  HWC_IO_STORE((*rInfo), vidDesktopOverlayStride, doStride);

  HWC_IO_STORE((*rInfo), vidOverlayStartCoords, 0);
  HWC_IO_STORE((*rInfo), vidOverlayEndScreenCoord,
               (driInfo.sPriv->height  << SST_OVERLAY_Y_SHIFT) |
               (driInfo.sPriv->width & SST_OVERLAY_X) );
  HWC_IO_STORE((*rInfo), vidOverlayDudx, driInfo.screenWidth);
  HWC_IO_STORE((*rInfo), vidOverlayDvdy, 0);
  HWC_IO_LOAD((*rInfo), vidScreenSize, vidScreenSize);
  vidScreenSize &= ~SST_VIDEO_SCREEN_DESKTOPADDR_FIFO_ENABLE;
  HWC_IO_STORE((*rInfo), vidScreenSize, vidScreenSize);
  HWC_IO_STORE((*rInfo), vidOverlayDudxOffsetSrcWidth,
               ((driInfo.sPriv->width << lg2desktopBpp)
                         << SST_OVERLAY_FETCH_SIZE_SHIFT)
                
                   & SST_OVERLAY_FETCH_SIZE);
  finalVidDesktopOverlayStride = doStride;
  *vidProcCfgAddr = vidProcCfg;
  return(FXTRUE);
} /* hwcInitVideoDesktopAndOverlaySurface */

FxBool
hwcInitVideo(hwcBoardInfo *bInfo, FxBool tiled, FxVideoTimingInfo
             *vidTiming, FxU32 pixFmt, FxBool overlay) {
  FxU32 stride, dramInit1, miscInit0;

  stride= (tiled) ? bInfo->buffInfo.bufStrideInTiles : bInfo->vidInfo.stride;

  /* Video pixel buffer threshold */
  {
    FxU32 vidPixelBufThold;
    FxU32 thold = 32;

    if (getenv("SSTVB_PIXTHOLD")) {
      thold = atoi(getenv("SSTVB_PIXTHOLD"));
    }

    thold &= 0x3f;

    vidPixelBufThold = (thold | (thold << 6) | (thold << 12));

    HWC_IO_STORE(bInfo->regInfo, vidPixelBufThold, vidPixelBufThold);
  }

  /* Check to see if the screen and overlay dimensions match. 
   * There are two cases that can happen in reality.
   *
   * scrXXX > appXXX: This is a 'real' case, and the overlay dimension
   * needs to mag scaled so that it fits the requested size.
   *
   * (scrXXX == appXXX) && (ovlXXX != scrXXX): This is a somewhat artificial
   * case where someone left the overlay set to some value, and these did
   * not get reset in the setVideoMode processing. (For example, if the user is
   * running an application which bus masters data directly to our video overlay
   * when launching a glide application). In this case we need to fiddle w/ the
   * overlay dimension so that it matches the requested resolution.
   *
   * (scrXXX < appXXX): If setVideoMode is actually working correctly, this cannot
   * happen because that code has to know that we can't do min scaling.
   */

  /* Get miscInit0 for y-sub  */
  HWC_IO_LOAD(bInfo->regInfo, miscInit0, miscInit0);

  /* Clear out relavent bits */
  miscInit0 &= ~SST_YORIGIN_TOP;
  miscInit0 |= ((driInfo.sPriv->height - 1)  << SST_YORIGIN_TOP_SHIFT);
  HWC_IO_STORE(bInfo->regInfo, miscInit0, miscInit0);

  /* Set up dramInit1 for triple or double buffering */
  HWC_IO_LOAD(bInfo->regInfo, dramInit1, dramInit1);
  if (bInfo->vidInfo.tripleBuffering)
    dramInit1 |= SST_TRIPLE_BUFFER_EN;
  else
    dramInit1 &= ~SST_TRIPLE_BUFFER_EN;
  HWC_IO_STORE(bInfo->regInfo, dramInit1, dramInit1);

  HWC_IO_STORE(bInfo->regInfo, vidMaxRGBDelta, 0x100810);

  return FXTRUE;
}

FxBool
hwcRestoreVideo(hwcBoardInfo *bInfo) {
  return FXTRUE;
}

static FxU32
calcBufferStride(hwcBoardInfo *bInfo, FxU32 xres, FxBool tiled)
{
  FxU32 strideInTiles;
  FxU32 cpp = (bInfo->h3pixelSize == 3) ? 4 : bInfo->h3pixelSize;

  if (tiled == FXTRUE) {
    /* Calculate tile width stuff */
    strideInTiles = (xres * cpp + HWC_TILE_WIDTH - 1)/HWC_TILE_WIDTH;
    return (strideInTiles * HWC_TILE_WIDTH);
  } else {
    return (xres * cpp);
  }
} /* calcBufferStride */

static FxU32
calcBufferHeightInTiles(hwcBoardInfo *bInfo, FxU32 yres)
{
  FxU32
    heightInTiles;            /* Height of buffer in tiles */

  yres = yres >> (bInfo->h3nwaySli>>1);

  /* Calculate tile height stuff */
  heightInTiles = yres >> 5;
  
  if (yres & (HWC_TILE_HEIGHT - 1))
    heightInTiles++;

  return heightInTiles;

} /* calcBufferHeightInTiles */

static FxU32
calcBufferSizeInTiles(hwcBoardInfo *bInfo, FxU32 xres, FxU32 yres) {
  FxU32
    bufSizeInTiles;           /* Size of buffer in tiles */

  bufSizeInTiles =
    calcBufferHeightInTiles(bInfo, yres) * 
    (calcBufferStride(bInfo, xres, FXTRUE) >> 7);

  return bufSizeInTiles;

} /* calcBufferSizeInTiles */

static FxU32
calcBufferSize(hwcBoardInfo *bInfo, FxU32 xres, FxU32 yres, FxBool tiled)
{
  FxU32
    stride,
    height,
    bufSize;                  /* Size of buffer in bytes */
  FxU32 shift = (bInfo->h3pixelSize>>1);

  if (tiled) {
    stride = calcBufferStride(bInfo, xres, tiled);
    height = HWC_TILE_HEIGHT * calcBufferHeightInTiles(bInfo, yres);
  } else {
    stride = xres << shift;
    height = yres >> (bInfo->h3nwaySli>>1);
  }

  bufSize = stride * height;
  
  return bufSize;

} /* calcBufferSize */

FxBool
hwcCheckMemSize(hwcBoardInfo *bInfo, FxU32 xres, FxU32 yres, FxU32 nColBuffers,
                FxU32 nAuxBuffers, FxBool tiled)
{
#define FN_NAME "hwcCheckMemSize"
  FxU32
    bufSize, totSize;

  bufSize = calcBufferSize(bInfo, xres, yres, tiled);

  totSize = (nColBuffers + nAuxBuffers) * bufSize;

  if (totSize < ((bInfo->h3Mem << 20) - 0x200000)) /* Need 2M for texture */
    return FXTRUE;
  else
    return FXFALSE;    
#undef FN_NAME
} /* hwcCheckMemSize */

FxU32 
hwcInitAGPFifo(hwcBoardInfo *bInfo, FxBool enableHoleCounting) 
{
  return hwcInitFifo(bInfo, enableHoleCounting); 
}

#define RED_SHIFT       16
#define GREEN_SHIFT     8
#define BLUE_SHIFT      0

FxBool
hwcGammaTable(hwcBoardInfo *bInfo, FxU32 nEntries, FxU32 *r, FxU32 *g, FxU32 *b)
{
  /* We don't allow them to store the gamma table, because X controls it */
  return FXTRUE;
#if 0
#define FN_NAME "hwcGammaTable"
  FxU32 gRamp[256];
  FxU32 i;
  FxU32 dacBase;
  
  /* Load the table into the Display driver as above */
  for (i = 0; i < nEntries; i++) {
    gRamp[i] =
      ((r[i] & 0xff) << RED_SHIFT) |
        ((g[i] & 0xff) << GREEN_SHIFT) |  
          ((b[i] & 0xff) << BLUE_SHIFT);
  }
  
  dacBase = 0;
 
  for (i = 0; i < nEntries; i++) {
    HWC_IO_STORE( bInfo->regInfo, dacAddr, dacBase + i);
    P6FENCE;
    HWC_IO_STORE( bInfo->regInfo, dacData, gRamp[i]);
    P6FENCE;
  }

  return FXTRUE;
  
#undef FN_NAME
#endif
} /* hwcGammaTable */

FxBool
hwcGetGammaTable(hwcBoardInfo *bInfo, FxU32 nEntries, FxU32 *r, FxU32 *g, FxU32 *b)
{
#define FN_NAME "hwcGetGammaTable"
  FxU32 i;
  FxU32 vidProcCfg;
  FxU32 dacBase;
  FxU32 dacAddr ;
  FxU32 dacData ;
 
  /*
   * AJB- Don't believe the hype.  Although we can do this
   * ourselves on Win9x/DOS, it's actually a piece of cake
   * to call the GDI Gamma ramp interface. This non-GDIish
   * implementation is here because it is faster than 
   * calling the GDI function (which thunks to the display
   * driver).
   */
  HWC_IO_LOAD( bInfo->regInfo, vidProcCfg, vidProcCfg);
  
  /* Determine which set of CLUT entries are selected */
  if (vidProcCfg & SST_OVERLAY_CLUT_SELECT)
    dacBase = 256;
  else
    dacBase = 0;
 
  /* Print out some useful info RE the vidProcCfg register */
  GDBG_INFO(80, "%s:  vidProcCFG(SST_OVERLAY_CLUT_SELECT) = %d\n",
            FN_NAME, (vidProcCfg & SST_OVERLAY_CLUT_SELECT) ? 1 : 0);
  GDBG_INFO(80, "%s:  vidProcCFG(SST_OVERLAY_EN) = %d\n",
            FN_NAME, (vidProcCfg & SST_OVERLAY_EN) ? 1 : 0);
  GDBG_INFO(80, "%s:  vidProcCFG(SST_OVERLAY_CLUT_BYPASS) = %d\n",
            FN_NAME, (vidProcCfg & SST_OVERLAY_CLUT_BYPASS) ? 1 : 0);
  
  for (i = 0; i < nEntries; i++) 
  {
    int repeat = 100 ;

    do
    {
      HWC_IO_STORE(bInfo->regInfo, dacAddr, dacBase + i);
      P6FENCE;
      HWC_IO_LOAD(bInfo->regInfo, dacAddr, dacAddr);
      P6FENCE;
      HWC_IO_LOAD(bInfo->regInfo, dacData, dacData);
      P6FENCE;
    } while (--repeat && (dacAddr != dacBase + i)) ;

    if (!repeat) 
    {
      GDBG_INFO(0, "%s: Error Reading DacData [%d, %x]. DacBase = %d\n",
                FN_NAME, i, dacData, dacBase);
    }

    r[i] = (dacData >> RED_SHIFT) & 0xFF ;
    g[i] = (dacData >> GREEN_SHIFT) & 0xFF ;
    b[i] = (dacData >> BLUE_SHIFT) & 0xFF ;
  }

  return FXTRUE;
#undef FN_NAME
} /* hwcGetGammaTable */

FxBool
hwcGammaRGB(hwcBoardInfo *bInfo, float gammaR, float gammaG, float gammaB)
{
  /* We don't allow glide to set the gamma as X controls it */
  return FXTRUE;
#if 0
#define FN_NAME "hwcGammaRGB"
  FxU32
    grRamp[256], ggRamp[256], gbRamp[256];
  int
    i;

  GDBG_INFO(80, FN_NAME "(0x%x, %1.2f, %1.2f, %1.2f)\n",
            bInfo, gammaR, gammaG, gammaB);

  /*
  ** NB:  The system eventually devised by Bob and Ken *may* require
  ** separate R, G, and B vectors.
  */

  for (i = 0; i < 256; i++) {
    grRamp[i] = (FxU32)((pow(i/255.0F, 1.0F/gammaR)) * 255.0F + 0.5F);
    ggRamp[i] = (FxU32)((pow(i/255.0F, 1.0F/gammaG)) * 255.0F + 0.5F);
    gbRamp[i] = (FxU32)((pow(i/255.0F, 1.0F/gammaB)) * 255.0F + 0.5F);
  }


  hwcGammaTable(bInfo, 256, grRamp, ggRamp, gbRamp); 

  /*
  **  Now that we have a gamma table, we can give it to the driver via
  **  a call to ExtEscape() when that is defined.....
  */

  return FXFALSE;

#undef FN_NAME
#endif
} /* hwcGammaRGB */

#define M 1
#define K 1

FxBool
hwcSetGrxClock(hwcBoardInfo *bInfo, FxU32 speedInMHz)
{
#define FN_NAME "hwcSetGrxClock"
  FxU32
    pllCtrl1,
    dramInit0 = 0xc17ae29, 
    dramInit1 = 0x26c031,
    n,
    m = 1;

  n = (FxU32) ((speedInMHz - 4.76f)/2.38f);

  pllCtrl1 =
    (K << SST_PLL_K_SHIFT) | (m << SST_PLL_M_SHIFT) | (n << SST_PLL_N_SHIFT);

  GDBG_INFO(80, "%s:  Setting Graphics Clock to %d\n", FN_NAME, speedInMHz);

  HWC_IO_STORE( bInfo->regInfo, dramInit0, dramInit0);
  HWC_IO_STORE( bInfo->regInfo, dramInit1, dramInit1);
  HWC_IO_STORE( bInfo->regInfo, pllCtrl1, pllCtrl1);
  
  return FXTRUE;

#undef FN_NAME  
} /* hwcSetGrxClock */

FxBool
hwcSetMemClock(hwcBoardInfo *bInfo, FxU32 speedInMHz)
{
#define FN_NAME "hwcSetMemClock"
  return FXFALSE;
#undef FN_NAME  
} /* hwcSetMemClock */

char *
hwcGetenv(char *a) 
{
  envitem *ptr;

  char *result;
  result=getenv(a);
  if (result) return result;
  if (!envinit) loadEnvFile();
  ptr=first;
  while (ptr) {
    if (!strcmp(ptr->env, a)) return ptr->val;
    ptr=ptr->next;
  }
  return 0;
}

FxBool
hwcResolutionSupported(hwcBoardInfo *bInfo, GrScreenResolution_t res,
		       GrScreenRefresh_t ref)
{
#define FN_NAME "hwcResolutionSupported"

  /* For now we're just going to return TRUE. We should check with
     vmode extension to see if the requested size is available */
  return FXTRUE;

#undef FN_NAME
} /* hwcResolutionSupported */

/* This two routines hwcSLIRead{Enable,Disable} are currently NOPs XXX */

void hwcSLIReadEnable(hwcBoardInfo *bInfo)
{
#if 0
  FxU32 cfgSliLfbCtrl;
  FxU32 cfgAALfbCtrl;
  FxU32 chipNumber;

  if(GETENV("FX_GLIDE_A0_READ_ABORT")) {
    if(bInfo->h3nwaySli > 1) {
      for(chipNumber = 0; chipNumber < bInfo->pciInfo.numChips; chipNumber++) {
        /* Turn on SLI Read */
        cfgSliLfbCtrl = hwcReadConfigRegister(bInfo, chipNumber, offsetof(SstPCIConfigRegs, cfgSliLfbCtrl));
        cfgSliLfbCtrl |= (SST_SLI_LFB_READ_ENABLE);
        hwcWriteConfigRegister(bInfo, chipNumber, offsetof(SstPCIConfigRegs, cfgSliLfbCtrl), cfgSliLfbCtrl);
        
        /* Turn off AA read if it is not supposed to be enabled */
        if(bInfo->h3pixelSample < 2) {
            cfgAALfbCtrl = hwcReadConfigRegister(bInfo, chipNumber, offsetof(SstPCIConfigRegs, cfgAALfbCtrl));
            cfgAALfbCtrl &= ~(SST_AA_LFB_READ_ENABLE);
            hwcWriteConfigRegister(bInfo, chipNumber, offsetof(SstPCIConfigRegs, cfgAALfbCtrl), cfgAALfbCtrl);
        } 
      }
    }           
  }
#endif
}


void hwcSLIReadDisable(hwcBoardInfo *bInfo)
{
#if 0
  FxU32 cfgSliLfbCtrl;
  FxU32 cfgAALfbCtrl;
  FxU32 chipNumber;
  
  if(GETENV("FX_GLIDE_A0_READ_ABORT")) {
    if(bInfo->h3nwaySli > 1) {
        for(chipNumber = 0; chipNumber < bInfo->pciInfo.numChips; chipNumber++) {
        /* Turn off SLI Read */
        cfgSliLfbCtrl = hwcReadConfigRegister(bInfo, chipNumber, offsetof(SstPCIConfigRegs, cfgSliLfbCtrl));
        cfgSliLfbCtrl &= ~(SST_SLI_LFB_READ_ENABLE);
        hwcWriteConfigRegister(bInfo, chipNumber, offsetof(SstPCIConfigRegs, cfgSliLfbCtrl), cfgSliLfbCtrl);
        
        /* Turn on AA Read */
        cfgAALfbCtrl = hwcReadConfigRegister(bInfo, chipNumber, offsetof(SstPCIConfigRegs, cfgAALfbCtrl));
        cfgAALfbCtrl |= (SST_AA_LFB_READ_ENABLE);
        hwcWriteConfigRegister(bInfo, chipNumber, offsetof(SstPCIConfigRegs, cfgAALfbCtrl), cfgAALfbCtrl);
      }
    }           
  }
#endif
}

#include <linutil.h>

/*-------------------------------------------------------------------
  Function: tlKbHit
  Date: 2/28
  Implementor(s): jdt
  Library: test library
  Description:
  Returns true if there are pending characters in the input queue
  Arguments:
  none
  Return:
  nonzero if keys in queue
  -------------------------------------------------------------------*/
int  hwcKbHit( void ) {
  return lin_kbhit();
}

char hwcGetCH( void ) {
  return lin_getch();
}

void grDRIImportFifo(int fifoPtr, int fifoRead)
{
  _grImportFifo(fifoPtr, fifoRead);
}

void grDRIInvalidateAll() {
  _grInvalidateAll();
}

void grDRIResetSAREA() {
  _grExportFifo(driInfo.fifoPtr, driInfo.fifoRead);
}

Bool hwcSetupFullScreen(hwcBoardInfo *bInfo, FxBool state) {
  driInfo.isFullScreen=state;
  if (state) {
    FxU32 desktopTiledStride, overlayTiledStride;
    FxU32 vidProcCfg = 0;

    /* Video pixel buffer threshold */
    {
      FxU32 vidPixelBufThold;
      FxU32 thold = 32;

      if (getenv("SSTVB_PIXTHOLD")) {
	thold = atoi(getenv("SSTVB_PIXTHOLD"));
      }

      thold &= 0x3f;

      vidPixelBufThold = (thold | (thold << 6) | (thold << 12));

      HWC_IO_STORE(bInfo->regInfo, vidPixelBufThold, vidPixelBufThold);
    }

    driInfo.stride=driInfo.fullScreenStride;
    if (driInfo.sliCount > 1) {
        vidProcCfg = BIT(28) | BIT(29);
    }
   /*
    * We place the desktop over the windowed front buffer.
    * We place the back buffer over the windowed back buffer.
    */
    desktopTiledStride = bInfo->buffInfo.bufStrideInTiles;
    overlayTiledStride = bInfo->buffInfo.bufStrideInTiles;
    hwcInitVideoDesktopAndOverlaySurface(
      &bInfo->regInfo,
      &vidProcCfg,              /* video processor configuration value */
      driInfo.fullScreenPixFmt, /* pixel format of OS and DS */
      bInfo->buffInfo.colBuffStart0[0],
      desktopTiledStride,
      bInfo->buffInfo.colBuffStart0[1],
      overlayTiledStride);
    {
      int chipNum, locLFBMemCfg, lfbTileCompare;

      for(chipNum = 0; chipNum < bInfo->pciInfo.numChips; chipNum++) {
	/* Load */
	if(chipNum == 0) {
	  HWC_IO_STORE(bInfo->regInfo, lfbMemoryConfig, SST_RAW_LFB_UPDATE_CONTROL);
	  HWC_IO_LOAD(bInfo->regInfo, lfbMemoryConfig, locLFBMemCfg);
	  HWC_IO_STORE(bInfo->regInfo, lfbMemoryConfig, SST_RAW_LFB_UPDATE_CONTROL|SST_RAW_LFB_READ_CONTROL);
	  HWC_IO_LOAD(bInfo->regInfo, lfbMemoryConfig, lfbTileCompare);
	  HWC_IO_STORE(bInfo->regInfo, lfbMemoryConfig, SST_RAW_LFB_UPDATE_CONTROL);
	} else {
	  HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, lfbMemoryConfig, SST_RAW_LFB_UPDATE_CONTROL);
	  HWC_IO_LOAD_SLAVE(chipNum, bInfo->regInfo, lfbMemoryConfig, locLFBMemCfg);
	  HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, lfbMemoryConfig, SST_RAW_LFB_UPDATE_CONTROL|SST_RAW_LFB_READ_CONTROL);
	  HWC_IO_LOAD_SLAVE(chipNum, bInfo->regInfo, lfbMemoryConfig, lfbTileCompare);
	  HWC_IO_STORE_SLAVE(chipNum, bInfo->regInfo, lfbMemoryConfig, SST_RAW_LFB_UPDATE_CONTROL);        
	}
      }
    }
    HWC_IO_STORE( bInfo->regInfo, vidDesktopOverlayStride,
		  ( driInfo.stride << 16 ) |
                  driInfo.stride );
    grSetSliCount(driInfo.sPriv->numChips, driInfo.sliCount);
#if	0
   /*
    * We do this in the X server.
    */
    if (driInfo.sliCount > 1) {
        _grEnableSliCtrl();
    }
#endif
   /*
    * Ok, we are all set up, so enable the video processor, and
    * we're on our way.
    */
    vidProcCfg |= SST_VIDEO_PROCESSOR_EN;
    HWC_IO_STORE(bInfo->regInfo, vidProcCfg, vidProcCfg);
  } else {
    FxU32 vidProcCfg = 0;
   /*
    * Just disable the video processor.
    */
    HWC_IO_LOAD(bInfo->regInfo, vidProcCfg, vidProcCfg);
    vidProcCfg &= ~SST_VIDEO_PROCESSOR_EN;
    HWC_IO_STORE(bInfo->regInfo, vidProcCfg, vidProcCfg);
#if	0
   /*
    * We will do this in the X server.
    */
    _grDisableSliCtrl();
#endif
    grSetSliCount(1, 1);
    driInfo.stride=driInfo.windowedStride;
  }
  return(FXTRUE);
}
