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
;; 3     10/07/98 9:43p Peter
;; triangle procs for 3DNow!(tm)
;; 
;; 2     10/05/98 7:43p Peter
;; 3DNow!(tm) happiness everywhere
;; 
;; 1     10/05/98 6:01p Peter
;; mmx stuff for 3DNow!(tm) capable processors
;; 

TITLE   xtexdl.asm
OPTION OLDSTRUCTS

.586P
.MMX
.K3D
    
EXTRN   __FifoMakeRoom: NEAR

;;; Definitions of cvg regs and glide root structures.
INCLUDE fxgasm.h

; Arguments (STKOFF = 12 from 3 dword pushes)
STACKOFFSET = 12
_gc$	    =  4 + STACKOFFSET
_baseAddr$  =  8 + STACKOFFSET
_maxS$	    = 12 + STACKOFFSET
_minT$	    = 16 + STACKOFFSET
_maxT$	    = 20 + STACKOFFSET
_texData$   = 24 + STACKOFFSET

    ;; NB: The first set of registers (eax-edx) are volatile across
    ;; function calls. The remaining registers are supposedly non-volatile
    ;; so they only store things that are non-volatile across the call.
fifo	TEXTEQU <eax>		; Current fifo ptr in inner loop
texAddr TEXTEQU <ebx>		; Physical download address of the current scanline    
gc	TEXTEQU	<ecx>		; Current graphics context
curS	TEXTEQU	<edx>		; Current texture scanline
    
maxT	TEXTEQU	<esi>		; Max scanline line value (inclusive)
dataPtr TEXTEQU <edi>		; Current user texture data ptr
curT	TEXTEQU	<ebp>		; Current s coordinate in inner loop

temp1	TEXTEQU	curS
temp2	TEXTEQU texAddr
temp3	TEXTEQU	gc    
    
GR_FIFO_WRITE   MACRO __addr, __offset, __data
    mov    [__addr + __offset], __data
ENDM ; GR_FIFO_WRITE

;--------------------------------------------------------------------------
_TEXT       SEGMENT PAGE PUBLIC USE32 'CODE'
            ASSUME DS: FLAT, SS: FLAT

            ALIGN  32

	    PUBLIC __grTexDownload_3DNow_MMX@24
__grTexDownload_3DNow_MMX@24 PROC NEAR

    ;; Function prologue type things
    ;; NB:   We are not bothering to preserve the contents
    ;;	     of eax, ebx, ecx, edx because they are volatile
    ;;	     by convention.
    
    ;; Enter 3DNow!(tm) state for the duration of the function
    ;; because we don't use or call anything that uses fp.
    femms
    
    mov	    gc, [esp + _gc$ - STACKOFFSET + 0]
    push    esi
    
    mov	    maxT, [esp + _maxT$ - STACKOFFSET + 4]
    push    edi

    shl	    maxT, 9		; Convert maxT to rowAddr format
    push    ebp
        
    mov	    dataPtr, [esp + _texData$]
    mov	    curT, [esp + _minT$]    

    ;; Pad out fifo so that we can use mmx writes the whole way w/o
    ;; any intermediate tests in the inner loop for fifo alignment.
    ;; Conveniently, the packet header is 2 dwords which matches
    ;; the size of the mmx write.
    mov	    fifo, [gc + fifoPtr]; Cache fifo ptr
    mov	    texAddr, [esp + _baseAddr$]; Texture physical address

    mov	    temp1, [esp + _maxS$]; Pre-convert maxS into packet 5 field format
    sub	    texAddr, [gc + tex_ptr]; Convert to hw base relative address            

    shl	    temp1, 2		; Write size dwords -> bytes
    mov	    [esp + _baseAddr$], texAddr

    shl	    curT, 9		; curT = TEX_ROW_ADDR_INCR(curT)      
    mov	    [esp + _maxS$], temp1; Write back converted s coordinate

    shl	    temp1, 1		; Write size to packet 5 field format
    test    fifo, 4h		; Aligned fifo ptr?
    
    mov	    temp2, [gc + fifoRoom]; temp2 = gc->fifoRoom
    mov	    [esp + _maxT$], temp1; Write back converted field format size
    
    jz	    __loopT

    ;; Check to make sure there's room in the fifo. If not then
    ;; we'll wrap and then it should be aligned for the remainder of
    ;; this function invocation.
    cmp	    temp2, 4h
    jg	    __mmxAlignFifo

    push    @Line		; Line # inside this function
    push    0h			; NULL file name

    push    4h			; fifo space required
    call    __FifoMakeRoom	; Get fifo room

    ;; Calling out to external code means that our registers can get
    ;; trashed in the same way that we trash things. Plus we need to
    ;; re-cache the fifoPtr since we may have wrapped.

    add	    esp, 12		; Pop the 3 DWORDs for the fifoWrap parameters
    mov	    gc, [esp + _gc$]

    ;; Setup the regs to do the alignment
    mov	    fifo, [gc + fifoPtr]
    test    fifo, 4h
    
    mov	    temp2, [gc + fifoRoom]
    jz	    __loopT

