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
;; Revision 1.1  2000/06/15 00:27:43  joseph
;; Initial checkin into SourceForge.
;;
;; 13    1/10/03  KoolSmoky first cut at SSE
;; 
;; 12    4/05/99 11:34a Atai
;; added GLIDE_ALT_TAB for xdraw2.inc to query context in the retail build
;; 
;; 11    3/19/99 11:26a Peter
;; expose direct fifo for gl
;; 
;; 10    10/14/98 12:05p Peter
;; fixed my effed up assumption about non-volatile regs
;; 
;; 9     10/12/98 9:51a Peter
;; dynamic 3DNow!(tm)
;; 
;; 8     9/24/98 11:17a Dow
;; AMD 3DNow! (tm) mods
;; 
;; 7     8/30/98 1:34p Dow
;; State & other optimizations
;; 
;; 6     8/29/98 8:12p Dow
;; Clip optimization
;; 
;; 5     7/01/98 8:41a Jdt
;; removed gc arg from trisetup funcs
;; 
;; 4     8/03/98 6:36a Jdt
;; Add GC to trisetup arglist
;; 
;; 3     6/09/98 11:59a Atai
;; 1. update glide header
;; 2. fix cull mode
;; 3. fix tri stats
;; 
;; 8     5/18/98 3:21p Peter
;; dynamic culling changes
; 
; 6     1/15/98 1:12p Peter
; dispatch w/o packing
; 
; 5     11/06/97 3:47p Peter
; dispatch code w/ simulator
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

.686P
IFDEF GL_AMD3D
.MMX
.K3D
ENDIF
IFDEF GL_SSE
.XMM
ENDIF

;;; Definitions of cvg regs and glide root structures.
INCLUDE fxgasm.h    

EXTRN   __GlideRoot:    DWORD
EXTRN   __grCommandTransportMakeRoom@12: NEAR
EXTRN   __grValidateState:NEAR
EXTRN   _hwcQueryContext:NEAR

IFDEF HAL_CSIM
EXTRN   _halStore32@8:  NEAR
ENDIF

_DATA   SEGMENT
    One         DD  03f800000r
    Area        DD  0
_DATA   ENDS

CONST   SEGMENT
$T2003  DD  046400000r          ; 12288
$T2005  DD  03f800000r          ; 1
$T2006  DD  043800000r          ; 256
CONST   ENDS

; Arguments (STKOFF = 16 from 4 pushes)
STKOFF  = 16
_va$    =  4 + STKOFF
_vb$    =  8 + STKOFF
_vc$    = 12 + STKOFF    

    ;; coordinate offsets into vertex.
    ;; NB:  These are constants and are not
    ;;      user settable like the rest of the
    ;;      parameter offset. Weird.
X       = 0
Y       = 4

PROC_TYPE MACRO procType:=<Default>
    IFDEF GL_AMD3D
        EXITM <__trisetup_3DNow_&procType&@12>
    ELSE
        IFDEF GL_SSE
            EXITM <__trisetup_SSE_&procType&@12>
        ELSE
            EXITM <__trisetup_Default_&procType&@12>
        ENDIF
    ENDIF
    ENDM        

;; enables/disables trisProcessed and trisDrawn counters
STATS = 1

;; offsets into vertex struct
X       = 0
Y       = 4

    ;; NB:  All of the base triangle procs expect to have the gc
    ;;      passed from the caller in edx so that we can avoid
    ;;      the agi from the far pointer. Screw w/ this at your
    ;;      own peril.
    ;;
    ;;      YOU HAVE BEEN WARNED

;--------------------------------------------------------------------------

_TEXT       SEGMENT PAGE PUBLIC USE32 'CODE'
            ASSUME DS: FLAT, SS: FLAT

            ALIGN    32
            PUBLIC   PROC_TYPE(clip_nocull_invalid)
PROC_TYPE(clip_nocull_invalid)        PROC    NEAR
           
GLIDE_VALIDATE_STATE textequ <1>
GLIDE_CLIP_COORDS    textequ <1>
GLIDE_CULLING        textequ <0>
GLIDE_PACK_RGB       textequ <0>
GLIDE_PACK_ALPHA     textequ <0>
GLIDE_GENERIC_SETUP  textequ <0>
INCLUDE xdraw2.inc
GLIDE_GENERIC_SETUP  textequ <0>    
GLIDE_PACK_ALPHA     textequ <0>
GLIDE_PACK_RGB       textequ <0>    
GLIDE_CULLING        textequ <0>
GLIDE_CLIP_COORDS    textequ <0>
GLIDE_VALIDATE_STATE textequ <0>

PROC_TYPE(clip_nocull_invalid) ENDP    

            ALIGN  32
            PUBLIC   PROC_TYPE(clip_cull_invalid)
