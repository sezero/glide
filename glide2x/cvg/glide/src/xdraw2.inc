;; -*-asm-*-
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

TITLE   xdraw2.inc
.586P
        
EXTRN   _GlideRoot:DWORD
EXTRN   _FifoMakeRoom:NEAR

        
; Ugly, but seems to workaround the problem with locally defined
; data segment globals not getting relocated properly when using
; djgpp.

zArea   TEXTEQU <One+04h>
zdxAB   TEXTEQU <One+08h>
zdxBC   TEXTEQU <One+0ch>
zdyAB   TEXTEQU <One+10h>
zdyBC   TEXTEQU <One+14h>
zculltest TEXTEQU <One+18h>

gc      TEXTEQU     <esi>       ; points to graphics context

    ;; Prologue stuff
    push    esi
    push    edi
    
    push    ebx
    push    ebp

    mov     gc, [_GlideRoot + curGC]    ;; GR_DCL_GC    

    	    align 4
IF GLIDE_CULLING
fa      TEXTEQU     <eax>       ; vtx a from caller
fb      TEXTEQU     <ebx>       ; vtx b from caller
fc      TEXTEQU     <ecx>       ; vtx c from caller

cull	TEXTEQU     <edx>
intArea TEXTEQU     <ebp>       ; temp Y storage

    ;; Pre-load the current culling mode before all of the
    ;; floating point area stuff.    
    mov     cull, [gc + cull_mode]    
    mov     fa, [esp + _va$]

    mov     fb, [esp + _vb$]
    mov     fc, [esp + _vc$]

    shl     cull, 31                    ; culltest << 31    
        
Area_Computation:    
; 47-3
; jmp ret_pop0f
    flds    [fa + X]            ;  xa
    fsubs   [fb + X]            ;  dxAB
    flds    [fb + X]            ;  |    xb
    fsubs   [fc + X]            ;  |    dxBC
    flds    [fb + Y]            ;  |    |    yb
    fsubs   [fc + Y]            ;  |    |    dyBC
    flds    [fa + Y]            ;  |    |    |    ya
    fsubs   [fb + Y]            ;  |    |    |    dyAB
    fld     st(3)               ;  |    |    |    |    dxAB
    fmul    st, st(2)           ;  |    |    |    |    t0         t0=dxAB*dyBC
    fld     st(3)               ;  |    |    |    |    |    dxBC
    fmul    st, st(2)           ;  |    |    |    |    |    t1    t1=dxBC*dyAB
    fsubp   st(1),st            ;  |    |    |    |    area
    fst     zArea               ;  |    |    |    |    area

    ;; Pop temp things from the sw culling off the fp stack
    fstp    st(0)   ; 4
    fstp    st(0)   ; 3
    fstp    st(0)   ; 2
    fstp    st(0)   ; 1
    fstp    st(0)   ; 0    

    mov     intArea, zArea        ; j = *(long *)&area
    xor	    eax, eax		  ; Clear the return value (0 == culled)

    ; Zero Area Triangle Check
    and     intArea, 7fffffffh    ; if ((j & 0x7FFFFFFF) == 0)
    jz      __triDone

    ;; Triangle area check vs culling mode
    mov     intArea, zArea              ; reload area just in case we're culling
    xor     intArea, cull               ; if (j ^ (culltest << 31))
    
    jge	    __triDone
ENDIF ; GLIDE_CULLING    

	    align 4
    ;; Check to make sure that we have enough room for
    ;; the complete triangle packet.
    mov	    eax, [_GlideRoot + curTriSize]
    mov	    ebx, [gc + fifoRoom]

    add	    eax, 4
    cmp	    ebx, eax

    jge	    __triBegin
    
    push    @Line
    push    0h
    
    push    eax
    call    _FifoMakeRoom

    add	    esp, 12

    ;; Send triangle parameters
    
dlp     TEXTEQU     <ebx>       ; points to dataList structure
fifo    TEXTEQU     <ebp>       ; points to next entry in fifo
vertex  TEXTEQU	    <edx>	; the current vertex
vOffset TEXTEQU	    <ecx>	; Current vertex offset

packCol TEXTEQU	    <edi>
tempVal TEXTEQU	    <edi>

GR_FIFO_WRITE	MACRO __addr, __offset, __data
    mov    [__addr + __offset], __data
ENDM ; GR_FIFO_WRITE

            align 4    
__triBegin:
    mov     fifo, [gc + fifoPtr]	; Fetch Fifo Ptr
    mov	    vOffset, 4			; Starting vertex

    mov	    eax, [gc + triPacketHdr]	; Packet 3 header
    nop

    GR_FIFO_WRITE fifo, 0, eax		; Write packet header to fifo    
    add	    fifo, 4			; Advance fifo for hdr & x/y coordinate

	    align 4    
