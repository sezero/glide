/*  Generic MTRR (Memory Type Range Register) driver.

    Copyright (C) 1997-1998  Richard Gooch

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  rgooch@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.

    Source: "Pentium Pro Family Developer's Manual, Volume 3:
    Operating System Writer's Guide" (Intel document number 242692),
    section 11.11.7

    ChangeLog

    Prehistory Martin Tischhäuser <martin@ikcbarka.fzk.de>
	       Initial register-setting code (from proform-1.0).
    19971216   Richard Gooch <rgooch@atnf.csiro.au>
               Original version for /proc/mtrr interface, SMP-safe.
  v1.0
    19971217   Richard Gooch <rgooch@atnf.csiro.au>
               Bug fix for ioctls()'s.
	       Added sample code in Documentation/mtrr.txt
  v1.1
    19971218   Richard Gooch <rgooch@atnf.csiro.au>
               Disallow overlapping regions.
    19971219   Jens Maurer <jmaurer@menuett.rhein-main.de>
               Register-setting fixups.
  v1.2
    19971222   Richard Gooch <rgooch@atnf.csiro.au>
               Fixups for kernel 2.1.75.
  v1.3
    19971229   David Wragg <dpw@doc.ic.ac.uk>
               Register-setting fixups and conformity with Intel conventions.
    19971229   Richard Gooch <rgooch@atnf.csiro.au>
               Cosmetic changes and wrote this ChangeLog ;-)
    19980106   Richard Gooch <rgooch@atnf.csiro.au>
               Fixups for kernel 2.1.78.
  v1.4
    19980119   David Wragg <dpw@doc.ic.ac.uk>
               Included passive-release enable code (elsewhere in PCI setup).
  v1.5
    19980131   Richard Gooch <rgooch@atnf.csiro.au>
               Replaced global kernel lock with private spinlock.
  v1.6
    19980201   Richard Gooch <rgooch@atnf.csiro.au>
               Added wait for other CPUs to complete changes.
  v1.7
    19980202   Richard Gooch <rgooch@atnf.csiro.au>
               Bug fix in definition of <set_mtrr> for UP.
  v1.8
    19980319   Richard Gooch <rgooch@atnf.csiro.au>
               Fixups for kernel 2.1.90.
    19980323   Richard Gooch <rgooch@atnf.csiro.au>
               Move SMP BIOS fixup before secondary CPUs call <calibrate_delay>
  v1.9
    19980325   Richard Gooch <rgooch@atnf.csiro.au>
               Fixed test for overlapping regions: confused by adjacent regions
    19980326   Richard Gooch <rgooch@atnf.csiro.au>
               Added wbinvd in <set_mtrr_prepare>.
    19980401   Richard Gooch <rgooch@atnf.csiro.au>
               Bug fix for non-SMP compilation.
    19980418   David Wragg <dpw@doc.ic.ac.uk>
               Fixed-MTRR synchronisation for SMP and use atomic operations
	       instead of spinlocks.
    19980418   Richard Gooch <rgooch@atnf.csiro.au>
	       Differentiate different MTRR register classes for BIOS fixup.
  v1.10
    19980419   David Wragg <dpw@doc.ic.ac.uk>
	       Bug fix in variable MTRR synchronisation.
  v1.11
    19980419   Richard Gooch <rgooch@atnf.csiro.au>
	       Fixups for kernel 2.1.97.
  v1.12
    19980421   Richard Gooch <rgooch@atnf.csiro.au>
	       Safer synchronisation across CPUs when changing MTRRs.
  v1.13
    19980423   Richard Gooch <rgooch@atnf.csiro.au>
	       Bugfix for SMP systems without MTRR support.
  v1.14
    19980427   Richard Gooch <rgooch@atnf.csiro.au>
	       Trap calls to <mtrr_add> and <mtrr_del> on non-MTRR machines.
  v1.15
    19980427   Richard Gooch <rgooch@atnf.csiro.au>
	       Use atomic bitops for setting SMP change mask.
  v1.16
    19980428   Richard Gooch <rgooch@atnf.csiro.au>
	       Removed spurious diagnostic message.
  v1.17
    19980429   Richard Gooch <rgooch@atnf.csiro.au>
	       Moved register-setting macros into this file.
	       Moved setup code from init/main.c to i386-specific areas.
  v1.18
    19980502   Richard Gooch <rgooch@atnf.csiro.au>
	       Moved MTRR detection outside conditionals in <mtrr_init>.
  v1.19
    19980502   Richard Gooch <rgooch@atnf.csiro.au>
	       Documentation improvement: mention Pentium II and AGP.
  v1.20
    19980521   Richard Gooch <rgooch@atnf.csiro.au>
	       Only manipulate interrupt enable flag on local CPU.
	       Allow enclosed uncachable regions.
  v1.21

  v1.21--      Emil Briggs <briggs@tick.physics.ncsu.edu>
               Backported to 2.0.x kernels, /proc and ioctl
	       interfaces removed and the result incorporated
	       into the 3dfx device driver. Not SMP safe. If
	       you have an SMP use the 2.2 series kernels.

  v1.22
    19991222   Joseph Kain <joseph@3dfx.com>
               Do kernel version checks here instead of in 
               Makefile.  The Makefile now only needs the
	       kernel version for install.
*/


