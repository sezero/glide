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
;; 10    8/17/99 6:35p Atai
;; fixed amd debug mode
;; 
;; 9     4/08/99 1:22p Atai
;; added contect check for _grTexDownload_3DNow_MMX
;; 
;; 8     3/19/99 11:26a Peter
;; expose direct fifo for gl
;; 
;; 7     2/02/99 4:36p Peter
;; download through lfb rather than texture port
;; 
;; 6     12/17/98 2:36p Atai
;; check in Norbert's fix for texture download width correction
;; 
;; 5     12/07/98 11:33a Peter
;; norbert's re-fixes of my merge
;; 
;; 4     11/02/98 5:34p Atai
;; merge direct i/o code
;; 
;; 3     10/20/98 5:34p Atai
;; added #ifdefs for hwc
;; 
;; 2     10/14/98 12:05p Peter
;; fixed my effed up assumption about non-volatile regs
;; 
;; 1     10/09/98 6:48p Peter
;; 3DNow!(tm) version of wide texture downloads
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
    
ifdef USE_PACKET_FIFO
EXTRN   __grCommandTransportMakeRoom@12: NEAR
endif
IFDEF HAL_CSIM
EXTRN   _halStore32@8:  NEAR
ENDIF

;;; Definitions of cvg regs and glide root structures.
INCLUDE fxgasm.h

; Arguments (STKOFF = 16 from 4 dword pushes)
STACKOFFSET = 16
_gc$        =  4 + STACKOFFSET
_baseAddr$  =  8 + STACKOFFSET
_maxS$      = 12 + STACKOFFSET
_minT$      = 16 + STACKOFFSET
_maxT$      = 20 + STACKOFFSET
_texData$   = 24 + STACKOFFSET

    ;; NB: The first set of registers (eax, ecx, and edx) are volatile across
    ;; function calls. The remaining registers are supposedly non-volatile
    ;; so they only store things that are non-volatile across the call.

fifo    TEXTEQU <ebp>           ; fifo ptr in inner loop
gc      TEXTEQU <esi>           ; graphics context
dataPtr TEXTEQU <edi>           ; pointer to exture data to be downloaded
curT    TEXTEQU <ebx>           ; counter for texture scan lines (t-coordinate)
curS    TEXTEQU <ecx>           ; texture s-coordinate
fRoom   TEXTEQU <edx>           ; room available in fifo (in bytes)
    
GR_FIFO_WRITE   MACRO __addr, __offset, __data
IFDEF HAL_CSIM
    pushad
    pushfd
    
    push    __data
    mov     eax, __addr
    add     eax, __offset
    push    eax
    call    halStore32@8

    popfd
    popad
ELSE
    mov    [__addr + __offset], __data
ENDIF
ENDM ; GR_FIFO_WRITE

;--------------------------------------------------------------------------

_TEXT         SEGMENT PAGE PUBLIC USE32 'CODE'
              ASSUME DS: FLAT, SS: FLAT

              ALIGN  32

              PUBLIC __grTexDownload_3DNow_MMX@24

__grTexDownload_3DNow_MMX@24 PROC NEAR

    push      ebx                       ; save caller's register variable
    mov       curT, [esp + _maxT$ - 12] ; curT = maxT

    push      esi                       ; save caller's register variable
    mov       eax, [esp + _minT$ - 8]   ; minT

    push      edi                       ; save caller's register variable
    mov       gc, [esp + _gc$ - 4]      ; gc

    push      ebp                       ; save caller's register variable
    mov       dataPtr, [esp + _texData$]; dataPtr

IFDEF GLIDE_ALT_TAB
    test      gc, gc
    je        __dlDone
    mov       edx, DWORD PTR [gc+lostContext]
    mov       ecx, [edx]
    test      ecx, 1
    jnz       __dlDone
