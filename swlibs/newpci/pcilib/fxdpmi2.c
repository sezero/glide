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

#include <conio.h>

#include <3dfx.h>
#define FX_DLL_DEFINITION
#include <fxdll.h>
#include "fxpci.h"
#include "pcilib.h"

#include <fxdpmi.h>
#include <fxmemmap.h>

/* Callback declarations */
static FxBool pciInitializeDPMI(void);
static FxBool pciShutdownDPMI(void);
static const char* pciIdentifierDPMI(void);

static FxU8  pciPortInByteDPMI(FxU16 port);
static FxU16 pciPortInWordDPMI(FxU16 port);
static FxU32 pciPortInLongDPMI(FxU16 port);
  
static FxBool pciPortOutByteDPMI(FxU16 port, FxU8 data);
static FxBool pciPortOutWordDPMI(FxU16 port, FxU16 data);
static FxBool pciPortOutLongDPMI(FxU16 port, FxU32 data);

static FxBool pciMapLinearDPMI(FxU32 busNumber, FxU32 physAddr,
                             FxU32* linearAddr, FxU32* length);
static FxBool pciUnmapLinearDPMI(FxU32 linearAddr, FxU32 length);

static FxBool pciSetPermissionDPMI(const FxU32 addrBase, const FxU32 addrLen,
                                 const FxBool writePermP);

static FxBool pciMsrGetDPMI(MSRInfo* in, MSRInfo* out);
static FxBool pciMsrSetDPMI(MSRInfo* in, MSRInfo* out);

static FxBool pciOutputStringDPMI(const char* msg);
static FxBool pciSetPassThroughBaseDPMI(FxU32* baseAddr, FxU32 baseAddrLen);

static char pciIdent[] = "@#% fxPCI for DOS";

static const FxPlatformIOProcs __ioProcsDPMI = {
  pciInitializeDPMI,
  pciShutdownDPMI,
  pciIdentifierDPMI,

  pciPortInByteDPMI,
  pciPortInWordDPMI,
  pciPortInLongDPMI,
  
  pciPortOutByteDPMI,
  pciPortOutWordDPMI,
  pciPortOutLongDPMI,

  pciMapLinearDPMI,
  pciUnmapLinearDPMI,
  pciSetPermissionDPMI,

  pciMsrGetDPMI,
  pciMsrSetDPMI,

  pciOutputStringDPMI,
  pciSetPassThroughBaseDPMI
};
const FxPlatformIOProcs* ioProcsDPMI = &__ioProcsDPMI;

FxBool 
pciPlatformInit(void)
{
  gCurPlatformIO = ioProcsDPMI;
  return FXTRUE;
}

/* Basic platform init/shutdown stuff */
static FxBool
pciInitializeDPMI(void)
{
  return FXTRUE;
}

static FxBool
pciShutdownDPMI(void)
{
  return FXTRUE;
}

static const char* 
pciIdentifierDPMI(void)
{
  return pciIdent;
}

/* Device address space management stuff */

static FxBool
pciMapLinearDPMI(FxU32 busNumber, FxU32 physical_addr,
               FxU32 *linear_addr, FxU32 *length)
{
  FxBool onWindows;

  /*  
  **  First, check to see if we're a DOS app under Windows, and if
  **  so, then check to see if there's already an app connected to
  **  the VXD.
  *
  * NB: To be compatible w/ the 'previous' pci libraries this is only
  * done the first time that we're mapping a board otherwise it will
  * fail on the vxd ref count because any previous mappings bump the
  * reference count.  
  */
  {
    static FxBool didVxdCheck = FXFALSE;

    if (!didVxdCheck) {
      DpmiCheckVxd((FxBool *) &onWindows, &pciVxdVer);
      didVxdCheck = FXTRUE;

      if (onWindows) {
        if (BYTE1(pciVxdVer) != FX_MAJOR_VER || BYTE0(pciVxdVer) < FX_MINOR_VER) {
          pciErrorCode = PCI_ERR_WRONGVXD;
          return FXFALSE;
        }
          
        if (VXDREFCOUNT(pciVxdVer) > 0) {
          pciErrorCode = PCI_ERR_VXDINUSE;
          return FXFALSE;
        }
      }
    }
  }

  /* If we got here, it's OK to map the memory */
  *linear_addr = DpmiMapPhysicalToLinear( physical_addr, *length );

  return FXTRUE;
}

static FxBool
pciUnmapLinearDPMI( FxU32 linear_addr, FxU32 length ) 
{
  DpmiUnmapMemory(linear_addr, length);
}

/* Platform port io stuff */
static FxU8
pciPortInByteDPMI(FxU16 port)
{
  return inp(port);
}

static FxU16
pciPortInWordDPMI(FxU16 port)
{
  return inpw(port);
}

static FxU32
pciPortInLongDPMI(FxU16 port)
{
  return inpd(port);
}
  
static FxBool
pciPortOutByteDPMI(FxU16 port, FxU8 data)
{
  return outp(port, data);
}

static FxBool
pciPortOutWordDPMI(FxU16 port, FxU16 data)
{
  return outpw(port, data);
}

static FxBool
pciPortOutLongDPMI(FxU16 port, FxU32 data)
{
  return outpd(port, data);
}

static FxBool 
pciMsrGetDPMI(MSRInfo* in, MSRInfo* out)
{
  return DpmiGetMSR((FxU32)in, (FxU32)out);
}

static FxBool 
pciMsrSetDPMI(MSRInfo* in, MSRInfo* out)
{
  return DpmiSetMSR((FxU32)in, (FxU32)out);
}

/* Platform utilities. */
static FxBool
pciOutputStringDPMI(const char* msg)
{
  return DpmiOutputDebugString(msg);
}

static FxBool
pciSetPermissionDPMI(const FxU32 addrBase, const FxU32 addrLen,
                   const FxBool writePermP)
{
  return DpmiLinearRangeSetPermission(addrBase, addrLen, writePermP);
}

static FxBool
pciSetPassThroughBaseDPMI(FxU32* baseAddr, FxU32 baseAddrLen)
{
  return DpmiSetPassThroughBase(baseAddr, baseAddrLen);
}