PROC_TYPE(clip_cull_invalid)   PROC    NEAR

GLIDE_VALIDATE_STATE textequ <1>
GLIDE_CLIP_COORDS    textequ <1>
GLIDE_CULLING        textequ <1>
GLIDE_PACK_RGB       textequ <0>
GLIDE_PACK_ALPHA     textequ <0>
GLIDE_GENERIC_SETUP  textequ <0>
INCLUDE xdraw2.inc
GLIDE_GENERIC_SETUP  textequ <0>    
GLIDE_PACK_ALPHA     textequ <0>
GLIDE_PACK_RGB       textequ <0>    
GLIDE_CULLING        textequ <0>
GLIDE_CLIP_COORDS    textequ <0>
GLIDE_VALIDATE_STATE textequ <0>

PROC_TYPE(clip_cull_invalid)   ENDP
        
            ALIGN    32
            PUBLIC   PROC_TYPE(clip_cull_valid)
PROC_TYPE(clip_cull_valid)  PROC    NEAR

GLIDE_VALIDATE_STATE textequ <0>
GLIDE_CLIP_COORDS    textequ <1>   
GLIDE_CULLING        textequ <1>
GLIDE_PACK_RGB       textequ <0>
GLIDE_PACK_ALPHA     textequ <0>
GLIDE_GENERIC_SETUP  textequ <0>
INCLUDE xdraw2.inc
GLIDE_GENERIC_SETUP  textequ <0>    
GLIDE_PACK_ALPHA     textequ <0>
GLIDE_PACK_RGB       textequ <0>    
GLIDE_CULLING        textequ <0>
GLIDE_CLIP_COORDS    textequ <0>
GLIDE_VALIDATE_STATE textequ <0>

PROC_TYPE(clip_cull_valid) ENDP
    
            ALIGN    32
            PUBLIC   PROC_TYPE(clip_nocull_valid)
PROC_TYPE(clip_nocull_valid)  PROC    NEAR

GLIDE_VALIDATE_STATE textequ <0>
GLIDE_CLIP_COORDS    textequ <1>   
GLIDE_CULLING        textequ <0>
GLIDE_PACK_RGB       textequ <0>
GLIDE_PACK_ALPHA     textequ <0>
GLIDE_GENERIC_SETUP  textequ <0>
INCLUDE xdraw2.inc
GLIDE_GENERIC_SETUP  textequ <0>    
GLIDE_PACK_ALPHA     textequ <0>
GLIDE_PACK_RGB       textequ <0>    
GLIDE_CULLING        textequ <0>
GLIDE_CLIP_COORDS    textequ <0>
GLIDE_VALIDATE_STATE textequ <0>

PROC_TYPE(clip_nocull_valid) ENDP

            ALIGN    32
            PUBLIC   PROC_TYPE(win_nocull_invalid)
PROC_TYPE(win_nocull_invalid) PROC    NEAR
         
GLIDE_VALIDATE_STATE textequ <1>
GLIDE_CLIP_COORDS    textequ <0>
GLIDE_CULLING        textequ <0>
GLIDE_PACK_RGB       textequ <0>
GLIDE_PACK_ALPHA     textequ <0>
GLIDE_GENERIC_SETUP  textequ <0>
INCLUDE xdraw2.inc
GLIDE_GENERIC_SETUP  textequ <0>    
GLIDE_PACK_ALPHA     textequ <0>
GLIDE_PACK_RGB       textequ <0>    
GLIDE_CULLING        textequ <0>
GLIDE_CLIP_COORDS    textequ <0>
GLIDE_VALIDATE_STATE textequ <0>

PROC_TYPE(win_nocull_invalid) ENDP    

            ALIGN    32
            PUBLIC   PROC_TYPE(win_cull_invalid)

PROC_TYPE(win_cull_invalid)  PROC    NEAR
  
GLIDE_VALIDATE_STATE textequ <1>
GLIDE_CLIP_COORDS    textequ <0>
GLIDE_CULLING        textequ <1>
GLIDE_PACK_RGB       textequ <0>
GLIDE_PACK_ALPHA     textequ <0>
GLIDE_GENERIC_SETUP  textequ <0>
INCLUDE xdraw2.inc
GLIDE_GENERIC_SETUP  textequ <0>    
GLIDE_PACK_ALPHA     textequ <0>
GLIDE_PACK_RGB       textequ <0>    
GLIDE_CULLING        textequ <0>
GLIDE_CLIP_COORDS    textequ <0>
GLIDE_VALIDATE_STATE textequ <0>

PROC_TYPE(win_cull_invalid) ENDP    

            ALIGN    32
            PUBLIC   PROC_TYPE(win_cull_valid)
PROC_TYPE(win_cull_valid)  PROC    NEAR