/* Include this first as it defines things that affect the kernel headers */
#include "kinfo.h"

/* MTRR support is only available in kerenls versioned 2.1.0 or higher.
 * For earlier kernels this code compiles to nothing.  */
#include <linux/version.h>

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 1, 0)


#ifndef _LINUX_MTRR_H
#define _LINUX_MTRR_H

struct mtrr_sentry
{
    unsigned long base;    /*  Base address     */
    unsigned long size;    /*  Size of region   */
    unsigned int type;     /*  Type of region   */
};

struct mtrr_gentry
{
    unsigned int regnum;   /*  Register number  */
    unsigned long base;    /*  Base address     */
    unsigned long size;    /*  Size of region   */
    unsigned int type;     /*  Type of region   */
};


/*  These are the region types  */
#define MTRR_TYPE_UNCACHABLE 0
#define MTRR_TYPE_WRCOMB     1
/*#define MTRR_TYPE_         2*/
/*#define MTRR_TYPE_         3*/
#define MTRR_TYPE_WRTHROUGH  4
#define MTRR_TYPE_WRPROT     5
#define MTRR_TYPE_WRBACK     6
#define MTRR_NUM_TYPES       7

static char *mtrr_strings[MTRR_NUM_TYPES] =
{
    "uncachable",               /* 0 */
    "write-combining",          /* 1 */
    "?",                        /* 2 */
    "?",                        /* 3 */
    "write-through",            /* 4 */
    "write-protect",            /* 5 */
    "write-back",               /* 6 */
};

#ifdef __KERNEL__

/*  The following functions are for use by other drivers  */
extern int mtrr_add (unsigned long base, unsigned long size,
		     unsigned int type, char increment);
extern int mtrr_del (int reg, unsigned long base, unsigned long size);

#endif

#endif  /*  _LINUX_MTRR_H  */

#ifdef MODULE
#include <linux/autoconf.h>
#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
#define MODVERSIONS
#endif
#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif
#endif
#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/timer.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <linux/string.h>
#include <linux/malloc.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/ctype.h>
#include <linux/proc_fs.h>
#define MTRR_NEED_STRINGS

#include <asm/io.h>
#include <asm/processor.h>
#include <asm/system.h>
#include <asm/pgtable.h>
#include <asm/segment.h>
#include <asm/bitops.h>
#include <asm/smp_lock.h>
#include <asm/atomic.h>
#include <linux/smp.h>

#undef __SMP__

#define MTRR_VERSION            "1.21 (19980521)"

#define TRUE  1
#define FALSE 0

#define X86_FEATURE_MTRR	0x1000		/* memory type registers */

#define MTRRcap_MSR     0x0fe
#define MTRRdefType_MSR 0x2ff

#define MTRRphysBase_MSR(reg) (0x200 + 2 * (reg))
#define MTRRphysMask_MSR(reg) (0x200 + 2 * (reg) + 1)

