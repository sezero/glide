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
** 
** 110   6/30/98 6:08p Jeske
** fixed bug where we tried to setup MTRRs on old (<p6) systems which
** didn't have them. 
** 
** 109   5/21/98 5:52p Peter
** mismatched pairs
** 
** 108   5/20/98 3:51p Peter
** no fifo glide
** 
** 107   5/18/98 12:15p Peter
** better point code
** 
** 106   4/21/98 5:53p Peter
** slidetect vs hw pointers
** 
** 105   4/08/98 3:52p Peter
** point coordinate truncation
** 
** 104   4/06/98 9:55a Peter
** sli slave detection for oem dll
** 
** 103   3/17/98 6:50p Peter
** sli paired vs active
** 
** 102   3/17/98 3:00p Peter
** removed unused stats
** 
** 101   3/13/98 5:08p Peter
** doh, got it backwards
** 
** 100   3/13/98 3:39p Peter
** voodoo1.5 silliness
** 
** 99    2/20/98 9:05a Peter
** removed remnants of comdex grot
** 
** 98    2/11/98 5:25p Peter
** detection dialog fixes
** 
** 97    1/30/98 4:51p Peter
** fixed sli-detect for multiple pairs
** 
** 96    1/30/98 4:27p Peter
** no uswc for sli slave
** 
** 95    1/24/98 12:29p Peter
** more caching fun
** 
** 94    1/20/98 11:03a Peter
** env var to force triple buffering
 * 
 * 93    1/16/98 5:41p Peter
 * fixed sense of lod_dither
 * 
 * 92    1/14/98 10:22a Peter
 * no more hacks
 * 
 * 91    1/08/98 7:09p Peter
 * real hw stuff modulo makefile change
 * 
 * 90    1/07/98 11:18a Atai
 * remove GrMipMapInfo and GrGC.mm_table in glide3
 * 
 * 89    1/07/98 10:22a Peter
 * lod dithering env var
 * 
 * 88    12/17/97 10:08a Peter
 * fast system comdex twiddling
 * 
 * 87    12/09/97 4:20p Peter
 * 0x100 fbiRev ofset for v2
 * 
 * 86    12/09/97 12:20p Peter
 * mac glide port
 * 
 * 85    12/05/97 4:26p Peter
 * watcom warnings
 * 
 * 84    12/03/97 2:36p Peter
 * upped comdex reset defaults
 * 
 * 83    12/02/97 9:48a Dow
 * Removed some spurious code I inadvertantly added.
 * 
 * 82    11/21/97 6:24p Dow
 * Banshee Lying about being Rush stuf
 * 
 * 81    11/21/97 11:19a Dow
 * Made Banshee report Voodoo2
 * 
 * 80    11/20/97 6:39p Peter
 * fixed direct_exec w/ csim
 * 
 * 79    11/19/97 2:49p Peter
 * env vars in registry for win32
 * 
 * 78    11/17/97 4:55p Peter
 * watcom warnings/chipfield stuff
 * 
 * 77    11/15/97 7:43p Peter
 * more comdex silliness
 * 
 * 76    11/14/97 11:10p Peter
 * open vs hw init confusion
 * 
 * 75    11/14/97 5:02p Peter
 * more comdex stuff
 * 
 * 74    11/14/97 12:09a Peter
 * comdex thing and some other stuff
 * 
 * 73    11/12/97 9:54p Peter
 * fixed all the muckage from new config
 * 
 * 72    11/12/97 9:37p Dow
 * Textures on Banshee half work
 * 
 * 71    11/12/97 9:22a Dow
 * h3 mods
 * 
 * 70    11/08/97 3:34p Peter
 * fixed stupid gdbg_info crasher
 * 
 * 69    11/04/97 4:00p Dow
 * Banshee Mods
 * 
 * 68    11/03/97 3:43p Peter
 * h3/cvg cataclysm
 * 
 * 67    11/01/97 12:11p Pgj
 * glide.dll ---> glide2x.dll
 * 
 * 66    10/31/97 8:53a Peter
 * last lying change, really
 * 
 * 65    10/30/97 3:42p Peter
 * protected the last bit of nonsense
 * 
 * 64    10/30/97 3:37p Peter
 * spoof sst1
 * 
 * 63    10/29/97 2:45p Peter
 * C version of Taco's packing code
 * 
 * 62    10/23/97 5:28p Peter
 * sli fifo thing
 * 
 * 61    9/15/97 7:31p Peter
 * more cmdfifo cleanup, fixed normal buffer clear, banner in the right
 * place, lfb's are on, Hmmmm.. probably more
 * 
 * 60    9/10/97 10:13p Peter
 * fifo logic from GaryT, non-normalized fp first cut
 * 
 * 59    9/05/97 5:29p Peter
 * changes for direct hw
 * 
 * 58    9/01/97 3:18p Peter
 * correct integer rounding for pts
 * 
 * 57    8/30/97 5:59p Tarolli
 * init and hal fixups
 * 
 * 56    8/30/97 1:19p Peter
 * first cut at using blit to clear, more to come to do inner rects
 * 
 * 55    8/18/97 3:52p Peter
 * pre-hw arrival fixes/cleanup
 * 
 * 54    7/30/97 2:42p Peter
 * shared and sanitized
 * 
 * 53    7/28/97 2:41p Peter
 * turned sli code back on for cvg, but waiting for hal
 * 
 * 52    7/25/97 11:40a Peter
 * removed dHalf, change field name to match real use for cvg
 * 
 * 51    7/08/97 2:47p Peter
 * fixed merge stupidity from last checkin
 * 
 * 50    7/02/97 12:28p Peter
 * removed spurious NOP, tex dl
 * 
 * 49    6/24/97 4:02p Peter
 * proper cmd fifo placement
 * 
 * 48    6/23/97 4:46p Peter
 * fixed my ,uckage
 * 47    6/23/97 4:43p Peter
 * cleaned up #defines etc for a nicer tree