__mmxAlignFifo:	    
    add	    fifo, 4h		; packetPtr++
    xor	    temp1, temp1	; Clear the nop packet
    
    mov	    [gc + fifoPtr], fifo; gc->fifoPtr = packetPtr
    sub	    temp2, 4h		; fifoRoom -= 4

    mov	    [gc + fifoRoom], temp2; gc->fifoRoom = fifoRoom
    GR_FIFO_WRITE fifo, -4, temp1; NOP packet(0)

	    align 4
__loopT:
    ;; Check for room to write the current texture scanline    
    mov	    temp1, [esp + _maxS$]; temp1 = width of scanline (bytes)
    mov	    temp2, [gc + fifoRoom]; temp2 = gc->fifoRoom (bytes)

    add	    temp1, 0Ch		; scanline width + sizeof(packet hdr) (bytes) + nop packet to mmx align
    cmp	    temp2, temp1	; fifo space required >= space availible ?
    
    jge	    __dlBegin		; Yes, start download now w/ no more checking

    push    @Line		; Line # inside this function
    push    0h			; NULL file name

    push    temp1		; fifo space required
    call    __FifoMakeRoom	; Get fifo room

    add	    esp, 12		; Pop the 3 DWORDs for the fifoWrap parameters
    nop
    
    ;; Calling out to external code means that our registers can get
    ;; trashed in the same way that we trash things. Plus we need to
    ;; re-cache the fifoPtr since we may have wrapped.
    mov	    gc, [esp + _gc$]        
    mov	    fifo, [gc + fifoPtr]

IFDEF GLIDE_DEBUG
    ;; Make sure that we have an mmx happy aligned fifoPtr
    test    fifo, 4
    jz	    @1

    ;; Fault right away because this would be a huge suck
    xor	    eax, eax
    mov	    [eax], eax
@1:      
ENDIF ; GLIDE_DEBUG    

    	    align 4
__dlBegin:

IFDEF GLIDE_DEBUG
    ;; Make sure that we have an mmx happy aligned fifoPtr
    test    fifo, 4
    jz	    @2

    ;; Fault right away because this would be a huge suck
    xor	    eax, eax
    mov	    [eax], eax
@2:      
ENDIF ; GLIDE_DEBUG
    
    ;; Compute packet header words
    ;;	hdr1: downloadSpace[31:30] numWords[21:3] packetType[2:0]
    ;;  hdr2: download address[29:0]
    mov	    texAddr, [esp + _baseAddr$]; Download base address
    mov	    temp1, [esp + _maxT$]; Pre-Converted # of words per packet/scanline
    
    mov	    temp3, 0C0000005h	; Base packet header (texture port | packet type 5)
    add	    fifo, 8		; Pre-increment fifo ptr (hdr1)
    
    or	    temp3, temp1	; Base packet hdr | # of words
    add	    texAddr, curT	; texAddr = texBaseAddr + TEX_ROW_ADDR_INCR(curT)
    
    GR_FIFO_WRITE fifo, -8, temp3; Write hdr1
    add	    curT, 200h		; curT += TEX_ROW_ADDR_INCR(1)
    
    GR_FIFO_WRITE fifo, -4, texAddr; write hdr2
    mov	    curS, [esp + _maxS$]; curS = maxS

	    align 4
    ;; S coordinate inner loop unrolled for 8 texels a write
__loopS:        
    movq    mm0, [dataPtr]	; load (mmx) 64 bit data (8 texels)
    add	    fifo, 8h		; pre-increment fifoPtr += 2 * sizeof(FxU32)

    add	    dataPtr, 8h		; dataPtr += 2 * sizeof(FxU32)
    sub	    curS, 8h		; curS -= 2 * sizeof(FxU32)

    movq    [fifo - 8], mm0	; *fifoPtr = texelData[64 bits]
    jnz	    __loopS		; if curS > 0

    mov	    gc, [esp + _gc$]	; Re-cache gc which was trashed in the dl loop
    mov	    temp1, fifo

    ;; Update gc->fifoPtr and gc->fifoRoom for the wrap/stall check
    mov	    temp2, [gc + fifoPtr]
    sub	    temp1, temp2	; # of bytes written to the fifo

    mov	    [gc + fifoPtr], fifo; gc->fifoPtr = packetPtr
    mov	    temp2, [gc + fifoRoom]
    
    sub	    temp2, temp1	; # of bytes left in fifo
    cmp	    curT, maxT		; if (curT <= maxT) ?    

    mov	    [gc + fifoRoom], temp2
    jle	    __loopT

__dlDone:	    
	    align 4

    femms			; Exit 3DNow!(tm) state    
    pop	    ebp
    
    pop	    edi
    pop	    esi
    
    ret 18h			; Pop 6 parameters and return

__grTexDownload_3DNow_MMX@24 ENDP

_TEXT ENDS

END