#define NUM_FIXED_RANGES 88
#define MTRRfix64K_00000_MSR 0x250
#define MTRRfix16K_80000_MSR 0x258
#define MTRRfix16K_A0000_MSR 0x259
#define MTRRfix4K_C0000_MSR 0x268
#define MTRRfix4K_C8000_MSR 0x269
#define MTRRfix4K_D0000_MSR 0x26a
#define MTRRfix4K_D8000_MSR 0x26b
#define MTRRfix4K_E0000_MSR 0x26c
#define MTRRfix4K_E8000_MSR 0x26d
#define MTRRfix4K_F0000_MSR 0x26e
#define MTRRfix4K_F8000_MSR 0x26f


#ifdef __SMP__
#  define MTRR_CHANGE_MASK_FIXED     0x01
#  define MTRR_CHANGE_MASK_VARIABLE  0x02
#  define MTRR_CHANGE_MASK_DEFTYPE   0x04
#endif

/* In the processor's MTRR interface, the MTRR type is always held in
   an 8 bit field: */
typedef u8 mtrr_type;

#define LINE_SIZE      80
#define JIFFIE_TIMEOUT 100

#define set_mtrr(reg,base,size,type) set_mtrr_up (reg, base, size, type,TRUE)


static unsigned int *usage_table = NULL;


struct set_mtrr_context
{
    unsigned long flags;
    unsigned long deftype_lo;
    unsigned long deftype_hi;
    unsigned long cr4val;
};

/*
 * Access to machine-specific registers (available on 586 and better only)
 * Note: the rd* operations modify the parameters directly (without using
 * pointer indirection), this allows gcc to optimize better
 */
#define rdmsr(msr,val1,val2) \
       __asm__ __volatile__("rdmsr" \
			    : "=a" (val1), "=d" (val2) \
			    : "c" (msr))

#define wrmsr(msr,val1,val2) \
     __asm__ __volatile__("wrmsr" \
			  : /* no outputs */ \
			  : "c" (msr), "a" (val1), "d" (val2))

#define rdtsc(low,high) \
     __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high))

#define rdpmc(counter,low,high) \
     __asm__ __volatile__("rdpmc" \
			  : "=a" (low), "=d" (high) \
			  : "c" (counter))


/* Put the processor into a state where MTRRs can be safely set. */
static void set_mtrr_prepare(struct set_mtrr_context *ctxt)
{
    unsigned long tmp;

    /* disable interrupts locally */
    save_flags (ctxt->flags); cli ();

    /* save value of CR4 and clear Page Global Enable (bit 7) */
    asm volatile ("movl  %%cr4, %0\n\t"
		  "movl  %0, %1\n\t"
		  "andb  $0x7f, %b1\n\t"
		  "movl  %1, %%cr4\n\t"
                  : "=r" (ctxt->cr4val), "=q" (tmp) : : "memory");

    /* disable and flush caches. Note that wbinvd flushes the TLBs as
       a side-effect. */
    asm volatile ("movl  %%cr0, %0\n\t"
		  "orl   $0x40000000, %0\n\t"
		  "wbinvd\n\t"
		  "movl  %0, %%cr0\n\t"
		  "wbinvd\n\t"
		  : "=r" (tmp) : : "memory");

    /* disable MTRRs, and set the default type to uncached. */
    rdmsr(MTRRdefType_MSR, ctxt->deftype_lo, ctxt->deftype_hi);
    wrmsr(MTRRdefType_MSR, ctxt->deftype_lo & 0xf300UL, ctxt->deftype_hi);
}   /*  End Function set_mtrr_prepare  */


/* Restore the processor after a set_mtrr_prepare */
static void set_mtrr_done(struct set_mtrr_context *ctxt)
{
    unsigned long tmp;

    /* flush caches and TLBs */
    asm volatile ("wbinvd" : : : "memory" );

    /* restore MTRRdefType */
    wrmsr(MTRRdefType_MSR, ctxt->deftype_lo, ctxt->deftype_hi);

    /* enable caches */
    asm volatile ("movl  %%cr0, %0\n\t"
		  "andl  $0xbfffffff, %0\n\t"
		  "movl  %0, %%cr0\n\t"
		  : "=r" (tmp) : : "memory");

    /* restore value of CR4 */
    asm volatile ("movl  %0, %%cr4"
                  : : "r" (ctxt->cr4val) : "memory");

    /* re-enable interrupts locally (if enabled previously) */
    restore_flags (ctxt->flags);
}   /*  End Function set_mtrr_done  */