**
*/

#include <stdio.h>
#include <string.h>

#include <3dfx.h>
#include <glidesys.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

/*-------------------------------------------------------------------
  Function: _grSstDetectResources
  Date: --
  Implementor(s): Dow, Gmt, Jdt
  Library: Glide
  Description:
  Discover devices on the PCI bus.
  Discover configuration of detected devices.
  Initialize all Glide GC's 

  Recognized devices depend upon compile time flags

  This code should NOT initialize the hardware 
  any more than is necessary for discovery of 
  configuration

  Arguments: none
  Return: 
  FXTRUE  - at least one device was detected
  FXFALSE - no devices were detected.
  -------------------------------------------------------------------*/
FxBool 
_grSstDetectResources(void)
{
  static FxBool calledP = FXFALSE;
  FxBool rv = FXFALSE;
  FxU32 ctx, device;

  GDBG_INFO(280, "_grSstDetectResources()\n");

#if (GLIDE_PLATFORM & GLIDE_HW_CVG)
  {
#if GLIDE_INIT_HAL
    HalInfo* halInfo = fxHalInit(0);
    if (halInfo == NULL) goto __errExit;
#else /* !GLIDE_INIT_HAL */
    FxDeviceInfo dummyDevInfo;
#endif /* !GLIDE_INIT_HAL */

    /* The first time through the init code we need to map 
     * all of the boards. Future calls can just grab this
     * info out of the halInfo that we have here.
     */
    if (!calledP) {
      FxU32 count = HAL_MAX_BOARDS;
      FxBool inSliPairP = FXFALSE;
      GrHwConfiguration* hwConfig = &_GlideRoot.hwConfig;

      /* NB: The hw will initially return fbiRevision == 2, but
       * the init code does some more dorking w/ stuff and detects
       * if it is really a CVG and sets it to 4.
       */
      const FxU32 hwRevNum = ((GETENV("FX_GLIDE_HW_REV") == NULL)
                              ? 4
                              : atol(GETENV("FX_GLIDE_HW_REV")));
      
      for(ctx = device = 0; device < count; device++) {
        const FxDeviceInfo* curDev = NULL;
        FxBool regInitP = FXFALSE;
        SstRegs* devRegs;

#if GLIDE_INIT_HAL
        devRegs = fxHalMapBoard(device);
        curDev = halInfo->boardInfo + device;
#else /* !GLIDE_INIT_HAL */
        devRegs = (SstRegs*)sst1InitMapBoard(device);
        if (devRegs != NULL) {
          if (sst1InitGetDeviceInfo((FxU32*)devRegs, &dummyDevInfo)) {
            dummyDevInfo.virtAddr[0] = devRegs;
            curDev = &dummyDevInfo;
          }
        }
#endif /* !GLIDE_INIT_HAL */

        if ((devRegs != NULL) && 
            (curDev != NULL) &&
            (curDev->fbiRevision >= hwRevNum)) {
          const FxU32 curSstNum = hwConfig->num_sst;
          FxU32 i;

#if (GLIDE_PLATFORM & GLIDE_HW_CVG)        
          /* Internally we always keep the real type. */
          hwConfig->SSTs[ctx].type = GR_SSTTYPE_Voodoo2;
#else
#error "What do you think you're doing?"
#endif

#if GLIDE_INIT_HAL
          if (!fxHalInitRegisters(curDev->virtAddr[0])) goto __errRegFailure;

#else /* !GLIDE_INIT_HAL */
          /* dpc - 23 jan 1998 - FixMe!
           * We set up the caching characteristics here so that we
           * mimic the actions of the nt driver, where everything is
           * happy, in win95.  However, if this is the slave of an sli
           * pair then we don't want to waste mtrr's that we're never
           * really going to write to.  
           */
          if (!inSliPairP && (_GlideRoot.CPUType >= 6)) {
		sst1InitCaching((FxU32*)devRegs, FXTRUE);
	  }

          if (!sst1InitRegisters((FxU32*)devRegs) ||
              !sst1InitGetDeviceInfo((FxU32*)devRegs, &dummyDevInfo)) goto __errRegFailure;
          curDev = &dummyDevInfo;
#endif /* !GLIDE_INIT_HAL */

          /* Set up pointers to the various address spaces within the hw */
          _GlideRoot.GCs[ctx].base_ptr  = (FxU32*)HW_BASE_PTR(devRegs);
          _GlideRoot.GCs[ctx].reg_ptr   = (FxU32*)HW_REG_PTR(devRegs);
          _GlideRoot.GCs[ctx].lfb_ptr   = (FxU32*)HW_LFB_PTR(devRegs);
          _GlideRoot.GCs[ctx].tex_ptr   = (FxU32*)HW_TEX_PTR(devRegs);

          /* Video parameters */
          _GlideRoot.GCs[ctx].grSstRez     = GR_RESOLUTION_NONE;
          _GlideRoot.GCs[ctx].grSstRefresh = curDev->fbiVideoRefresh;
        
          /* Chip configuration */
          _GlideRoot.GCs[ctx].num_tmu   = curDev->numberTmus;
          _GlideRoot.GCs[ctx].fbuf_size = hwConfig->SSTs[curSstNum].sstBoard.Voodoo2Config.fbRam;

          /* Independet board or physical sli master */
          if (!inSliPairP) {
            _GlideRoot.gcMap[curSstNum] = _GlideRoot.gcNum;
            
            /* Voodoo^2 glide2x returns that it is an sst1, but the
             * revision is bumped so that developers can really tell.
             * Additionally, we now have the fun on Voodoo^1.5 which is
             * just a Voodoo^2 in a 2200 configuration. 
             */
            hwConfig->SSTs[curSstNum].sstBoard.Voodoo2Config.fbiRev = (curDev->fbiRevision + 
                                                                       ((curDev->numberTmus == 1)
                                                                        ? 0x080 
                                                                        : 0x100));
            
            hwConfig->SSTs[curSstNum].sstBoard.Voodoo2Config.fbRam     = curDev->fbiMemSize;
            hwConfig->SSTs[curSstNum].sstBoard.Voodoo2Config.nTexelfx  = curDev->numberTmus;
            hwConfig->SSTs[curSstNum].sstBoard.Voodoo2Config.sliDetect = curDev->sliDetected;

            for(i = 0; i < curDev->numberTmus; i++) {
              const FxU32 curTmuMemSize = curDev->tmuMemSize[i];
              
              hwConfig->SSTs[curSstNum].sstBoard.Voodoo2Config.tmuConfig[i].tmuRev = curDev->tmuRevision;
              hwConfig->SSTs[curSstNum].sstBoard.Voodoo2Config.tmuConfig[i].tmuRam = curTmuMemSize;
            }

            hwConfig->num_sst++;
          }

          /* Clear the tmu state */
          for(i = 0; i < curDev->numberTmus; i++) {
            const FxU32 curTmuMemSize = curDev->tmuMemSize[i];
            
            memset(&_GlideRoot.GCs[ctx].tmu_state[i], 0, sizeof(_GlideRoot.GCs[ctx].tmu_state[i]));
            _GlideRoot.GCs[ctx].tmu_state[i].total_mem = (curTmuMemSize << 20);
            
            _GlideRoot.GCs[ctx].tmu_state[i].ncc_mmids[0] = 
              _GlideRoot.GCs[ctx].tmu_state[i].ncc_mmids[1] = GR_NULL_MIPMAP_HANDLE;
          }

          /* sst1GetDeviceInfo will not return any pairing information
           * until after the boards are paired w/ sst1InitSli which we
           * have not yet done. Currently we assume that the next
           * discovered board will be the slave.  
           */
          _GlideRoot.GCs[ctx].scanline_interleaved = curDev->sliDetected;
          _GlideRoot.GCs[ctx].sliPairP             = curDev->sliPaired;
          _GlideRoot.GCs[ctx].slave_ptr            = NULL;

          /* Are we currenly mapping a physcial sli-slave? */
          if (curDev->sliDetected && inSliPairP) {
            _GlideRoot.GCs[ctx - 1].slave_ptr = _GlideRoot.GCs[ctx].base_ptr;

            /* Report the minimum of the two boards' memory if they
             * are different.  The hw does not care becuase it is just
             * not going to access the memory.
             */
            {
              const FxU32 curSstNum = hwConfig->num_sst - 1;
              GrTMUConfig_t* tmuConfig = hwConfig->SSTs[curSstNum].sstBoard.Voodoo2Config.tmuConfig;

              /* Fbi memory */
              if ((FxU32)hwConfig->SSTs[curSstNum].sstBoard.Voodoo2Config.fbRam != curDev->fbiMemSize) {
                hwConfig->SSTs[curSstNum].sstBoard.Voodoo2Config.fbRam = curDev->fbiMemSize;
              }

              /* Tmu memory */
              for(i = 0; i < curDev->numberTmus; i++) {
                if ((FxU32)tmuConfig[i].tmuRam != curDev->tmuMemSize[i]) {
                  tmuConfig[i].tmuRam = MIN((FxU32)tmuConfig[i].tmuRam, curDev->tmuMemSize[i]);
                }
              }
            }
          }

          /* On an sli system the boards cannot be addressed
           * separately so pairing and detection are different things,
           * and we need to keep track of these paired boards as an
           * atomic unit.
           */
          inSliPairP = (curDev->sliPaired ? !inSliPairP : FXFALSE);

          /* This device is ready to go. */
          regInitP = FXTRUE;
          _GlideRoot.gcNum++;
          _GlideRoot.GCs[ctx].hwInitP = FXTRUE;
        
          rv = FXTRUE;
          ctx++;

          __errRegFailure:
          /* For some reason we failed to init the board that we just mapped. */
          if (!regInitP && (devRegs != NULL)) {
#if GLIDE_INIT_HAL
            fxHalShutdown(devRegs);
#else /* !GLIDE_INIT_HAL */
            if (_GlideRoot.CPUType >= 6) {
              sst1InitCaching((FxU32*)devRegs, FXFALSE);
            }
            pciUnmapPhysical((FxU32)devRegs, 0x1000000UL);
#endif /* !GLIDE_INIT_HAL */
          }
        }
      }

      /* Done setting up. Don't do the silly mapping thing again. */
      calledP = FXTRUE;
    } else {
      /* Did we previously find boards? */
      rv = (_GlideRoot.hwConfig.num_sst != 0);
    }

    goto __errExit; /* Keep warnings happy */
__errExit:
    ;
  }
#else
#  error "Write code for this chip"
#endif  
  
  return rv;
} /* _grSstDetectResources */


