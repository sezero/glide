/*
 * CPU detection code
 *
 * $Header$
 * $Log$
 */


#include <signal.h>
#include <setjmp.h>
#include <string.h>

#include "cpuid.h"

typedef unsigned long word32;

/* These are the bit flags that get set on calling cpuid
 * with register eax set to 1
 */
#define _MMX_FEATURE_BIT	0x00800000
#define _SSE_FEATURE_BIT	0x02000000
#define _SSE2_FEATURE_BIT	0x04000000

/* This bit is set when cpuid is called with
 * register set to 80000001h (only applicable to AMD)
 */
#define _3DNOW_FEATURE_BIT	0x80000000
#define _3DNOWPLUS_FEATURE_BIT	0x40000000
#define _MMXPLUS_FEATURE_BIT	0x00400000

/* Testing code:
 * TEST_SSE       = xorps xmm0, xmm0
 * TEST_SSE2      = xorpd xmm0, xmm0
 * TEST_3DNOW     = femms
 * TEST_MMX       = emms
 * TEST_3DNOWPLUS = femms | pswapd mm0, mm0 | femms
 * TEST_MMXPLUS   = emms | pminsw mm0, mm0 | emms
 */
#ifdef __GNUC__
#define TEST_CPUID(f)    __asm __volatile ("cpuid"::"a"(f):"%ebx", "%ecx", "%edx")
#define TEST_SSE()       __asm __volatile (".byte 0x0f, 0x57, 0xc0")
#define TEST_SSE2()      __asm __volatile (".byte 0x66, 0x0f, 0x57, 0xc0")
#define TEST_3DNOW()     __asm __volatile (".byte 0x0f, 0x0e")
#define TEST_MMX()       __asm __volatile (".byte 0x0f, 0x77")
#define TEST_3DNOWPLUS() __asm __volatile (".byte 0x0f, 0x0e, 0x0f, 0x0f, 0xc0, 0xbb, 0x0f, 0x0e")
#define TEST_MMXPLUS()   __asm __volatile (".byte 0x0f, 0x77, 0x0f, 0xea, 0xc0, 0x0f, 0x77")
#else
#define TEST_CPUID(f)    __asm { _asm mov eax, f _asm cpuid }
#define TEST_SSE()       __asm { _asm _emit 0x0f _asm _emit 0x57 _asm _emit 0xc0 }
#define TEST_SSE2()      __asm { _asm _emit 0x66 _asm _emit 0x0f _asm _emit 0x57 _asm _emit 0xc0 }
#define TEST_3DNOW()     __asm { _asm _emit 0x0f _asm _emit 0x0e }
#define TEST_MMX()       __asm { _asm _emit 0x0f _asm _emit 0x77 }
#define TEST_3DNOWPLUS() __asm { _asm _emit 0x0f _asm _emit 0x0e _asm _emit 0x0f _asm _emit 0x0f _asm _emit 0xc0 _asm _emit 0xbb _asm _emit 0x0f _asm _emit 0x0e }
#define TEST_MMXPLUS()   __asm { _asm _emit 0x0f _asm _emit 0x77 _asm _emit 0x0f _asm _emit 0xea _asm _emit 0xc0 _asm _emit 0x0f _asm _emit 0x77 }
#endif



static jmp_buf j;



/* Desc: signal handler
 *
 * In  : signal number
 * Out : -
 *
 * Note: returns by `longjmp'ing
 */
static void handler (int signal)
{
 longjmp(j, 1);
 (void)signal; /* silence compiler warning */
}



/* Desc: check if CPU has specific feature
 *
 * In  : feature request
 * Out : non-zero if requested feature supported
 *
 * Note: this should be in the `has_feature' body. The reason it isn't:
 *       under some systems (notably Linux), the `setjmp' may thrash EBX,
 *       which is used for PositionIndependentCode/GlobalOffsetTable system.
 *       Since EBX is non-volatile register, it should be restored upon return.
 */
static int check_feature (int feature)
{
 if (setjmp(j)) {
    /* we got here only when `longjmp'ed by signal handlers */
    return 0;
 } else {
    /* we have signals and jump buffer set */
    switch (feature) {
           case _CPU_HAS_CPUID:         TEST_CPUID(0);    break;
           case _CPU_FEATURE_SSE:       TEST_SSE();       break;
           case _CPU_FEATURE_SSE2:      TEST_SSE2();      break;
           case _CPU_FEATURE_3DNOW:     TEST_3DNOW();     break;
           case _CPU_FEATURE_MMX:       TEST_MMX();       break;
           case _CPU_FEATURE_3DNOWPLUS: TEST_3DNOWPLUS(); break;
           case _CPU_FEATURE_MMXPLUS:   TEST_MMXPLUS();   break;
           default: return 0;
    }
    return !0;
 }
}



/* Desc: perform (possibly faulting) instructions in a safe manner
 *
 * In  : feature request
 * Out : non-zero if requested feature supported
 *
 * Note: uses standard ANSI signal mechanism
 */