GLIDE_VALIDATE_STATE textequ <0>
GLIDE_CLIP_COORDS    textequ <0>   
GLIDE_CULLING        textequ <1>
GLIDE_PACK_RGB       textequ <0>
GLIDE_PACK_ALPHA     textequ <0>
GLIDE_GENERIC_SETUP  textequ <0>
INCLUDE xdraw2.inc
GLIDE_GENERIC_SETUP  textequ <0>    
GLIDE_PACK_ALPHA     textequ <0>
GLIDE_PACK_RGB       textequ <0>    
GLIDE_CULLING        textequ <0>
GLIDE_CLIP_COORDS    textequ <0>
GLIDE_VALIDATE_STATE textequ <0>

PROC_TYPE(win_cull_valid) ENDP
    
            ALIGN    32
            PUBLIC   PROC_TYPE(win_nocull_valid)
PROC_TYPE(win_nocull_valid)  PROC    NEAR

GLIDE_VALIDATE_STATE textequ <0>
GLIDE_CLIP_COORDS    textequ <0>   
GLIDE_CULLING        textequ <0>
GLIDE_PACK_RGB       textequ <0>
GLIDE_PACK_ALPHA     textequ <0>
GLIDE_GENERIC_SETUP  textequ <0>
INCLUDE xdraw2.inc
GLIDE_GENERIC_SETUP  textequ <0>    
GLIDE_PACK_ALPHA     textequ <0>
GLIDE_PACK_RGB       textequ <0>    
GLIDE_CULLING        textequ <0>
GLIDE_CLIP_COORDS    textequ <0>
GLIDE_VALIDATE_STATE textequ <0>

PROC_TYPE(win_nocull_valid) ENDP

IFDEF       GL_AMD3D    
            ALIGN   32
            PUBLIC  __trisetup_clip_coor_thunk@12
__trisetup_clip_coor_thunk@12 PROC NEAR

procPtr TEXTEQU <eax>    
vPtr    TEXTEQU <ecx>
gc      TEXTEQU <edx>           ; Current graphics context passed implicitly through edx
    
    ;; Call through to the gc->curArchProcs.drawTrianglesProc w/o
    ;; adding extra stuff to the stack. I wish we could actually
    ;; do a direct return here w/o too much work.
    lea     vPtr, [esp + _va$ - STKOFF]         ; Get vertex pointer address
    mov     procPtr, [gc + drawTrianglesProc]   ; Prefetch drawTriangles proc addr
    
    push    vPtr                ; vertex array address
    push    3                   ; 3 vertices

    ;; If debugging make sure that we're in clip coordinates
IFDEF GLIDE_DEBUG
    mov     eax, [gc + CoordinateSpace]
    test    eax, 1
    jnz     __clipSpace
    xor     eax, eax
    mov     [eax], eax
__clipSpace:    
ENDIF ; GLIDE_DEBUG

    push    1                   ; mode = grDrawVertexArray
    call    procPtr             ; (*gc->curArchProcs.drawTrianglesProc)(grDrawVertexArray, 3, vPtr)

    ret     12                  ; pop 3 dwords (vertex addrs) and return    
__trisetup_clip_coor_thunk@12 ENDP

ENDIF ; GL_AMD3D    

IFDEF GL_SSE
            ALIGN   32
            PUBLIC  __trisetup_SSE_clip_coor_thunk@12
__trisetup_SSE_clip_coor_thunk@12 PROC NEAR

procPtr TEXTEQU <eax>    
vPtr    TEXTEQU <ecx>
gc      TEXTEQU <edx>           ; Current graphics context passed implicitly through edx
    
    ;; Call through to the gc->curArchProcs.drawTrianglesProc w/o
    ;; adding extra stuff to the stack. I wish we could actually
    ;; do a direct return here w/o too much work.
    lea     vPtr, [esp + _va$ - STKOFF]         ; Get vertex pointer address
    mov     procPtr, [gc + drawTrianglesProc]   ; Prefetch drawTriangles proc addr
    
    push    vPtr                ; vertex array address
    push    3                   ; 3 vertices

    ;; If debugging make sure that we're in clip coordinates
IFDEF GLIDE_DEBUG
    mov     eax, [gc + CoordinateSpace]
    test    eax, 1
    jnz     __clipSpace
    xor     eax, eax
    mov     [eax], eax
__clipSpace:    
ENDIF ; GLIDE_DEBUG

    push    1                   ; mode = grDrawVertexArray
    call    procPtr             ; (*gc->curArchProcs.drawTrianglesProc)(grDrawVertexArray, 3, vPtr)

    ret     12                  ; pop 3 dwords (vertex addrs) and return    
__trisetup_SSE_clip_coor_thunk@12 ENDP

ENDIF ; GL_SSE

_TEXT   ENDS
        END