static void
displayBoardInfo(int i, GrHwConfiguration *hwc)
{
  if ((hwc->SSTs[i].type == GR_SSTTYPE_VOODOO) ||
      (hwc->SSTs[i].type == GR_SSTTYPE_Voodoo2)) {
    int tmuNum;

    GDBG_INFO(80,"SST board %d: 3Dfx Voodoo%s\n", 
              i, ((hwc->SSTs[i].type == GR_SSTTYPE_VOODOO) ? " Graphics" : "^2"));
    if (hwc->SSTs[i].sstBoard.VoodooConfig.sliDetect) {
      GDBG_INFO(80,"\tScanline Interleaved\n");
    }

    GDBG_INFO(80,"\tPixelfx rev 0x%lX with %d MB Frame Buffer\n",
              hwc->SSTs[i].sstBoard.VoodooConfig.fbiRev,
              hwc->SSTs[i].sstBoard.VoodooConfig.fbRam);
    GDBG_INFO(80,"\t%d Texelfx chips:\n",
              hwc->SSTs[i].sstBoard.VoodooConfig.nTexelfx);
    for (tmuNum = 0;
         tmuNum < hwc->SSTs[i].sstBoard.VoodooConfig.nTexelfx;
         tmuNum++) {
      GDBG_INFO(80,"\t\tTexelfx %d: Rev 0x%lX, %d MB Texture\n", tmuNum,
                hwc->SSTs[i].sstBoard.VoodooConfig.tmuConfig[tmuNum].tmuRev,
                hwc->SSTs[i].sstBoard.VoodooConfig.tmuConfig[tmuNum].tmuRam);
    }
  } else if (hwc->SSTs[i].type == GR_SSTTYPE_SST96) {
    GDBG_INFO(80,"SST board %d: 3Dfx Voodoo Rush\n", i);
    GDBG_INFO(80,"\tFBI Jr. with %d MB Frame Buffer\n",
              hwc->SSTs[i].sstBoard.SST96Config.fbRam);
    GDBG_INFO(80,"\tTexelfx chips:  1\n");
  } else {
    GDBG_INFO(80,"error: SSTs %d: unknown type\n",i);
  }
} /* displayBoardInfo */