__vertexStart:
    mov	    vertex, [esp + STKOFF + vOffset]	; Current vertex
    add	    fifo, 8    

    nop						; Avoid p5 agi w/ load of vertex ptr
    nop
    
    mov	    eax, DWORD PTR [vertex + x]		; X
    lea	    dlp, [gc + tsuDataList]		; Reset the dataList

    GR_FIFO_WRITE fifo, -8, eax			; PCI write X
    mov	    eax, DWORD PTR [vertex + y]		; Y 

    xor	    packCol, packCol			; Clear packed color
    GR_FIFO_WRITE fifo, -4, eax			; PCI write Y

IF GLIDE_PACK_RGB
    fld	    DWORD PTR [vertex + b]		; B
    fadd    DWORD PTR _GlideRoot + fBiasLo	; BC GC

    fld	    DWORD PTR [vertex + g]		; G B
    fadd    DWORD PTR _GlideRoot + fBiasHi	; GC B
    
    fld	    DWORD PTR [vertex + r]		; R GC BC
    fadd    DWORD PTR _GlideRoot + fBiasHi	; RC GC BC

    fxch    st(2)				; BC GC RC
    fstp    DWORD PTR bias0			; GC RC

    fstp    DWORD PTR bias1			; RC
    mov	    packCol, DWORD PTR bias0		; B + bias

    fstp    DWORD PTR bias0
    mov	    eax, DWORD PTR bias1		; G + bias
    
IF GLIDE_PACK_ALPHA
    fld	    DWORD PTR [vertex + a]
    fadd    DWORD PTR _GlideRoot + fBiasHi

    and	    packCol, 00FFh			; B color component
    and	    eax, 0000FF00h			; G component << 8

    add	    dlp, 8				; Packed RGB + A dataList entry
    nop

    or	    packCol, eax			; 0000GGBB
    nop

    fstp    DWORD PTR bias1
    mov	    eax, DWORD PTR bias0		; R + bias
    
    mov	    esi, DWORD PTR bias1		; A + bias
    and	    eax, 0000FF00h			; R component << 8
    
    and	    esi, 0FFFFFF00h			; A component << 8
    shl	    eax, 8				; R << 16
    
    or	    packCol, eax			; 00RRGGBB
    shl	    esi, 16				; A << 16

    or	    packCol, esi			; AARRGGBB
    nop
ELSE ; !GLIDE_PACK_ALPHA    
    and	    packCol, 00FFh			; B color component
    and	    eax, 0000FF00h			; G component << 8

    add	    dlp, 4				; Next dataList item    
    or	    packCol, eax
    
    mov	    eax, DWORD PTR bias0		; R + bias
    and	    eax, 0000FF00h			; R component << 8

    shl	    eax, 8				; R << 16
    or	    packCol, eax			; 00RRGGBB
ENDIF ; !GLIDE_PACK_ALPHA

    GR_FIFO_WRITE fifo, 0, packCol		; PCI write packed color value
    add	    fifo, 4
ENDIF ; GLIDE_PACK_RGB

__doParams:
    mov	    eax, DWORD PTR [dlp]		; Get first offset from the data list
    add	    dlp, 4				; dlp++
    
    cmp	    eax, 0				; Are we done?
    je	    __nextVertex

    ;; Not using align directive here because it sometimes
    ;; introduces an agi for the eax use below.
    nop
    nop
        
__paramLoop:
    mov     tempVal, DWORD PTR [eax + vertex]	; Get the parameter from teh vertex
    add     fifo, 4				; fifoPtr += sizeof(FxU32)

    mov     eax, DWORD PTR [dlp]		; offset = *(dlp + 1)
    add     dlp, 4				; dlp++
    
    cmp     eax, 0				; Are we done?
    GR_FIFO_WRITE fifo, -4, tempVal		; *fifoPtr = data
    
    jne     SHORT __paramLoop

		align 4        
__nextVertex:	
    ;; On to the next vertex
    add	    vOffset, 4
    mov     gc, [_GlideRoot + curGC]		; Reload gc incase we trashed it as a temp

    cmp	    vOffset, 16				; Offset of one past last vertex?
    jne	    __vertexStart

    ;; Update gc->fifoPtr and gc->fifoRoom
    mov	    eax, fifo
    mov	    ebx, [gc + fifoPtr]
    
    mov	    [gc + fifoPtr], fifo
    sub	    eax, ebx

    mov	    ebx, [_GlideRoot + trisDrawn]		; _GlideRoot.stats.trisDrawn++;    
    sub	    [gc + fifoRoom], eax

    add	    ebx, 1
    mov	    [_GlideRoot + trisDrawn], ebx

    ;; return 1 (triangle drawn)    
    mov     eax, 1h

__triDone:    
    ;; Restore trashed registers
    mov	    esi, [_GlideRoot + trisProcessed]
    pop     ebp
        
    add     esi, 1    ; _GlideRoot.stats.trisProcessed++;    
    pop     ebx
    
    pop     edi
    mov	    [_GlideRoot + trisProcessed], esi
        
    pop     esi
    ret     12