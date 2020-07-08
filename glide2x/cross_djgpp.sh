#! /bin/sh

# make sure the DJGPP toolchain is on your PATH

test -d swlibs || ln -s ../swlibs swlibs
make -f Makefile.DJ FX_GLIDE_HW=cvg clean
make -f Makefile.DJ FX_GLIDE_HW=cvg USE_X86=1 USE_3DNOW=1 USE_MMX=1 CC=i586-pc-msdosdjgpp-gcc AR=i586-pc-msdosdjgpp-ar
make -f Makefile.DJ FX_GLIDE_HW=h3 clean
make -f Makefile.DJ FX_GLIDE_HW=h3 USE_X86=1 USE_3DNOW=1 USE_MMX=1 CC=i586-pc-msdosdjgpp-gcc AR=i586-pc-msdosdjgpp-ar
make -f Makefile.DJ FX_GLIDE_HW=sst1 clean
make -f Makefile.DJ FX_GLIDE_HW=sst1 USE_X86=1 USE_3DNOW=1 USE_MMX=1 CC=i586-pc-msdosdjgpp-gcc AR=i586-pc-msdosdjgpp-ar
make -f Makefile.DJ FX_GLIDE_HW=sst96 clean
make -f Makefile.DJ FX_GLIDE_HW=sst96 USE_X86=1 USE_3DNOW=1 USE_MMX=1 CC=i586-pc-msdosdjgpp-gcc AR=i586-pc-msdosdjgpp-ar

