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
 * 
 * 2     7/25/97 2:35p Joseph
 * Now calls pciClose before exiting.  The alleviates a problem that was
 * causing D3D apps to hang after mojo had been run.
 * 
 * 11    9/08/96 5:25p Dow
 * Changed to use init code instead of PCI library for mapping board
 * 
 * 10    7/20/96 11:11a Dow
 * Added code to handle bogus # of TMUs
 * 
 * 9     7/20/96 9:46a Dow
 * Removed double include of 3dfx.h and removed redefinition of
 * MAX_NUM_SST.
 * 
 * 8     6/25/96 11:58a Sellers
 * added TMU config
 * 
 * 7     6/07/96 5:55p Sellers
 * added -v option (verbose register output)
 * 
 * 6     6/07/96 5:06p Sellers
 * added fbi Power On Sense
 * 
 * 5     5/29/96 4:31p Dow
 * Got rid of fbiMemType reference
 * 
 * 4     4/05/96 9:51p Tarolli
 * fixed a spelling mistake
 * 
 * 3     4/04/96 11:16a Dow
 * move tmu memory printout 1 space right.
 * 
 * 2     4/04/96 11:09a Dow
 * added register init and changed memory printouts
 * 
 * 1     4/04/96 10:48a Dow
**
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <3dfx.h>
#include <fxpci.h>
#include <sst.h>
#include <sst1init.h>
#include <glide.h>              /* Included for declaration of MAX_NUM_SST */

char *_rgblanes_str[] = {"ARGB", "ABGR", "RGBA", "BGRA"};

char *_lfbformat_str[] = { "565",  "555", "1555", "**3*",
                           "888", "8888", "**6*", "**7*",
                           "**8*", "**9*", "*10*", "*11*",
                           "Z565", "Z555", "Z1555","ZZ"};

void dumpBasics();
void dumpRegisters();

void main(int argc, char **argv) {
    int verbose = 0;

    while(--argc && (**(++argv))=='-') {
        switch( *(*argv+1)) {
            case 'v':
                verbose = 1;
                break;
            default:
                fprintf(stderr, "usage: mojo [-v]\n");
                exit(0);
                break;
        }
    }
    if(verbose)
        dumpRegisters();
    else
        dumpBasics();
}

void dumpBasics() {
  FxU32
    tmu,
    boardsFound = 0,
    boardNum,                   /* Which board? */
    *sst;                       /* ptr to Voodoo */
  sst1DeviceInfoStruct
    devInfo;

  for (boardNum = 0; boardNum < MAX_NUM_SST; boardNum++) {
    if ((sst = sst1InitMapBoard(boardNum)) == NULL)
        break;

    sst1InitRegisters(sst);

    if (sst1InitGetDeviceInfo(sst, &devInfo) == FXFALSE) {
      fprintf(stderr, "Couldn't get info for Voodoo # %d\n", boardNum);
      exit(-1);
    }

    ++boardsFound;
    printf("Info for Voodoo board # %d:\n", boardNum);
    printf("=====================================================\n");
    printf("Virtual Base Address:                       0x%x\n",
           devInfo.virtAddr); 
    printf("Physical Base Address:                      0x%x\n",
           devInfo.physAddr); 
    printf("PCI Device Number:                          0x%x\n", 
           devInfo.deviceNumber);
    printf("Vendor ID:                                  0x%x\n",
           devInfo.vendorID);
    printf("Device ID:                                  0x%x\n",
           devInfo.deviceID);
    printf("FBI Revision:                               %d\n",
           devInfo.fbiRevision);
    printf("FBI Memory:                                 %d MB\n",
           devInfo.fbiMemSize);
    printf("FBI PowerOn Sense:                          0x%x\n",
           devInfo.fbiConfig);
    printf("TMU PowerOn Sense:                          0x%x\n",
           devInfo.tmuConfig);
    printf("FBI DAC Output Color Format:                %s\n",
           (devInfo.fbiVideo16BPP) ? "16BPP" : "24BPP");
    printf("Scan-Line Interleaved?                      %s\n",
           (devInfo.sstSliDetect) ? "Yes" : "No");
    printf("TMU Revision:                               %d\n",
           devInfo.tmuRevision);
    printf("Number TMUs:                                %d\n",
           devInfo.numberTmus);

    if (devInfo.numberTmus > GLIDE_NUM_TMU) {
      printf("WARNING: Board %d:  Bogus number of TMUs (%d)!\n", 
              boardNum, devInfo.numberTmus);
      continue;
    }

    for (tmu = 0; tmu <  devInfo.numberTmus; tmu++) {
      printf("TMU %d RAM:                                  %d MB\n",
             tmu, devInfo.tmuMemSize[tmu]);
    }
    sst1InitShutdown(sst);
  }

  if (boardsFound == 0) {
    fprintf(stderr, "No Voodoo boards found\n");
    exit(-1);
  }

  pciClose();


} /* dumpBasics */