/* this function returns the number of variable MTRRs */
static unsigned int get_num_var_ranges (void)
{
    unsigned long config, dummy;

    rdmsr(MTRRcap_MSR, config, dummy);
    return (config & 0xff);
}   /*  End Function get_num_var_ranges  */


/* non-zero if we have the write-combining memory type. */
static int have_wrcomb (void)
{
    unsigned long config, dummy;

    rdmsr(MTRRcap_MSR, config, dummy);
    return (config & (1<<10));
}


static void get_mtrr (unsigned int reg, unsigned long *base,
		      unsigned long *size, mtrr_type *type)
{
    unsigned long dummy, mask_lo, base_lo;

    rdmsr(MTRRphysMask_MSR(reg), mask_lo, dummy);
    if ((mask_lo & 0x800) == 0) {
	/* Invalid (i.e. free) range. */
	*base = 0;
	*size = 0;
	*type = 0;
	return;
    }

    rdmsr(MTRRphysBase_MSR(reg), base_lo, dummy);

    /* We ignore the extra address bits (32-35). If someone wants to
       run x86 Linux on a machine with >4GB memory, this will be the
       least of their problems. */

    /* Clean up mask_lo so it gives the real address mask. */
    mask_lo = (mask_lo & 0xfffff000UL);
    
    /* This works correctly if size is a power of two, i.e. a
       contiguous range. */
    *size = ~(mask_lo - 1);

    *base = (base_lo & 0xfffff000UL);
    *type = (base_lo & 0xff);
}   /*  End Function get_mtrr  */


static void set_mtrr_up (unsigned int reg, unsigned long base,
			 unsigned long size, mtrr_type type, int do_safe)
/*  [SUMMARY] Set variable MTRR register on the local CPU.
    <reg> The register to set.
    <base> The base address of the region.
    <size> The size of the region. If this is 0 the region is disabled.
    <type> The type of the region.
    <do_safe> If TRUE, do the change safely. If FALSE, safety measures should
    be done externally.
*/
{
    struct set_mtrr_context ctxt;

    if (do_safe) set_mtrr_prepare (&ctxt);
    if (size == 0)
    {
	/* The invalid bit is kept in the mask, so we simply clear the
	   relevant mask register to disable a range. */
	wrmsr (MTRRphysMask_MSR (reg), 0, 0);
    }
    else
    {
	wrmsr (MTRRphysBase_MSR (reg), base | type, 0);
	wrmsr (MTRRphysMask_MSR (reg), ~(size - 1) | 0x800, 0);
    }
    if (do_safe) set_mtrr_done (&ctxt);
}   /*  End Function set_mtrr_up  */

     
#ifdef __SMP__

struct mtrr_var_range
{
    unsigned long base_lo;
    unsigned long base_hi;
    unsigned long mask_lo;
    unsigned long mask_hi;
};


/* Get the MSR pair relating to a var range. */
static void get_mtrr_var_range (unsigned int index,
					   struct mtrr_var_range *vr)
{
    rdmsr (MTRRphysBase_MSR (index), vr->base_lo, vr->base_hi);
    rdmsr (MTRRphysMask_MSR (index), vr->mask_lo, vr->mask_hi);
}   /*  End Function get_mtrr_var_range  */


/* Set the MSR pair relating to a var range. Returns TRUE if
   changes are made. */
static int set_mtrr_var_range_testing (unsigned int index,
						  struct mtrr_var_range *vr)
{
    unsigned int lo, hi;
    int changed = FALSE;
    
    rdmsr(MTRRphysBase_MSR(index), lo, hi); 

    if ((vr->base_lo & 0xfffff0ffUL) != (lo & 0xfffff0ffUL)
	|| (vr->base_hi & 0xfUL) != (hi & 0xfUL)) {
	wrmsr(MTRRphysBase_MSR(index), vr->base_lo, vr->base_hi); 
	changed = TRUE;
    }