static int has_feature (int feature)
{
 int rv;

 /* register signal handlers */
 void (*old_sigill)(int) = signal(SIGILL, handler);

 rv = check_feature(feature);

 /* restore the signal handlers */
 signal(SIGILL, old_sigill);
 return rv;
}



/* Desc: maps family and model to processor name
 *
 * In  : family, model, vendor, ptr to store name
 * Out : -
 *
 * Note: incomplete
 */
static void map_mname (int family, int model, const char *v_name, char *m_name)
{
 if (!strncmp("AuthenticAMD", v_name, 12)) {
    switch (family) { /* extract family code */
           case 4: /* Am486/AM5x86 */
                strcpy (m_name,"AMD Am486");
                break;
           case 5: /* K6 */
                switch (model) { /* extract model code */
                       case 0:
                       case 1:
                       case 2:
                       case 3:
                            strcpy (m_name,"AMD K5");
                            break;
                       case 4:
                       case 5:
                            break;  /* Not really used */
                       case 6:
                       case 7:
                            strcpy (m_name,"AMD K6");
                            break;
                       case 8:
                            strcpy (m_name,"AMD K6-2");
                            break;
                       case 9:
                       case 10:
                       case 11:
                       case 12:
                       case 13:
                       case 14:
                       case 15:
                            strcpy (m_name,"AMD K6-3");
                            break;
                       default:
                            strcpy (m_name, "Unknown");
                }
                break;
           case 6: /* Athlon */
                switch (model) { /* extract model code */
                       case 3:
                            strcpy (m_name,"AMD Duron");
                            break;
                       default:
                            strcpy (m_name,"AMD Athlon");
                }
                break;
    }
 } else if (!strncmp("GenuineIntel", v_name, 12)) {
    switch (family) { /* extract family code */
           case 4:
                switch (model) { /* extract model code */
                       case 0:
                       case 1:
                            strcpy (m_name,"INTEL 486DX");
                            break;
                       case 2:
                            strcpy (m_name,"INTEL 486SX");
                            break;
                       case 3:
                            strcpy (m_name,"INTEL 486DX2");
                            break;
                       case 4:
                            strcpy (m_name,"INTEL 486SL");
                            break;
                       case 5:
                            strcpy (m_name,"INTEL 486SX2");
                            break;
                       case 7:
                            strcpy (m_name,"INTEL 486DX2E");
                            break;
                       case 8:
                            strcpy (m_name,"INTEL 486DX4");
                            break;
                       default:
                            strcpy (m_name, "Unknown");
                }
                break;
           case 5:
                switch (model) { /* extract model code */
                       case 1:
                       case 2:
                       case 3:
                            strcpy (m_name,"INTEL Pentium");
                            break;
                       case 4:
                            strcpy (m_name,"INTEL Pentium-MMX");
                            break;
                       default:
                            strcpy (m_name, "Unknown");
                }
                break;
           case 6:
                switch (model) { /* extract model code */
                       case 1:
                            strcpy (m_name,"INTEL Pentium-Pro");
                            break;
                       case 3:
                            strcpy (m_name,"INTEL Pentium-II");
                            break;
                       case 5:
                            strcpy (m_name,"INTEL Pentium-II");
                            break;  /* actual differentiation depends on cache settings */
                       case 6:
                            strcpy (m_name,"INTEL Celeron");
                            break;
                       case 7:
                            strcpy (m_name,"INTEL Pentium-III");
                            break;  /* actual differentiation depends on cache settings */
                       default:
                            strcpy (m_name, "Unknown");
                }
                break;
    }
 } else if (!strncmp("CyrixInstead", v_name,12)) {
    strcpy (m_name, "Unknown");
 } else if (!strncmp("CentaurHauls", v_name,12)) {
    strcpy (m_name, "Unknown");
 } else {
    strcpy (m_name, "Unknown");
 }
}



/* Desc: get CPU info
 *
 * In  : pointer to _p_info
 * Out : features
 *
 * Note: -
 */