void dumpRegisters()
{
    FxU32 boardsFound = 0;
    FxU32 boardNum;
    FxU32 x;
    Sstregs *sst;

    for (boardNum = 0; boardNum < MAX_NUM_SST; boardNum++) {
        if ((sst = (Sstregs *) sst1InitMapBoard(boardNum)) == NULL)
             break;
        ++boardsFound;

#define OFFSET(a,b) (((int)(&a->b)) - (int)a)

        printf("  Register Name      Data  Address\n");
        printf("---------------  -------- --------\n");
    
        //--------------------------------------------------
        x = sst->status;
        printf("         status: %08x      %3x\n",x,OFFSET(sst,status));
        printf("\t\t       %02x : pci fifo free space (%d)\n",
            x&SST_FIFOLEVEL,x&SST_FIFOLEVEL);
        printf("\t\t\t%d : vertical retrace\n",(x&SST_VRETRACE)!=0);
        printf("\t\t\t%d : fbi busy\n",(x&SST_FBI_BUSY)!=0);
        printf("\t\t\t%d : tmu busy\n",(x&SST_TREX_BUSY)!=0);
        printf("\t\t\t%d : sst busy\n",(x&SST_BUSY)!=0);
        printf("\t\t\t%d : displayed buffer\n",
            (x&SST_DISPLAYED_BUFFER)>>SST_DISPLAYED_BUFFER_SHIFT);
        printf("\t\t     %04x : mem fifo free space (%d)\n",
            (x&SST_MEMFIFOLEVEL)>>SST_MEMFIFOLEVEL_SHIFT,
            (x&SST_MEMFIFOLEVEL)>>SST_MEMFIFOLEVEL_SHIFT);
        printf("\t\t\t%d : swap buffers pending\n",
            (x&SST_SWAPBUFPENDING)>>SST_SWAPBUFPENDING_SHIFT);

        //--------------------------------------------------
        x = sst->fbzColorPath;
        printf("   fbzColorPath: %08x      %3x\n",x,OFFSET(sst,fbzColorPath));
        //--------------------------------------------------
        x = sst->fogMode;
        printf("        fogMode: %08x      %3x\n",x,OFFSET(sst,fogMode));
        if (x & SST_ENFOGGING ) printf("\t\t\t  : ENFOGGING\n");
        if (x & SST_FOGADD ) printf("\t\t\t  : FOGADD\n");
        if (x & SST_FOGMULT ) printf("\t\t\t  : FOGMULT\n");
        if (x & SST_FOG_ALPHA ) printf("\t\t\t  : FOG_ALPHA\n");
        if (x & SST_FOG_Z ) printf("\t\t\t  : FOG_Z\n");
        if (x & SST_FOG_CONSTANT) printf("\t\t\t  : FOG_CONSTANT\n");
        //--------------------------------------------------
        x = sst->alphaMode;
        printf("      alphaMode: %08x      %3x\n",x,OFFSET(sst,alphaMode));
        //--------------------------------------------------
        x = sst->fbzMode;
        printf("        fbzMode: %08x      %3x\n",x,OFFSET(sst,fbzMode));
        if (x & SST_ENRECTCLIP ) printf("\t\t\t  : ENRECTCLIP\n");
        if (x & SST_ENCHROMAKEY ) printf("\t\t\t  : ENCHROMAKEY\n");
        if (x & SST_ENSTIPPLE ) printf("\t\t\t  : ENSTIPPLE\n");
        if (x & SST_WBUFFER ) printf("\t\t\t  : WBUFFER\n");
        if (x & SST_ENDEPTHBUFFER) printf("\t\t\t  : ENDEPTHBUFFER\n");
        printf("\t\t      %s%s%s : zfunction\n",
        x & SST_ZFUNC_LT ? "<" : " ",
        x & SST_ZFUNC_GT ? ">" : " ",
        x & SST_ZFUNC_EQ ? "=" : " ");
        if (x & SST_ENDITHER ) printf("\t\t\t  : ENDITHER\n");
        if (x & SST_RGBWRMASK ) printf("\t\t\t  : RGBWRMASK\n");
        if (x & SST_ZAWRMASK ) printf("\t\t\t  : ZAWRMASK\n");
        if (x & SST_DITHER2x2 ) printf("\t\t\t  : DITHER2x2\n");
        if (x & SST_ENSTIPPLEPATTERN) printf("\t\t\t  : ENSTIPPLEPATTERN\n");
        if (x & SST_ENALPHAMASK ) printf("\t\t\t  : ENALPHAMASK\n");
        printf("\t\t\t%d : drawbuffer (0=front, 1=back)\n",
        (x & SST_DRAWBUFFER)>>SST_DRAWBUFFER_SHIFT);
        if (x & SST_ENZBIAS ) printf("\t\t\t  : ENZBIAS\n");
        if (x & SST_YORIGIN ) printf("\t\t\t  : YORIGIN\n");
        if (x & SST_ENALPHABUFFER) printf("\t\t\t  : ENALPHABUFFER\n");
        if (x & SST_ENDITHERSUBTRACT) printf("\t\t\t  : ENDITHERSUBTRACT\n");
        //--------------------------------------------------
        x = sst->lfbMode;
        printf("        lfbMode: %08x      %3x\n",x,OFFSET(sst,lfbMode));
        printf("\t\t    %5s : lfb format\n",
        _lfbformat_str[(x & SST_LFB_FORMAT)>>SST_LFB_FORMAT_SHIFT]);
        printf("\t\t\t%d : writebuffer (0=front, 1=back, 2=aux)\n",
        (x & SST_LFB_WRITEBUFSELECT)>>SST_LFB_WRITEBUFSELECT_SHIFT);
        printf("\t\t\t%d : readbuffer (0=front, 1=back, 2=aux)\n",
        (x & SST_LFB_READBUFSELECT)>>SST_LFB_READBUFSELECT_SHIFT);
        printf("\t\t     %4s : rgba lanes\n",
        _rgblanes_str[(x & SST_LFB_RGBALANES)>>SST_LFB_RGBALANES_SHIFT]);
        if (x & SST_LFB_ENPIXPIPE ) printf("\t\t\t  : ENPIXPIPE\n");
        if (x & SST_LFB_WRITE_SWAP16 ) printf("\t\t\t  : WRITE_SWAP16\n");
        if (x & SST_LFB_WRITE_BYTESWAP ) printf("\t\t\t  : WRITE_BYTESWAP\n");
        if (x & SST_LFB_YORIGIN) printf("\t\t\t  : YORIGIN\n");
        if (x & SST_LFB_WSELECT) printf("\t\t\t  : WSELECT\n");
        if (x & SST_LFB_READ_SWAP16 ) printf("\t\t\t  : READ_SWAP16 \n");
        if (x & SST_LFB_READ_BYTESWAP ) printf("\t\t\t  : READ_BYTESWAP\n");
        //--------------------------------------------------
        x = sst->clipLeftRight;
        printf("  clipLeftRight: %08x      %3x\n",x,OFFSET(sst,clipLeftRight));
        x = sst->clipBottomTop;
        printf("  clipBottomTop: %08x      %3x\n",x,OFFSET(sst,clipBottomTop));
        x = sst->stipple;
        printf("        stipple: %08x      %3x\n",x,OFFSET(sst,stipple));
        x = sst->c0;
        printf("             c0: %08x      %3x\n",x,OFFSET(sst,c0));
        x = sst->c1;
        printf("             c1: %08x      %3x\n",x,OFFSET(sst,c1));
        x = sst->stats.fbiPixelsIn;
        printf("    fbiPixelsIn: %08x      %3x\n",x,OFFSET(sst,stats.fbiPixelsIn));
        x = sst->stats.fbiChromaFail;
        printf("  fbiChromaFail: %08x      %3x\n",x,OFFSET(sst,stats.fbiChromaFail));
        x = sst->stats.fbiZfuncFail;
        printf("   fbiZfuncFail: %08x      %3x\n",x,OFFSET(sst,stats.fbiZfuncFail));
        x = sst->stats.fbiAfuncFail;
        printf("   fbiAfuncFail: %08x      %3x\n",x,OFFSET(sst,stats.fbiAfuncFail));
        x = sst->stats.fbiPixelsOut;
        printf("   fbiPixelsOut: %08x      %3x\n",x,OFFSET(sst,stats.fbiPixelsOut));
        x = sst->fbiInit4;
        printf("       fbiInit4: %08x      %3x\n",x,OFFSET(sst,fbiInit4));
        x = sst->vRetrace;
        printf("       vRetrace: %08x      %3x\n",x,OFFSET(sst,vRetrace));
        x = sst->backPorch;
        printf("      backPorch: %08x      %3x\n",x,OFFSET(sst,backPorch));
        x = sst->videoDimensions;
        printf("videoDimensions: %08x      %3x\n",x,OFFSET(sst,videoDimensions));
        x = sst->fbiInit0;
        printf("       fbiInit0: %08x      %3x\n",x,OFFSET(sst,fbiInit0));
        x = sst->fbiInit1;
        printf("       fbiInit1: %08x      %3x\n",x,OFFSET(sst,fbiInit1));
        x = sst->fbiInit2;
        printf("       fbiInit2: %08x      %3x\n",x,OFFSET(sst,fbiInit2));
        x = sst->fbiInit3;
        printf("       fbiInit3: %08x      %3x\n",x,OFFSET(sst,fbiInit3));
    }

    if (boardsFound == 0) {
        fprintf(stderr, "No Voodoo boards found\n");
        exit(-1);
    }

    pciClose();
} /* dumpRegisters */