    rdmsr(MTRRphysMask_MSR(index), lo, hi); 

    if ((vr->mask_lo & 0xfffff800UL) != (lo & 0xfffff800UL)
	|| (vr->mask_hi & 0xfUL) != (hi & 0xfUL)) {
	wrmsr(MTRRphysMask_MSR(index), vr->mask_lo, vr->mask_hi); 
	changed = TRUE;
    }
    
    return changed;
}


static void get_fixed_ranges(mtrr_type *frs)
{
    unsigned long *p = (unsigned long *)frs;
    int i;

    rdmsr(MTRRfix64K_00000_MSR, p[0], p[1]);

    for (i = 0; i < 2; i++)
	rdmsr(MTRRfix16K_80000_MSR + i, p[2 + i*2], p[3 + i*2]);
 
    for (i = 0; i < 8; i++)
	rdmsr(MTRRfix4K_C0000_MSR + i, p[6 + i*2], p[7 + i*2]);
}


static int set_fixed_ranges_testing(mtrr_type *frs)
{
    unsigned long *p = (unsigned long *)frs;
    int changed = FALSE;
    int i;
    unsigned long lo, hi;

    rdmsr(MTRRfix64K_00000_MSR, lo, hi);
    if (p[0] != lo || p[1] != hi) {
	wrmsr(MTRRfix64K_00000_MSR, p[0], p[1]);
	changed = TRUE;
    }

    for (i = 0; i < 2; i++) {
	rdmsr(MTRRfix16K_80000_MSR + i, lo, hi);
	if (p[2 + i*2] != lo || p[3 + i*2] != hi) {
	    wrmsr(MTRRfix16K_80000_MSR + i, p[2 + i*2], p[3 + i*2]);
	    changed = TRUE;
	}
    }

    for (i = 0; i < 8; i++) {
	rdmsr(MTRRfix4K_C0000_MSR + i, lo, hi);
	if (p[6 + i*2] != lo || p[7 + i*2] != hi) {
	    wrmsr(MTRRfix4K_C0000_MSR + i, p[6 + i*2], p[7 + i*2]);
	    changed = TRUE;
	}
    }

    return changed;
}


struct mtrr_state
{
    unsigned int num_var_ranges;
    struct mtrr_var_range *var_ranges;
    mtrr_type fixed_ranges[NUM_FIXED_RANGES];
    unsigned char enabled;
    mtrr_type def_type;
};


/* Grab all of the MTRR state for this CPU into *state. */
static void get_mtrr_state(struct mtrr_state *state)
{
    unsigned int nvrs, i;
    struct mtrr_var_range *vrs;
    unsigned long lo, dummy;

    nvrs = state->num_var_ranges = get_num_var_ranges();
    vrs = state->var_ranges 
              = kmalloc(nvrs * sizeof(struct mtrr_var_range), GFP_KERNEL);
    if (vrs == NULL)
	nvrs = state->num_var_ranges = 0;

    for (i = 0; i < nvrs; i++)
	get_mtrr_var_range(i, &vrs[i]);
    
    get_fixed_ranges(state->fixed_ranges);

    rdmsr(MTRRdefType_MSR, lo, dummy);
    state->def_type = (lo & 0xff);
    state->enabled = (lo & 0xc00) >> 10;
}   /*  End Function get_mtrr_state  */


/* Free resources associated with a struct mtrr_state */
static void finalize_mtrr_state(struct mtrr_state *state)
{
    if (state->var_ranges) kfree (state->var_ranges);
}   /*  End Function finalize_mtrr_state  */


static unsigned long set_mtrr_state (struct mtrr_state *state,
						struct set_mtrr_context *ctxt)