ENDIF

    sub       curT, eax                 ; curT = maxT - minT
    mov       fifo, [gc + fifoPtr]      ; fifoPtr

    mov       curS, [esp + _maxS$]      ; curS = maxS 
    add       curT, 1                   ; curT = maxT - minT + 1

    femms                               ; we'll use MMX/3DNow!, make sure FPU register cleared

    mov       edx, curS                 ; curS = maxS = scanline width in DWORDs
    movd      mm3, [esp + _baseAddr$]   ; 0 | address of texture to download

    shl       curS, 2                   ; scan line width (in bytes)
    mov       eax, [esp + _minT$]       ; 0 | minT

    mov       [esp + _maxS$], curS      ; save scan line width (in bytes)
    shl       edx, 3                    ; packetHdr<21:3> = maxS = scanline width in DWORDs

    imul      eax, curS                 ; TEX_ROW_ADDR_INCR(minT) = minT * TEX_ROW_ADDR_INCR(1)

    movd      mm2, curS                 ; 0 | TEX_ROW_ADDR_INCR(1)
    or        edx, 00000005h            ; packetHdr<31:30> = lfb port
                                        ; packetHdr<21:3>  = maxS
                                        ; packetHdr<2:0>   = packetType 5 

    movd      mm1, edx                  ; 0 | packetHdr
    movd      mm4, eax                  ; 0 | TEX_ROW_ADDR_INCR(minT)

    psllq     mm2, 32                   ; TEX_ROW_ADDR_INCR(1) | 0
    paddd     mm3, mm4                  ; 0 | texAddr = texBaseAddr + TEX_ROW_ADDR_INCR(minT)

    mov       fRoom, [gc + fifoRoom]    ; get available fifoRoom (in bytes)
    punpckldq mm1, mm3                  ; hdr2 = texAddr | hdr1 = packetHdr

    ;; ebx = curT, edi = dataPtr, esi = gc, ebp = fifo, ecx = curS = maxS
    ;; edx = fifoRoom, mm1 = texAddr|packetHdr, mm2 = TEX_ROW_ADDR_INCR(1)|0

    test      fifo, 4                   ; is fifo QWORD aligned ?
    jz        __startDownload           ; yup, start texture download

    cmp       fRoom, 4                  ; enough room for NULL packet in fifo?
    jge       __mmxAlignFifo            ; yes, write NULL packet to align fifo

ifdef USE_PACKET_FIFO
    push      @Line                     ; Line # inside this function
    push      0                         ; NULL file name

    push      4                         ; fifo space required (bytes)
    call      __grCommandTransportMakeRoom@12 ; make fifo room
endif
    
    mov       fifo, [gc + fifoPtr]      ; fifoPtr modified by _grCommandTransportMakeRoom, reload

    mov       fRoom, [gc + fifoRoom]    ; fifoRoom modified by _grCommandTransportMakeRoom, reload
    mov       curS, [esp + _maxS$]      ; reload maxS (destroyed by call to _grCommandTransportMakeRoom)

    test      fifo, 4                   ; new fifoPtr QWORD aligned ?
    jz        __startDownload           ; yup, start texture download

__mmxAlignFifo:

    mov       DWORD PTR [fifo], 0       ; write NULL packet
    sub       fRoom, 4                  ; fifoRoom -= 4

    mov       [gc + fifoRoom], fRoom    ; store new fifoRoom
    add       fifo, 4                   ; fifoPtr += 4

IFDEF GLIDE_DEBUG
    mov       [gc + checkPtr], fifo      ; checkPtr
ENDIF

    mov       [gc + fifoPtr], fifo      ; store new fifoPtr
    jmp       __startDownload           ; fifo aligned, download texture now

    align 32

    ;; ebx = curT, edi = dataPtr, esi = gc, ebp = fifo, ecx = maxS = curS
    ;; edx=fifoRoom, mm1 = texAddr|packetHdr, mm2 = TEX_ROW_ADDR_INCR(1)|0

__loopT:

IFDEF GLIDE_DEBUG

    ;; Make sure that we have a QWORD aligned fifoPtr; force GP if not aligned

    test      fifo, 4                   ; is fifoPtr QWORD aligned ?
    jz        __alignmentOK             ; yup, continue

    xor       eax, eax                  ; create 0
    mov       [eax], eax                ; move to DS:[0] forces GP 
__alignmentOK:      
ENDIF ; GLIDE_DEBUG

    ;; Compute packet header words
    ;;  hdr1: downloadSpace[31:30] numWords[21:3] packetType[2:0]
    ;;  hdr2: download address[29:0]

    movq      [fifo], mm1               ; store hdr2 | hdr1
    add       fifo, 8                   ; increment fifo ptr (hdr1 + hdr2)

    ;; S coordinate inner loop unrolled for 8 texels a write

__loopS:        

    movq      mm0, [dataPtr]            ; load 64 bit data (8 texels)
    add       fifo, 8                   ; pre-increment fifoPtr += 2 * sizeof(FxU32)

    add       dataPtr, 8                ; dataPtr += 2 * sizeof(FxU32)
    sub       curS, 8                   ; curS -= 2 * sizeof(FxU32)

    movq      [fifo - 8], mm0           ; *fifoPtr = texelData[64 bits]
    jnz       __loopS                   ; loop while curS > 0

    mov       ecx, [gc + fifoPtr]       ; old fifo ptr
    nop                                 ; filler

    mov       eax, fifo                 ; new fifo ptr
    mov       [gc + fifoPtr], fifo      ; save new fifo ptr

IFDEF GLIDE_DEBUG
    mov       [gc + checkPtr], fifo      ; checkPtr
ENDIF

    sub       eax, ecx                  ; new fifo ptr - old fifo ptr = fifo space used up
    mov       curS, [esp + _maxS$]      ; curS = maxS = width of scanline (bytes)

    sub       fRoom, eax                ; new fifo space available = old fifo space available - fifo space used up = new fifo space available
    sub       curT, 1                   ; curT--

    mov       [gc + fifoRoom], fRoom    ; save new fifo space available 
    jz        __dlDone                  ; loop while curT > 0

    ;; Check for room to write the next texture scanline

    ;; ebx = curT, edi = dataPtr, esi = gc, ebp = fifo
    ;; edx = fifoRoom, mm1 = texAddr|packetHdr, mm2 = TEX_ROW_ADDR_INCR(1)|0

    paddd     mm1, mm2                  ; texAddr+=TEX_ROW_ADDR_INCR(1) | packetHdr
    mov       esp, esp                  ; filler
__startDownload:
    lea       eax, [curS+8]             ; fifo space needed = scan line width + header size
 
    cmp       fRoom, eax                ; fifo space available >= fifo space required ?
    jge       __loopT                   ; yup, write next scan line

ifdef USE_PACKET_FIFO
    push      @Line                     ; Line # inside this function
    push      0h                        ; NULL file name

    push      eax                       ; fifo space required
    call      __grCommandTransportMakeRoom@12 ; make fifo room (if fifoPtr QWORD aligned before
endif
    
    mov       fifo, [gc + fifoPtr]      ; fifoPtr was modified by _grCommandTransportMakeRoom, reload

    mov       fRoom, [gc + fifoRoom]    ; fifoRoom was modified by _grCommandTransportMakeRoom, reload
    mov       curS, [esp + _maxS$]      ; curS = maxS = width of scanline (bytes)
    jmp       __loopT                   ; we now have enough fifo room, write next scanline

__dlDone:           

    femms                               ; exit 3DNow!(tm) state
    
    pop       ebp                       ; restore caller's register variable
    pop       edi                       ; restore caller's register variable
    
    pop       esi                       ; restore caller's register variable
    pop       ebx                       ; restore caller's register variable
    
    ret       24                        ; pop 6 DWORD parameters and return

__grTexDownload_3DNow_MMX@24 ENDP

_TEXT ENDS

END
