#ifdef __alpha__

#include <stdio.h>
#include <string.h>

#include <3dfx.h>
#include <glidesys.h>

#define FX_DLL_DEFINITION
#include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

#define	NOT_PENTIUM	4 /* see cpudetect.S */

extern FxI32 GR_CDECL
_cpu_detect_asm(void)
{
    return(NOT_PENTIUM);
}
#endif