/*  [SUMMARY] Set the MTRR state for this CPU.
    <state> The MTRR state information to read.
    <ctxt> Some relevant CPU context.
    [NOTE] The CPU must already be in a safe state for MTRR changes.
    [RETURNS] 0 if no changes made, else a mask indication what was changed.
*/
{
    unsigned int i;
    unsigned long change_mask = 0;

    for (i = 0; i < state->num_var_ranges; i++)
	if (set_mtrr_var_range_testing(i, &state->var_ranges[i]))
	    change_mask |= MTRR_CHANGE_MASK_VARIABLE;

    if (set_fixed_ranges_testing(state->fixed_ranges))
	change_mask |= MTRR_CHANGE_MASK_FIXED;
    
    /* set_mtrr_restore restores the old value of MTRRdefType,
       so to set it we fiddle with the saved value. */
    if ((ctxt->deftype_lo & 0xff) != state->def_type
	|| ((ctxt->deftype_lo & 0xc00) >> 10) != state->enabled)
    {
	ctxt->deftype_lo |= (state->def_type | state->enabled << 10);
	change_mask |= MTRR_CHANGE_MASK_DEFTYPE;
    }

    return change_mask;
}   /*  End Function set_mtrr_state  */

 
static atomic_t undone_count;
static void (*handler_func) (struct set_mtrr_context *ctxt, void *info);
static void *handler_info;
static volatile int wait_barrier_execute = FALSE;
static volatile int wait_barrier_cache_enable = FALSE;

static void sync_handler (void)
/*  [SUMMARY] Synchronisation handler. Executed by "other" CPUs.
    [RETURNS] Nothing.
*/
{
    struct set_mtrr_context ctxt;

    set_mtrr_prepare (&ctxt);
    /*  Notify master CPU that I'm at the barrier and then wait  */
    atomic_dec (&undone_count);
    while (wait_barrier_execute) barrier ();
    /*  The master has cleared me to execute  */
    (*handler_func) (&ctxt, handler_info);
    /*  Notify master CPU that I've executed the function  */
    atomic_dec (&undone_count);
    /*  Wait for master to clear me to enable cache and return  */
    while (wait_barrier_cache_enable) barrier ();
    set_mtrr_done (&ctxt);
}   /*  End Function sync_handler  */

static void do_all_cpus (void (*handler) (struct set_mtrr_context *ctxt,
					  void *info),
			 void *info, int local)
/*  [SUMMARY] Execute a function on all CPUs, with caches flushed and disabled.
    [PURPOSE] This function will synchronise all CPUs, flush and disable caches
    on all CPUs, then call a specified function. When the specified function
    finishes on all CPUs, caches are enabled on all CPUs.
    <handler> The function to execute.
    <info> An arbitrary information pointer which is passed to <<handler>>.
    <local> If TRUE <<handler>> is executed locally.
    [RETURNS] Nothing.
*/
{
    unsigned long timeout;
    struct set_mtrr_context ctxt;

    mtrr_hook = sync_handler;
    handler_func = handler;
    handler_info = info;
    wait_barrier_execute = TRUE;
    wait_barrier_cache_enable = TRUE;
    /*  Send a message to all other CPUs and wait for them to enter the
	barrier  */
    atomic_set (&undone_count, smp_num_cpus - 1);
    smp_message_pass (MSG_ALL_BUT_SELF, MSG_MTRR_CHANGE, 0, 0);
    /*  Wait for it to be done  */
    timeout = jiffies + JIFFIE_TIMEOUT;
    while ( (atomic_read (&undone_count) > 0) && (jiffies < timeout) )
	barrier ();
    if (atomic_read (&undone_count) > 0)
    {
	panic ("mtrr: timed out waiting for other CPUs\n");
    }
    mtrr_hook = NULL;
    /*  All other CPUs should be waiting for the barrier, with their caches
	already flushed and disabled. Prepare for function completion
	notification  */
    atomic_set (&undone_count, smp_num_cpus - 1);
    /*  Flush and disable the local CPU's cache	and release the barier, which
	should cause the other CPUs to execute the function. Also execute it
	locally if required  */
    set_mtrr_prepare (&ctxt);
    wait_barrier_execute = FALSE;
    if (local) (*handler) (&ctxt, info);
    /*  Now wait for other CPUs to complete the function  */
    while (atomic_read (&undone_count) > 0) barrier ();
    /*  Now all CPUs should have finished the function. Release the barrier to
	allow them to re-enable their caches and return from their interrupt,
	then enable the local cache and return  */
    wait_barrier_cache_enable = FALSE;
    set_mtrr_done (&ctxt);
    handler_func = NULL;
    handler_info = NULL;
}   /*  End Function do_all_cpus  */


