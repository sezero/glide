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
*/

#include <stdlib.h>
#include <stdio.h>
#include <3dfx.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <machine/cpufunc.h>
#include <fcntl.h>
#include "fxpci.h"
#include "pcilib.h"

static const char* pciIdentifyFreeBSD(void);
static FxBool pciOutputStringFreeBSD(const char *msg);
static FxBool pciInitializeFreeBSD(void);
static FxBool pciShutdownFreeBSD(void);
static FxBool pciMapLinearFreeBSD(FxU32, FxU32 physical_addr, FxU32 *linear_addr,
				FxU32 *length);
static FxBool pciUnmapLinearFreeBSD(FxU32 linear_addr, FxU32 length);
static FxBool pciSetPermissionFreeBSD(const FxU32, const FxU32, const FxBool);
static FxU8 pciPortInByteFreeBSD(unsigned short port);
static FxU16 pciPortInWordFreeBSD(unsigned short port);
static FxU32 pciPortInLongFreeBSD(unsigned short port);
static FxBool pciPortOutByteFreeBSD(unsigned short port, FxU8 data);
static FxBool pciPortOutWordFreeBSD(unsigned short port, FxU16 data);
static FxBool pciPortOutLongFreeBSD(unsigned short port, FxU32 data);
static FxBool pciMsrGetFreeBSD(MSRInfo *, MSRInfo *);
static FxBool pciMsrSetFreeBSD(MSRInfo *, MSRInfo *);
static FxBool pciSetPassThroughBaseFreeBSD(FxU32 *, FxU32);

const FxPlatformIOProcs ioProcsFreeBSD = {
  pciInitializeFreeBSD,
  pciShutdownFreeBSD,
  pciIdentifyFreeBSD,

  pciPortInByteFreeBSD,
  pciPortInWordFreeBSD,
  pciPortInLongFreeBSD,

  pciPortOutByteFreeBSD,
  pciPortOutWordFreeBSD,
  pciPortOutLongFreeBSD,

  pciMapLinearFreeBSD,
  pciUnmapLinearFreeBSD,
  pciSetPermissionFreeBSD,

  pciMsrGetFreeBSD,
  pciMsrSetFreeBSD,

  pciOutputStringFreeBSD,
  pciSetPassThroughBaseFreeBSD
};

const FxU32 PCI_VENDOR_ID_FREEBSD = 0x0;
const FxU32 PCI_DEVICE_ID_FREEBSD = 0x2;
const FxU32 PCI_COMMAND_FREEBSD = 0x4;
const FxU32 PCI_BASE_ADDRESS_0_FREEBSD = 0x10;
const FxU32 SST1_PCI_INIT_ENABLE_FREEBSD = 0x40;
const FxU32 SST1_PCI_BUS_SNOOP0_FREEBSD = 0x44;
const FxU32 SST1_PCI_BUS_SNOOP1_FREEBSD = 0x48;

static int freebsdDevFd=-1;
static int freebsdIoFd=-1;

struct pioData {
  short port;
  short size;
  int device;
  void *value;
};

FxBool
pciPlatformInit(void)
{
  gCurPlatformIO = &ioProcsFreeBSD;
  return FXTRUE;
}

FxBool 
hasDev3DfxLinux(void) 
{
  if (freebsdDevFd==-1) return FXFALSE;
  return FXTRUE;
}

FxU32
pciFetchRegisterLinux(FxU32 cmd, FxU32 size, FxU32 device) 
{
  struct pioData desc;
  char cval;
  short sval;
  int ival;

  if (freebsdDevFd==-1) return -1;
  desc.port=cmd;
  desc.size=size;
  desc.device=device;
  switch (size) {
  case 1:
    desc.value=&cval;
    break;
  case 2:
    desc.value=&sval;
    break;
  case 4:
    desc.value=&ival;
    break;
  default:
    return 0;
  }
  if (ioctl(freebsdDevFd, _IOR('3', 3, sizeof(struct pioData)), &desc)==-1)
    return 0;
  switch (size) {
  case 1:
    return cval;
  case 2:
    return sval;
  case 4:
    return ival;
  default:
    return 0;
  }
}

FxBool
pciUpdateRegisterLinux(FxU32 cmd, FxU32 data, FxU32 size, FxU32 device) 
{
  struct pioData desc;

  if (freebsdDevFd==-1) return -1;
  desc.port=cmd;
  desc.size=size;
  desc.device=device;
  desc.value=&data;
  if (ioctl(freebsdDevFd, _IOW('3', 4, sizeof(struct pioData)), &desc)==-1)
    return FXFALSE;
  return FXTRUE;
}

int 
getNumDevicesLinux(void) 
{
  if (freebsdDevFd==-1) return -1;
  return ioctl(freebsdDevFd, _IO('3', 2));
}

static const char*
pciIdentifyFreeBSD(void)
{
  return "fxPCI for FreeBSD";
}

static FxBool 
pciOutputStringFreeBSD(const char *msg) 
{
  printf(msg);
  return FXTRUE;
}

static FxBool
pciInitializeFreeBSD(void)
{
  if (geteuid()) freebsdDevFd=open("/dev/3dfx", O_RDWR, 0);
  if (freebsdDevFd==-1) {
    freebsdIoFd=open("/dev/io", O_RDWR, 0);
    if (freebsdIoFd<0) {
      pciErrorCode = PCI_ERR_NO_IO_PERM;
      return FXFALSE;
    }
  }
  return FXTRUE;
}

