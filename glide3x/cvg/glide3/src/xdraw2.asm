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
;; 1     10/08/98 11:30a Brent
;; 
;; 5     10/07/98 9:43p Peter
;; triangle procs for 3DNow!(tm)
;; 
;; 4     9/21/98 7:02p Dow
;; 3DNow! (tm) mods
;; 
;; 3     7/24/98 1:41p Hohn
;; 
;; 2     6/05/98 5:14p Atai
;; added condition code for win/clip and aa/non-aa
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
.MMX
.K3D

EXTRN   __GlideRoot	    : DWORD
EXTRN   __FifoMakeRoom	    : NEAR

_DATA   SEGMENT
    One         DD  03f800000r
    Area        DD  0
_DATA   ENDS    

;;; Definitions of cvg regs and glide root structures.
INCLUDE fxgasm.h

;; enables/disables trisProcessed and trisDrawn counters
STATS = 1

; Arguments (STKOFF = 16 from 4 pushes)
STKOFF  = 16
_va$    =  4 + STKOFF
_vb$    =  8 + STKOFF
_vc$    = 12 + STKOFF    

    ;; coordinate offsets into vertex.
    ;; NB:  These are constants and are not
    ;;	    user settable like the rest of the
    ;;	    parameter offset. Weird.
X       = 0
Y       = 4

CONST   SEGMENT
$T2003  DD  046400000r          ; 12288
$T2005  DD  03f800000r          ; 1
$T2006  DD  043800000r          ; 256
CONST   ENDS

PROC_TYPE MACRO procType:=<Default>
    IFDEF GL_AMD3D
	EXITM <__trisetup_3DNow_&procType&@12>
    ELSE
	EXITM <__trisetup_Default_&procType&@12>
    ENDIF
    ENDM    

;--------------------------------------------------------------------------

_TEXT       SEGMENT PAGE PUBLIC USE32 'CODE'
            ASSUME DS: FLAT, SS: FLAT

            ALIGN  32

            PUBLIC  PROC_TYPE(cull)
PROC_TYPE(cull)  PROC    NEAR

GLIDE_CULLING       textequ <1>
GLIDE_PACK_RGB      textequ <0>
GLIDE_PACK_ALPHA    textequ <0>
GLIDE_GENERIC_SETUP textequ <0>
INCLUDE xdraw2.inc
GLIDE_GENERIC_SETUP textequ <0>    
GLIDE_PACK_ALPHA    textequ <0>
GLIDE_PACK_RGB      textequ <0>    
GLIDE_CULLING       textequ <0>

PROC_TYPE(cull) ENDP

            ALIGN  32

            PUBLIC  PROC_TYPE()
PROC_TYPE()  PROC    NEAR

GLIDE_CULLING       textequ <0>
GLIDE_PACK_RGB      textequ <0>
GLIDE_PACK_ALPHA    textequ <0>
GLIDE_GENERIC_SETUP textequ <0>
INCLUDE xdraw2.inc
GLIDE_GENERIC_SETUP textequ <0>    
GLIDE_PACK_ALPHA    textequ <0>
GLIDE_PACK_RGB      textequ <0>    
GLIDE_CULLING       textequ <0>

PROC_TYPE() ENDP

IFNDEF GL_AMD3D    
	    ALIGN   32
	    PUBLIC  __trisetup_clip_coor_thunk@12
__trisetup_clip_coor_thunk@12 PROC NEAR

gc	TEXTEQU	<eax>		; Current graphics context
procPtr TEXTEQU <ebx>    
vPtr	TEXTEQU	<ecx>
    
    ;; Call through to the gc->curArchProcs.drawTrianglesProc w/o
    ;; adding extra stuff to the stack. I wish we could actually
    ;; do a direct return here w/o too much work.
    lea	    vPtr, [esp + _va$ - STKOFF]	; Get vertex pointer address
    mov     gc, [__GlideRoot + curGC]; GR_DCL_GC

    ;; If debugging make sure that we're in clip coordinates
IFDEF GLIDE_DEBUG
    mov     ebx, [gc + CoordinateSpace]
    test    ebx, 1
    jnz	    __clipSpace
    xor	    eax, eax
    mov	    [eax], eax
__clipSpace:    
ENDIF ; GLIDE_DEBUG

    mov	    procPtr, [gc + drawTrianglesProc]; Prefetch drawTriangles proc addr
    push    vPtr		; vertex array address
    
    push    3			; 3 vertices
    push    1			; mode = grDrawVertexArray

    call    procPtr		; (*gc->curArchProcs.drawTrianglesProc)(grDrawVertexArray, 3, vPtr)

    ret	    12			; pop 3 dwords (vertex addrs) and return    
__trisetup_clip_coor_thunk@12 ENDP

ENDIF ; !GL_AMD3D
    
_TEXT	ENDS
	END