struct set_mtrr_data
{
    unsigned long smp_base;
    unsigned long smp_size;
    unsigned int smp_reg;
    mtrr_type smp_type;
};

static void set_mtrr_handler (struct set_mtrr_context *ctxt, void *info)
{
    struct set_mtrr_data *data = info;

    set_mtrr_up (data->smp_reg, data->smp_base, data->smp_size, data->smp_type,
		 FALSE);
}   /*  End Function set_mtrr_handler  */



/* A warning that is common to the module and non-module cases. */
/* Some BIOS's are fucked and don't set all MTRRs the same! */
#ifdef MODULE
static void mtrr_state_warn (unsigned long mask)
#else
static void mtrr_state_warn (unsigned long mask)
#endif
{
    if (!mask) return;
    if (mask & MTRR_CHANGE_MASK_FIXED)
	printk ("mtrr: your CPUs had inconsistent fixed MTRR settings\n");
    if (mask & MTRR_CHANGE_MASK_VARIABLE)
	printk ("mtrr: your CPUs had inconsistent variable MTRR settings\n");
    if (mask & MTRR_CHANGE_MASK_DEFTYPE)
	printk ("mtrr: your CPUs had inconsistent MTRRdefType settings\n");
    printk ("mtrr: probably your BIOS does not setup all CPUs\n");
}   /*  End Function mtrr_state_warn  */

#ifdef MODULE
/* As a module, copy the MTRR state using an IPI handler. */

static volatile unsigned long smp_changes_mask = 0;

static void copy_mtrr_state_handler (struct set_mtrr_context *ctxt, void *info)
{
    unsigned long mask, count;
    struct mtrr_state *smp_mtrr_state = info;

    mask = set_mtrr_state (smp_mtrr_state, ctxt);
    /*  Use the atomic bitops to update the global mask  */
    for (count = 0; count < sizeof mask * 8; ++count)
    {
	if (mask & 0x01) set_bit (count, &smp_changes_mask);
	mask >>= 1;
    }
}   /*  End Function copy_mtrr_state_handler  */

/* Copies the entire MTRR state of this CPU to all the others. */
static void copy_mtrr_state (void)
{
    struct mtrr_state ms;
 
    get_mtrr_state (&ms);
    do_all_cpus (copy_mtrr_state_handler, &ms, FALSE);
    finalize_mtrr_state (&ms);
    mtrr_state_warn (smp_changes_mask);
}   /*  End Function copy_mtrr_state  */

#endif /* MODULE */
#endif  /*  __SMP__  */

static char *attrib_to_str (int x)
{
    return (x <= 6) ? mtrr_strings[x] : "?";
}   /*  End Function attrib_to_str  */

static void init_table (void)
{
    int i, max;

    max = get_num_var_ranges ();
    if ( ( usage_table = kmalloc (max * sizeof *usage_table, GFP_KERNEL) )
	 == NULL )
    {
	printk ("mtrr: could not allocate\n");
	return;
    }
    for (i = 0; i < max; i++) usage_table[i] = 1;

}   /*  End Function init_table  */

int mtrr_add (unsigned long base, unsigned long size, unsigned int type,
	      char increment)
