;; THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
;; PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
;; TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
;; INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
;; DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
;; THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
;; EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
;; FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
;; 
;; USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
;; RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
;; TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
;; AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
;; SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
;; THE UNITED STATES.  
;; 
;; COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
;;
;; $Header$
;; $Revision$
;; $Log$
;; 
;; 7     5/18/98 12:16p Peter
;; culling enabling
; 
; 6     1/15/98 1:12p Peter
; dispatch w/o packing
; 
; 5     11/06/97 3:47p Peter
; 
; 4     11/04/97 5:04p Peter
; cataclysm part deux
; 
; 3     11/01/97 10:01a Peter
; tri dispatch stuff
; 
; 2     10/30/97 6:53p Peter
; first real cut at tri asm
; 
; 1     10/30/97 4:29p Peter
; asm tri code
; 
; 2     7/07/97 2:14p Jdt
; assembly now on par with C code.
; 
; 1     7/07/97 8:37a Jdt
; B4 Chip field fix.
;;

TITLE   xdraw2.asm
OPTION OLDSTRUCTS       
.586P

.model FLAT,C                   ; Flat memory, mangle publics with leading '_'
        
; some useful floating load and store macros <ala gmt>
flds    TEXTEQU <fld  DWORD PTR>
fsubs   TEXTEQU <fsub DWORD PTR>
fmuls   TEXTEQU <fmul DWORD PTR>

_DATA   SEGMENT
    One         DD  03f800000r
    Area        DD  0
    dxAB        DD  0
    dxBC        DD  0
    dyAB        DD  0
    dyBC        DD  0
    culltest    DD  0

    ;; Extra junk to pad out to the next cache line.
    bias0	DD  0
    pad1	DD  0
    pad2	DD  0
    pad3	DD  0
    pad4	DD  0
    pad5	DD  0
    pad6	DD  0
    pad7	DD  0
    bias1	DD  0
_DATA   ENDS

; Ugly, but seems to workaround the problem with locally defined
; data segment globals not getting relocated properly when using
; djgpp.

zArea   TEXTEQU <One+04h>
zdxAB   TEXTEQU <One+08h>
zdxBC   TEXTEQU <One+0ch>
zdyAB   TEXTEQU <One+10h>
zdyBC   TEXTEQU <One+14h>
zculltest TEXTEQU <One+18h>

;;; Definitions of cvg regs and glide root structures.
INCLUDE fxgasm.h

;; enables/disables trisProcessed and trisDrawn counters
STATS = 1

;--------------------------------------------------------------------------        
; Arguments (STKOFF = 16 from 4 pushes)
STKOFF  = 16
_va$    =  4 + STKOFF
_vb$    =  8 + STKOFF
_vc$    = 12 + STKOFF

X       = 0
Y       = 4
                                ; edx is used as index, loading from *src
gc      TEXTEQU     <esi>       ; points to graphics context

_TEXT       SEGMENT

            align 4
            PUBLIC  _trisetup_cull@12
_trisetup_cull@12  PROC    NEAR
            .code

GLIDE_CULLING	    textequ <1>
GLIDE_PACK_RGB	    textequ <0>
GLIDE_PACK_ALPHA    textequ <0>
GLIDE_GENERIC_SETUP textequ <0>
INCLUDE xdraw2.inc
GLIDE_GENERIC_SETUP textequ <0>    
GLIDE_PACK_ALPHA    textequ <0>
GLIDE_PACK_RGB	    textequ <0>    
GLIDE_CULLING	    textequ <0>

_trisetup_cull@12 ENDP

IF GLIDE_PACKED_RGB
            align 4
            PUBLIC  _trisetup_cull_rgb@12
_trisetup_cull_rgb@12  PROC    NEAR
            .code

GLIDE_CULLING	    textequ <1>
GLIDE_PACK_RGB	    textequ <1>
GLIDE_PACK_ALPHA    textequ <0>
GLIDE_GENERIC_SETUP textequ <0>
INCLUDE xdraw2.inc
GLIDE_GENERIC_SETUP textequ <0>    
GLIDE_PACK_ALPHA    textequ <0>
GLIDE_PACK_RGB	    textequ <0>    
GLIDE_CULLING	    textequ <0>

_trisetup_cull_rgb@12 ENDP

            align 4
            PUBLIC  _trisetup_cull_argb@12
_trisetup_cull_argb@12  PROC    NEAR
            .code

GLIDE_CULLING	    textequ <1>
GLIDE_PACK_RGB	    textequ <1>
GLIDE_PACK_ALPHA    textequ <1>
GLIDE_GENERIC_SETUP textequ <0>
INCLUDE xdraw2.inc
GLIDE_GENERIC_SETUP textequ <0>    
GLIDE_PACK_ALPHA    textequ <0>
GLIDE_PACK_RGB	    textequ <0>    
GLIDE_CULLING	    textequ <0>

_trisetup_cull_argb@12 ENDP
ENDIF ; GLIDE_PACKED_RGB
    
            align 4
            PUBLIC  _trisetup@12
_trisetup@12  PROC    NEAR
            .code

GLIDE_CULLING	    textequ <0>
GLIDE_PACK_RGB	    textequ <0>
GLIDE_PACK_ALPHA    textequ <0>
GLIDE_GENERIC_SETUP textequ <0>
INCLUDE xdraw2.inc
GLIDE_GENERIC_SETUP textequ <0>    
GLIDE_PACK_ALPHA    textequ <0>
GLIDE_PACK_RGB	    textequ <0>    
GLIDE_CULLING	    textequ <0>

_trisetup@12 ENDP

IF GLIDE_PACKED_RGB    
            align 4
            PUBLIC  _trisetup_rgb@12
_trisetup_rgb@12  PROC    NEAR
            .code

GLIDE_CULLING	    textequ <0>
GLIDE_PACK_RGB	    textequ <1>
GLIDE_PACK_ALPHA    textequ <0>
GLIDE_GENERIC_SETUP textequ <0>
INCLUDE xdraw2.inc
GLIDE_GENERIC_SETUP textequ <0>    
GLIDE_PACK_ALPHA    textequ <0>
GLIDE_PACK_RGB	    textequ <0>    
GLIDE_CULLING	    textequ <0>

_trisetup_rgb@12 ENDP

            align 4
            PUBLIC  _trisetup_argb@12
_trisetup_argb@12  PROC    NEAR
            .code

GLIDE_CULLING	    textequ <0>
GLIDE_PACK_RGB	    textequ <1>
GLIDE_PACK_ALPHA    textequ <1>
GLIDE_GENERIC_SETUP textequ <0>
INCLUDE xdraw2.inc
GLIDE_GENERIC_SETUP textequ <0>    
GLIDE_PACK_ALPHA    textequ <0>
GLIDE_PACK_RGB	    textequ <0>    
GLIDE_CULLING	    textequ <0>

_trisetup_argb@12 ENDP
ENDIF ; GLIDE_PACKED_RGB

_TEXT ENDS
END