static FxBool
pciShutdownFreeBSD(void)
{
  if (freebsdDevFd!=-1) close(freebsdDevFd);
  freebsdDevFd=-1;
  if (freebsdIoFd!=-1) close(freebsdIoFd);
  freebsdIoFd=-1;
  return FXTRUE;
}

static FxBool 
pciMapLinearFreeBSD(FxU32 bus, FxU32 physical_addr,
		    FxU32 *linear_addr, FxU32 *length)
{
  int fd;
  if (freebsdDevFd!=-1) {
    fd=freebsdDevFd;
  } else {
    if ((fd=open("/dev/mem", O_RDWR))<0) {
      pciErrorCode=PCI_ERR_NO_MEM_PERM;
      return FXFALSE;
    }
  }
  if (((*linear_addr)=(FxU32)mmap(0, *length, PROT_READ|PROT_WRITE,
				  MAP_SHARED, fd, physical_addr))<0) {
    if (fd!=freebsdDevFd) close(fd);
    return FXFALSE;
  }
  if (fd!=freebsdDevFd) close(fd);
  return FXTRUE;
}

static FxBool
pciUnmapLinearFreeBSD( FxU32 linear_addr, FxU32 length ) {
  munmap((void*)linear_addr, length);
  return FXTRUE;
}

static FxBool
pciSetPermissionFreeBSD(const FxU32 addrBase, const FxU32 addrLen, 
			const FxBool writePermP)
{
  return FXTRUE;
}

static FxU8
pciPortInByteFreeBSD(unsigned short port)
{
  char tmp;
  struct pioData desc;

  if (freebsdDevFd==-1) {
    tmp=inb(port);
    /* fprintf(stderr, "Read byte at %x got %d\n", port, tmp); */
    return tmp;
  }
  desc.port=port;
  desc.size=sizeof(tmp);
  desc.value=&tmp;
  /* fprintf(stderr, "Read byte desc at %x tmp at %x\n", &desc, &tmp); */
  ioctl(freebsdDevFd, _IOR(0, 0, sizeof(struct pioData)), &desc);
  /* fprintf(stderr, "Got byte %d versus %d\n", tmp, inb(port)); */
  return tmp;
}

static FxU16
pciPortInWordFreeBSD(unsigned short port)
{
  short tmp;
  struct pioData desc;

  if (freebsdDevFd==-1) {
    tmp=inw(port);
    /* fprintf(stderr, "Read word at %x got %x\n", port, tmp); */
    return tmp;
  }
  desc.port=port;
  desc.size=sizeof(tmp);
  desc.value=&tmp;
  /* fprintf(stderr, "Read word desc at %x tmp at %x\n", &desc, &tmp); */
  ioctl(freebsdDevFd, _IOR(0, 0, sizeof(struct pioData)), &desc);
  /* fprintf(stderr, "Got word %d versus %d\n", tmp, inw(port)); */
  return tmp;
}

static FxU32
pciPortInLongFreeBSD(unsigned short port)
{
  int tmp;
  struct pioData desc;

  if (freebsdDevFd==-1) {
    tmp=inl(port);
    /* fprintf(stderr, "Read long at %x got %x\n", port, tmp); */
    return tmp;
  }
  desc.port=port;
  desc.size=sizeof(tmp);
  desc.value=&tmp;
  /* fprintf(stderr, "Read long desc at %x tmp at %x\n", &desc, &tmp); */
  ioctl(freebsdDevFd, _IOR(0, 0, sizeof(struct pioData)), &desc);
  /* fprintf(stderr, "Got long %x versus %x\n", tmp, inl(port)); */
  return tmp;
}

static FxBool
pciPortOutByteFreeBSD(unsigned short port, FxU8 data)
{
  struct pioData desc;
  /* fprintf(stderr, "write byte=%d desc at %x data at %x\n", data,
      &desc, &data); */
  if (freebsdDevFd==-1) {
    outb(port, data);
    return FXTRUE;
  }
  desc.port=port;
  desc.size=sizeof(data);
  desc.value=&data;
  return ioctl(freebsdDevFd, _IOW(0, 1, sizeof(struct pioData)), &desc)!=-1;
}

static FxBool
pciPortOutWordFreeBSD(unsigned short port, FxU16 data)
{
  struct pioData desc;
  /* fprintf(stderr, "write word=%x to port=%x desc at %x data at %x\n", 
	  data, port, &desc, &data); */
  if (freebsdDevFd==-1) {
    outw(port, data);
    return FXTRUE;
  }
  desc.port=port;
  desc.size=sizeof(data);
  desc.value=&data;
  return ioctl(freebsdDevFd, _IOW(0, 1, sizeof(struct pioData)), &desc)!=-1;
}

static FxBool
pciPortOutLongFreeBSD(unsigned short port, FxU32 data)
{
  struct pioData desc;
  /* fprintf(stderr, "write long=%x to port=%x desc at %x data at %x\n", 
	  data, port, &desc, &data); */
  if (freebsdDevFd==-1) {
    outl(port, data);
    return FXTRUE;
  }
  desc.port=port;
  desc.size=sizeof(data);
  desc.value=&data;
  return ioctl(freebsdDevFd, _IOW(0, 1, sizeof(struct pioData)), &desc)!=-1;
}

static FxBool
pciMsrGetFreeBSD(MSRInfo *in, MSRInfo *out)
{
  return FXTRUE;
}

static FxBool
pciMsrSetFreeBSD(MSRInfo *in, MSRInfo *out)
{
  return FXTRUE;
}

static FxBool
pciSetPassThroughBaseFreeBSD(FxU32 *baseAddr, FxU32 baseAddrLen)
{
  return FXTRUE;
}
