/*
   kinfo.c: A small program that includes several kernel heades 
            and builds a header defining options for 3dfx_driver.c.

   1999/12/22   Joseph Kain     <joseph@3dfx.com>
   
   * Initial version
*/

#include <stdio.h>

/* These include files will bring in the information we need */
#include <linux/autoconf.h>
#include <linux/version.h>


int
main (int argc, char **argv)
{
  /* Here is the strategy.  The old Makefile would have grepped the kernel
   * headers and put these defines on the command line.  Now we include
   * the kernel headers and extract the information.  Then we create a 
   * new header with all of the options we need. */

  if (argc == 1)
  {
    FILE *f = fopen ("kinfo.h", "w");

#ifdef CONFIG_MTRR
    fprintf (f, "#define HAVE_MTRR\n");
#endif
    
#ifdef CONFIG_SMP
    fprintf (f, "#define __SMP__\n");
#endif
    
#ifdef CONFIG_MODVERSIONS
    fprintf (f, "#define MODVERSIONS\n");
#endif

    fclose (f);
  }
  else
  {
    if (strcmp (argv[1], "--UTS") == 0)
    {
      printf ("%s", UTS_RELEASE);
    }
  }
}


