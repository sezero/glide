#ifndef __H3REGS_H__
#define __H3REGS_H__

/* -*-c++-*- */
/* $Header$ */
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
** $Revision$
** $Date$
**
** $History: h3regs.h $
** 
** *****************  Version 1  *****************
** User: Sapphire     Date: 3/16/99    Time: 7:28p
** Created in $/Releases/Voodoo3/V3_OEM_100/3dfx/devel/H3/incsrc
** 
** *****************  Version 21  *****************
** User: Stb_srogers  Date: 2/09/99    Time: 9:55a
** Updated in $/devel/h3/win95/dx/inc
** 
** *****************  Version 20  *****************
** User: Michael      Date: 9/08/98    Time: 4:16p
** Updated in $/devel/h3/Win95/dx/inc
** MarkL's fix for 2281.  Modify members of "stats" structure from ints to
** longs.
*/

// STB Begin Changes
// STB-SR 1/13/98 Adding code for bj
#ifdef _H2INC
typedef unsigned long   FxU32;
#endif
// STB End Changes

//----------------- SST chip I/O layout -------------------------
// I/O registers remapped into memory space
// Includes init, dac/pll, video, and VGA registers
typedef volatile struct sstioregs {     

  // Init Registers
    FxU32 status;                       // aliased status register
    FxU32 pciInit0;
    FxU32 sipMonitor;
    FxU32 lfbMemoryConfig;
    FxU32 miscInit0;
    FxU32 miscInit1;
    FxU32 dramInit0;
    FxU32 dramInit1;
    FxU32 agpInit;
    FxU32 tmuGbeInit;
    FxU32 vgaInit0;
    FxU32 vgaInit1;
    FxU32 dramCommand;
    FxU32 dramData;
    FxU32 reservedZ[2];

  // PLL Registers
    FxU32 pllCtrl0;
    FxU32 pllCtrl1;
    FxU32 pllCtrl2;

  // DAC Registers
    FxU32 dacMode;
    FxU32 dacAddr;
    FxU32 dacData;

  // Video Registers I
    FxU32 vidMaxRGBDelta;
    FxU32 vidProcCfg;
    FxU32 hwCurPatAddr;
    FxU32 hwCurLoc;
    FxU32 hwCurC0;
    FxU32 hwCurC1;
    FxU32 vidInFormat;
    FxU32 vidInStatus;
    FxU32 vidSerialParallelPort;
    FxU32 vidInXDecimDeltas;
    FxU32 vidInDecimInitErrs;
    FxU32 vidInYDecimDeltas;
    FxU32 vidPixelBufThold;
    FxU32 vidChromaMin;
    FxU32 vidChromaMax;
    FxU32 vidCurrentLine;
    FxU32 vidScreenSize;
    FxU32 vidOverlayStartCoords;
    FxU32 vidOverlayEndScreenCoord;
    FxU32 vidOverlayDudx;
    FxU32 vidOverlayDudxOffsetSrcWidth;
    FxU32 vidOverlayDvdy;

  // VGA Registers
    FxU32 vgaRegister[12];

  // Video Registers II
    FxU32 vidOverlayDvdyOffset;
    FxU32 vidDesktopStartAddr;
    FxU32 vidDesktopOverlayStride;
    FxU32 vidInAddr0;
    FxU32 vidInAddr1;
    FxU32 vidInAddr2;
    FxU32 vidInStride;
    FxU32 vidCurrOverlayStartAddr;
} SstIORegs;

typedef volatile struct cmdfifo {
    FxU32 baseAddrL;
    FxU32 baseSize;
    FxU32 bump;
    FxU32 readPtrL;
    FxU32 readPtrH;
    FxU32 aMin;
    FxU32 unusedA;
    FxU32 aMax;
    FxU32 unusedB;
    FxU32 depth;
    FxU32 holeCount;
    FxU32 reserved;
} CmdFifo;