/*  [SUMMARY] Add an MTRR entry.
    <base> The starting (base) address of the region.
    <size> The size (in bytes) of the region.
    <type> The type of the new region.
    <increment> If true and the region already exists, the usage count will be
    incremented.
    [RETURNS] The MTRR register on success, else a negative number indicating
    the error code.
    [NOTE] This routine uses a spinlock.
*/
{
    int i, max;
    mtrr_type ltype;
    unsigned long lbase, lsize, last;

    if (!usage_table) init_table();
    if ( (base & 0xfff) || (size & 0xfff) )
    {
	printk ("mtrr: size and base must be multiples of 4kB\n");
	printk ("mtrr: size: %lx  base: %lx\n", size, base);
	return -EINVAL;
    }
    if (base + size < 0x100000)
    {
	printk ("mtrr: cannot set region below 1 MByte (0x%lx,0x%lx)\n",
		base, size);
	return -EINVAL;
    }
    /*  Check upper bits of base and last are equal and lower bits are 0 for
	base and 1 for last  */
    last = base + size - 1;
    for (lbase = base; !(lbase & 1) && (last & 1);
	 lbase = lbase >> 1, last = last >> 1);
    if (lbase != last)
    {
	printk ("mtrr: base(0x%lx) is not aligned on a size(0x%lx) boundary\n",
		base, size);
	return -EINVAL;
    }
    if (type >= MTRR_NUM_TYPES)
    {
	printk ("mtrr: type: %u illegal\n", type);
	return -EINVAL;
    }
    /*  If the type is WC, check that this processor supports it  */
    if ( (type == MTRR_TYPE_WRCOMB) && !have_wrcomb () )
    {
        printk ("mtrr: your processor doesn't support write-combining\n");
        return -ENOSYS;
    }
    increment = increment ? 1 : 0;
    max = get_num_var_ranges ();
    /*  Search for existing MTRR  */

    for (i = 0; i < max; ++i)
    {
	get_mtrr (i, &lbase, &lsize, &ltype);
	if (base >= lbase + lsize) continue;
	if ( (base < lbase) && (base + size <= lbase) ) continue;
	/*  At this point we know there is some kind of overlap/enclosure  */
	if ( (base < lbase) || (base + size > lbase + lsize) )
	{
	    printk ("mtrr: 0x%lx,0x%lx overlaps existing 0x%lx,0x%lx\n",
		    base, size, lbase, lsize);
	    return -EINVAL;
	}
	/*  New region is enclosed by an existing region  */
	if (ltype != type)
	{
	    if (type == MTRR_TYPE_UNCACHABLE) continue;
	    printk ( "mtrr: type mismatch for %lx,%lx old: %s new: %s\n",
		     base, size, attrib_to_str (ltype), attrib_to_str (type) );
	    return -EINVAL;
	}
	if (increment) ++usage_table[i];
	return i;
    }
    /*  Search for an empty MTRR  */
    for (i = 0; i < max; ++i)
    {
	get_mtrr (i, &lbase, &lsize, &ltype);
	if (lsize > 0) continue;
	set_mtrr (i, base, size, type);
	usage_table[i] = 1;
	return i;
    }
    printk ("mtrr: no more MTRRs available\n");
    return -ENOSPC;
}   /*  End Function mtrr_add  */

int mtrr_del (int reg, unsigned long base, unsigned long size)
/*  [SUMMARY] Delete MTRR/decrement usage count.
    <reg> The register. If this is less than 0 then <<base>> and <<size>> must
    be supplied.
    <base> The base address of the region. This is ignored if <<reg>> is >= 0.
    <size> The size of the region. This is ignored if <<reg>> is >= 0.
    [RETURNS] The register on success, else a negative number indicating
    the error code.
    [NOTE] This routine uses a spinlock.
*/
{
    int i, max;
    mtrr_type ltype;
    unsigned long lbase, lsize;

    if (!usage_table) init_table();
    max = get_num_var_ranges ();
    if (reg < 0)
    {
	/*  Search for existing MTRR  */
	for (i = 0; i < max; ++i)
	{
	    get_mtrr (i, &lbase, &lsize, &ltype);
	    if ( (lbase == base) && (lsize == size) )
	    {
		reg = i;
		break;
	    }
	}
	if (reg < 0)
	{
	    printk ("mtrr: no MTRR for %lx,%lx found\n", base, size);
	    return -EINVAL;
	}
    }
    if (reg >= max)
    {
	printk ("mtrr: register: %d too big\n", reg);
	return -EINVAL;
    }
    get_mtrr (reg, &lbase, &lsize, &ltype);
    if (lsize < 1)
    {
	printk ("mtrr: MTRR %d not used\n", reg);
	return -EINVAL;
    }
    if (usage_table[reg] < 1)
    {
	printk ("mtrr: reg: %d has count=0\n", reg);
	return -EINVAL;
    }
    if (--usage_table[reg] < 1) set_mtrr (reg, 0, 0, 0);
    return reg;
}   /*  End Function mtrr_del  */

#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 1, 0) */