int _cpuid (_p_info *pinfo)
{
 word32 dwStandard = 0;
 word32 dwFeature = 0;
 word32 dwMax = 0;
 word32 dwExt = 0;
 int feature = 0, os_support = 0;
 union {
        char cBuf[12+1];
        struct {
                word32 dw0;
                word32 dw1;
                word32 dw2;
        } dummy;
 } Ident;

 if (!has_feature(_CPU_HAS_CPUID)) {
    return 0;
 }

#ifdef __GNUC__
 __asm("\n\
	pushl	%%ebx			\n\
	/* get the vendor string */	\n\
	xorl	%%eax, %%eax		\n\
	cpuid				\n\
	movl	%%eax, %0		\n\
	movl	%%ebx, %4		\n\
	movl	%%edx, %5		\n\
	movl	%%ecx, %6		\n\
	/* get the Standard bits */	\n\
	movl	$1, %%eax		\n\
	cpuid				\n\
	movl	%%eax, %2		\n\
	movl	%%edx, %3		\n\
	/* get AMD-specials */		\n\
	movl	$0x80000000, %%eax	\n\
	cpuid				\n\
	cmpl	$0x80000000, %%eax	\n\
	jc	0f			\n\
	movl	$0x80000001, %%eax	\n\
	cpuid				\n\
	movl	%%edx, %1		\n\
	popl	%%ebx			\n\
 0:					\n\
 ":"=g"(dwMax), "=g"(dwExt),
   "=g"(dwStandard), "=g"(dwFeature),
   "=g"(Ident.dummy.dw0), "=g"(Ident.dummy.dw1), "=g"(Ident.dummy.dw2)
 ::"%eax", "%ebx", "%ecx", "%edx");
#else
    _asm
    {
        push ebx
        push ecx
        push edx

        /* get the vendor string */
        xor eax,eax
        cpuid
        mov dwMax,eax
        mov Ident.dummy.dw0,ebx
        mov Ident.dummy.dw1,edx
        mov Ident.dummy.dw2,ecx

        /* get the Standard bits */
        mov eax,1
        cpuid
        mov dwStandard,eax
        mov dwFeature,edx

        /* get AMD-specials */
        mov eax,80000000h
        cpuid
        cmp eax,80000000h
        jc notamd
        mov eax,80000001h
        cpuid
        mov dwExt,edx

notamd:
        pop ecx
        pop ebx
        pop edx
    }
#endif

 if (dwFeature & _MMX_FEATURE_BIT) {
    feature |= _CPU_FEATURE_MMX;
    if (has_feature(_CPU_FEATURE_MMX))
       os_support |= _CPU_FEATURE_MMX;
 }
 if (dwExt & _3DNOW_FEATURE_BIT) {
    feature |= _CPU_FEATURE_3DNOW;
    if (has_feature(_CPU_FEATURE_3DNOW))
       os_support |= _CPU_FEATURE_3DNOW;
 }
 if (dwExt & _3DNOWPLUS_FEATURE_BIT) {
    feature |= _CPU_FEATURE_3DNOWPLUS;
    if (has_feature(_CPU_FEATURE_3DNOWPLUS))
       os_support |= _CPU_FEATURE_3DNOWPLUS;
 }
 if (dwExt & _MMXPLUS_FEATURE_BIT) {
    feature |= _CPU_FEATURE_MMXPLUS;
    if (has_feature(_CPU_FEATURE_MMXPLUS))
       os_support |= _CPU_FEATURE_MMXPLUS;
 }
 if (dwFeature & _SSE_FEATURE_BIT) {
    feature |= _CPU_FEATURE_SSE;
    if (has_feature(_CPU_FEATURE_SSE))
       os_support |= _CPU_FEATURE_SSE;
 }
 if (dwFeature & _SSE2_FEATURE_BIT) {
    feature |= _CPU_FEATURE_SSE2;
    if (has_feature(_CPU_FEATURE_SSE2))
       os_support |= _CPU_FEATURE_SSE2;
 }

 if (pinfo) {
    memset(pinfo, 0, sizeof(_p_info));
    pinfo->os_support = os_support;
    pinfo->feature = feature;
    pinfo->family = (dwStandard >> 8)&0xF; /* retrieving family */
    pinfo->model = (dwStandard >> 4)&0xF;  /* retrieving model */
    pinfo->stepping = (dwStandard) & 0xF;  /* retrieving stepping */
    Ident.cBuf[12] = 0;
    strcpy(pinfo->v_name, Ident.cBuf);
    map_mname(pinfo->family, pinfo->model, pinfo->v_name, pinfo->model_name);
    pinfo->checks = _CPU_FEATURE_MMX |
                    _CPU_FEATURE_SSE |
                    _CPU_FEATURE_SSE2 |
                    _CPU_FEATURE_3DNOW;
 }

 return feature;
}



#if CPUTEST
#include <stdio.h>
/* Desc:
 *
 * In  :
 * Out :
 *
 * Note:
 */
int main (void)
{
 _p_info p;
 _cpuid(&p);
 printf("vendor  : %s\n", p.v_name);
 printf("model   : %s\n", p.model_name);
 printf("family  : %d\n", p.family);
 printf("model   : %d\n", p.model);
 printf("stepping: %X\n", p.stepping);
 printf("feature : %08x\n", p.feature);
 printf("checks  : %08x\n", p.checks);
 printf("--------\n");
 printf("cpuid   : %d\n", has_feature(_CPU_HAS_CPUID));
 printf("MMX     : %d\n", has_feature(_CPU_FEATURE_MMX));
 printf("SSE     : %d\n", has_feature(_CPU_FEATURE_SSE));
 printf("SSE2    : %d\n", has_feature(_CPU_FEATURE_SSE2));
 printf("3DNow!  : %d\n", has_feature(_CPU_FEATURE_3DNOW));
 printf("3DNow!+ : %d\n", has_feature(_CPU_FEATURE_3DNOWPLUS));
 printf("MMX+    : %d\n", has_feature(_CPU_FEATURE_MMXPLUS));
 return 0;
}
#endif