//---------- SST chip AGP/CMD Transfer/Misc Register layout ------------------
typedef volatile struct sstcregs {
  // AGP
    FxU32 agpReqSize;
    FxU32 hostAddrLow;
    FxU32 hostAddrHigh;
    FxU32 graphicsAddr;
    FxU32 graphicsStride;
    FxU32 moveCMD;
    FxU32 reservedL[2];

  // CMD FIFO 0,1
    CmdFifo cmdFifo0;
    CmdFifo cmdFifo1;

    FxU32 cmdFifoThresh;
    FxU32 cmdHoleInit;
    FxU32 reservedO[6];
    FxU32 reservedP[8];
    FxU32 reservedQ[8];
    FxU32 reservedR[8];
  // misc
    FxU32 yuvBaseAddr;
    FxU32 yuvStride;
    FxU32 reservedS[6];
    FxU32 crc1;
    FxU32 reservedT[3];
    FxU32 crc2;
} SstCRegs;

//----------------- SST chip 2D layout -------------------------
typedef volatile struct sstgregs {      // THE 2D CHIP
    FxU32 status;                       // aliased status register
    FxU32 unused0;
    FxU32 clip0min;
    FxU32 clip0max;
    FxU32 dstBaseAddr;
    FxU32 dstFormat;
    FxU32 srcColorkeyMin;
    FxU32 srcColorkeyMax;
    FxU32 dstColorkeyMin;
    FxU32 dstColorkeyMax;
    FxU32 bresError0;
    FxU32 bresError1;
    FxU32 rop;
    FxU32 srcBaseAddr;
    FxU32 commandEx;
    FxU32 lineStipple;
    FxU32 lineStyle;
    FxU32 pattern0alias;
    FxU32 pattern1alias;
    FxU32 clip1min;
    FxU32 clip1max;
    FxU32 srcFormat;
    FxU32 srcSize;
    FxU32 srcXY;
    FxU32 colorBack;
    FxU32 colorFore;
    FxU32 dstSize;
    FxU32 dstXY;
    FxU32 command;
    FxU32 reserved[3];
    FxU32 launch[32];
    FxU32 colorPattern[64];
#if COLORTRANSLUT
    FxU32 colorTransLut[256];
#endif
} SstGRegs;

// STB Begin changes
// STB-SR 1/13/99 Adding code for bj
#ifndef _H2INC
// STB End changes

#if defined(__alpha__) || defined(__LP64__)
typedef unsigned int Reg32u;
typedef int Reg32;
#else
typedef unsigned long Reg32u;
typedef long Reg32;
#endif

//----------------- SST chip 3D layout -------------------------
// registers are in groups of 8 for easy decode
typedef struct vertex_Rec {
    Reg32u x;            // 12.4 format
    Reg32u y;            // 12.4
} vtxRec;