#if defined(FX_DLL_ENABLE) && (GLIDE_PLATFORM & GLIDE_OS_WIN32)
#include <windows.h>
#endif

void
_GlideInitEnvironment(void)
{
#define FN_NAME "_GlideInitEnvironment"
  int i;

  if (_GlideRoot.initialized)          /* only execute once */
    return;
  GDBG_INIT();                          /* init the GDEBUG libraray */
  GDBG_INFO(80,"%s()\n", FN_NAME);

#ifdef __WIN32__
  grErrorSetCallback(_grErrorWindowsCallback);
#else
  grErrorSetCallback(_grErrorDefaultCallback);
#endif /* __WIN32__ */

#ifdef GLIDE_DEBUG
  GDBG_INFO(0,"GLIDE DEBUG LIBRARY\n"); /* unconditional display */
#endif

#if defined(FX_DLL_ENABLE) && (GLIDE_PLATFORM & GLIDE_OS_WIN32)
  { /* GMT: display the DLL pathname for sanity checking */
    char buf[132];
    if (GetModuleFileName(GetModuleHandle("glide2x.dll"), buf, sizeof(buf))) {
#ifdef GLIDE_DEBUG
      GDBG_INFO(0,"DLL path: %s\n",buf); /* unconditional display */
#else
      GDBG_INFO(80,"DLL path: %s\n",buf);
#endif
    } /* silent failure */
  }
#endif

  _GlideRoot.CPUType = _cpu_detect_asm();
  if (GETENV("FX_CPU")) _GlideRoot.CPUType = atoi(GETENV("FX_CPU"));

  /* Check for user environment tweaks */
  {
    const char* envStr;

#define GLIDE_GETENV(__envVar, __defVal) \
    (((envStr = GETENV(__envVar)) == NULL) ? (__defVal) : atol(envStr))

    _GlideRoot.environment.triBoundsCheck    = (GETENV("FX_GLIDE_BOUNDS_CHECK") != NULL);
    _GlideRoot.environment.noSplash          = (GETENV("FX_GLIDE_NO_SPLASH") != NULL);
    _GlideRoot.environment.shamelessPlug     = (GETENV("FX_GLIDE_SHAMELESS_PLUG") != NULL);
    _GlideRoot.environment.ignoreReopen      = (GETENV("FX_GLIDE_IGNORE_REOPEN") != NULL);
    _GlideRoot.environment.disableDitherSub  = (GETENV("FX_GLIDE_NO_DITHER_SUB") != NULL);
    _GlideRoot.environment.texLodDither      = ((GETENV("FX_GLIDE_LOD_DITHER") == NULL)
                                                ? 0x00UL
                                                : SST_TLODDITHER);
    
    _GlideRoot.environment.nColorBuffer      = GLIDE_GETENV("FX_GLIDE_ALLOC_COLOR", -1L);
    _GlideRoot.environment.nAuxBuffer        = GLIDE_GETENV("FX_GLIDE_ALLOC_AUX", -1L);    
    _GlideRoot.environment.swFifoLWM         = GLIDE_GETENV("FX_GLIDE_LWM", -1L);

    _GlideRoot.environment.swapInterval      = GLIDE_GETENV("FX_GLIDE_SWAPINTERVAL", -1L);
    if ((envStr != NULL) && (_GlideRoot.environment.swapInterval < 0)) {
      _GlideRoot.environment.swapInterval = 0;
    }
    
    _GlideRoot.environment.snapshot          = GLIDE_GETENV("FX_SNAPSHOT", 0);
    
    GDBG_INFO(80,"    triBoundsCheck: %d\n",_GlideRoot.environment.triBoundsCheck);
    GDBG_INFO(80,"      swapInterval: %d\n",_GlideRoot.environment.swapInterval);
    GDBG_INFO(80,"          noSplash: %d\n",_GlideRoot.environment.noSplash);
    GDBG_INFO(80,"     shamelessPlug: %d\n",_GlideRoot.environment.shamelessPlug);
    GDBG_INFO(80,"               cpu: %d\n",_GlideRoot.CPUType);
    GDBG_INFO(80,"          snapshot: %d\n",_GlideRoot.environment.snapshot);
    GDBG_INFO(80,"  disableDitherSub: %d\n",_GlideRoot.environment.disableDitherSub);
  }

  /* constant pool */
  _GlideRoot.pool.f0   =   0.0F;
  _GlideRoot.pool.fHalf=   0.5F;
  _GlideRoot.pool.f1   =   1.0F;
  _GlideRoot.pool.f255 = 255.0F;

#if GLIDE_PACKED_RGB
  _GlideRoot.pool.fBiasHi = (float)(0x01 << 15);
  _GlideRoot.pool.fBiasLo = (float)(0x01 << 23);
#endif /* GLIDE_PACKED_RGB */

  _GlideRoot.current_sst = 0;                    /* make sure there's a valid GC */
  _GlideRoot.curGC = &_GlideRoot.GCs[0];         /* just for 'booting' the library */
  {
    const FxBool hwDetectP = _grSstDetectResources();

    if (!hwDetectP) {
      char s[128];
      const char* errStr = s;
      
      if (pciGetErrorCode() == PCI_ERR_NOERR) {
#ifndef __linux__
        sprintf(s, "%s: glide2x.dll expected %s, none detected\n",
                FN_NAME, GLIDE_DRIVER_NAME);
#else
        sprintf(s, "%s: libglide2x.so expected %s, none detected\n",
                FN_NAME, GLIDE_DRIVER_NAME);
#endif
      } else {
        errStr = pciGetErrorString();
      }
      
      GrErrorCallback(errStr, FXTRUE);
    }
    
    /* GMT: this isn't really necessary since GlideRoot is static */
    for (i = 0; i < _GlideRoot.hwConfig.num_sst; i++) {
      _GlideRoot.GCs[i].mm_table.free_mmid = 0;
      displayBoardInfo(i, &_GlideRoot.hwConfig);
    }
    
    _grMipMapInit();
    _GlideRoot.initialized = hwDetectP;               /* save this for the end */
  }
} /* _GlideInitEnvironment */
