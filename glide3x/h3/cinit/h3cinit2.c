/*
 * non-BIOS mode switch for 3dfx Voodoo3/Avenger
 * Thx to Hannu Mallat <hmallat@cc.hut.fi> for his tdfxfb.c
 *
 *  Copyright (c) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include <stdlib.h>

#include "glide.h"

#ifdef __DJGPP__
#include <sys/exceptn.h>
#include <sys/farptr.h>
#define __INLINE __inline
#else
#define __INLINE
#endif
 
/* 8.3 fun */
#ifdef __DOS32__
#include "h3cini~1.h"
#else
#include "h3cinitdd.h"
#endif
#include "h3defs.h"
#include "h3regs.h"

extern FxU16 mode_table[][24];

#define MISC_W	0x3c2
#define MISC_R	0x3cc
#define SEQ_I	0x3c4
#define SEQ_D	0x3c5
#define CRT_I	0x3d4
#define CRT_D	0x3d5
#define ATT_IW	0x3c0
#define IS1_R	0x3da
#define GRA_I	0x3ce
#define GRA_D	0x3cf

static __INLINE FxU8 vga_inb (FxU32 reg) { return _inp(reg); }
static __INLINE void vga_outb (FxU32 reg, FxU8 val) { _outp(reg, val); }

static FxU8 seq_inb (FxU32 idx) { vga_outb(SEQ_I, idx); return vga_inb(SEQ_D); }
static void seq_outb (FxU32 idx, FxU8 val) { vga_outb(SEQ_I, idx); vga_outb(SEQ_D, val); }

static void crt_outb (FxU32 idx, FxU8 val) { vga_outb(CRT_I, idx); vga_outb(CRT_D, val); }
static FxU8 crt_inb (FxU32 idx) { vga_outb(CRT_I, idx); return vga_inb(CRT_D); }

static void gra_outb (FxU32 idx, FxU8 val) { vga_outb(GRA_I, idx); vga_outb(GRA_D, val); }

static void att_outb (FxU32 idx, FxU8 val)
{
 FxU8 tmp;
 tmp = vga_inb(IS1_R);
 vga_outb(ATT_IW, idx);
 vga_outb(ATT_IW, val);
}

static void vga_enable_video (void)
{
 FxU8 s;
 s = seq_inb(0x01) & 0xdf;
 seq_outb(0x00, 0x01);
 seq_outb(0x01, s);
 seq_outb(0x00, 0x03);
}

static FxU8 reg_crt[25];
static FxU8 reg_gra[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0f, 0xff };
static FxU8 reg_seq[] = { 0x03, 0x01 /* fixme: clkdiv2? */, 0x0f, 0x00, 0x0e };
static FxU8 reg_att[] = {
       0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
       0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
       0x41, 0x00, 0x0f, 0x00, 0x00
};

static void fill_crt (FxU16 *reg)
{
 int i, r;

 for (i=0, r=0; i<8; i++, r++) {
     reg_crt[i] = reg[r];
 }
 reg_crt[i+1] = reg[r++];
 for (i=0x10; i<0x13; i++, r++) {
     reg_crt[i] = reg[r];
 }
 reg_crt[i++] = reg[1];
 for (++i; i<0x17; i++, r++) {
     reg_crt[i] = reg[r];
 }
 reg_crt[i] = 0xc3;
 reg_crt[i+1] = 0xff;
}

/* This is basically just:
 *   h3InitFindVideoMode(xRes, yRes, refresh);
 * only `refresh' is approximated.
 */
static FxU16 *h3ScanVideoMode (FxU32 xRes, FxU32 yRes, FxU32 refresh)
{
 int i = 0;
 int best = -1;
 FxU32 error = -1;

 while (mode_table[i][0] != 0) {
       if ((mode_table[i][0] == xRes) && (mode_table[i][1] == yRes)) {
          FxU32 d = abs(mode_table[i][2] - refresh);
          if (error > d) {
             error = d;
             best = i;
          }
       }

       i++;
 }

 return (best == -1) ? NULL : (&mode_table[best][3]);
}

/* This is basically just:
 *   h3InitSetVideoMode(regBase, xRes, yRes, refresh, FXFALSE);
 *   ISET32(vidProcCfg, IGET32(vidProcCfg) | SST_VIDEO_PROCESSOR_EN);
 */
FxBool h3VideoMode (FxU32 regBase, FxU32 xRes, FxU32 yRes, FxU32 refresh)
{
 int i;
 FxU16 vdispend;
 FxU16 *rs = h3ScanVideoMode(xRes, yRes, refresh);
 FxU32 vidProcCfg;

 if (rs == NULL) {
    return FXFALSE;
 }

 fill_crt(rs);
 reg_seq[1] = rs[17] & ((FxU16) ~BIT(5));
 vdispend = (((rs[15]<<8)&0x400) | ((rs[7]<<3)&0x200) | ((rs[7]<<7)&0x100) | rs[11]) + 1;

 /*
  * All hell breaks loose
  */

 crt_outb(0x11, crt_inb(0x11) & 0x7f); /* CRT unprotect */

 ISET32(vgaInit1, IGET32(vgaInit1) & 0x001FFFFF);

 vidProcCfg = IGET32(vidProcCfg);
 ISET32(vidProcCfg, vidProcCfg & ~SST_VIDEO_PROCESSOR_EN);

 ISET32(pllCtrl0, (rs[19] << 8) | rs[18]);

 vga_outb(MISC_W, rs[16] | BIT(0));

 for (i=0; i<5; i++)
     seq_outb(i, reg_seq[i]);

 for (i=0; i<25; i++)
     crt_outb(i, reg_crt[i]);

 for (i=0; i<9; i++)
     gra_outb(i, reg_gra[i]);

 for (i=0; i<21; i++)
     att_outb(i, reg_att[i]);

 crt_outb(0x1a, rs[14]);
 crt_outb(0x1b, rs[15]);

 vga_enable_video();

 ISET32(vgaInit0, IGET32(vgaInit0)|BIT(12));
 ISET32(dacMode,  rs[20]);

 vidProcCfg &= ~(SST_VIDEO_2X_MODE_EN | SST_HALF_MODE);
 if (vdispend != yRes) {
    vidProcCfg |= SST_HALF_MODE;
    yRes <<= 1;
 }
 if (rs[20])
    vidProcCfg |= SST_VIDEO_2X_MODE_EN;

 ISET32(vidScreenSize, (yRes << 12) | (xRes & 0xfff));
 ISET32(vidOverlayStartCoords, 0);
 ISET32(vidOverlayEndScreenCoord, (((yRes - 1) << 12) | ((xRes - 1) & 0xfff)));

 ISET32(vidProcCfg, vidProcCfg | SST_VIDEO_PROCESSOR_EN);

#ifdef __DJGPP__
 /* Inform BIOS we're leaving for exotic places, but don't tell exactly where */
 _farpokew(__djgpp_dos_sel, 0x449, 0x7f | (xRes<<5));
#endif
 return FXTRUE; /* success! */
}