typedef volatile struct sstregs {       // THE 3D CHIP
                                        // EXTERNAL registers
    FxU32 status;               // chip status, Read Only
    FxU32 intrCtrl;             // interrupt control
    vtxRec vA;                  // Vertex A,B,C
    vtxRec vB;
    vtxRec vC;

    Reg32 r;             // 12.12        Parameters
    Reg32 g;             // 12.12
    Reg32 b;             // 12.12
    Reg32 z;             // 20.12
    Reg32 a;             // 12.12
    Reg32 s;             // 14.18
    Reg32 t;             // 14.18
    Reg32 w;             //  2.30

    Reg32 drdx;                  // X Gradients
    Reg32 dgdx;
    Reg32 dbdx;
    Reg32 dzdx;
    Reg32 dadx;
    Reg32 dsdx;
    Reg32 dtdx;
    Reg32 dwdx;

    Reg32 drdy;                  // Y Gradients
    Reg32 dgdy;
    Reg32 dbdy;
    Reg32 dzdy;
    Reg32 dady;
    Reg32 dsdy;
    Reg32 dtdy;
    Reg32 dwdy;

    Reg32u triangleCMD;  // execute a triangle command (float)
    Reg32u reservedA;
    vtxRec FvA;                 // floating point version
    vtxRec FvB;
    vtxRec FvC;

    Reg32 Fr;                    // floating point version
    Reg32 Fg;
    Reg32 Fb;
    Reg32 Fz;
    Reg32 Fa;
    Reg32 Fs;
    Reg32 Ft;
    Reg32 Fw;

    Reg32 Fdrdx;
    Reg32 Fdgdx;
    Reg32 Fdbdx;
    Reg32 Fdzdx;
    Reg32 Fdadx;
    Reg32 Fdsdx;
    Reg32 Fdtdx;
    Reg32 Fdwdx;

    Reg32 Fdrdy;
    Reg32 Fdgdy;
    Reg32 Fdbdy;
    Reg32 Fdzdy;
    Reg32 Fdady;
    Reg32 Fdsdy;
    Reg32 Fdtdy;
    Reg32 Fdwdy;

    Reg32u FtriangleCMD;         // execute a triangle command
    Reg32u fbzColorPath;         // color select and combine
    Reg32u fogMode;              // fog Mode
    Reg32u alphaMode;            // alpha Mode
    Reg32u fbzMode;              // framebuffer and Z mode
    Reg32u lfbMode;              // linear framebuffer Mode
    Reg32u clipLeftRight;        // (6)10(6)10
    Reg32u clipBottomTop;        // (6)10(6)10

    Reg32u nopCMD;       // execute a nop command
    Reg32u fastfillCMD;  // execute a fast fill command
    Reg32u swapbufferCMD;// execute a swapbuffer command
    Reg32u fogColor;             // (8)888
    Reg32u zaColor;              // 8(8)16
    Reg32u chromaKey;            // (8)888
    Reg32u chromaRange;
    Reg32u userIntrCmd;

    Reg32u stipple;              // 32 bits, MSB masks pixels
    Reg32u c0;                   // 8.8.8.8 (ARGB)
    Reg32u c1;                   // 8.8.8.8 (ARGB)
    struct {                            // statistic gathering variables
        Reg32u fbiPixelsIn;
        Reg32u fbiChromaFail;
        Reg32u fbiZfuncFail;
        Reg32u fbiAfuncFail;
        Reg32u fbiPixelsOut;
    } stats;

    Reg32u fogTable[32];         // 64 entries, 2 per word, 2 bytes each

    Reg32u reservedB[3];

    Reg32u colBufferAddr;
    Reg32u colBufferStride;
    Reg32u auxBufferAddr;
    Reg32u auxBufferStride;
    Reg32u reservedC;

    Reg32u clipLeftRight1;
    Reg32u clipBottomTop1;
    Reg32u reservedD[6];         // NOTE: used to store TMUprivate ptr
                                        

    Reg32u reservedE[8];

    Reg32u reservedF[3];  
    Reg32u swapBufferPend;
    Reg32u leftOverlayBuf;
    Reg32u rightOverlayBuf;
    Reg32u fbiSwapHistory;
    Reg32u fbiTrianglesOut;      // triangles out counter

    FxU32 sSetupMode;
    FxU32 sVx;
    FxU32 sVy;
    FxU32 sARGB;
    FxU32 sRed;
    FxU32 sGreen;
    FxU32 sBlue;
    FxU32 sAlpha;

    FxU32 sVz;
    FxU32 sOowfbi;
    FxU32 sOow0;
    FxU32 sSow0;
    FxU32 sTow0;
    FxU32 sOow1;
    FxU32 sSow1;
    FxU32 sTow1;

    FxU32 sDrawTriCMD;
    FxU32 sBeginTriCMD;
    Reg32u reservedG[6];

    Reg32u reservedH[8];

    Reg32u reservedI[8];

    Reg32u textureMode;          // texture Mode
    Reg32u tLOD;                 // texture LOD settings
    Reg32u tDetail;              // texture detail settings
    Reg32u texBaseAddr;          // current texture base address
    Reg32u texBaseAddr1;
    Reg32u texBaseAddr2;
    Reg32u texBaseAddr38;
    Reg32u trexInit0;            // hardware init bits
    Reg32u trexInit1;            // hardware init bits
   
    Reg32u nccTable0[12];        // NCC decode tables, bits are packed
    Reg32u nccTable1[12];        // 4 words Y, 4 words I, 4 words Q

    Reg32u tChromaKeyMin;
    Reg32u tChromaKeyMax;
} SstRegs;

// STB Begin changes
#endif // #ifndef _H2INC
// STB End changes

#endif /* !__H3REGS_H__ */
