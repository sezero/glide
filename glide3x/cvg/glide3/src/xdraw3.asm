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

;;--------------------------------------------------------------------------
;; start AMD3D version
;;--------------------------------------------------------------------------

ifdef GL_AMD3D

TITLE   xdraw3.asm
.586P
.MMX
.K3D

;;; include listing.inc
INCLUDE fxgasm.h

EXTRN   __GlideRoot:DWORD
EXTRN   __FifoMakeRoom:NEAR

CONST   SEGMENT
        ALIGN 8
_F256_F256      DQ    04380000043800000h ; 256 | 256
CONST   ENDS

_DATA   SEGMENT
        ALIGN   8
btab            DD    8 DUP(0)
atab            DD    8 DUP(0)
vSize           DD    0
strideinbytes   DD    0
vertices        DD    0
_DATA    ENDS

_TEXT         SEGMENT PAGE PUBLIC USE32 'CODE'
              ASSUME DS: FLAT, SS: FLAT

_pktype   = 20
_type     = 24
_mode     = 28
_count    = 32
_pointers = 36

gc            TEXTEQU  <edi>             ; points to graphics context
fifo          TEXTEQU  <ecx>             ; points to next entry in fifo
dlp           TEXTEQU  <ebp>             ; points to dataList structure
vertexCount   TEXTEQU  <esi>             ; Current vertex counter in the packet
vertexPtr     TEXTEQU  <ebx>             ; Current vertex pointer (in deref mode)
vertex        TEXTEQU  <ebx>             ; Current vertex (in non-deref mode)
dlpStart      TEXTEQU  <edx>             ; Pointer to start of offset list

X TEXTEQU     <0>
Y TEXTEQU     <4>

                  ALIGN  32

    PUBLIC  __grDrawVertexList_3DNow_Window@20
__grDrawVertexList_3DNow_Window@20 PROC NEAR
; 132  : {

    push      edi                        ; save caller's register variable
    mov       gc, [__GlideRoot + curGC]  ; get current graphics context

    push      esi                        ; save caller's register variable
    mov       vertexCount, [esp+_count-8]; number of vertices in strip/fan

    push      ebx                        ; save caller's register variable
    push      ebp                        ; save frame pointer
    
    mov       vertexPtr, [esp+_pointers] ; get current vertex pointer (deref mode)
                                         ; get current vertex (non-deref mode)
    test      vertexCount, vertexCount   ; number of vertices <= 0 ?
    
    jle       strip_done                 ; yup, the strip/fan is done
  
;;;     vSize = gc->state.vData.vSize
;;;     if (stride == 0)
;;;       stride = gc->state.vData.vStride;

;; We can operate in one of two modes:
;;
;; 0. We are stepping through an array of vertices, in which case
;; the stridesize is equal to the size of the vertex data, and
;; always > 4, since vertex data must a least contain x,y (ie 8 bytes).
;; vertexPtr is pointing to the array of vertices.
;;
;; 1. We are stepping through an array of pointers to vertices
;; in which case the stride is 4 bytes and we need to dereference
;; the pointers to get at the vertex data. vertexPtr is pointing
;; to the array of pointers to vertices.

    mov       edx, [esp + _mode]         ; get mode (0 or 1)
    mov       eax, [gc + vertexSize]     ; size of vertex data in bytes

    test      edx, edx                   ; mode 0 (array of vertices) ?
    mov       edx, [gc + vertexStride]   ; get stride in DWORDs
    
    jnz       deref_mode                 ; nope, it's mode 1 (array of pointers to vertices)

    femms                                ; we'll use MMX; clear MMX/3DX state      

    shl       edx, 2                     ; stride in bytes
    mov       [strideinbytes], edx       ; save off stride (in bytes)

;;;     Draw the first (or possibly only) set.  This is necessary because
;;;     the packet is 3_BDDDDDD, and in the next set, the packet is 3_DDDDDD
;;;     We try to make tstrip code simple to read. We combine the original code
;;;     into a single loop by adding an extra packet type assignment at the end of the loop.
;;; 
;;;     if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {
;;;       while (count > 0) {
;;;         FxI32 k, vcount = count >= 15 ? 15 : count;
;;;         GR_SET_EXPECTED_SIZE(vcount * vSize, 1);
;;;         TRI_STRIP_BEGIN(type, vcount, vSize, pktype);


win_coords_loop_ND:

    sub       vertexCount, 15            ; vertexCount >= 15 ? CF=0 : CF=1
    mov       ecx, [gc + vertexSize]     ; bytes of data for each vertex 

    sbb       eax, eax                   ; vertexCount >= 15 ? 00000000:ffffffff

    and       vertexCount, eax           ; vertexCount >= 15 ? 0 : vertexcount-15
    add       vertexCount, 15            ; vertexcount >= 15 ? 15 :vertexcount

    imul      ecx, vertexCount           ; total amount of vertex data we'll send

    mov       eax, [gc + fifoRoom]       ; fifo space available
    add       ecx, 4                     ; add header size ==> total packet size

    cmp       eax, ecx                   ; fifo space avail >= packet size ?
    jge       win_strip_begin_ND         ; yup, start writing strip data

    push      @Line                      ; line number inside this function
    push      0h                         ; pointer to function name = NULL

    push      ecx                        ; fifo space needed
    call      __FifoMakeRoom             ; note: updates fifoPtr

    lea       esp, [esp+12]              ; remove 3 DWORD arguments from stack


win_strip_begin_ND:

;;;     Setup packet header
;;;
    mov       eax, vertexCount           ; number of vertices in strip/fan
    mov       edx, [esp + _type]         ; setup mode

    mov       fifo, [gc + fifoPtr]       ; get fifoPtr
    shl       edx, 22                    ; <27:22> = setup mode (kSetupStrip or kSetupFan)

    mov       ebp, [gc + cullStripHdr]   ; <2:0> = type
    shl       eax, 6                     ; <9:6> = vertex count (max 15)

    or        eax, edx                   ; setup mode and vertex count
    mov       edx, [esp + _pktype]       ; <5:3> = command (SSTCP_PKT3_BDDBDD, SSTCP_PKT3_BDDDDD, or SSTCP_PKT3_DDDDDD)

    or        eax, ebp                   ; setup mode, vertex count, and type
    mov       ebp, 4                     ; test bit 2

    or        eax, edx                   ; setup mode, vertex count, type, and command
    lea       dlpStart, [gc+tsuDataList] ; pointer to start of offset list

    test      fifo, ebp                  ; fifoPtr QWORD aligned ?
    jz        fifo_aligned_ND            ; yup

    mov       [fifo], eax                ; PCI write packet type
    add       fifo, 4                    ; fifo pointer now QWORD aligned

;;;     for (k = 0; k < vcount; k++) {
;;;       FxI32 i;
;;;       FxU32 dataElem;
;;;       float *vPtr;
;;;       vPtr = pointers;
;;;       if (mode)
;;;         vPtr = *(float **)vPtr
;;;       (float *)pointers += stride;
;;;       TRI_SETF(FARRAY(vPtr, 0));
;;;       dataElem = 0;
;;;       TRI_SETF(FARRAY(vPtr, 4));
;;;       i = gc->tsuDataList[dataElem];

win_vertex_loop_ND_WB0:                  ; nothing in "write buffer"

    mov       eax, [dlpStart]            ; get first offset from offset list
    lea       dlp, [dlpStart+4]          ; point to start of offset list

    movq      mm1, [vertex+X]            ; get vertex x,y
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxU32)

    nop                                  ; filler
    test      eax, eax                   ; if offset == 0, end of list

    movq      [fifo-8], mm1              ; PCI write x, y
    jz        win_datalist_end_ND_WB0    ; no more vertex data, nothing in "write buffer" 

;;;       while (i != GR_DLIST_END) {
;;;         TRI_SETF(FARRAY(vPtr, i));
;;;         dataElem++;
;;;         i = gc->tsuDataList[dataElem];
;;;       }

win_datalist_loop_ND_WB0:                ; nothing in "write buffer"

    movd      mm1, [vertex + eax]        ; get next parameter
    mov       eax, [dlp]                 ; get next offset from offset list

    test      eax, eax                   ; at end of offset list (offset == 0) ?
    jz        win_datalist_end_ND_WB1    ; exit, write buffer contains one DWORD

    movd      mm2, [vertex + eax]        ; get next parameter
    add       dlp, 8                     ; dlp++

    mov       eax, [dlp-4]               ; get next offset from offset list
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxU32)

    test      eax, eax                   ; at end of offset list (offset == 0) ?
    punpckldq mm1, mm2                   ; current param | previous param

    movq      [fifo-8], mm1              ; PCI write current param | previous param
    jnz       win_datalist_loop_ND_WB0   ; nope, copy next parameter

win_datalist_end_ND_WB0:

    mov       eax, [strideinbytes]       ; get offset to next vertex
    sub       vertexCount, 1             ; another vertex done. Any left?

    lea       vertex, [vertex + eax]     ; points to next vertex
    jnz       win_vertex_loop_ND_WB0     ; yup, output next vertex

win_vertex_end_ND_WB0:

;;;       TRI_END;
;;;     Prepare for the next packet (if the strip size is longer than 15)
;;;       GR_CHECK_SIZE();
;;;       count -= 15;
;;;       pktype = SSTCP_PKT3_DDDDDD;
;;;     }
  
    mov       ebp, 16                    ; pktype = SSTCP_PKT3_DDDDDD (strip continuation)
    mov       eax, [gc + fifoPtr]        ; old fifoPtr

    mov       [esp + _pktype], ebp       ; pktype = SSTCP_PKT3_DDDDDD (strip continuation)
    mov       ebp, [gc + fifoRoom]       ; old number of bytes available in fifo

    mov       vertexCount, [esp + _count]; remaining vertices before previous loop
    sub       eax, fifo                  ; old fifoPtr - new fifoPtr (fifo room used)

    mov       [gc + fifoPtr], fifo       ; save current fifoPtr 
    add       ebp, eax                   ; new number of bytes available in fifo

    sub       vertexCount, 15            ; remaining number of vertices to process
    mov       [gc + fifoRoom], ebp       ; save current number of bytes available in fifo

    mov       [esp + _count], vertexCount; remaining number of vertices to process 
    test      vertexCount, vertexCount   ; any vertices left to process ?

    nop                                  ; filler
    jg        win_coords_loop_ND         ; loop if number of vertices to process >= 0

    femms                                ; no more MMX code; clear MMX/FPU state

    pop       ebp                        ; restore frame pointer
    pop       ebx                        ; restore caller's register variable

    pop       esi                        ; restore caller's register variable
    pop       edi                        ; restire caller's register variable

    ret       20                         ; return, pop 5 DWORD parameters off stack


fifo_aligned_ND:

    movd      mm1, eax                   ; move header into "write buffer"

win_vertex_loop_ND_WB1:                  ; one DWORD in "write buffer"

    movd      mm2, [vertex + X]          ; 0 | x of vertex
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxU32)

    lea       dlp, [dlpStart + 4]        ; point to start of offset list
    nop                                  ; filler

    punpckldq mm1, mm2                   ; packet header | x of vertex
    mov       eax, [dlp-4]               ; first offset in offset list

    movq      [fifo-8], mm1              ; PCI write packet header | x of vertex
    movd      mm1, [vertex + Y]          ; 0 | y of vertex

    cmp       eax, 0                     ; offset == 0 (list empty) ?
    jz        win_datalist_end_ND_WB1    ; yup, no more vertex data, one DWORD in "write buffer"

;;;       while (i != GR_DLIST_END) {
;;;         TRI_SETF(FARRAY(vPtr, i));
;;;         dataElem++;
;;;         i = gc->tsuDataList[dataElem];
;;;       }

win_datalist_loop_ND_WB1:                ; one DWORD in "write buffer" 

    movd      mm2, [vertex + eax]        ; get next parameter
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxU32)

    mov       eax, [dlp]                 ; get next offset from offset list
    add       dlp, 8                     ; dlp += 2

    punpckldq mm1, mm2                   ; current param | previous param
    cmp       eax, 0                     ; at end of offset list (offset == 0) ?

    movq      [fifo-8], mm1              ; PCI write current param | previous param
    jz        win_datalist_end_ND_WB0    ; yes, exit, "write buffer" empty

    movd      mm1, [vertex+eax]          ; get next parameter
    mov       eax, [dlp-4]               ; get next offset from offset list

    test      eax, eax                   ; at end of offset list (offset == 0) ?
    jnz       win_datalist_loop_ND_WB1   ; nope, copy next parameter

win_datalist_end_ND_WB1:

    mov       eax, [strideinbytes]       ; get offset to next vertex
    sub       vertexCount, 1             ; another vertex done. Any left?

    lea       vertex, [vertex + eax]     ; points to next vertex
    jnz       win_vertex_loop_ND_WB1     ; yup, output next vertex

win_vertex_end_ND_WB1:

    movd      [fifo], mm1                ; flush "write buffer"
    add       fifo, 4                    ; fifoPtr += sizeof(FxU32)

;;;       TRI_END;
;;;     Prepare for the next packet (if the strip size is longer than 15)
;;;       GR_CHECK_SIZE();
;;;       count -= 15;
;;;       pktype = SSTCP_PKT3_DDDDDD;
;;;     }

    mov       ebp, 16                    ; pktype = SSTCP_PKT3_DDDDDD (strip continuation)
    mov       eax, [gc + fifoPtr]        ; old fifoPtr

    mov       [esp+_pktype], ebp         ; pktype = SSTCP_PKT3_DDDDDD (strip continuation)
    mov       ebp, [gc + fifoRoom]       ; old number of bytes available in fifo

    mov       vertexCount, [esp + _count]; remaining vertices before previous loop
    sub       eax, fifo                  ; old fifoPtr - new fifoPtr (fifo room used)

    mov       [gc + fifoPtr], fifo       ; save current fifoPtr 
    add       ebp, eax                   ; new number of bytes available in fifo

    mov       [gc + fifoRoom], ebp       ; save current number of bytes available in fifo
    sub       vertexCount, 15            ; remaining number of vertices to process

    mov       [esp + _count], vertexCount; remaining number of vertices to process 
    test      vertexCount, vertexCount   ; any vertices left to process ?

    nop                                  ; filler
    jg        win_coords_loop_ND         ; loop if number of vertices to process >= 0

    femms                                ; no more MMX code; clear MMX/FPU state

    pop       ebp                        ; restore frame pointer
    pop       ebx                        ; restore caller's register variable

    pop       esi                        ; restore caller's register variable
    pop       edi                        ; restire caller's register variable

    ret       20                         ; return, pop 5 DWORD parameters off stack

    db        08dh,064h,024h,000h        ; filler (not reachable)
    
deref_mode:

    femms                                ; we'll use MMX; clear FPU/MMX state

    prefetch  [vertexPtr]                ; pre-load first group of pointers

win_coords_loop_D:

    sub       vertexCount, 15            ; vertexCount >= 15 ? CF=0 : CF=1
    mov       ecx, [gc + vertexSize]     ; bytes of data for each vertex 

    sbb       eax, eax                   ; vertexCount >= 15 ? 00000000:ffffffff

    and       vertexCount, eax           ; vertexCount >= 15 ? 0 : vertexcount-15
    add       vertexCount, 15            ; vertexcount >= 15 ? 15 :vertexcount

    imul      ecx, vertexCount           ; total amount of vertex data we'll send

    mov       eax, [gc + fifoRoom]       ; fifo space available
    add       ecx, 4                     ; add header size ==> total packet size

    cmp       eax, ecx                   ; fifo space avail >= packet size ?
    jge       win_strip_begin_D          ; yup, start writing strip data

    push      @Line                      ; line number inside this function
    push      0h                         ; pointer to function name = NULL

    push      ecx                        ; fifo space needed
    call      __FifoMakeRoom             ; note: updates fifoPtr

    add       esp, 12                    ; remove 3 DWORD arguments from stack
    nop                                  ; filler

win_strip_begin_D:

;;;     Setup packet header
;;;
    mov       eax, vertexCount           ; number of vertices in strip/fan
    mov       edx, [esp + _type]         ; setup mode

    mov       fifo, [gc + fifoPtr]       ; get fifoPtr
    shl       edx, 22                    ; <27:22> = setup mode (kSetupStrip or kSetupFan)

    mov       ebp, [gc + cullStripHdr]   ; <2:0> = type
    shl       eax, 6                     ; <9:6> = vertex count (max 15)

    or        eax, edx                   ; setup mode and vertex count
    mov       edx, [esp + _pktype]       ; <5:3> = command (SSTCP_PKT3_BDDBDD, SSTCP_PKT3_BDDDDD, or SSTCP_PKT3_DDDDDD)

    or        eax, ebp                   ; setup mode, vertex count, and type
    mov       ebp, 4                     ; test bit 2

    or        eax, edx                   ; setup mode, vertex count, type, and command
    lea       dlpStart, [gc+tsuDataList] ; pointer to start of offset list

    test      fifo, ebp                  ; fifoPtr QWORD aligned ?
    jz        fifo_aligned_D             ; yup

    mov       [fifo], eax                ; PCI write packet type
    add       fifo, 4                    ; fifo pointer now QWORD aligned

;;;     for (k = 0; k < vcount; k++) {
;;;       FxI32 i;
;;;       FxU32 dataElem;
;;;       float *vPtr;
;;;       vPtr = pointers;
;;;       if (mode)
;;;         vPtr = *(float **)vPtr
;;;       (float *)pointers += stride;
;;;       TRI_SETF(FARRAY(vPtr, 0));
;;;       dataElem = 0;
;;;       TRI_SETF(FARRAY(vPtr, 4));
;;;       i = gc->tsuDataList[dataElem];


win_vertex_loop_D_WB0:                   ; nothing in "write buffer"

    mov       edx, [vertexPtr]           ; dereference pointer, edx points to vertex
    add       vertexPtr, 4               ; next pointer

    lea       dlp, [gc + tsuDataList]    ; get pointer to offset list; dlp ++
    add       dlp, 4                     ; dlp ++

    movq      mm1, [edx + X]             ; get vertex x,y
    add       fifo, 8                    ; fifo += 2

    mov       eax, [dlp - 4]             ; get first offset from offset list
    movq      [fifo-8], mm1              ; PCI write x, y

    cmp       eax, 0                     ; if offset == 0, end of offset list
    je        win_datalist_end_D_WB0     ; no more vertex data, nothing in "write buffer" 

;;;       while (i != GR_DLIST_END) {
;;;         TRI_SETF(FARRAY(vPtr, i));
;;;         dataElem++;
;;;         i = gc->tsuDataList[dataElem];
;;;       }

win_datalist_loop_D_WB0:                 ; nothing in "write buffer"

    movd      mm1, [edx + eax]           ; get next parameter
    mov       eax, [dlp]                 ; get next offset from offset list

    cmp       eax, 0                     ; at end of offset list (offset == 0) ?
    jz        win_datalist_end_D_WB1     ; exit, write buffer contains one DWORD

    add       dlp, 8                     ; dlp++
    movd      mm2, [edx + eax]           ; get next parameter

    mov       eax, [dlp-4]               ; get next offset from offset list
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxU32)

    punpckldq mm1, mm2                   ; current param | previous param
    test      eax, eax                   ; at end of offset list (offset == 0) ?

    movq      [fifo-8], mm1              ; PCI write current param | previous param
    jnz       win_datalist_loop_D_WB0    ; nope, copy next parameter

win_datalist_end_D_WB0:

    dec       vertexCount                ; another vertex done. Any left?
    jnz       win_vertex_loop_D_WB0      ; yup, output next vertex

win_vertex_end_D_WB0:

;;;       TRI_END;
;;;     Prepare for the next packet (if the strip size is longer than 15)
;;;       GR_CHECK_SIZE();
;;;       count -= 15;
;;;       pktype = SSTCP_PKT3_DDDDDD;
;;;     }

    mov       ebp, 16                    ; pktype = SSTCP_PKT3_DDDDDD (strip continuation)
    mov       eax, [gc + fifoPtr]        ; old fifoPtr

    mov       [esp + _pktype], ebp       ; pktype = SSTCP_PKT3_DDDDDD (strip continuation)
    mov       ebp, [gc + fifoRoom]       ; old number of bytes available in fifo

    mov       vertexCount, [esp + _count]; remaining vertices before previous loop
    sub       eax, fifo                  ; old fifoPtr - new fifoPtr (fifo room used)

    add       ebp, eax                   ; new number of bytes available in fifo
    mov       [gc + fifoPtr], fifo       ; save current fifoPtr 

    sub       vertexCount, 15            ; remaining number of vertices to process
    mov       [gc + fifoRoom], ebp       ; save current number of bytes available in fifo

    mov       [esp + _count], vertexCount; remaining number of vertices to process 
    test      vertexCount, vertexCount   ; any vertices left to process ?

    nop                                  ; filler
    jg        win_coords_loop_D          ; loop if number of vertices to process >= 0

    femms                                ; no more MMX code; clear MMX/FPU state

    pop       ebp                        ; restore frame pointer
    pop       ebx                        ; restore caller's register variable

    pop       esi                        ; restore caller's register variable
    pop       edi                        ; restire caller's register variable

    ret       20                         ; return, pop 5 DWORD parameters off stack
    mov       esp, esp                   ; filler (unreachable)

fifo_aligned_D:

    movd      mm1, eax                   ; move header into "write buffer"

win_vertex_loop_D_WB1:                   ; one DWORD in "write buffer"

    mov       edx, [vertexPtr]           ; dereference pointer, edx points to vertex
    add       vertexPtr, 4               ; next pointer

    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxU32)
    lea       dlp, [gc + tsuDataList]    ; get pointer to start of offset list

    movd      mm2, [edx + X]             ; 0 | x of vertex
    add       dlp, 4                     ; dlp++
 
    mov       eax, [dlp-4]               ; first offset in offset list
    punpckldq mm1, mm2                   ; packet header | x of vertex

    movq      [fifo-8], mm1              ; PCI write packet header | x of vertex
    movd      mm1, [edx + Y]             ; 0 | y of vertex

    cmp       eax, 0                     ; offset == 0 (list empty) ?
    je        win_datalist_end_D_WB1     ; yup, no more vertex data, one DWORD in "write buffer"

;;;       while (i != GR_DLIST_END) {
;;;         TRI_SETF(FARRAY(vPtr, i));
;;;         dataElem++;
;;;         i = gc->tsuDataList[dataElem];
;;;       }

win_datalist_loop_D_WB1:                ; one DWORD in "write buffer" = MM1

    movd      mm2, [edx + eax]          ; get next parameter
    add       fifo, 8                   ; fifoPtr += 2*sizeof(FxU32)

    mov       eax, [dlp]                ; get next offset from offset list
    add       dlp, 8                    ; dlp += 2

    punpckldq mm1, mm2                  ; current param | previous param
    test      eax, eax                  ; at end of offset list (offset == 0) ?

    movq      [fifo-8], mm1             ; PCI write current param | previous param
    jz        win_datalist_end_D_WB0    ; yes, exit, "write buffer" empty

    movd      mm1, [edx + eax]          ; get next parameter
    mov       eax, [dlp-4]              ; get next offset from offset list

    test      eax, eax                  ; at end of offset list (offset == 0) ?
    jnz       win_datalist_loop_D_WB1   ; nope, copy next parameter

win_datalist_end_D_WB1:

    dec       vertexCount               ; another vertex done. Any left?
    jnz       win_vertex_loop_D_WB1     ; yup, output next vertex

win_vertex_end_D_WB1:

    movd      [fifo], mm1               ; flush "write buffer"
    add       fifo, 4                   ; fifoPtr++

;;;       TRI_END;
;;;     Prepare for the next packet (if the strip size is longer than 15)
;;;       GR_CHECK_SIZE();
;;;       count -= 15;
;;;       pktype = SSTCP_PKT3_DDDDDD;
;;;     }

    mov       ebp, 16                    ; pktype = SSTCP_PKT3_DDDDDD (strip continuation)
    mov       eax, [gc + fifoPtr]        ; old fifoPtr

    mov       [esp+_pktype], ebp         ; pktype = SSTCP_PKT3_DDDDDD (strip continuation)
    mov       ebp, [gc + fifoRoom]       ; old number of bytes available in fifo

    mov       vertexCount, [esp + _count]; remaining vertices before previous loop
    nop                                  ; filler

    sub       eax, fifo                  ; old fifoPtr - new fifoPtr (fifo room used)
    mov       [gc + fifoPtr], fifo       ; save current fifoPtr 

    sub       vertexCount, 15            ; remaining number of vertices to process
    add       ebp, eax                   ; new number of bytes available in fifo

    mov       [gc + fifoRoom], ebp       ; save current number of bytes available in fifo
    cmp       vertexCount, 0             ; any vertices left to process ?

    mov       [esp + _count], vertexCount; remaining number of vertices to process 
    jg        win_coords_loop_D          ; loop if number of vertices to process >= 0

    femms                                ; no more MMX code; clear MMX/FPU state

strip_done: 
    pop       ebp                        ; restore frame pointer
    pop       ebx                        ; restore caller's register variable

    pop       esi                        ; restore caller's register variable
    pop       edi                        ; restire caller's register variable

    ret       20                         ; return, pop 5 DWORD parameters off stack

__grDrawVertexList_3DNow_Window@20 ENDP

    PUBLIC  __grDrawVertexList_3DNow_Clip@20
__grDrawVertexList_3DNow_Clip@20 PROC NEAR
; 132  : {

    push      edi                        ; save caller's register variable
    mov       gc, [__GlideRoot + curGC]  ; get current graphics context

    push      esi                        ; save caller's register variable
    mov       vertexCount, [esp+_count-8]; number of vertices in strip/fan

    push      ebx                        ; save caller's register variable
    push      ebp                        ; save frame pointer
    
    mov       vertexPtr, [esp+_pointers] ; get current vertex pointer (deref mode)
                                         ; get current vertex (non-deref mode)
    test      vertexCount, vertexCount   ; number of vertices <= 0 ?
    
    jle       strip_done                 ; yup, the strip/fan is done
  
;;;     vSize = gc->state.vData.vSize
;;;     if (stride == 0)
;;;       stride = gc->state.vData.vStride;

;; We can operate in one of two modes:
;;
;; 0. We are stepping through an array of vertices, in which case
;; the stridesize is equal to the size of the vertex data, and
;; always > 4, since vertex data must a least contain x,y (ie 8 bytes).
;; vertexPtr is pointing to the array of vertices.
;;
;; 1. We are stepping through an array of pointers to vertices
;; in which case the stride is 4 bytes and we need to dereference
;; the pointers to get at the vertex data. vertexPtr is pointing
;; to the array of pointers to vertices.

    mov       edx, [esp + _mode]         ; get mode (0 or 1)
    mov       eax, [gc + vertexSize]     ; size of vertex data in bytes

    test      edx, edx                   ; mode 0 (array of vertices) ?
    mov       edx, [gc + vertexStride]   ; get stride in DWORDs

    movd      mm6, [__GlideRoot+pool_f255]; GlideRoot.pool.f255     
    mov       [strideinbytes], 4         ; array of pointers    
        
    jnz       clip_coords_begin          ; nope, it's mode 1

clip_coordinates_ND:

    shl       edx, 2                     ; stride in bytes
    mov       [strideinbytes], edx       ; save off stride (in bytes)

    align   32
clip_coords_begin:

dataElem      textequ <ebp>              ; number of vertex components processed    

;;;   {
;;;     float oow;
;;;       while (count > 0) {
;;;       FxI32 k, vcount = count >= 15 ? 15 : count;

    sub       vertexCount, 15            ; vertexCount >= 15 ? CF=0 : CF=1
    mov       ecx, [gc + vertexSize]     ; bytes of data for each vertex 

    sbb       eax, eax                   ; vertexCount >= 15 ? 00000000:ffffffff
    and       vertexCount, eax           ; vertexCount >= 15 ? 0 : vertexcount-15

    mov       eax, [gc+fifoRoom]         ; fifo space available
    add       vertexCount, 15            ; vertexcount >= 15 ? 15 :vertexcount

    imul      ecx, vertexCount           ; total amount of vertex data we'll send

    add       ecx, 4                     ; add header size ==> total packet size
    nop                                  ; filler

    cmp       eax, ecx                   ; fifo space avail >= packet size ?
    jge       clip_strip_begin           ; yup, start writing strip data

    push      @Line                      ; line number inside this function
    push      0h                         ; pointer to function name = NULL

    push      ecx                        ; fifo space needed
    call      __FifoMakeRoom             ; note: updates fifoPtr

    add       esp, 12                    ; remove 3 DWORD arguments from stack

clip_strip_begin:

;;;     TRI_STRIP_BEGIN(type, vcount, vSize, pktype)

    mov       edx, [esp + _type]         ; setup mode
    mov       eax, vertexCount           ; number of vertices in strip/fan

    mov       fifo, [gc + fifoPtr]       ; get fifoPtr
    shl       eax, 6                     ; <9:6> = vertex count (max 15)

    mov       ebp, [gc + cullStripHdr]   ; <2:0> = type
    shl       edx, 22                    ; <27:22> = setup mode (kSetupStrip or kSetupFan)

    or        eax, edx                   ; setup mode and vertex count
    mov       edx, [esp + _pktype]       ; <5:3> = command (SSTCP_PKT3_BDDBDD, SSTCP_PKT3_BDDDDD, or SSTCP_PKT3_DDDDDD)

    or        eax, ebp                   ; setup mode, vertex count, and type
    add       fifo, 4                    ; fifoPtr += sizeof(FxU32)

    or        eax, edx                   ; setup mode, vertex count, type, and command
    mov       [fifo-4], eax              ; PCI write header

;;;     for (k = 0; k < vcount; k++) {
;;;       float *vPtr
;;;       vPtr = pointers
  
clip_for_begin:

;;;       if (mode)
;;;         vPtr = *(float **)vPtr

    mov       edx, vertexPtr             ; vertex = vertexPtr (assume no-deref mode)
    mov       eax, [esp+_mode]           ; mode 0 = no deref, mode 1 = deref

    mov       [vertices], vertexCount    ; save numnber of vertices
    test      eax, eax                   ; deref mode ?

    mov       eax, [gc+wInfo_offset]     ; get offset of W into vertex struct
    jz        clip_noderef               ; yup, no-deref mode

    mov       edx, [vertexPtr]           ; vertex = *vertexPtr
    lea       esp, [esp]                 ; filler

clip_noderef:

;;;       oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset)

    movd      mm0, [edx+eax]             ; 0 | W of current vertex
    pfrcp     mm1, mm0                   ; 0 | 1/W approx

    mov       ebp, [strideinbytes]       ; offset to next vertex/vertexPtr
    movq      mm2, [edx]                 ; y | x of current vertex

    pfrcpit1  mm0, mm1                   ; 0 | 1/W refine
    movq      mm3, [gc+vp_hwidth]        ; gc->state.Viewport.hheight | gc->state.Viewport.hwidth

    movq      mm4, [gc+vp_ox]            ; gc->state.Viewport.oy | gc->state.Viewport.ox
    add       vertexPtr, ebp             ; point to next vertex/VertexPtr

    pfrcpit2  mm0, mm1                   ; oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset
    mov       esi, [gc+paramIndex]       ; gc->state.paramIndex

;;;       /* x, y */
;;;       TRI_SETF(FARRAY(vPtr, 0)
;;;         *oow*gc->state.Viewport.hwidth + gc->state.Viewport.ox)
;;;       TRI_SETF(FARRAY(vPtr, 4)
;;;         *oow*gc->state.Viewport.hheight + gc->state.Viewport.oy)

    pfmul     mm2, mm3                   ; TRI_SETF(FARRAY(vPtr,0)*state.Viewport.hheight | TRI_SETF(FARRAY(vPtr,4)*state.Viewport.hwidth
    xor       dataElem, dataElem         ; dataElem = 0

    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)
    punpckldq mm0, mm0                   ; oow | oow

    pfmul     mm2, mm0                   ; TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.height | TRI_SETF(FARRAY(vPtr, 0)*oow*gc->state.Viewport.hwidth
    pfadd     mm2, mm4                   ; TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.hheight + gc->state.Viewport.oy) |

    test      esi, 3                     ; STATE_REQUIRES_IT_DRGB | STATE_REQUIRES_IT_ALPHA ?
    mov       eax, [gc+tsuDataList]      ; first entry from offset list

;;;       (float *)pointers += stride
;;;       TRI_VP_SETFS(vPtr, oow);

    movq      [fifo-8], mm2              ; PCI write transformed x, y
    jz        clip_setup_ooz             ; nope, no color at all needed
  
    cmp       DWORD PTR [gc+colorType], 0; gc->state.vData.colorType == GR_FLOAT ?
    jne       clip_setup_pargb           ; nope, packed ARGB format
  
    test      esi, 1                     ; STATE_REQUIRES_IT_DRGB ?
    jz        clip_setup_a               ; no, but definitely A

    movd      mm2, [edx + eax]           ; 0 | r
    mov       eax, [gc+tsuDataList+4]    ; offset of g part of vertex data

    pfmul     mm2, mm6                   ; 0 | r * 255.0f
    movd      mm3, [edx + eax]           ; 0 | g

    mov       eax, [gc+tsuDataList+8]    ; offset of b part of vertex data
    movd      [fifo], mm2                ; PCI write r*255

    pfmul     mm3, mm6                   ; 0 | g * 255.0f
    movd      mm2, [edx + eax]           ; 0 | b

    movd      [fifo+4], mm3              ; PCI write g*255
    mov       dataElem, 12               ; dataElem = 3

    pfmul     mm2, mm6                   ; 0 | b * 255.0f
    mov       eax, [gc+tsuDataList+12]   ; offset of A part of vertex data

    test      esi, 2                     ; STATE_REQUIRES_IT_ALPHA ?
    lea       fifo, [fifo+12]            ; fifoPtr += 3*sizeof(FxFloat)

    movd      [fifo-4], mm2              ; PCI write b*255
    jz        clip_setup_ooz             ; nope, no alpha, proceeed with ooz

clip_setup_a:
    movd      mm2, [eax+edx]             ; 0 | a
    add       fifo, 4                    ; fifoPtr += sizeof(FxFloat)

    mov       esp, esp                   ; filler
    add       dataElem, 4                ; dataElem++ 

    pfmul     mm2, mm6                   ; 0 | a * 255.0f
    mov       eax, [gc+dataElem+tsuDataList]; offset of next part of vertex data

    movd      [fifo-4], mm2              ; PCI write a*255
    jmp       clip_setup_ooz             ; check whether we need to push out z

    ALIGN     32

clip_setup_pargb:
    movd      mm2, [eax+edx]             ; get packed ARGB data
    add       fifo, 4                    ; fifoPtr += sizeof(FxU32)

    mov       dataElem, 4                ; dataElem = 1 (namely pargb)
    mov       eax, [gc+tsuDataList+4]    ; offset of next part of vertex data

    movd      [fifo-4], mm2              ; PCI write packed ARGB

clip_setup_ooz:
  
    test      esi, 4                     ; STATE_REQUIRES_OOZ ?
    jz        clip_setup_qow             ; nope

    movd      mm2, [eax+edx]             ; 0 | z component of vertex
    add       fifo, 4                    ; fifoPtr += sizeof(FxFloat)

    add       dataElem, 4                ; dataElem += 1
    movd      mm3, [gc+vp_hdepth]        ; 0 | gc->state.Viewport.hdepth

    pfmul     mm2, mm0                   ; TRI_SETF(FARRAY(_s, i)*_oow
    movd      mm4, [gc+vp_oz]            ; 0 | gc->state.Viewport.oz

    pfmul     mm2, mm3                   ; 0 | TRI_SETF(FARRAY(_s, i)*_oow*gc->state.Viewport.hdepth
    mov       eax, [gc+dataElem+tsuDataList]; offset of next vertex component

    pfadd     mm2, mm4                   ; 0 | TRI_SETF(FARRAY(_s, i)*_oow*gc->state.Viewport.hdepth+gc->state.Viewport.oz
    movd      [fifo-4], mm2              ; PCI write transformed Z

clip_setup_qow:
    test      esi, 8                     ; STATE_REQUIRES_OOW_FBI ?
    jz        clip_setup_qow0            ; nope

    cmp       DWORD PTR [gc+qInfo_mode],0; does vertex have Q component ?
    je        clip_setup_oow             ; nope, not Q but W

    add       fifo, 4                    ; fifoPtr += sizeof(FxFloat)
    mov       eax, [gc+qInfo_offset]     ; offset of Q component of vertex

    add       dataElem, 4                ; dataElem++
    movd      mm2, [edx+eax]             ; 0 | q of vertex

    mov       eax,[gc+dataElem+tsuDataList]; pointer to next vertex component
    pfmul     mm2, mm0                   ; q*oow

    movd      [fifo-4], mm2              ; PCI write transformed Q
    jmp       clip_setup_qow0            ; continue with q0

    ALIGN     32

clip_setup_oow:
    add       fifo, 4                    ; fifoPtr += sizeof(FxFloat) 
    add       dataElem, 4                ; dataElem++

    movd      [fifo-4], mm0              ; PCI write oow
    mov       eax,[gc+dataElem+tsuDataList]; pointer to next vertex component

clip_setup_qow0:
    test      esi, 16                    ; STATE_REQUIRES_W_TMU0 ?
    jz        clip_setup_stow0           ; nope 

    cmp       DWORD PTR [gc+q0Info_mode],0; does vertex have Q component ?
    je        clip_setup_oow0            ; nope, not Q but W

    mov       eax, [gc+q0Info_offset]    ; offset of Q component of vertex
    add       fifo, 4                    ; fifoPtr += sizeof(FxFloat)

    add       dataElem, 4                ; dataElem++
    movd      mm2, [edx+eax]             ; 0 | q0 of vertex

    mov       eax,[gc+dataElem+tsuDataList]; pointer to next vertex component
    pfmul     mm2, mm0                   ; q0*oow

    movd      [fifo-4], mm2              ; PCI write transformed q0
    jmp       clip_setup_stow0           ; continue with stow0

    ALIGN     32

clip_setup_oow0:
    add       fifo, 4                    ; fifoPtr += sizeof(FxFloat) 
    add       dataElem, 4                ; dataElem++

    movd      [fifo-4], mm0              ; PCI write oow
    mov       eax,[gc+dataElem+tsuDataList]; pointer to next vertex component

clip_setup_stow0:

    test      esi, 32                    ; STATE_REQUIRES_ST_TMU0 ?
    jz        clip_setup_qow1            ; nope

    movq      mm7, [gc + tmu0_s_scale]   ; state.tmu_config[0].t_scale | state.tmu_config[0].s_scale
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    movd      mm2, [edx+eax]             ; param1
    mov       eax,[gc+dataElem+tsuDataList+4];pointer to next vertex component

    pfmul     mm7, mm0                   ; oow*tmu0_t_scale | oow*tmu0_s_scale
    add       dataElem, 8                ; dataElem += 2

    movd      mm3, [edx+eax]             ; param2
    punpckldq mm2, mm3                   ; param2 | param1

    pfmul     mm2, mm7                   ; param2*oow*tmu0_t_scale | param1*oow*tmu0_s_scale

    movq      [fifo-8], mm2              ; PCI write param2*oow*tmu0_t_scale | param1*oow*tmu0_s_scale 
    mov       eax, [gc+dataElem+tsuDataList]; pointer to next vertex component

clip_setup_qow1:
    test      esi, 64                    ; STATE_REQUIRES_W_TMU1 ?
    jz        clip_setup_stow1           ; nope

    cmp       DWORD PTR [gc+q1Info_mode],0; does vertex have Q component ?
    je        clip_setup_oow1            ; nope, not Q but W

    mov       eax, [gc+q1Info_offset]    ; offset of Q component of vertex
    add       fifo, 4                    ; fifoPtr += sizeof(FxFloat)

    add       dataElem, 4                ; dataElem++
    movd      mm2, [edx+eax]             ; 0 | q1 of vertex

    mov       eax,[gc+dataElem+tsuDataList]; pointer to next vertex component
    pfmul     mm2, mm0                   ; q1*oow

    movd      [fifo-4], mm2              ; PCI write transformed q1
    jmp       clip_setup_stow1           ; continue with stow1

    ALIGN     32

clip_setup_oow1:
    add       fifo, 4                    ; fifoPtr += sizeof(FxFloat) 
    add       dataElem, 4                ; dataElem++

    movd      [fifo-4], mm0              ; PCI write oow
    mov       eax,[gc+dataElem+tsuDataList]; pointer to next vertex component

clip_setup_stow1:

    test      esi, 128                   ; STATE_REQUIRES_ST_TMU1 ?
    mov       vertexCount, [vertices]    ; get number of vertices

    movq      mm7, [gc + tmu1_s_scale]   ; state.tmu_config[1].t_scale | state.tmu_config[1].s_scale
    jz        clip_setup_end             ; nope

    movd      mm2, [edx+eax]             ; param1
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    mov       eax,[gc+dataElem+tsuDataList+4]; pointer to next vertex component
    pfmul     mm7, mm0                   ; oow*state.tmu_config[1].t_scale | oow*state.tmu_config[1].s_scale

    movd      mm3, [edx+eax]             ; param2
    punpckldq mm2, mm3                   ; param2 | param1

    pfmul     mm2, mm7                   ; param2*oow*state.tmu_config[1].t_scale | param1*oow*state.tmu_config[1].s_scale
    movq      [fifo-8], mm2              ; PCI write param2*oow*state.tmu_config[1].t_scale | param1*oow*state.tmu_config[1].s_scale

clip_setup_end:

; 206  :       for (k = 0; k < vcount; k++) {

    dec       vertexCount                ; vcount--
    jnz       clip_for_begin             ; until 
clip_for_end:

; 221  :       }
; 222  :       TRI_END;

    mov       eax, [gc + fifoPtr]        ; old fifoPtr
    mov       esp, esp                   ; filler

    nop                                  ; filler
    mov       ebp, [gc + fifoRoom]       ; old number of bytes available in fifo

    mov       vertexCount, [esp + _count]; remaining vertices before previous loop
    sub       eax, fifo                  ; old fifoPtr - new fifoPtr (fifo room used)

    mov       [gc + fifoPtr], fifo       ; save current fifoPtr 
    add       ebp, eax                   ; new number of bytes available in fifo

    sub       vertexCount, 15            ; remaining number of vertices to process
    mov       [gc + fifoRoom], ebp       ; save current number of bytes available in fifo

    mov       [esp + _count], vertexCount; remaining number of vertices to process 
    cmp       vertexCount, 0             ; any vertices left to process ?

    mov       DWORD PTR [esp+_pktype], 16; pktype = SSTCP_PKT3_DDDDDD (strip continuation)
    jg        clip_coords_begin          ; loop if number of vertices to process >= 0

    femms                                ; no more MMX code; clear MMX/FPU state    

strip_done:
;;;    }
;;;  #undef FN_NAME
;;;  } /* _grDrawVertexList */

    pop       ebp                        ; restore frame pointer 
    pop       ebx                        ; restore caller's register variable

    pop       esi                        ; restore caller's register variable
    pop       edi                        ; restore caller's register variable

    ret       20                         ; return, pop 5 DWORD parameters off stack

__grDrawVertexList_3DNow_Clip@20 ENDP    

    ALIGN     32

_a$      TEXTEQU <20>
_b$      TEXTEQU <24>

gc       TEXTEQU <esi>
vb       TEXTEQU <edi>
va       TEXTEQU <ebx>
i        TEXTEQU <ebp>
j        TEXTEQU <edx>
dlp      TEXTEQU <edx>
fifo     TEXTEQU <ecx>
dlpStart TEXTEQU <ebp>
ADX      TEXTEQU <__GlideRoot+pool_fTemp1>
ADY      TEXTEQU <__GlideRoot+pool_fTemp2>

         PUBLIC  __grDrawTextureLine_3DNow@8
__grDrawTextureLine_3DNow@8 PROC NEAR

; 227  : {
; 228  : #define FN_NAME "grDrawTextureLine"
; 229  : 
; 230  :   GR_BEGIN_NOFIFOCHECK(FN_NAME, 91);
; 231  : 
; 232  : #define  DX _GlideRoot.pool.ftemp1
; 233  : #define ADY _GlideRoot.pool.ftemp2
; 234  : 
; 235  :   GR_FLUSH_STATE();

    push      esi                        ; save caller's register variable
    mov       gc, [__GlideRoot + curGC]  ; get current graphics context

    push      edi                        ; save caller's register variable
    mov       vb, _b$[esp-8]             ; b

    push      ebx                        ; save caller's register variable
    push      ebp                        ; save caller's frame pointer

    femms                                ; we'll use MMX; empty FPU/MMX state    
    mov       va, _a$[esp]               ; a

; 236  : 
; 237  :   {
; 238  :     FxI32 i, j;
; 239  :     FxU32 dataElem;
; 240  :     FxU32 vSize;
; 241  :     
; 242  :     /*
; 243  :     ** compute absolute deltas and draw from low Y to high Y
; 244  :     */
; 245  :     ADY = FARRAY(b, 4) - FARRAY(a, 4);
; 246  :     i = *(long *)&ADY;
; 247  :     if (i < 0) {
; 248  :       const void *tv;
; 249  :       tv = a; a = b; b = tv;
; 250  :       i ^= 0x80000000;            /* ady = -ady; */
; 251  :       /* (*(long *)&ADY) = i; */
; 252  :     }

    movq      mm1, [vb]                  ; FARRAY(b,4) | FARRAY(b,0)
    nop                                  ; filler

    movq      mm0, [va]                  ; FARRAY(a,4) | FARRAY(a,0)
    movq      mm6, mm1                   ; FARRAY(b,4) | FARRAY(b,0)

    pfsub     mm6, mm0                   ; FARRAY(b,4)-FARRAY(a,4) | FARRAY(b,0)-FARRAY(a,0)
    mov       eax, [gc+vertexSize]       ; size of vertex data (in bytes)

    movq      mm5, mm6                   ; FARRAY(b,4)-FARRAY(a,4) | FARRAY(b,0)-FARRAY(a,0)
    mov       ecx, [gc+fifoRoom]         ; fifo room available (in bytes)

    punpckhdq mm6, mm6                   ; FARRAY(b,4)-FARRAY(a,4) | FARRAY(b,4)-FARRAY(a,4)
    movd      [ADY], mm6                 ; ADY = FARRAY(b, 4) - FARRAY(a, 4)

    movd      [ADX], mm5                 ; DX = FARRAY(b, 0) - FARRAY(a, 0)
    mov       i, [ADY]                   ; i = *(long *)&ADY

    test      i, i                       ; i < 0 ?
    jge       $dont_swap_ij              ; nope, no need to swap i and j 

    xor       va, vb                     ; va ^ vb
    xor       vb, va                     ; vb ^ (va ^ vb) = va

    xor       va, vb                     ; (va ^ vb) ^ va = vb
    xor       i, 80000000h               ; i ^= 0x80000000

$dont_swap_ij:

; 253  :     
; 254  :     DX = FARRAY(b, 0) - FARRAY(a, 0);
; 255  :     j = *(long *)&DX;
; 256  :     if (j < 0) {
; 257  :       j ^= 0x80000000;            /* adx = -adx; */
; 258  :     }
; 259  :     
; 260  :     /* check for zero-length lines */
; 261  :     if ((j >= i) && (j == 0))
; 262  :       return;
; 263  :     

    mov       j, [ADX]                   ; j = *(long *)&DX
    and       j, 7fffffffh               ; j = abs(j)

    cmp       j, i                       ; j < i ?
    jl        $j_lt_i                    ; yup

    test      j, j                       ; j == 0 ?
    jz        $line_all_done             ; yup, nothing to draw

$j_lt_i:

; 264  :     vSize = gc->state.vData.vSize + 8;
; 265  :     GR_SET_EXPECTED_SIZE((vSize<< 2), 1);

    lea       eax, DWORD PTR [eax*4+36]  ; we have vertices + 4 bytes for header
    cmp       ecx, eax                   ; fifo room avail >= fifo room required ?

    jge       $enough_fifo_room          ; yup, sufficient fifo room
    push      j                          ; preserve j

    push      @Line                      ; line number in source file
    push      0                          ; pointer to filename = NULL

    push      eax                        ; fifo space required
    call      __FifoMakeRoom             ; allocate new fifo space (modified fifoPtr)

    add       esp, 12                    ; pop 3 DWORD parameters off stack
    pop       j                          ; restore j

$enough_fifo_room:

; 266  :     TRI_STRIP_BEGIN(kSetupCullDisable | kSetupStrip, 4, vSize, 
; 267  :                     SSTCP_PKT3_BDDDDD | (1<<15));
; 268  :     /* x major */
; 269  :     if (j >= i) {

    mov       fifo, [gc+fifoPtr]         ; gc->cmdTransportInfo.fifoPtr
    pxor      mm3, mm3                   ; 0 | 0

    movd      mm2, [__GlideRoot+pool_fHalf] ; 0 | _GlideRoot.pool.fHalf
    movq      mm4, [_F256_F256]          ; 256.0f | 256.0f

    test      fifo, 4                    ; fifo QWORD aligned ?
    jz        $drawline_fifo_aligned     ; yup

    add       fifo, 4                    ; fifoPtr += sizeof(FxU32)
    mov       eax, [gc+cullStripHdr]     ; gc->cmdTransportInfo.cullStripHdr

    psllq     mm2, 32                    ; _GlideRoot.pool.fHalf | 0
    punpckldq mm4, mm3                   ; 0 | 256.0f 

    or        eax, 8108h                 ; kSetupCullDisable|kSetupStrip|4<<6|SSTCP_PKT3_BDDDDD<<3|1<<15 
    cmp       j, i                       ; j < i ?

    mov       [fifo-4], eax              ; PCI write header (fifo now aligned)
    jl        $j_lt_i2_WB0               ; yup, j < i

; 270  :       TRI_SETF(FARRAY(b, 0));
; 271  :       dataElem = 0;
; 272  :       TRI_SETF(FARRAY(b, 4) - _GlideRoot.pool.fHalf);
; 273  :       i = gc->tsuDataList[dataElem];
; 274  :       while (i != GR_DLIST_END) {
; 275  :         TRI_SETF(FARRAY(b, i));
; 276  :         dataElem++;
; 277  :         i = gc->tsuDataList[dataElem];
; 278  :       }

    add       fifo, 8                    ; fifoPtr += sizeof(FxFloat)
    movq      mm6, mm1                   ; FARRAY(b,4) | FARRAY(b,0)

    lea       dlpStart, [gc+tsuDataList] ; gc->tsuDataList
    pfsub     mm6, mm2                   ; FARRAY(b,4)-_GlideRoot.pool.fHalf | FARRAY(b,0)

    mov       eax, [dlpStart]            ; i = gc->tsuDataList[dataElem]
    mov       dlp, dlpStart              ; dataElem = 0

    add       dlp, 4                     ; point to next entry in offset list
    movq      [fifo-8], mm6              ; PCI write FARRAY(b,4)-_GlideRoot.pool.fHalf | FARRAY(b,0)

    test      eax, eax                   ; i != GR_DLIST_END ?
    je        $vertex_loop1_done_WB0     ; i == GR_DLIST_END, no further components need to be pushed out

$vertex_loop1_WB0:
    movd      mm7, [vb + eax]            ; FARRAY(b,i)
    mov       eax, [dlp]                 ; get next offset from offset list

    test      eax, eax                   ; offset == 0 (end of offset list) ?
    jz        $vertex_loop1_done_WB1     ; nope, output next vertex component

    movd      mm6, [vb + eax]            ; FARRAY(b,i)
    add       dlp, 8                     ; point to next entry in offset list

    mov       eax, [dlp-4]               ; get next offset from offset list
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    punpckldq mm7, mm6                   ; current component | previous component
    test      eax, eax                   ; offset == 0 (end of offset list) ?

    movq      [fifo-8], mm7              ; PCI write current component | previous component
    jnz       $vertex_loop1_WB0          ; offset != 0, process next component

$vertex_loop1_done_WB0:

; 279  :       TRI_SETF(0.f);
; 280  :       TRI_SETF(0.f);
; 281  :       
; 282  :       TRI_SETF(FARRAY(a, 0));
; 283  :       dataElem = 0;
; 284  :       TRI_SETF(FARRAY(a, 4) - _GlideRoot.pool.fHalf);
; 285  :       i = gc->tsuDataList[dataElem];
; 286  :       while (i != GR_DLIST_END) {
; 287  :         TRI_SETF(FARRAY(a, i));
; 288  :         dataElem++;
; 289  :         i = gc->tsuDataList[dataElem];
; 290  :       }

    add       fifo, 16                   ; fifoPtr += 4*sizeof(FxFloat) 
    movq      mm6, mm0                   ; FARRAY(a,4) | FARRAY(a,0)

    movq      [fifo-16], mm3             ; PCI write TRI_SETF(0.f) | TRI_SETF(0.f)
    pfsub     mm6, mm2                   ; FARRAY(a,4)-_GlideRoot.pool.fHalf | FARRAY(a,0) 

    mov       eax, [dlpStart]            ; i = gc->tsuDataList[dataElem]
    mov       dlp, dlpStart              ; dataElem = 0

    add       dlp, 4                     ; point to next entry in offset list
    movq      [fifo-8], mm6              ; PCI write FARRAY(a,4)-_GlideRoot.pool.fHalf | FARRAY(a,0)

    cmp       eax, 0                     ; i == GR_DLIST_END ?
    je        $vertex_loop2_done_WB0     ; i == GR_DLIST_END, no further components need to be pushed out

$vertex_loop2_WB0:
    movd      mm7, [va + eax]            ; FARRAY(a,i)
    mov       eax, [dlp]                 ; get next offset from offset list

    test      eax, eax                   ; offset == 0 (end of offset list) ?
    jz        $vertex_loop2_done_WB1     ; nope, output next vertex component

    movd      mm6, [va + eax]            ; FARRAY(a,i)
    add       dlp, 8                     ; point to next entry in offset list

    mov       eax, [dlp-4]               ; get next offset from offset list
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    punpckldq mm7, mm6                   ; current component | previous component
    test      eax, eax                   ; offset == 0 (end of offset list) ?

    movq      [fifo-8], mm7              ; PCI write current component | previous component
    jnz       $vertex_loop2_WB0          ; offset != 0, process next component

$vertex_loop2_done_WB0:

; 291  :       TRI_SETF(0.f);
; 292  :       TRI_SETF(0.f);
; 293  :       
; 294  :       TRI_SETF(FARRAY(b, 0));
; 295  :       dataElem = 0;
; 296  :       TRI_SETF(FARRAY(b, 4) + _GlideRoot.pool.fHalf);
; 297  :       i = gc->tsuDataList[dataElem];
; 298  :       while (i != GR_DLIST_END) {
; 299  :         TRI_SETF(FARRAY(b, i));
; 300  :         dataElem++;
; 301  :         i = gc->tsuDataList[dataElem];
; 302  :       }

    add       fifo, 16                   ; fifoPtr += 4*sizeof(FxFloat)
    movq      mm6, mm1                   ; FARRAY(b,4) | FARRAY(b,0)

    movq      [fifo-16], mm3             ; PCI write TRI_SETF(0.f) | TRI_SETF(0.f)
    pfadd     mm6, mm2                   ; FARRAY(b,4)+_GlideRoot.pool.fHalf | FARRAY(b,0)

    mov       eax, [dlpStart]            ; i = gc->tsuDataList[dataElem]
    mov       dlp, dlpStart              ; dataElem = 0

    add       dlp, 4                     ; point to next entry in offset list
    test      eax, eax                   ; i != GR_DLIST_END ?

    movq      [fifo-8], mm6              ; PCI write FARRAY(b,4)+_GlideRoot.pool.fHalf | FARRAY(b,0)
    je        $vertex_loop3_done_WB0     ; i == GR_DLIST_END, no further components need to be pushed out

$vertex_loop3_WB0:
    movd      mm7, [vb + eax]            ; FARRAY(b, i)
    mov       eax, [dlp]                 ; get next offset from offset list

    test      eax, eax                   ; offset == 0 (end of offset list) ?
    jz        $vertex_loop3_done_WB1     ; nope, output next vertex component

    movd      mm6, [vb + eax]            ; FARRAY(b, i)
    add       dlp, 8                     ; point to next entry in offset list

    mov       eax, [dlp-4]               ; get next offset from offset list
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    punpckldq mm7, mm6                   ; current component | previous component
    cmp       eax, 0                     ; offset == 0 (end of offset list) ?

    movq      [fifo-8], mm7              ; PCI write current component | previous component
    jne       $vertex_loop3_WB0          ; offset != 0, process next component

$vertex_loop3_done_WB0:

; 303  :       TRI_SETF(256.f);
; 304  :       TRI_SETF(0.f);
; 305  :       
; 306  :       TRI_SETF(FARRAY(a, 0));
; 307  :       dataElem = 0;
; 308  :       TRI_SETF(FARRAY(a, 4) + _GlideRoot.pool.fHalf);
; 309  :       i = gc->tsuDataList[dataElem];
; 310  :       while (i != GR_DLIST_END) {
; 311  :         TRI_SETF(FARRAY(a, i));
; 312  :         dataElem++;
; 313  :         i = gc->tsuDataList[dataElem];
; 314  :       }
; 315  :       TRI_SETF(256.f);
; 316  :       TRI_SETF(0.f);

    add       fifo, 16                   ; fifoPtr += 4*sizeof(FxFloat)
    movq      mm6, mm0                   ; FARRAY(a,4) | FARRAY(a,0)

    movq      [fifo-16], mm4             ; PCI write TRI_SETF(0.f) | TRI_SETF(256.f)
    pfadd     mm6, mm2                   ; FARRAY(a,4)+_GlideRoot.pool.fHalf | FARRAY(a,0) 

    mov       eax, [dlpStart]            ; i = gc->tsuDataList[dataElem]
    mov       dlp, dlpStart              ; dataElem = 0

    add       dlp, 4                     ; point to next entry in offset list
    movq      [fifo-8], mm6              ; PCI write FARRAY(a,4)+_GlideRoot.pool.fHalf | FARRAY(a,0)

    test      eax, eax                   ; i != GR_DLIST_END ?
    jz        $vertex_loop4_done_WB0     ; i == GR_DLIST_END, done with all four vertices

$vertex_loop4_WB0:
    movd      mm7, [va + eax]            ; FARRAY(a, i)
    mov       eax, [dlp]                 ; get next offset from offset list

    test      eax, eax                   ; offset == 0 (end of offset list) ?
    jz        $vertex_loop4_done_WB1     ; nope, output next vertex component

    movd      mm6, [va + eax]            ; FARRAY(a, i)
    add       dlp, 8                     ; point to next entry in offset list

    mov       eax, [dlp-4]               ; get next offset from offset list
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    punpckldq mm7, mm6                   ; current component | previous component
    test      eax, eax                   ; offset == 0 (end of offset list) ?

    movq      [fifo-8], mm7              ; PCI write current component | previous component
    jnz       $vertex_loop4_WB0          ; offset != 0, process next component

$vertex_loop4_done_WB0:
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)
    lea       esp, [esp]                 ; filler

    movq      [fifo-8], mm4              ; PCI write TRI_SETF(0.f) | TRI_SETF(256.f) 
    jmp       $line_done_WB0             ; last vertex done

$j_lt_i2_WB0:

; 317  :     } else { /* y major */
; 318  :       TRI_SETF(FARRAY(b, 0) - _GlideRoot.pool.fHalf);
; 319  :       dataElem = 0;
; 320  :       TRI_SETF(FARRAY(b, 4));
; 321  :       i = gc->tsuDataList[dataElem];
; 322  :       while (i != GR_DLIST_END) {
; 323  :         TRI_SETF(FARRAY(b, i));
; 324  :         dataElem++;
; 325  :         i = gc->tsuDataList[dataElem];
; 326  :       }

    psrlq     mm2, 32                    ; 0 | _GlideRoot.pool.fHalf
    movq      mm6, mm1                   ; FARRAY(b,4) | FARRAY(b,0)
   
    pfsub     mm6, mm2                   ; FARRAY(b,4) | FARRAY(b,0)-_GlideRoot.pool.fHalf 
    lea       dlpStart, [gc+tsuDataList] ; gc->tsuDataList

    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)
    mov       eax, [dlpStart]            ; i = gc->tsuDataList[dataElem]

    lea       dlp, [dlpStart + 4]        ; next dataElem = 1
    movq      [fifo-8], mm6              ; PCI write FARRAY(b,4)-_GlideRoot.pool.fHalf | FARRAY(b,0)

    cmp       eax, 0                     ; i != GR_DLIST_END ?
    je        $vertex_loop5_done_WB0     ; i == GR_DLIST_END, no more components in vertex

$vertex_loop5_WB0:
    movd      mm7, [vb + eax]            ; FARRAY(b,i)
    mov       eax, [dlp]                 ; get next offset from offset list

    test      eax, eax                   ; offset == 0 (end of offset list) ?
    jz        $vertex_loop5_done_WB1     ; nope, output next vertex component

    movd      mm6, [vb + eax]            ; FARRAY(b,i)
    add       dlp, 8                     ; point to next entry in offset list

    mov       eax, [dlp-4]               ; get next offset from offset list
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    punpckldq mm7, mm6                   ; current component | previous component
    test      eax, eax                   ; offset == 0 (end of offset list) ?

    movq      [fifo-8], mm7              ; PCI write current component | previous component
    jnz       $vertex_loop5_WB0          ; offset != 0, process next component

$vertex_loop5_done_WB0:

; 327  :       TRI_SETF(0.f);
; 328  :       TRI_SETF(0.f);
; 329  :           
; 330  :       TRI_SETF(FARRAY(a, 0) - _GlideRoot.pool.fHalf);
; 331  :       dataElem = 0;
; 332  :       TRI_SETF(FARRAY(a, 4));
; 333  :       i = gc->tsuDataList[dataElem];
; 334  :       while (i != GR_DLIST_END) {
; 335  :         TRI_SETF(FARRAY(a, i));
; 336  :         dataElem++;
; 337  :         i = gc->tsuDataList[dataElem];
; 338  :       }

    add       fifo, 16                   ; fifoPtr += 4*sizeof(FxFloat)
    movq      mm6, mm0                   ; FARRAY(a,4) | FARRAY(a,0)

    movq      [fifo-16], mm3             ; PCI write TRI_SETF(0.f) | TRI_SETF(0.f)
    pfsub     mm6, mm2                   ; FARRAY(a,4) | FARRAY(a,0)-_GlideRoot.pool.fHalf

    mov       eax, [dlpStart]            ; i = gc->tsuDataList[dataElem]
    mov       dlp, dlpStart              ; dataElem = 0

    add       dlp, 4                     ; point to next entry in offset list
    movq      [fifo-8], mm6              ; PCI write FARRAY(a,4) | FARRAY(a,0)-_GlideRoot.pool.fHalf

    test      eax, eax                   ; i != GR_DLIST_END ?
    jz        $vertex_loop6_done_WB0     ; i == GR_DLIST_END, no further components need to be pushed out

$vertex_loop6_WB0:
    movd      mm7, [va + eax]            ; get next vertex component
    mov       eax, [dlp]                 ; get next offset from offset list

    cmp       eax, 0                     ; offset == 0 (end of offset list) ?
    je        $vertex_loop6_done_WB1     ; nope, output next vertex component

    movd      mm6, [va + eax]            ; get next vertex component
    add       dlp, 8                     ; point to next entry in offset list

    mov       eax, [dlp-4]               ; get next offset from offset list
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    punpckldq mm7, mm6                   ; current component | previous component
    test      eax, eax                   ; offset == 0 (end of offset list) ?

    movq      [fifo-8], mm7              ; PCI write current component | previous component
    jnz       $vertex_loop6_WB0          ; offset != 0, process next component

$vertex_loop6_done_WB0:

; 339  :       TRI_SETF(0.f);
; 340  :       TRI_SETF(0.f);
; 341  :       
; 342  :       TRI_SETF(FARRAY(b, 0) + _GlideRoot.pool.fHalf);
; 343  :       dataElem = 0;
; 344  :       TRI_SETF(FARRAY(b, 4));
; 345  :       i = gc->tsuDataList[dataElem];
; 346  :       while (i != GR_DLIST_END) {
; 347  :         TRI_SETF(FARRAY(b, i));
; 348  :         dataElem++;
; 349  :         i = gc->tsuDataList[dataElem];
; 350  :       }

    add       fifo, 16                   ; fifoPtr += 4*sizeof(FxFloat)
    movq      mm6, mm1                   ; FARRAY(b,4) | FARRAY(b,0)

    movq      [fifo-16], mm3             ; PCI write TRI_SETF(0.f) | TRI_SETF(0.f)
    pfadd     mm6, mm2                   ; FARRAY(b,4) | FARRAY(b,0)+_GlideRoot.pool.fHalf

    mov       eax, [dlpStart]            ; i = gc->tsuDataList[dataElem]
    mov       dlp, dlpStart              ; dataElem = 0

    add       dlp, 4                     ; point to next entry in offset list
    movq      [fifo-8], mm6              ; PCI write FARRAY(b,4) | FARRAY(b,0)+_GlideRoot.pool.fHalf

    test      eax, eax                   ; i != GR_DLIST_END ?
    jz        $vertex_loop7_done_WB0     ; i == GR_DLIST_END, no further components need to be pushed out

$vertex_loop7_WB0:

    movd      mm7, [vb + eax]            ; FARRAY(b, i)
    mov       eax, [dlp]                 ; get next offset from offset list

    test      eax, eax                   ; offset == 0 (end of offset list) ?
    jz        $vertex_loop7_done_WB1     ; nope, output next vertex component

    movd      mm6, [vb + eax]            ; FARRAY(b, i)
    add       dlp, 8                     ; point to next entry in offset list

    mov       eax, [dlp-4]               ; get next offset from offset list
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    punpckldq mm7, mm6                   ; current component | previous component
    test      eax, eax                   ; offset == 0 (end of offset list) ?

    movq      [fifo-8], mm7              ; PCI write current component | previous component
    jnz       $vertex_loop7_WB0          ; offset != 0, process next component

$vertex_loop7_done_WB0:

; 351  :       TRI_SETF(256.f);
; 352  :       TRI_SETF(0.f);
; 353  :       
; 354  :       TRI_SETF(FARRAY(a, 0) + _GlideRoot.pool.fHalf);
; 355  :       dataElem = 0;
; 356  :       TRI_SETF(FARRAY(a, 4));
; 357  :       i = gc->tsuDataList[dataElem];
; 358  :       while (i != GR_DLIST_END) {
; 359  :         TRI_SETF(FARRAY(a, i));
; 360  :         dataElem++;
; 361  :         i = gc->tsuDataList[dataElem];
; 362  :       }
; 363  :       TRI_SETF(256.f);
; 364  :       TRI_SETF(0.f);

    add       fifo, 16                   ; fifoPtr += 4*sizeof(FxFloat)
    movq      mm6, mm0                   ; FARRAY(a,4) | FARRAY(a,0)

    movq      [fifo-16], mm4             ; PCI write TRI_SETF(0.f) | TRI_SETF(256.f)
    pfadd     mm6, mm2                   ; FARRAY(a,4)+_GlideRoot.pool.fHalf | FARRAY(a,0) 

    mov       eax, [dlpStart]            ; i = gc->tsuDataList[dataElem]
    mov       dlp, dlpStart              ; dataElem = 0

    add       dlp, 4                     ; point to next entry in offset list
    movq      [fifo-8], mm6              ; PCI write FARRAY(a,4)+_GlideRoot.pool.fHalf | FARRAY(a,0)

    test      eax, eax                   ; i != GR_DLIST_END ?
    jz        $vertex_loop8_done_WB0     ; i == GR_DLIST_END, no further components need to be pushed out

$vertex_loop8_WB0:
    movd      mm7, [va + eax]            ; FARRAY(a, i)
    mov       eax, [dlp]                 ; get next offset from offset list

    cmp       eax, 0                     ; offset == 0 (end of offset list) ?
    je        $vertex_loop8_done_WB1     ; nope, output next vertex component

    movd      mm6, [va + eax]            ; FARRAY(a, i)
    add       dlp, 8                     ; point to next entry in offset list

    mov       eax, [dlp-4]               ; get next offset from offset list
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    punpckldq mm7, mm6                   ; current component | previous component
    movq      [fifo-8], mm7              ; PCI write current component | previous component

    test      eax, eax                   ; offset == 0 (end of offset list) ?
    jnz       $vertex_loop8_WB0          ; offset != 0, process next component

$vertex_loop8_done_WB0:

    movq      [fifo], mm4                ; PCI write TRI_SETF(0.f) | TRI_SETF(256.f) 
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    nop                                  ; filler
    jmp       $line_done_WB0             ; done with line

$drawline_fifo_aligned:

    mov       eax, [gc+cullStripHdr]     ; gc->cmdTransportInfo.cullStripHdr
    psllq     mm2, 32                    ; _GlideRoot.pool.fHalf | 0

    punpckldq mm4, mm3                   ; 0 | 256.0f
    or        eax, 8108h                 ; kSetupCullDisable|kSetupStrip|4<<6|SSTCP_PKT3_BDDDDD<<3|1<<15 

    movd      mm7, eax                   ; move header to "write buffer" = MM7

    cmp       j, i                       ; j < i ?
    jl        $j_lt_i2_WB1               ; yup, j < i

; 270  :       TRI_SETF(FARRAY(b, 0));
; 271  :       dataElem = 0;
; 272  :       TRI_SETF(FARRAY(b, 4) - _GlideRoot.pool.fHalf);
; 273  :       i = gc->tsuDataList[dataElem];
; 274  :       while (i != GR_DLIST_END) {
; 275  :         TRI_SETF(FARRAY(b, i));
; 276  :         dataElem++;
; 277  :         i = gc->tsuDataList[dataElem];
; 278  :       }

    add       fifo, 8                    ; fifoPtr += sizeof(FxFloat)
    movq      mm6, mm1                   ; FARRAY(b,4) | FARRAY(b,0)

    lea       dlpStart, [gc+tsuDataList] ; gc->tsuDataList
    pfsub     mm6, mm2                   ; FARRAY(b,4)-_GlideRoot.pool.fHalf | FARRAY(b,0)

    mov       eax, [dlpStart]            ; i = gc->tsuDataList[dataElem]
    lea       dlp, [dlpStart + 4]        ; next dataElem = 1

    punpckldq mm7, mm1                   ; FARRAY(b,0) | header
    cmp       eax, 0                     ; i == GR_DLIST_END ?

    movq      [fifo-8], mm7              ; PCI write FARRAY(b,0) | header
    movq      mm7, mm6                   ; FARRAY(b,4)-_GlideRoot.pool.fHalf | FARRAY(b,0)

    punpckhdq mm7, mm7                   ; write buffer = FARRAY(b,4)-_GlideRoot.pool.fHalf 
    jz        $vertex_loop1_done_WB1     ; i == GR_DLIST_END, no further components need to be pushed out

$vertex_loop1_WB1:
    movd      mm6, [vb + eax]            ; FARRAY(b,i)
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    mov       eax, [dlp]                 ; get next offset from offset list
    add       dlp, 8                     ; point to next offset

    punpckldq mm7, mm6                   ; current component | previous component
    test      eax, eax                   ; i == GR_DLIST_END ?

    movq      [fifo-8], mm7              ; PCI write current component | previous component
    jz        $vertex_loop1_done_WB0     ; yup, i == GR_DLIST_END

    movd      mm7, [vb + eax]            ; FARRAY(b,i)
    mov       eax, [dlp-4]               ; get next offset from offset list

    cmp       eax, 0                     ; i == GR_DLIST_END ?
    jne       $vertex_loop1_WB1          ; nope, more components to handle

$vertex_loop1_done_WB1:

; 279  :       TRI_SETF(0.f);
; 280  :       TRI_SETF(0.f);
; 281  :       
; 282  :       TRI_SETF(FARRAY(a, 0));
; 283  :       dataElem = 0;
; 284  :       TRI_SETF(FARRAY(a, 4) - _GlideRoot.pool.fHalf);
; 285  :       i = gc->tsuDataList[dataElem];
; 286  :       while (i != GR_DLIST_END) {
; 287  :         TRI_SETF(FARRAY(a, i));
; 288  :         dataElem++;
; 289  :         i = gc->tsuDataList[dataElem];
; 290  :       }

    add       fifo, 16                   ; fifoPtr += 4*sizeof(FxFloat) 
    movq      mm6, mm0                   ; FARRAY(a,4) | FARRAY(a,0)

    punpckldq mm7, mm3                   ; 0 | previous component
    pfsub     mm6, mm2                   ; FARRAY(a,4)-_GlideRoot.pool.fHalf | FARRAY(a,0) 

    movq      [fifo-16], mm7             ; PCI write 0 | previous component
    movq      mm7, mm3                   ; 0 | 0

    punpckldq mm7, mm0                   ; FARRAY(a,0) | 0
    mov       eax, [dlpStart]            ; i = gc->tsuDataList[dataElem]

    movq      [fifo-8], mm7              ; PCI write FARRAY(a,0) | 0
    movq      mm7, mm6                   ; FARRAY(a,4)-_GlideRoot.pool.fHalf | FARRAY(a,0)

    punpckhdq mm7, mm7                   ; write buffer = FARRAY(a,4)-_GlideRoot.pool.fHalf
    lea       dlp, [dlpStart+4]          ; point to next entry in offset list

    test      eax, eax                   ; i == GR_DLIST_END ?
    je        $vertex_loop2_done_WB1     ; i == GR_DLIST_END, no further components need to be pushed out

$vertex_loop2_WB1:
    movd      mm6, [va + eax]            ; FARRAY(a,i)
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    mov       eax, [dlp]                 ; get next offset from offset list
    add       dlp, 8                     ; point to next offset

    punpckldq mm7, mm6                   ; current component | previous component
    test      eax, eax                   ; i == GR_DLIST_END ?

    movq      [fifo-8], mm7              ; PCI write current component | previous component
    jz        $vertex_loop2_done_WB0     ; yup, i == GR_DLIST_END

    movd      mm7, [va + eax]            ; FARRAY(a,i)
    mov       eax, [dlp-4]               ; get next offset from offset list

    test      eax, eax                   ; i == GR_DLIST_END ?
    jnz       $vertex_loop2_WB1          ; nope, more components to handle

$vertex_loop2_done_WB1:

; 291  :       TRI_SETF(0.f);
; 292  :       TRI_SETF(0.f);
; 293  :       
; 294  :       TRI_SETF(FARRAY(b, 0));
; 295  :       dataElem = 0;
; 296  :       TRI_SETF(FARRAY(b, 4) + _GlideRoot.pool.fHalf);
; 297  :       i = gc->tsuDataList[dataElem];
; 298  :       while (i != GR_DLIST_END) {
; 299  :         TRI_SETF(FARRAY(b, i));
; 300  :         dataElem++;
; 301  :         i = gc->tsuDataList[dataElem];
; 302  :       }

    add       fifo, 16                   ; fifoPtr += 4*sizeof(FxFloat)
    movq      mm6, mm1                   ; FARRAY(b,4) | FARRAY(b,0)

    punpckldq mm7, mm3                   ; 0 | previous component
    pfadd     mm6, mm2                   ; FARRAY(b,4)+_GlideRoot.pool.fHalf | FARRAY(b,0)

    movq      [fifo-16], mm7             ; PCI write 0 | previous component
    movq      mm7, mm3                   ; 0 | 0

    punpckldq mm7, mm1                   ; FARRAY(b,0) | 0
    mov       eax, [dlpStart]            ; i = gc->tsuDataList[dataElem]

    movq      [fifo-8], mm7              ; PCI write FARRAY(b,0) | 0
    movq      mm7, mm6                   ; FARRAY(b,4)+_GlideRoot.pool.fHalf | FARRAY(b,0)

    punpckhdq mm7, mm7                   ; write buffer = FARRAY(a,4)-_GlideRoot.pool.fHalf
    lea       dlp, [dlpStart+4]          ; point to next entry in offset list

    test      eax, eax                   ; i == GR_DLIST_END ?
    jz        $vertex_loop3_done_WB1     ; yup, i == GR_DLIST_END, no further components need to be pushed out

$vertex_loop3_WB1:
    movd      mm6, [vb + eax]            ; FARRAY(b,i)
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    mov       eax, [dlp]                 ; get next offset from offset list
    add       dlp, 8                     ; point to next offset

    punpckldq mm7, mm6                   ; current component | previous component
    test      eax, eax                   ; i == GR_DLIST_END ?

    movq      [fifo-8], mm7              ; PCI write current component | previous component
    jz        $vertex_loop3_done_WB0     ; yup, i == GR_DLIST_END

    movd      mm7, [vb + eax]            ; FARRAY(b,i)
    mov       eax, [dlp-4]               ; get next offset from offset list

    cmp       eax, 0                     ; i == GR_DLIST_END ?
    jnz       $vertex_loop3_WB1          ; nope, more components to handle

$vertex_loop3_done_WB1:

; 303  :       TRI_SETF(256.f);
; 304  :       TRI_SETF(0.f);
; 305  :       
; 306  :       TRI_SETF(FARRAY(a, 0));
; 307  :       dataElem = 0;
; 308  :       TRI_SETF(FARRAY(a, 4) + _GlideRoot.pool.fHalf);
; 309  :       i = gc->tsuDataList[dataElem];
; 310  :       while (i != GR_DLIST_END) {
; 311  :         TRI_SETF(FARRAY(a, i));
; 312  :         dataElem++;
; 313  :         i = gc->tsuDataList[dataElem];
; 314  :       }
; 315  :       TRI_SETF(256.f);
; 316  :       TRI_SETF(0.f);

    add       fifo, 16                   ; fifoPtr += 4*sizeof(FxFloat) 
    movq      mm6, mm0                   ; FARRAY(a,4) | FARRAY(a,0)

    punpckldq mm7, mm4                   ; TRI_SETF(256.f) | previous component
    pfadd     mm6, mm2                   ; FARRAY(a,4)+_GlideRoot.pool.fHalf | FARRAY(a,0) 

    movq      [fifo-16], mm7             ; PCI write 0 | previous component
    movq      mm7, mm3                   ; 0 | 0

    punpckldq mm7, mm0                   ; FARRAY(a,0) | 0
    mov       eax, [dlpStart]            ; i = gc->tsuDataList[dataElem]

    movq      [fifo-8], mm7              ; PCI write FARRAY(a,0) | 0
    movq      mm7, mm6                   ; FARRAY(a,4)+_GlideRoot.pool.fHalf | FARRAY(a,0)

    punpckhdq mm7, mm7                   ; write buffer = FARRAY(a,4)-_GlideRoot.pool.fHalf
    lea       dlp, [dlpStart+4]          ; point to next entry in offset list

    test      eax, eax                   ; i == GR_DLIST_END ?
    jz        $vertex_loop4_WB1          ; i == GR_DLIST_END, done with all four vertices

$vertex_loop4_WB1:
    movd      mm6, [va + eax]            ; FARRAY(a,i)
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    mov       eax, [dlp]                 ; get next offset from offset list
    add       dlp, 8                     ; point to next offset

    punpckldq mm7, mm6                   ; current component | previous component
    test      eax, eax                   ; i == GR_DLIST_END ?

    movq      [fifo-8], mm7              ; PCI write current component | previous component
    jz        $vertex_loop4_done_WB0     ; yup, i == GR_DLIST_END

    movd      mm7, [va + eax]            ; FARRAY(a,i)
    mov       eax, [dlp-4]               ; get next offset from offset list

    test      eax, eax                   ; i == GR_DLIST_END ?
    jnz       $vertex_loop4_WB1          ; nope, more components to handle

$vertex_loop4_done_WB1:
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)
    punpckldq mm7, mm4                   ; previous component | TRI_SETF(256.f)

    movq      [fifo-8], mm7              ; PCI write previous component | TRI_SETF(256.f)
    movq      mm7, mm4                   ; TRI_SETF(0.f) | TRI_SETF(256.f)

    punpckhdq mm7, mm7                   ; write buffer = TRI_SETF(0.f)
    jmp       $line_done_WB1             ; last vertex done

$j_lt_i2_WB1:

; 317  :     } else { /* y major */
; 318  :       TRI_SETF(FARRAY(b, 0) - _GlideRoot.pool.fHalf);
; 319  :       dataElem = 0;
; 320  :       TRI_SETF(FARRAY(b, 4));
; 321  :       i = gc->tsuDataList[dataElem];
; 322  :       while (i != GR_DLIST_END) {
; 323  :         TRI_SETF(FARRAY(b, i));
; 324  :         dataElem++;
; 325  :         i = gc->tsuDataList[dataElem];
; 326  :       }

    psrlq     mm2, 32                    ; 0 | _GlideRoot.pool.fHalf
    movq      mm6, mm1                   ; FARRAY(b,4) | FARRAY(b,0)
   
    pfsub     mm6, mm2                   ; FARRAY(b,4) | FARRAY(b,0)-_GlideRoot.pool.fHalf 
    lea       dlpStart, [gc+tsuDataList] ; gc->tsuDataList

    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)
    mov       eax, [dlpStart]            ; i = gc->tsuDataList[dataElem]

    punpckldq mm7, mm6                   ; FARRAY(b,0)-_GlideRoot.pool.fHalf | header
    lea       dlp, [dlpStart+4]          ; next dataElem = 1

    movq      [fifo-8], mm7              ; PCI write FARRAY(b,0)-_GlideRoot.pool.fHalf | header
    movq      mm7, mm6                   ; FARRAY(b,4) | FARRAY(b,0)-_GlideRoot.pool.fHalf

    punpckhdq mm7, mm7                   ; "write buffer" = FARRAY(b,4)
    cmp       eax, 0                     ; i != GR_DLIST_END ?

    nop                                  ; filler
    je        $vertex_loop5_done_WB1     ; i == GR_DLIST_END, no more components in vertex

$vertex_loop5_WB1:
    movd      mm6, [vb + eax]            ; FARRAY(b,i)
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    mov       eax, [dlp]                 ; get next offset from offset list
    add       dlp, 8                     ; point to next offset

    punpckldq mm7, mm6                   ; current component | previous component
    test      eax, eax                   ; i == GR_DLIST_END ?

    movq      [fifo-8], mm7              ; PCI write current component | previous component
    jz        $vertex_loop5_done_WB0     ; yup, i == GR_DLIST_END

    movd      mm7, [vb + eax]            ; FARRAY(b,i)
    mov       eax, [dlp-4]               ; get next offset from offset list

    test      eax, eax                   ; i == GR_DLIST_END ?
    jnz       $vertex_loop5_WB1          ; nope, more components to handle

$vertex_loop5_done_WB1:

; 327  :       TRI_SETF(0.f);
; 328  :       TRI_SETF(0.f);
; 329  :           
; 330  :       TRI_SETF(FARRAY(a, 0) - _GlideRoot.pool.fHalf);
; 331  :       dataElem = 0;
; 332  :       TRI_SETF(FARRAY(a, 4));
; 333  :       i = gc->tsuDataList[dataElem];
; 334  :       while (i != GR_DLIST_END) {
; 335  :         TRI_SETF(FARRAY(a, i));
; 336  :         dataElem++;
; 337  :         i = gc->tsuDataList[dataElem];
; 338  :       }

    add       fifo, 16                   ; fifoPtr += 4*sizeof(FxFloat) 
    movq      mm6, mm0                   ; FARRAY(a,4) | FARRAY(a,0)

    punpckldq mm7, mm3                   ; 0 | previous component
    pfsub     mm6, mm2                   ; FARRAY(a,4) | FARRAY(a,0)-_GlideRoot.pool.fHalf

    movq      [fifo-16], mm7             ; PCI write 0 | previous component
    movq      mm7, mm3                   ; 0 | 0

    punpckldq mm7, mm6                   ; FARRAY(a,0)-_GlideRoot.pool.fHalf | 0
    mov       eax, [dlpStart]            ; i = gc->tsuDataList[dataElem]

    movq      [fifo-8], mm7              ; PCI write FARRAY(a,0) | 0
    movq      mm7, mm6                   ; FARRAY(a,4) | FARRAY(a,0)-_GlideRoot.pool.fHalf

    punpckhdq mm7, mm7                   ; write buffer = FARRAY(a,4)
    lea       dlp, [dlpStart+4]          ; point to next entry in offset list

    test      eax, eax                   ; i == GR_DLIST_END ?
    jz        $vertex_loop6_done_WB1     ; i == GR_DLIST_END, no further components need to be pushed out

$vertex_loop6_WB1:
    movd      mm6, [va + eax]            ; FARRAY(a,i)
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    mov       eax, [dlp]                 ; get next offset from offset list
    add       dlp, 8                     ; point to next offset

    punpckldq mm7, mm6                   ; current component | previous component
    movq      [fifo-8], mm7              ; PCI write current component | previous component

    test      eax, eax                   ; i == GR_DLIST_END ?
    jz        $vertex_loop6_done_WB0     ; yup, i == GR_DLIST_END

    movd      mm7, [va + eax]            ; FARRAY(a,i)
    mov       eax, [dlp-4]               ; get next offset from offset list

    cmp       eax, 0                     ; i == GR_DLIST_END ?
    jne       $vertex_loop6_WB1          ; nope, more components to handle

$vertex_loop6_done_WB1:

; 339  :       TRI_SETF(0.f);
; 340  :       TRI_SETF(0.f);
; 341  :       
; 342  :       TRI_SETF(FARRAY(b, 0) + _GlideRoot.pool.fHalf);
; 343  :       dataElem = 0;
; 344  :       TRI_SETF(FARRAY(b, 4));
; 345  :       i = gc->tsuDataList[dataElem];
; 346  :       while (i != GR_DLIST_END) {
; 347  :         TRI_SETF(FARRAY(b, i));
; 348  :         dataElem++;
; 349  :         i = gc->tsuDataList[dataElem];
; 350  :       }

    add       fifo, 16                   ; fifoPtr += 4*sizeof(FxFloat)
    movq      mm6, mm1                   ; FARRAY(b,4) | FARRAY(b,0)

    punpckldq mm7, mm3                   ; 0 | previous component
    pfadd     mm6, mm2                   ; FARRAY(b,4) | FARRAY(b,0)+_GlideRoot.pool.fHalf

    movq      [fifo-16], mm7             ; PCI write 0 | previous component
    movq      mm7, mm3                   ; 0 | 0

    punpckldq mm7, mm6                   ; FARRAY(b,0)+_GlideRoot.pool.fHalf | 0
    mov       eax, [dlpStart]            ; i = gc->tsuDataList[dataElem]

    movq      [fifo-8], mm7              ; PCI write FARRAY(b,0)+_GlideRoot.pool.fHalf | 0
    movq      mm7, mm6                   ; FARRAY(b,4) | FARRAY(b,0)+_GlideRoot.pool.fHalf

    punpckhdq mm7, mm7                   ; write buffer = FARRAY(b,4)
    lea       dlp, [dlpStart+4]          ; point to next entry in offset list

    cmp       eax, 0                     ; i == GR_DLIST_END ?
    je        $vertex_loop7_done_WB1     ; i == GR_DLIST_END, no further components need to be pushed out

$vertex_loop7_WB1:
    movd      mm6, [vb + eax]            ; FARRAY(b,i)
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    mov       eax, [dlp]                 ; get next offset from offset list
    add       dlp, 8                     ; point to next offset

    punpckldq mm7, mm6                   ; current component | previous component
    cmp       eax, 0                     ; i == GR_DLIST_END ?

    movq      [fifo-8], mm7              ; PCI write current component | previous component
    jz        $vertex_loop7_done_WB0     ; yup, i == GR_DLIST_END

    movd      mm7, [vb + eax]            ; FARRAY(b,i)
    mov       eax, [dlp-4]               ; get next offset from offset list

    test      eax, eax                   ; i == GR_DLIST_END ?
    jnz       $vertex_loop7_WB1          ; nope, more components to handle

$vertex_loop7_done_WB1:

; 351  :       TRI_SETF(256.f);
; 352  :       TRI_SETF(0.f);
; 353  :       
; 354  :       TRI_SETF(FARRAY(a, 0) + _GlideRoot.pool.fHalf);
; 355  :       dataElem = 0;
; 356  :       TRI_SETF(FARRAY(a, 4));
; 357  :       i = gc->tsuDataList[dataElem];
; 358  :       while (i != GR_DLIST_END) {
; 359  :         TRI_SETF(FARRAY(a, i));
; 360  :         dataElem++;
; 361  :         i = gc->tsuDataList[dataElem];
; 362  :       }
; 363  :       TRI_SETF(256.f);
; 364  :       TRI_SETF(0.f);

    add       fifo, 16                   ; fifoPtr += 4*sizeof(FxFloat)
    movq      mm6, mm0                   ; FARRAY(a,4) | FARRAY(a,0)

    punpckldq mm7, mm4                   ; 256.0f | previous component
    pfadd     mm6, mm2                   ; FARRAY(a,4) | FARRAY(a,0)+_GlideRoot.pool.fHalf

    movq      [fifo-16], mm7             ; PCI write 0 | previous component
    movq      mm7, mm3                   ; 0 | 0

    punpckldq mm7, mm6                   ; FARRAY(a,0)+_GlideRoot.pool.fHalf | 0
    mov       eax, [dlpStart]            ; i = gc->tsuDataList[dataElem]

    movq      [fifo-8], mm7              ; PCI write FARRAY(b,0)+_GlideRoot.pool.fHalf | 0
    movq      mm7, mm6                   ; FARRAY(a,4) | FARRAY(a,0)+_GlideRoot.pool.fHalf

    punpckhdq mm7, mm7                   ; write buffer = FARRAY(a,4)
    lea       dlp, [dlpStart+4]          ; point to next entry in offset list

    test      eax, eax                   ; i == GR_DLIST_END ?
    jz        $vertex_loop8_done_WB1     ; i == GR_DLIST_END, no further components need to be pushed out

$vertex_loop8_WB1:
    movd      mm6, [va + eax]            ; FARRAY(a,i)
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    mov       eax, [dlp]                 ; get next offset from offset list
    add       dlp, 8                     ; point to next offset

    cmp       eax, 0                     ; i == GR_DLIST_END ?
    punpckldq mm7, mm6                   ; current component | previous component

    movq      [fifo-8], mm7              ; PCI write current component | previous component
    jz        $vertex_loop8_done_WB0     ; yup, i == GR_DLIST_END

    movd      mm7, [va + eax]            ; FARRAY(a,i)
    mov       eax, [dlp-4]               ; get next offset from offset list

    cmp       eax, 0                     ; i == GR_DLIST_END ?
    jnz       $vertex_loop8_WB1          ; nope, more components to handle

$vertex_loop8_done_WB1:

    punpckldq mm7, mm4                   ; 256.0f | previous component
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    movq      [fifo-8], mm7              ; PCI write TRI_SETF(256.f) | previous component
    movq      mm7, mm4                   ; 256.0f | 0

    punpckhdq mm7, mm7                   ; write buffer = 256.0f
    nop                                  ; filler

$line_done_WB1:

    movd      [fifo], mm7                ; flush "write buffer"
    add       fifo, 4                    ; fifoPtr += sizeof(FxFloat)

$line_done_WB0:

; 365  :     }
; 366  :     TRI_END;
; 367  :     GR_CHECK_SIZE();
; 368  :   }
; 369  :   _GlideRoot.stats.linesDrawn++;
; 370  :   _GlideRoot.stats.othertrisDrawn+=2;
; 371  : 
; 372  : #undef FN_NAME
; 373  : } /* grDrawTextureLine */

    mov       eax, [gc + fifoPtr]        ; old fifoPtr
    mov       [gc + fifoPtr], fifo       ; save new fifoPtr

    mov       edx, [gc + fifoRoom]       ; fifo space available before
    nop                                  ; filler

    sub       eax, fifo                  ; fifo space used up
    mov       ebx, [__GlideRoot+stats_linesDrawn]     ; _GlideRoot.stats.linesDrawn

    add       edx, eax                   ; fifo space available now
    mov       ebp, [__GlideRoot+stats_otherTrisDrawn]     ; _GlideRoot.stats.othertrisDrawn

    mov       [gc + fifoRoom], edx       ; save available fifo space
    inc       ebx                        ; _GlideRoot.stats.linesDrawn++

    mov       [__GlideRoot+stats_linesDrawn], ebx     ; save _GlideRoot.stats.linesDrawn
    add       ebp, 2                     ; _GlideRoot.stats.othertrisDrawn+=2
    
    mov       [__GlideRoot+stats_otherTrisDrawn], ebp     ; _GlideRoot.stats.othertrisDrawn+=2
    nop                                  ; filler
$line_all_done:

    femms                                ; done with MMX; empty FPU/MMX state

    pop     ebp                          ; restore frame pointer
    pop     ebx                          ; restore caller's register variable

    pop     edi                          ; restore caller's register variable
    pop     esi                          ; restore caller's register variable

    ret     8                            ; return and pop 2 DWORD parameters
__grDrawTextureLine_3DNow@8 ENDP

        PUBLIC  __grDrawTriangles_3DNow@12
__grDrawTriangles_3DNow@12 PROC NEAR

_mode     = 20
_count    = 24
_pointers = 28    

; 930  : {
; 931  : #define FN_NAME "_grDrawTriangles_3DNow"
; 932  : 
; 933  :   FxI32
; 934  : #ifdef GLIDE_DEBUG
; 935  :     vSize,
; 936  : #endif
; 937  :     k;
; 938  :   FxI32 stride = mode;
; 939  :   float *vPtr;
; 940  : 
; 941  :   GR_BEGIN_NOFIFOCHECK(FN_NAME, 90);
; 942  : 
; 943  :   GDBG_INFO_MORE(gc->myLevel, "(count = %d, pointers = 0x%x)\n",
; 944  :                  count, pointers);
; 945  : 
; 946  :   GR_FLUSH_STATE();

gc            TEXTEQU  <edi>             ; points to graphics context
fifo          TEXTEQU  <ecx>             ; points to next entry in fifo
dlp           TEXTEQU  <ebp>             ; points to dataList structure
vertexCount   TEXTEQU  <esi>             ; Current vertex counter in the packet
vertexPtr     TEXTEQU  <ebx>             ; Current vertex pointer (in deref mode)
vertex        TEXTEQU  <ebx>             ; Current vertex (in non-deref mode)
dlpStart      TEXTEQU  <edx>             ; Pointer to start of offset list

    push      edi                        ; save caller's register variable
    mov       gc, [__GlideRoot + curGC]  ; get current graphics context

    push      esi                        ; save caller's register variable
    mov       vertexCount, [esp+_count-8]; number of vertices in triangles

    push      ebx                        ; save caller's register variable
    push      ebp                        ; save frame pointer
    
    mov       vertexPtr, [esp+_pointers] ; get current vertex pointer (deref mode)
    test      vertexCount, vertexCount   ; number of vertices <= 0 ?
    
    jle       $tris_done                 ; yup, triangles are done

; 947  : 
; 948  : #ifdef GLIDE_DEBUG
; 949  :   GDBG_INFO(110, "%s:  vSize = %d\n", FN_NAME, vSize);
; 950  : 
; 951  :   GDBG_INFO(110, "%s:  paramMask = 0x%x\n", FN_NAME, gc->cmdTransportInfo.paramMask);
; 952  : #endif
; 953  : 
; 954  :   if (stride == 0)
; 955  :     stride = gc->state.vData.vStride;
; 956  : 
; 957  : 
; 958  :   _GlideRoot.stats.trisProcessed+=(count/3);
; 959  : 
; 960  :   if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {

;; We can operate in one of two modes:
;;
;; 0. We are stepping through an array of vertices, in which case
;; the stridesize is equal to the size of the vertex data, and
;; always > 4, since vertex data must a least contain x,y (ie 8 bytes).
;; vertexPtr is pointing to the array of vertices.
;;
;; 1. We are stepping through an array of pointers to vertices
;; in which case the stride is 4 bytes and we need to dereference
;; the pointers to get at the vertex data. vertexPtr is pointing
;; to the array of pointers to vertices.

    mov       eax, [esp + _count]        ; count
    mov       ebp, 0AAAAAAABh            ; 1/3*2^32*2

    femms                                ; we'll use MMX; clear MMX/3DX state      

    mul       ebp                        ; edx:eax = 1/3*2*2^32*count; edx = 1/3*2*count

    mov       eax, [__GlideRoot+trisProcessed] ; trisProcessed
    shr       edx, 1                     ; count/3

    add       eax, edx                   ; trisProcessed += count/3
    mov       edx, [esp + _mode]         ; get mode (0 or 1)

    mov       ecx, [gc + CoordinateSpace]; coordinates space (window/clip)
    mov       [__GlideRoot+trisProcessed], eax ; trisProcessed

    test      edx, edx                   ; mode 0 (array of vertices) ?
    jnz       $deref_mode                ; nope, it's mode 1 (array of pointers to vertices)

    mov       edx, [gc + vertexStride]   ; get stride in DWORDs
    nop                                  ; filler

    shl       edx, 2                     ; stride in bytes
    cmp       ecx, 0                     ; coordinate space == 0 (window) ?

    mov       [strideinbytes], edx       ; save off stride (in bytes)
    jnz       $clip_coordinates_ND       ; nope, coordinate space != window  

; 961  :     while (count > 0) {
; 962  :       FxI32 vcount = count >=15 ? 15 : count;
; 963  :       GR_SET_EXPECTED_SIZE(vcount * gc->state.vData.vSize, 1);
; 964  :       TRI_STRIP_BEGIN(kSetupStrip, vcount, gc->state.vData.vSize, SSTCP_PKT3_BDDBDD);
; 965  :       

$win_coords_loop_ND:

    sub       vertexCount, 15            ; vertexCount >= 15 ? CF=0 : CF=1
    mov       ecx, [gc + vertexSize]     ; bytes of data for each vertex 

    sbb       eax, eax                   ; vertexCount >= 15 ? 00000000:ffffffff

    and       vertexCount, eax           ; vertexCount >= 15 ? 0 : vertexcount-15
    add       vertexCount, 15            ; vertexcount >= 15 ? 15 :vertexcount

    imul      ecx, vertexCount           ; total amount of vertex data we'll send

    mov       eax, [gc + fifoRoom]       ; fifo space available
    add       ecx, 4                     ; add header size ==> total packet size

    cmp       eax, ecx                   ; fifo space avail >= packet size ?
    jge       $win_tri_begin_ND          ; yup, start writing triangle data

    push      @Line                      ; line number inside this function
    push      0h                         ; pointer to function name = NULL

    push      ecx                        ; fifo space needed
    call      __FifoMakeRoom             ; note: updates fifoPtr

    add       esp, 12                    ; remove 3 DWORD arguments from stack

$win_tri_begin_ND:

    mov       eax, vertexCount           ; number of vertices in triangles
    mov       fifo, [gc + fifoPtr]       ; get fifoPtr

    mov       ebp, [gc + cullStripHdr]   ; <2:0> = type
    shl       eax, 6                     ; <9:6> = vertex count (max 15)

    or        eax, ebp                   ; setup vertex count and type
    mov       ebp, 4                     ; test bit 2

    lea       dlpStart, [gc+tsuDataList] ; pointer to start of offset list
    test      fifo, ebp                  ; fifoPtr QWORD aligned ?

    jz        $fifo_aligned_ND           ; yup

    mov       [fifo], eax                ; PCI write packet type
    add       fifo, 4                    ; fifo pointer now QWORD aligned

; 966  :       for (k = 0; k < vcount; k++) {
; 967  :         FxI32 i;
; 968  :         FxU32 dataElem = 0;
; 969  :         
; 970  :         vPtr = pointers;
; 971  :         if (mode)
; 972  :           vPtr = *(float **)vPtr;
; 973  :         (float *)pointers += stride;
; 974  :         
; 975  :         i = gc->tsuDataList[dataElem];
; 976  :         
; 977  :         TRI_SETF(FARRAY(vPtr, 0));
; 978  :         TRI_SETF(FARRAY(vPtr, 4));
; 979  :         while (i != GR_DLIST_END) {
; 980  :           TRI_SETF(FARRAY(vPtr, i));
; 981  :           dataElem++;
; 982  :           i = gc->tsuDataList[dataElem];
; 983  :         }
; 984  :       }
; 985  :       TRI_END;
; 986  :       GR_CHECK_SIZE();
; 987  :       count -= 15;
; 988  :     }

$win_vertex_loop_ND_WB0:                 ; nothing in "write buffer"

    mov       eax, [dlpStart]            ; get first offset from offset list
    mov       dlp, dlpStart              ; point to start of offset list

    movq      mm1, [vertex]              ; get vertex x,y
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxU32)

    add       dlp, 4                     ; dlp++
    test      eax, eax                   ; if offset == 0, end of list

    movq      [fifo-8], mm1              ; PCI write x, y
    jz        $win_datalist_end_ND_WB0   ; no more vertex data, nothing in "write buffer" 

$win_datalist_loop_ND_WB0:               ; nothing in "write buffer"

    movd      mm1, [vertex + eax]        ; get next parameter
    mov       eax, [dlp]                 ; get next offset from offset list

    cmp       eax, 0                     ; at end of offset list (offset == 0) ?
    jz        $win_datalist_end_ND_WB1   ; exit, write buffer contains one DWORD

    movd      mm2, [vertex + eax]        ; get next parameter
    add       dlp, 8                     ; dlp++

    mov       eax, [dlp-4]               ; get next offset from offset list
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxU32)

    cmp       eax, 0                     ; at end of offset list (offset == 0) ?
    punpckldq mm1, mm2                   ; current param | previous param

    movq      [fifo-8], mm1              ; PCI write current param | previous param
    jnz       $win_datalist_loop_ND_WB0  ; nope, copy next parameter

$win_datalist_end_ND_WB0:

    mov       eax, [strideinbytes]       ; get offset to next vertex
    dec       vertexCount                ; another vertex done. Any left?

    lea       vertex, [vertex + eax]     ; points to next vertex
    jnz       $win_vertex_loop_ND_WB0    ; yup, output next vertex

$win_vertex_end_ND_WB0:

    mov       eax, [gc + fifoPtr]        ; old fifoPtr
    mov       ebp, [gc + fifoRoom]       ; old number of bytes available in fifo

    mov       vertexCount, [esp + _count]; remaining vertices before previous loop
    sub       eax, fifo                  ; old fifoPtr - new fifoPtr (fifo room used)

    mov       [gc + fifoPtr], fifo       ; save current fifoPtr 
    add       ebp, eax                   ; new number of bytes available in fifo

    sub       vertexCount, 15            ; remaining number of vertices to process
    mov       [gc + fifoRoom], ebp       ; save current number of bytes available in fifo

    mov       [esp + _count], vertexCount; remaining number of vertices to process 
    test      vertexCount, vertexCount   ; any vertices left to process ?

    nop                                  ; filler
    jg        $win_coords_loop_ND        ; loop if number of vertices to process >= 0

    femms                                ; no more MMX code; clear MMX/FPU state

    pop       ebp                        ; restore frame pointer
    pop       ebx                        ; restore caller's register variable

    pop       esi                        ; restore caller's register variable
    pop       edi                        ; restore caller's register variable

    ret       12                         ; return, pop 3 DWORD parameters off stack

$fifo_aligned_ND:

    movd      mm1, eax                   ; move header into "write buffer"

$win_vertex_loop_ND_WB1:                 ; one DWORD in "write buffer"

    movd      mm2, [vertex]              ; 0 | x of vertex
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxU32)

    lea       dlp, [dlpStart + 4]        ; point to start of offset list
    nop                                  ; filler

    punpckldq mm1, mm2                   ; packet header | x of vertex
    mov       eax, [dlp-4]               ; first offset in offset list

    movq      [fifo-8], mm1              ; PCI write packet header | x of vertex
    movd      mm1, [vertex+4]            ; 0 | y of vertex

    test      eax, eax                   ; offset == 0 (list empty) ?
    jz        $win_datalist_end_ND_WB1   ; yup, no more vertex data, one DWORD in "write buffer"

$win_datalist_loop_ND_WB1:               ; one DWORD in "write buffer" 

    movd      mm2, [vertex + eax]        ; get next parameter
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxU32)

    mov       eax, [dlp]                 ; get next offset from offset list
    add       dlp, 8                     ; dlp += 2

    punpckldq mm1, mm2                   ; current param | previous param
    cmp       eax, 0                     ; at end of offset list (offset == 0) ?

    movq      [fifo-8], mm1              ; PCI write current param | previous param
    jz        $win_datalist_end_ND_WB0   ; yes, exit, "write buffer" empty

    movd      mm1, [vertex + eax]        ; get next parameter
    mov       eax, [dlp-4]               ; get next offset from offset list

    test      eax, eax                   ; at end of offset list (offset == 0) ?
    jnz       $win_datalist_loop_ND_WB1  ; nope, copy next parameter

$win_datalist_end_ND_WB1:

    mov       eax, [strideinbytes]       ; get offset to next vertex
    dec       vertexCount                ; another vertex done. Any left?

    lea       vertex, [vertex + eax]     ; points to next vertex
    jnz       $win_vertex_loop_ND_WB1    ; yup, output next vertex

$win_vertex_end_ND_WB1:

    movd      [fifo], mm1                ; flush "write buffer"
    mov       eax, [gc + fifoPtr]        ; old fifoPtr

    add       fifo, 4                    ; fifoPtr += sizeof(FxU32)
    mov       ebp, [gc + fifoRoom]       ; old number of bytes available in fifo

    mov       vertexCount, [esp + _count]; remaining vertices before previous loop
    sub       eax, fifo                  ; old fifoPtr - new fifoPtr (fifo room used)

    mov       [gc + fifoPtr], fifo       ; save current fifoPtr 
    add       ebp, eax                   ; new number of bytes available in fifo

    mov       [gc + fifoRoom], ebp       ; save current number of bytes available in fifo
    sub       vertexCount, 15            ; remaining number of vertices to process

    mov       [esp + _count], vertexCount; remaining number of vertices to process 
    test      vertexCount, vertexCount   ; any vertices left to process ?

    nop                                  ; filler
    jg        $win_coords_loop_ND        ; loop if number of vertices to process >= 0

    femms                                ; no more MMX code; clear MMX/FPU state

    pop       ebp                        ; restore frame pointer
    pop       ebx                        ; restore caller's register variable

    pop       esi                        ; restore caller's register variable
    pop       edi                        ; restore caller's register variable

    ret       12                         ; return, pop 3 DWORD parameters off stack

$deref_mode:

    prefetch  [vertexPtr]                ; pre-load first group of pointers

    test      ecx, ecx                   ; coordinate space == 0 (window) ?
    jnz       $clip_coordinates_D        ; nope, coordinate space != window

$win_coords_loop_D:

    sub       vertexCount, 15            ; vertexCount >= 15 ? CF=0 : CF=1
    mov       ecx, [gc + vertexSize]     ; bytes of data for each vertex 

    sbb       eax, eax                   ; vertexCount >= 15 ? 00000000:ffffffff

    and       vertexCount, eax           ; vertexCount >= 15 ? 0 : vertexcount-15
    add       vertexCount, 15            ; vertexcount >= 15 ? 15 :vertexcount

    imul      ecx, vertexCount           ; total amount of vertex data we'll send

    mov       eax, [gc + fifoRoom]       ; fifo space available
    add       ecx, 4                     ; add header size ==> total packet size

    cmp       eax, ecx                   ; fifo space avail >= packet size ?
    jge       $win_tri_begin_D           ; yup, start writing triangle data

    push      @Line                      ; line number inside this function
    push      0h                         ; pointer to function name = NULL

    push      ecx                        ; fifo space needed
    call      __FifoMakeRoom             ; note: updates fifoPtr

    add       esp, 12                    ; remove 3 DWORD arguments from stack
    nop                                  ; filler

$win_tri_begin_D:

    mov       eax, vertexCount           ; number of vertices in triangles
    mov       fifo, [gc + fifoPtr]       ; get fifoPtr

    mov       ebp, [gc + cullStripHdr]   ; <2:0> = type
    shl       eax, 6                     ; <9:6> = vertex count (max 15)

    or        eax, ebp                   ; setup mode, vertex count, and type
    mov       ebp, 4                     ; test bit 2

    lea       dlpStart, [gc+tsuDataList] ; pointer to start of offset list
    test      fifo, ebp                  ; fifoPtr QWORD aligned ?

    nop                                  ; filler
    jz        $fifo_aligned_D            ; yup

    mov       [fifo], eax                ; PCI write packet type
    add       fifo, 4                    ; fifo pointer now QWORD aligned

$win_vertex_loop_D_WB0:                  ; nothing in "write buffer"

    mov       edx, [vertexPtr]           ; dereference pointer, edx points to vertex
    add       vertexPtr, 4               ; next pointer

    lea       dlp, [gc + tsuDataList]    ; get pointer to offset list
    movq      mm1, [edx]                 ; get vertex x,y

    mov       eax, [dlp]                 ; get first offset from offset list
    add       dlp, 4                     ; dlp++

    movq      [fifo], mm1                ; PCI write x, y
    add       fifo, 8                    ; fifo += 2

    test      eax, eax                   ; if offset == 0, end of offset list
    je        $win_datalist_end_D_WB0    ; no more vertex data, nothing in "write buffer" 

$win_datalist_loop_D_WB0:                ; nothing in "write buffer"

    movd      mm1, [edx + eax]           ; get next parameter
    mov       eax, [dlp]                 ; get next offset from offset list

    test      eax, eax                   ; at end of offset list (offset == 0) ?
    jz        $win_datalist_end_D_WB1    ; exit, write buffer contains one DWORD

    movd      mm2, [edx + eax]           ; get next parameter
    add       dlp, 8                     ; dlp++

    mov       eax, [dlp-4]               ; get next offset from offset list
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxU32)

    punpckldq mm1, mm2                   ; current param | previous param
    cmp       eax, 0                     ; at end of offset list (offset == 0) ?

    movq      [fifo-8], mm1              ; PCI write current param | previous param
    jnz       $win_datalist_loop_D_WB0   ; nope, copy next parameter

$win_datalist_end_D_WB0:

    dec       vertexCount                ; another vertex done. Any left?
    jnz       $win_vertex_loop_D_WB0     ; yup, output next vertex

$win_vertex_end_D_WB0:

    mov       eax, [gc + fifoPtr]        ; old fifoPtr
    mov       ebp, [gc + fifoRoom]       ; old number of bytes available in fifo

    mov       vertexCount, [esp + _count]; remaining vertices before previous loop
    sub       eax, fifo                  ; old fifoPtr - new fifoPtr (fifo room used)

    add       ebp, eax                   ; new number of bytes available in fifo
    mov       [gc + fifoPtr], fifo       ; save current fifoPtr 

    sub       vertexCount, 15            ; remaining number of vertices to process
    nop                                  ; filler

    mov       [gc + fifoRoom], ebp       ; save current number of bytes available in fifo
    test      vertexCount, vertexCount   ; any vertices left to process ?

    mov       [esp + _count], vertexCount; remaining number of vertices to process
    jg        $win_coords_loop_D         ; loop if number of vertices to process >= 0

    femms                                ; no more MMX code; clear MMX/FPU state

    pop       ebp                        ; restore frame pointer
    pop       ebx                        ; restore caller's register variable

    pop       esi                        ; restore caller's register variable
    pop       edi                        ; restore caller's register variable

    ret       12                         ; return, pop 3 DWORD parameters off stack

$fifo_aligned_D:

    movd      mm1, eax                   ; move header into "write buffer"

$win_vertex_loop_D_WB1:                  ; one DWORD in "write buffer"

    mov       edx, [vertexPtr]           ; dereference pointer, edx points to vertex
    add       vertexPtr, 4               ; next pointer

    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxU32)
    lea       dlp, [gc + tsuDataList]    ; get pointer to start of offset list

    movd      mm2, [edx]                 ; 0 | x of vertex
    add       dlp, 4                     ; dlp++
 
    mov       eax, [dlp-4]               ; first offset in offset list
    punpckldq mm1, mm2                   ; packet header | x of vertex

    movq      [fifo-8], mm1              ; PCI write packet header | x of vertex
    movd      mm1, [edx + 4]             ; 0 | y of vertex

    cmp       eax, 0                     ; offset == 0 (list empty) ?
    je        $win_datalist_end_D_WB1    ; yup, no more vertex data, one DWORD in "write buffer"

$win_datalist_loop_D_WB1:                ; one DWORD in "write buffer" = MM1

    movd      mm2, [edx + eax]           ; get next parameter
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxU32)

    mov       eax, [dlp]                 ; get next offset from offset list
    add       dlp, 8                     ; dlp += 2

    punpckldq mm1, mm2                   ; current param | previous param
    test      eax, eax                   ; at end of offset list (offset == 0) ?

    movq      [fifo-8], mm1              ; PCI write current param | previous param
    jz        $win_datalist_end_D_WB0    ; yes, exit, "write buffer" empty

    movd      mm1, [edx + eax]           ; get next parameter
    mov       eax, [dlp-4]               ; get next offset from offset list

    test      eax,  eax                  ; at end of offset list (offset == 0) ?
    jnz       $win_datalist_loop_D_WB1   ; nope, copy next parameter

$win_datalist_end_D_WB1:

    dec       vertexCount                ; another vertex done. Any left?
    jnz       $win_vertex_loop_D_WB1     ; yup, output next vertex

$win_vertex_end_D_WB1:

    movd      [fifo], mm1                ; flush "write buffer"
    mov       eax, [gc + fifoPtr]        ; old fifoPtr

    add       fifo, 4                    ; fifoPtr++
    mov       ebp, [gc + fifoRoom]       ; old number of bytes available in fifo

    mov       vertexCount, [esp + _count]; remaining vertices before previous loop
    sub       eax, fifo                  ; old fifoPtr - new fifoPtr (fifo room used)

    mov       [gc + fifoPtr], fifo       ; save current fifoPtr 
    sub       vertexCount, 15            ; remaining number of vertices to process

    add       ebp, eax                   ; new number of bytes available in fifo
    mov       [gc + fifoRoom], ebp       ; save current number of bytes available in fifo

    mov       [esp + _count], vertexCount; remaining number of vertices to process 
    cmp       vertexCount, 0             ; any vertices left to process ?

    nop                                  ; filler
    jg        $win_coords_loop_D         ; loop if number of vertices to process >= 0

    femms                                ; no more MMX code; clear MMX/FPU state

    pop       ebp                        ; restore frame pointer
    pop       ebx                        ; restore caller's register variable

    pop       esi                        ; restore caller's register variable
    pop       edi                        ; restore caller's register variable

    ret       12                         ; return, pop 3 DWORD parameters off stack

    ALIGN     32

; 989  :   }
; 990  :   else {
; 991  :     /*
; 992  :      * first cut of clip space coordinate code, no optimization.
; 993  :      */
; 994  :     float oow;
; 995  :     
; 996  :     while (count > 0) {
; 997  :       FxI32 vcount = count >= 15 ? 15 : count;
; 998  :       
; 999  :       GR_SET_EXPECTED_SIZE(vcount * gc->state.vData.vSize, 1);
; 1000 :       TRI_STRIP_BEGIN(kSetupStrip, vcount, gc->state.vData.vSize, SSTCP_PKT3_BDDBDD);
; 1001 :       
; 1002 :       for (k = 0; k < vcount; k++) {
; 1003 :         vPtr = pointers;
; 1004 :         if (mode)
; 1005 :           vPtr = *(float **)pointers;
; 1006 :         oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset);
; 1007 :         
; 1008 :         /* x, y */
; 1009 :         TRI_SETF(FARRAY(vPtr, 0)
; 1010 :                  *oow*gc->state.Viewport.hwidth + gc->state.Viewport.ox);
; 1011 :         TRI_SETF(FARRAY(vPtr, 4)
; 1012 :                  *oow*gc->state.Viewport.hheight + gc->state.Viewport.oy);
; 1013 :         (float *)pointers += stride;
; 1014 :         
; 1015 :         TRI_VP_SETFS(vPtr,oow);
; 1016 :       }
; 1017 :       TRI_END;

ifndef GLIDE3_SCALER

$clip_coordinates_D:

    mov       [strideinbytes], 4         ; unit stride for array of pointers to vertices

$clip_coordinates_ND:

dataElem      textequ <ebp>              ; number of vertex components processed

    movd      mm6, [__GlideRoot+pool_f255]      ; GlideRoot.pool.f255 

$clip_coords_begin:

;;;     }
;;;     else {
;;;     /*
;;;      * first cut of clip space coordinate code, no optimization.
;;;      */
;;;     float oow;
;;;       while (count > 0) {
;;;       FxI32 k, vcount = count >= 15 ? 15 : count;

    sub       vertexCount, 15            ; vertexCount >= 15 ? CF=0 : CF=1
    mov       ecx, [gc + vertexSize]     ; bytes of data for each vertex 

    sbb       eax, eax                   ; vertexCount >= 15 ? 00000000:ffffffff

    and       vertexCount, eax           ; vertexCount >= 15 ? 0 : vertexcount-15
    add       vertexCount, 15            ; vertexcount >= 15 ? 15 :vertexcount

    imul      ecx, vertexCount           ; total amount of vertex data we'll send

    mov       eax, [gc + fifoRoom]       ; fifo space available
    add       ecx, 4                     ; add header size ==> total packet size

    cmp       eax, ecx                   ; fifo space avail >= packet size ?
    jge       $clip_tri_begin            ; yup, start writing triangle data

    push      @Line                      ; line number inside this function
    push      0h                         ; pointer to function name = NULL

    push      ecx                        ; fifo space needed
    call      __FifoMakeRoom             ; note: updates fifoPtr

    add       esp, 12                    ; remove 3 DWORD arguments from stack

$clip_tri_begin:

    mov       edx, vertexCount           ; number of vertices in triangles
    mov       fifo, [gc + fifoPtr]       ; get fifoPtr

    mov       ebp, [gc + cullStripHdr]   ; <2:0> = type
    shl       edx, 6                     ; <9:6> = vertex count (max 15)

    or        edx, ebp                   ; setup mode, vertex count, and type

    mov       [fifo], edx                ; PCI write packet type
    add       fifo, 4                    ; fifo pointer now QWORD aligned

$clip_for_begin:

    mov       edx, vertexPtr             ; vertex = vertexPtr (assume no-deref mode)
    mov       eax, [esp+_mode]           ; mode 0 = no deref, mode 1 = deref

    mov       [vertices], vertexCount    ; save numnber of vertices
    test      eax, eax                   ; deref mode ?

    mov       eax, [gc + wInfo_offset]   ; get offset of W into vertex struct
    jz        $clip_noderef              ; yup, no-deref mode

    mov       edx, [vertexPtr]           ; vertex = *vertexPtr

$clip_noderef:

    movd      mm0, [edx + eax]           ; 0 | W of current vertex
    pfrcp     mm1, mm0                   ; 0 | 1/W approx

    mov       ebp, [strideinbytes]       ; offset to next vertex/vertexPtr
    movq      mm2, [edx]                 ; y | x of current vertex

    pfrcpit1  mm0, mm1                   ; 0 | 1/W refine
    movq      mm3, [gc + vp_hwidth]      ; gc->state.Viewport.hheight | gc->state.Viewport.hwidth

    movq      mm4, [gc + vp_ox]          ; gc->state.Viewport.oy | gc->state.Viewport.ox
    add       vertexPtr, ebp             ; point to next vertex/VertexPtr

    pfrcpit2  mm0, mm1                   ; oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset
    mov       esi, [gc + paramIndex]     ; gc->state.paramIndex

    pfmul     mm2, mm3                   ; TRI_SETF(FARRAY(vPtr,0)*state.Viewport.hheight | TRI_SETF(FARRAY(vPtr,4)*state.Viewport.hwidth
    xor       dataElem, dataElem         ; dataElem = 0

    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)
    punpckldq mm0, mm0                   ; oow | oow

    pfmul     mm2, mm0                   ; TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.height | TRI_SETF(FARRAY(vPtr, 0)*oow*gc->state.Viewport.hwidth
    pfadd     mm2, mm4                   ; TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.hheight + gc->state.Viewport.oy) |

    test      esi, 3                     ; STATE_REQUIRES_IT_DRGB | STATE_REQUIRES_IT_ALPHA ?
    mov       eax, [gc + tsuDataList]    ; first entry from offset list

    movq      [fifo-8], mm2              ; PCI write transformed x, y
    jz        $clip_setup_ooz            ; nope, no color at all needed
  
    cmp       DWORD PTR [gc+colorType], 0; gc->state.vData.colorType == GR_FLOAT ?
    jne       $clip_setup_pargb          ; nope, packed ARGB format
  
    test      esi, 1                     ; STATE_REQUIRES_IT_DRGB ?
    jz        $clip_setup_a              ; no, but definitely A

    movd      mm2, [edx + eax]           ; 0 | r
    mov       eax, [gc + tsuDataList+4]  ; offset of g part of vertex data

    pfmul     mm2, mm6                   ; 0 | r * 255.0f
    movd      mm3, [edx + eax]           ; 0 | g

    mov       eax, [gc + tsuDataList + 8]; offset of b part of vertex data
    movd      [fifo], mm2                ; PCI write r*255

    pfmul     mm3, mm6                   ; 0 | g * 255.0f
    movd      mm2, [edx + eax]           ; 0 | b

    movd      [fifo+4], mm3              ; PCI write g*255
    mov       dataElem, 12               ; dataElem = 3

    pfmul     mm2, mm6                   ; 0 | b * 255.0f
    mov       eax, [gc + tsuDataList+12] ; offset of A part of vertex data

    test      esi, 2                     ; STATE_REQUIRES_IT_ALPHA ?
    lea       fifo, [fifo+12]            ; fifoPtr += 3*sizeof(FxFloat)

    movd      [fifo-4], mm2              ; PCI write b*255
    jz        $clip_setup_ooz            ; nope, no alpha, proceeed with ooz

$clip_setup_a:
    movd      mm2, [eax+edx]             ; 0 | a
    add       fifo, 4                    ; fifoPtr += sizeof(FxFloat)

    mov       esp, esp                   ; filler
    add       dataElem, 4                ; dataElem++ 

    pfmul     mm2, mm6                   ; 0 | a * 255.0f
    mov       eax, [gc+dataElem+tsuDataList]; offset of next part of vertex data

    movd      [fifo-4], mm2              ; PCI write a*255
    jmp       $clip_setup_ooz            ; check whether we need to push out z

$clip_setup_pargb:
    movd      mm2, [eax+edx]             ; get packed ARGB data
    add       fifo, 4                    ; fifoPtr += sizeof(FxU32)

    mov       dataElem, 4                ; dataElem = 1 (namely pargb)
    mov       eax, [gc+tsuDataList+4]    ; offset of next part of vertex data

    movd      [fifo-4], mm2              ; PCI write packed ARGB
    nop                                  ; filler

$clip_setup_ooz:
  
    test      esi, 4                     ; STATE_REQUIRES_OOZ ?
    jz        $clip_setup_qow            ; nope

    movd      mm2, [eax + edx]           ; 0 | z component of vertex
    add       fifo, 4                    ; fifoPtr += sizeof(FxFloat)

    add       dataElem, 4                ; dataElem += 1
    movd      mm3, [gc + vp_hdepth]      ; 0 | gc->state.Viewport.hdepth

    pfmul     mm2, mm0                   ; TRI_SETF(FARRAY(_s, i)*_oow
    movd      mm4, [gc + vp_oz]          ; 0 | gc->state.Viewport.oz

    pfmul     mm2, mm3                   ; 0 | TRI_SETF(FARRAY(_s, i)*_oow*gc->state.Viewport.hdepth
    mov       eax, [gc+dataElem+tsuDataList]; offset of next vertex component

    pfadd     mm2, mm4                   ; 0 | TRI_SETF(FARRAY(_s, i)*_oow*gc->state.Viewport.hdepth+gc->state.Viewport.oz
    movd      [fifo-4], mm2              ; PCI write transformed Z

$clip_setup_qow:
    test      esi, 8                     ; STATE_REQUIRES_OOW_FBI ?
    jz        $clip_setup_qow0           ; nope

    cmp       DWORD PTR [gc+qInfo_mode],0; does vertex have Q component ?
    je        $clip_setup_oow            ; nope, not Q but W

    mov       eax, [gc + qInfo_offset]   ; offset of Q component of vertex
    add       fifo, 4                    ; fifoPtr += sizeof(FxFloat)

    add       dataElem, 4                ; dataElem++
    movd      mm2, [edx + eax]           ; 0 | q of vertex

    mov       eax,[gc+dataElem+tsuDataList]; pointer to next vertex component
    pfmul     mm2, mm0                   ; q*oow

    movd      [fifo-4], mm2              ; PCI write transformed Q
    jmp       $clip_setup_qow0           ; continue with q0

$clip_setup_oow:
    add       fifo, 4                    ; fifoPtr += sizeof(FxFloat) 
    add       dataElem, 4                ; dataElem++

    movd      [fifo-4], mm0              ; PCI write oow
    mov       eax,[gc+dataElem+tsuDataList]; pointer to next vertex component

$clip_setup_qow0:
    test      esi, 16                    ; STATE_REQUIRES_W_TMU0 ?
    jz        $clip_setup_stow0          ; nope 

    cmp       DWORD PTR [gc+q0Info_mode],0; does vertex have Q component ?
    je        $clip_setup_oow0           ; nope, not Q but W

    mov       eax, [gc+q0Info_offset]    ; offset of Q component of vertex
    add       fifo, 4                    ; fifoPtr += sizeof(FxFloat)

    add       dataElem, 4                ; dataElem++
    movd      mm2, [edx+eax]             ; 0 | q0 of vertex

    mov       eax,[gc+dataElem+tsuDataList]; pointer to next vertex component
    pfmul     mm2, mm0                   ; q0*oow

    movd      [fifo-4], mm2              ; PCI write transformed q0
    jmp       $clip_setup_stow0          ; continue with stow0

    nop                                  ; filler

$clip_setup_oow0:
    add       fifo, 4                    ; fifoPtr += sizeof(FxFloat) 
    add       dataElem, 4                ; dataElem++

    movd      [fifo-4], mm0              ; PCI write oow
    mov       eax,[gc+dataElem+tsuDataList]; pointer to next vertex component

$clip_setup_stow0:

    test      esi, 32                    ; STATE_REQUIRES_ST_TMU0 ?
    jz        $clip_setup_qow1           ; nope

    movq      mm7, [gc + tmu0_s_scale]   ; state.tmu_config[0].t_scale | state.tmu_config[0].s_scale
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    movd      mm2, [edx + eax]           ; param1
    mov       eax,[gc+dataElem+tsuDataList+4];pointer to next vertex component

    pfmul     mm7, mm0                   ; oow*tmu0_t_scale | oow*tmu0_s_scale
    add       dataElem, 8                ; dataElem += 2

    movd      mm3, [edx + eax]           ; param2
    punpckldq mm2, mm3                   ; param2 | param1

    pfmul     mm2, mm7                   ; param2*oow*tmu0_t_scale | param1*oow*tmu0_s_scale
    nop                                  ; filler

    movq      [fifo-8], mm2              ; PCI write param2*oow*tmu0_t_scale | param1*oow*tmu0_s_scale
    mov       eax, [gc+dataElem+tsuDataList]; pointer to next vertex component

$clip_setup_qow1:
    test      esi, 64                    ; STATE_REQUIRES_W_TMU1 ?
    jz        $clip_setup_stow1          ; nope

    cmp       DWORD PTR [gc+q1Info_mode],0; does vertex have Q component ?
    je        $clip_setup_oow1           ; nope, not Q but W

    mov       eax, [gc+q1Info_offset]    ; offset of Q component of vertex
    add       fifo, 4                    ; fifoPtr += sizeof(FxFloat)

    add       dataElem, 4                ; dataElem++
    movd      mm2, [edx + eax]           ; 0 | q1 of vertex

    mov       eax,[gc+dataElem+tsuDataList]; pointer to next vertex component
    pfmul     mm2, mm0                  ; q1*oow

    movd      [fifo-4], mm2              ; PCI write transformed q1
    jmp       $clip_setup_stow1          ; continue with stow1

$clip_setup_oow1:
    add       fifo, 4                    ; fifoPtr += sizeof(FxFloat) 
    add       dataElem, 4                ; dataElem++

    movd      [fifo-4], mm0              ; PCI write oow
    mov       eax,[gc+dataElem+tsuDataList]; pointer to next vertex component

$clip_setup_stow1:

    test      esi, 128                   ; STATE_REQUIRES_ST_TMU1 ?
    mov       vertexCount, [vertices]    ; get number of vertices

    movq      mm7, [gc + tmu1_s_scale]   ; state.tmu_config[1].t_scale | state.tmu_config[1].s_scale
    jz        $clip_setup_end            ; nope

    movd      mm2, [edx + eax]           ; param1
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    mov       eax,[gc+dataElem+tsuDataList+4]; pointer to next vertex component
    pfmul     mm7, mm0                   ; oow*state.tmu_config[1].t_scale | oow*state.tmu_config[1].s_scale

    movd      mm3, [edx + eax]           ; param2
    punpckldq mm2, mm3                   ; param2 | param1

    pfmul     mm2, mm7                   ; param2*oow*state.tmu_config[1].t_scale | param1*oow*state.tmu_config[1].s_scale
    movq      [fifo-8], mm2              ; PCI write param2*oow*state.tmu_config[1].t_scale | param1*oow*state.tmu_config[1].s_scale

$clip_setup_end:

    dec       vertexCount                ; vcount--
    jnz       $clip_for_begin            ; until 

$clip_for_end:

    mov       eax, [gc + fifoPtr]        ; old fifoPtr
    mov       ebp, [gc + fifoRoom]       ; old number of bytes available in fifo

    mov       vertexCount, [esp + _count]; remaining vertices before previous loop
    sub       eax, fifo                  ; old fifoPtr - new fifoPtr (fifo room used)

    mov       [gc + fifoPtr], fifo       ; save current fifoPtr 
    add       ebp, eax                   ; new number of bytes available in fifo

    sub       vertexCount, 15            ; remaining number of vertices to process
    mov       [gc + fifoRoom], ebp       ; save current number of bytes available in fifo

    mov       [esp + _count], vertexCount; remaining number of vertices to process 
    cmp       vertexCount, 0             ; any vertices left to process ?

    jg        $clip_coords_begin         ; loop if number of vertices to process >= 0

    femms                                ; no more MMX code; clear MMX/FPU state

else ; GLIDE3_SCALER

;----------------------------------------------------------------------------



;----------------------------------------------------------------------------
; table based method

;----------------------------------------------------------------------------



;----------------------------------------------------------------------------

dataElem      textequ <ebp>              ; number of vertex components processed

    nop                                  ; filler for code alignment

$clip_coordinates_ND:

    pxor      mm0, mm0                   ; load 0
    movd      mm1, [__GlideRoot+pool_f255]; GlideRoot.pool.f255 

    movd      [atab], mm0                ; atable[0] = 0.0f
    movd      mm5, [gc + vp_oz]          ; gc->state.Viewport.oz

    movq      mm2, [gc + vp_hwidth]      ; gc->state.Viewport.hheight | gc->state.Viewport.hwidth
    movd      [atab+8], mm1              ; atable[2] = GlideRoot.pool.f255

    movq      mm1, [gc + vp_ox]          ; gc->state.Viewport.oy | gc->state.Viewport.ox
    movd      [btab+4], mm5              ; btable[1] = gc->state.Viewport.oz

    movq      QWORD PTR [btab+8], mm0    ; btable[3] = 0.0f | btable[2] = 0.0f
    nop                                  ; filler

    movq      QWORD PTR [btab+16], mm0   ; btable[5] = 0.0f | btable[4] = 0.0f
    movq      QWORD PTR [btab+24], mm0   ; btable[7] = 0.0f | btable[6] = 0.0f

$clip_coords_begin_ND:

    sub       vertexCount, 15            ; vertexCount >= 15 ? CF=0 : CF=1
    mov       ecx, [gc + vertexSize]     ; bytes of data for each vertex 

    sbb       eax, eax                   ; vertexCount >= 15 ? 00000000:ffffffff

    and       vertexCount, eax           ; vertexCount >= 15 ? 0 : vertexcount-15
    add       vertexCount, 15            ; vertexcount >= 15 ? 15 :vertexcount

    imul      ecx, vertexCount           ; total amount of vertex data we'll send

    mov       eax, [gc + fifoRoom]       ; fifo space available
    add       ecx, 4                     ; add header size ==> total packet size

    cmp       eax, ecx                   ; fifo space avail >= packet size ?
    jge       $clip_tri_begin_ND         ; yup, start writing triangle data

    push      @Line                      ; line number inside this function
    push      0h                         ; pointer to function name = NULL

    push      ecx                        ; fifo space needed
    call      __FifoMakeRoom             ; note: updates fifoPtr

    add       esp, 12                    ; remove 3 DWORD arguments from stack
    nop                                  ; filler for code alignment

$clip_tri_begin_ND:

    mov       eax, vertexCount           ; number of vertices in triangles
    mov       fifo, [gc + fifoPtr]       ; get fifoPtr

    mov       ebp, [gc + cullStripHdr]   ; <2:0> = type
    shl       eax, 6                     ; <9:6> = vertex count (max 15)

    or        eax, ebp                   ; setup vertex count and type
    mov       ebp, 4                     ; test bit 2

    test      fifo, ebp                  ; fifoPtr QWORD aligned ?

    mov       edx, [gc + wInfo_offset]   ; gc->state.vData.wInfo.offset
    jz        $clip_fifo_aligned_ND      ; yup

    mov       [fifo], eax                ; PCI write packet type
    add       fifo, 4                    ; fifo pointer now QWORD aligned

$clip_for_begin_WB0_ND:                  ; "write buffer" = MM7 is empty

    ;; here:  ebx = vertex
    ;;        ecx = fifo
    ;;        edx = gc->state.vData.wInfo.offset
    ;;        esi = vertexCount
    ;;        edi = gc
    ;;        mm1 = gc->state.Viewport.oy|gc->state.Viewport.ox
    ;;        mm2 = gc->state.Viewport.hheight|gc->state.Viewport.hwidth
    ;; avail: eax, ebp, mm0, mm3, mm4, mm5, mm6, mm7

    movd      mm0, [vertex + edx]        ; 0 | W of current vertex
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    movq      mm7, [vertex]              ; y | x of current vertex
    mov       dataElem, 0                ; dataElem = 0

    pfrcp     mm4, mm0                   ; 1/W approx | 1/W approx
    punpckldq mm0, mm0                   ; W | W

    pfrcpit1  mm0, mm4                   ; 1/W refine | 1/W refine
    mov       eax, [gc + colorType]      ; gc->state.vData.colorType

    pfmul     mm7, mm2                   ; TRI_SETF(FARRAY(vPtr,0)*state.Viewport.hheight | TRI_SETF(FARRAY(vPtr,4)*state.Viewport.hwidth
    movd      mm6, [gc + vp_hdepth]      ; gc->state.Viewport.hdepth

    pfrcpit2  mm0, mm4                   ; oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset | oow = ...
    cmp       eax, 1                     ; gc->state.vData.colorType == GR_U8

    mov       eax, [gc + tsuDataList]    ; get offset of first component from offset list
    pfmul     mm7, mm0                   ; TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.height | TRI_SETF(FARRAY(vPtr, 0)*oow*gc->state.Viewport.hwidth

    movd      [atab+28], mm0             ; atable[7] = oow
    pfmul     mm6, mm0                   ; oow*gc->state.Viewport.hdepth

    movq      mm3, [gc + tmu0_s_scale]   ; gc->state.tmu_config[0].t_scale | gc->state.tmu_config[0].s_scale
    movd      [btab], mm0                ; btable[0] = oow

    movd      [atab+4], mm6              ; atable[1] = oow*gc->state.Viewport.hdepth
    pfadd     mm7, mm1                   ; TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.hheight + gc->state.Viewport.oy) |

    pfmul     mm3, mm0                   ; oow*gc->state.tmu_config[0].t_scale | oow*gc->state.tmu_config[0].s_scale
    movq      mm4, [gc + tmu1_s_scale]   ; gc->state.tmu_config[1].t_scale | gc->state.tmu_config[1].s_scale

    movq      [fifo-8], mm7              ; PCI write transformed x, y; write buffer=mm7 empty
    nop                                  ; filler

    movq      QWORD PTR [atab+12], mm3   ; atable[4] = oow*gc->state.tmu_config[0].t_scale | atable[3] = oow*gc->state.tmu_config[0].s_scale
    pfmul     mm4, mm0                   ; oow*gc->state.tmu_config[1].t_scale | oow*gc->state.tmu_config[1].s_scale

    movq      QWORD PTR [atab+20], mm4   ; atable[6] = oow*gc->state.tmu_config[1].t_scale | atable[5] = oow*gc->state.tmu_config[1].s_scale
    jne       $not_pargb_WB0_ND          ; nope, gc->state.vData.colorType != GR_U8

    movd      mm7, [vertex + eax]        ; get packed ARGB data; 1 DWORD in "write buffer"=mm7
    nop                                  ; filler

    mov       eax, [gc + tsuDataList + 4]; get offset of next vertex component (after pargb)
    add       dataElem, 4                ; dataElem = 1 (namely pargb)

$not_pargb_WB1_ND:

    test      eax, eax                   ; end of offset list ?
    jz        $clip_setup_end_WB1_ND     ; yup

    ;; here:  eax = offset into vertex
    ;;        ebx = vertex
    ;;        ecx = fifo
    ;;        esi = vertexCount
    ;;        edi = gc
    ;;        ebp = dataElem
    ;;        mm1 = gc->state.Viewport.oy|gc->state.Viewport.ox
    ;;        mm2 = gc->state.Viewport.hheight|gc->state.Viewport.hwidth
    ;;        mm7 = write buffer
    ;; avail: edx, mm0, mm3, mm4, mm5, mm6

$inner_loop_WB1_ND:
    mov       edx, [gc+dataElem+tsuDatLstScal]; iscaler
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    movd      mm4, [vertex + eax]        ; TRI_SETF(FARRAY(vPtr, i)
    movd      mm6, atab[edx*4]           ; atable [iscaler]

    movd      mm5, btab[edx*4]           ; btable [iscaler]
    pfmul     mm4, mm6                   ; TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]

    mov       eax, [gc+dataElem+tsuDataList+4]; next offset from offset list
    pfadd     mm4, mm5                   ; TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]+btable [iscaler]

    punpckldq mm7, mm4                   ; current component | previous component
    test      eax, eax                   ; offset == 0 (end of offset list) ?

    mov       edx, [gc+dataElem+tsuDatLstScal+4]; next iscaler
    movq      [fifo-8], mm7              ; PCI write  new param | previous param

    jz        $clip_setup_end_WB0_ND     ; yup, end of offset list, this vertex done
    add       dataElem, 8                ; dataElem++

    movd      mm7, [vertex+eax]          ; TRI_SETF(FARRAY(vPtr, i)
    movd      mm6, atab[edx*4]           ; atable [iscaler]

    movd      mm5, btab[edx*4]           ; btable [iscaler]
    pfmul     mm7, mm6                   ; TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]

    mov       eax, [gc+dataElem+tsuDataList]; next offset from offset list
    pfadd     mm7, mm5                   ; TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]+btable [iscaler]

    cmp       eax, 0                     ; offset == 0 (end of offset list) ?
    jnz       $inner_loop_WB1_ND         ; nope, get next component (1 DWORD in "write buffer")

$clip_setup_end_WB1_ND:

    mov       eax, [strideinbytes]       ; offset to next vertex
    nop                                  ; filler

    mov       edx, [gc + wInfo_offset]   ; gc->state.vData.wInfo.offset
    dec       vertexCount                ; one less vertex to handle

    lea       vertex, [vertex + eax]     ; points to next vertex
    jnz       $clip_for_begin_WB1_ND     ; until all vertices done; 1 DWORD in "write buffer" = MM7

    movd      [fifo], mm7                ; flush "write buffer"
    add       fifo, 4                    ; fifoPtr += sizeof(FxFloat)

    mov       esp, esp                   ; filler
    jmp       $clip_setup_done_ND        ; all vertices handled, tri strip done

$clip_fifo_aligned_ND:                       

    movd      mm7, eax                   ; write buffer has 1 DWORD now

;;;     for (k = 0; k < vcount; k++) {
;;;       float *vPtr
;;;       vPtr = pointers
;;;       oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset)
;;;       /* x, y */
;;;       TRI_SETF(FARRAY(vPtr, 0)
;;;         *oow*gc->state.Viewport.hwidth + gc->state.Viewport.ox)
;;;       TRI_SETF(FARRAY(vPtr, 4)
;;;         *oow*gc->state.Viewport.hheight + gc->state.Viewport.oy)
  
$clip_for_begin_WB1_ND:

    ;; here:  ebx = vertex
    ;;        ecx = fifo
    ;;        edx = gc->state.vData.wInfo.offset
    ;;        esi = vertexCount
    ;;        edi = gc
    ;;        mm1 = gc->state.Viewport.oy|gc->state.Viewport.ox
    ;;        mm2 = gc->state.Viewport.hheight|gc->state.Viewport.hwidth
    ;;        mm7 = write buffer
    ;; avail: eax, ebp, mm0, mm3, mm4, mm5, mm6

    mov       dataElem, 0                ; dataElem = 0
    movd      mm0, [vertex + edx]        ; 0 | W of current vertex

    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)
    movq      mm5, [vertex]              ; y | x of current vertex

    pfrcp     mm4, mm0                   ; 1/W approx | 1/W approx
    mov       eax, [gc + colorType]      ; gc->state.vData.colorType

    punpckldq mm0, mm0                   ; W | W
    movd      mm6, [gc + vp_hdepth]      ; gc->state.Viewport.hdepth

    cmp       eax, 1                     ; gc->state.vData.colorType == GR_U8
    pfrcpit1  mm0, mm4                   ; 1/W refine | 1/W refine

    pfmul     mm5, mm2                   ; TRI_SETF(FARRAY(vPtr,0)*state.Viewport.hheight | TRI_SETF(FARRAY(vPtr,4)*state.Viewport.hwidth
    movq      mm3, [gc + tmu0_s_scale]   ; gc->state.tmu_config[0].t_scale | gc->state.tmu_config[0].s_scale

    pfrcpit2  mm0, mm4                   ; oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset | oow = ...
    mov       eax, [gc + tsuDataList]    ; get offset of first component from offset list

    pfmul     mm5, mm0                   ; TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.height | TRI_SETF(FARRAY(vPtr, 0)*oow*gc->state.Viewport.hwidth
    movd      [atab+28], mm0             ; atable[7] = oow

    pfmul     mm6, mm0                   ; oow*gc->state.Viewport.hdepth
    movd      [btab], mm0                ; btable[0] = oow

    pfmul     mm3, mm0                   ; oow*gc->state.tmu_config[0].t_scale | oow*gc->state.tmu_config[0].s_scale
    movd      [atab+4], mm6              ; atable[1] = oow*gc->state.Viewport.hdepth

    pfadd     mm5, mm1                   ; TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.hheight + gc->state.Viewport.oy) |
    movq      QWORD PTR [atab+12], mm3   ; atable[4] = oow * gc->state.tmu_config[0].t_scale | atable[3] = oow * gc->state.tmu_config[0].s_scale
 
    movq      mm3, [gc + tmu1_s_scale]   ; gc->state.tmu_config[1].t_scale | gc->state.tmu_config[1].s_scale
    punpckldq mm7, mm5                   ; header | transformed x

    punpckhdq mm5, mm5                   ; transformed y | transformed y
    movq      [fifo-8], mm7              ; PCI write header | transformed x

    pfmul     mm3, mm0                   ; oow*gc->state.tmu_config[1].t_scale | oow*gc->state.tmu_config[1].s_scale
    movq      mm7, mm5                   ; 1 DWORD in "write buffer" (transformed y)

    movq      QWORD PTR [atab+20], mm3   ; atable[6] = oow * gc->state.tmu_config[1].t_scale | oow * gc->state.tmu_config[1].s_scale
    jne       $not_pargb_WB1_ND          ; nope, gc->state.vData.colorType != GR_U8

    movd      mm6, [vertex + eax]        ; get packed ARGB data;
    mov       dataElem, 4                ; dataElem = 1 (namely pargb)

    punpckldq mm7, mm6                   ; mm7 = pargb | transformed y
    mov       eax, [gc + tsuDataList + 4]; get offset of next vertex component

    movq      [fifo], mm7                ; PCI write pargb | transformed y
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

$not_pargb_WB0_ND:
    cmp       eax, 0                     ; end of data offset list ?
    jz        $clip_setup_end_WB0_ND     ; yup, this vertex done

    ;; here:  eax = offset into vertex
    ;;        ebx = vertex
    ;;        ecx = fifo
    ;;        esi = vertexCount
    ;;        edi = gc
    ;;        ebp = dataElem
    ;;        mm1 = gc->state.Viewport.oy|gc->state.Viewport.ox
    ;;        mm2 = gc->state.Viewport.hheight|gc->state.Viewport.hwidth
    ;;        mm3 = F256_F256
    ;; avail: edx, mm0, mm4, mm5, mm6, mm7

$inner_loop_WB0_ND:

    mov       edx, [gc+dataElem+tsuDatLstScal]; iscaler
    movd      mm7, [vertex + eax]        ; TRI_SETF(FARRAY(vPtr, i)

    movd      mm6, atab[edx*4]           ; atable[iscaler]
    movd      mm5, btab[edx*4]           ; btable[iscaler]

    pfmul     mm7, mm6                   ; TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]
    mov       eax, [gc+dataElem+tsuDataList+4] ; next offset from offset list

    pfadd     mm7, mm5                   ; TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]+btable[iscaler]
    mov       edx, [gc+dataElem+tsuDatLstScal+4]; next iscaler

    cmp       eax, 0                     ; offset == 0 (end of offset list) ?
    jz        $clip_setup_end_WB1_ND     ; yup, vertex done, 1 DWORD in "write buffer"

    movd      mm4, [vertex + eax]        ; TRI_SETF(FARRAY(vPtr, i)
    movd      mm6, atab[edx*4]           ; atable[iscaler]

    movd      mm5, btab[edx*4]           ; btable[iscaler]
    add       dataElem, 8                ; dataElem += 2

    add       fifo, 8                    ; fifoPtr += 2*sizeof*FxFloat)
    pfmul     mm4, mm6                   ; TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]

    nop                                  ; filler
    mov       eax, [gc+dataElem+tsuDataList]; get next offset from offset list

    pfadd     mm4, mm5                   ; TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]+btable[iscaler]
    test      eax, eax                   ; offset == 0 (end of offset list) ? 

    punpckldq mm7, mm4                   ; current component | previous component
    nop                                  ; filler

    movq      [fifo-8], mm7              ; PCI write current component | previous component
    jnz       $inner_loop_WB0_ND         ; nope, vertex not done yet; "write buffer" empty

$clip_setup_end_WB0_ND:

    mov       eax, [strideinbytes]       ; offset to next vertex
    mov       esp, esp                   ; filler

    mov       edx, [gc + wInfo_offset]   ; gc->state.vData.wInfo.offset
    sub       vertexCount, 1             ; one less vertex to process, any left?

    lea       vertex, [vertex + eax]     ; points to next vertex
    jnz       $clip_for_begin_WB0_ND     ; yup, vertices left, push out next vertex

$clip_setup_done_ND:
    mov       eax, [gc + fifoPtr]        ; old fifoPtr
    mov       ebp, [gc + fifoRoom]       ; old number of bytes available in fifo

    mov       vertexCount, [esp + _count]; remaining vertices before previous loop
    sub       eax, fifo                  ; old fifoPtr - new fifoPtr (fifo room used)

    mov       [gc + fifoPtr], fifo       ; save current fifoPtr 
    add       ebp, eax                   ; new number of bytes available in fifo

    sub       vertexCount, 15            ; remaining number of vertices to process
    mov       [gc + fifoRoom], ebp       ; save current number of bytes available in fifo

    mov       [esp + _count], vertexCount; remaining number of vertices to process 
    cmp       vertexCount, 0             ; any vertices left to process ?

    nop                                  ; filler
    jg        $clip_coords_begin_ND      ; loop if number of vertices to process >= 0

    femms                                ; no more MMX code; clear MMX/FPU state

    pop       ebp                        ; restore frame pointer
    pop       ebx                        ; restore caller's register variable

    pop       esi                        ; restore caller's register variable
    pop       edi                        ; restore caller's register variable

    ret       12                         ; return, pop 5 DWORD parameters off stack


    ALIGN     32

    nop                                  ; filler for code alignment

$clip_coordinates_D:

    pxor      mm0, mm0                   ; load 0
    movd      mm1, [__GlideRoot+pool_f255]; GlideRoot.pool.f255 

    movd      [atab], mm0                ; atable[0] = 0.0f
    movd      mm5, [gc + vp_oz]          ; gc->state.Viewport.oz

    movq      QWORD PTR [btab+8], mm0    ; btable[3] = 0.0f | btable[2] = 0.0f
    movq      mm2, [gc + vp_hwidth]      ; gc->state.Viewport.hheight | gc->state.Viewport.hwidth

    movd      [atab+8], mm1              ; atable[2] = GlideRoot.pool.f255
    movq      mm1, [gc + vp_ox]          ; gc->state.Viewport.oy | gc->state.Viewport.ox

    movd      [btab+4], mm5              ; btable[1] = gc->state.Viewport.oz
    nop                                  ; filler

    movq      QWORD PTR [btab+16], mm0   ; btable[5] = 0.0f | btable[4] = 0.0f
    movq      QWORD PTR [btab+24], mm0   ; btable[7] = 0.0f | btable[6] = 0.0f

$clip_coords_begin_D:

    sub       vertexCount, 15            ; vertexCount >= 15 ? CF=0 : CF=1
    mov       ecx, [gc + vertexSize]     ; bytes of data for each vertex 

    sbb       eax, eax                   ; vertexCount >= 15 ? 00000000:ffffffff

    and       vertexCount, eax           ; vertexCount >= 15 ? 0 : vertexcount-15
    add       vertexCount, 15            ; vertexcount >= 15 ? 15: vertexcount

    imul      ecx, vertexCount           ; total amount of vertex data we'll send

    mov       eax, [gc + fifoRoom]       ; fifo space available
    add       ecx, 4                     ; add header size ==> total packet size

    cmp       eax, ecx                   ; fifo space avail >= packet size ?
    jge       $clip_tri_begin_D          ; yup, start writing triangle data

    push      @Line                      ; line number inside this function
    push      0h                         ; pointer to function name = NULL

    push      ecx                        ; fifo space needed
    call      __FifoMakeRoom             ; note: updates fifoPtr

    add       esp, 12                    ; remove 3 DWORD arguments from stack
    nop                                  ; filler

$clip_tri_begin_D:

    mov       edx, vertexCount           ; number of vertices in triangles
    mov       fifo, [gc + fifoPtr]       ; get fifoPtr

    mov       ebp, [gc + cullStripHdr]   ; <2:0> = type
    shl       edx, 6                     ; <9:6> = vertex count (max 15)

    or        edx, ebp                   ; setup vertex count and type
    mov       ebp, 4                     ; test bit 2

    test      fifo, ebp                  ; fifoPtr QWORD aligned ?

    mov       eax, [gc + wInfo_offset]   ; gc->state.vData.wInfo.offset
    jz        $clip_fifo_aligned_D       ; yup

    mov       [fifo], edx                ; PCI write packet type
    add       fifo, 4                    ; fifo pointer now QWORD aligned

$clip_for_begin_WB0_D :                  ; "write buffer" = MM7 is empty

    ;; here:  eax = gc->state.vData.wInfo.offset
    ;;        ebx = vertexPtr
    ;;        ecx = fifo
    ;;        eax = gc->state.vData.wInfo.offset
    ;;        esi = vertexCount
    ;;        edi = gc
    ;;        mm1 = gc->state.Viewport.oy|gc->state.Viewport.ox
    ;;        mm2 = gc->state.Viewport.hheight|gc->state.Viewport.hwidth
    ;; avail: edx, ebp, mm3, mm4, mm5, mm6, mm7

    mov       edx, [vertexPtr]           ; vertex = *vertexPtr
    add       vertexPtr, 4               ; next vertexPtr

    mov       dataElem, 0                ; dataElem = 0
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    movd      mm0, [edx + eax]           ; 0 | W of current vertex
    movq      mm7, [edx]                 ; y | x of current vertex

    pfrcp     mm4, mm0                   ; 1/W approx | 1/W approx
    mov       eax, [gc + colorType]      ; gc->state.vData.colorType

    punpckldq mm0, mm0                   ; W | W
    movd      mm6, [gc + vp_hdepth]      ; gc->state.Viewport.hdepth

    cmp       eax, 1                     ; gc->state.vData.colorType == GR_U8 
    pfrcpit1  mm0, mm4                   ; 1/W refine | 1/W refine

    pfmul     mm7, mm2                   ; TRI_SETF(FARRAY(vPtr,0)*state.Viewport.hheight | TRI_SETF(FARRAY(vPtr,4)*state.Viewport.hwidth
    mov       eax, [gc + tsuDataList]    ; get offset of first component from offset list

    pfrcpit2  mm0, mm4                   ; oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset
    mov       [vertices], vertexCount    ; save off vertex count 

    movq      mm3, [gc + tmu0_s_scale]   ; gc->state.tmu_config[0].t_scale | gc->state.tmu_config[0].s_scale
    movd      [atab+28], mm0             ; atable[7] = oow
    
    pfmul     mm7, mm0                   ; TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.height | TRI_SETF(FARRAY(vPtr, 0)*oow*gc->state.Viewport.hwidth
    movd      [btab], mm0                ; btable[0] = oow

    pfmul     mm6, mm0                   ; oow*gc->state.Viewport.hdepth
    movq      mm4, [gc + tmu1_s_scale]   ; gc->state.tmu_config[1].t_scale | gc->state.tmu_config[1].s_scale

    pfmul     mm3, mm0                   ; oow * gc->state.tmu_config[0].t_scale | oow * gc->state.tmu_config[0].s_scale
    pfadd     mm7, mm1                   ; TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.hheight + gc->state.Viewport.oy) |

    movd      [atab+4], mm6              ; atable[1] = oow*gc->state.Viewport.hdepth
    pfmul     mm4, mm0                   ; oow * gc->state.tmu_config[1].t_scale | oow * gc->state.tmu_config[1].s_scale

    movq      QWORD PTR [atab+12], mm3   ; atable[4] = oow * gc->state.tmu_config[0].t_scale | atable[3] = oow * gc->state.tmu_config[0].s_scale 
    movq      [fifo-8], mm7              ; PCI write transformed x, y; write buffer=mm7 empty

    movq      QWORD PTR [atab+20], mm4   ; atable[6] = oow * gc->state.tmu_config[1].t_scale | atable[5] = oow * gc->state.tmu_config[1].s_scale
    jne       $not_pargb_WB0_D           ; nope, gc->state.vData.colorType != GR_U8

    movd      mm7, [edx + eax]           ; get packed ARGB data; 1 DWORD in "write buffer"=mm7
    nop                                  ; filler

    mov       eax, [gc + tsuDataList + 4]; get offset of next vertex component (after pargb)
    add       dataElem, 4                ; dataElem = 1 (namely pargb)

$not_pargb_WB1_D:

    test      eax, eax                   ; end of offset list ?
    jz        $clip_setup_end_WB1_D      ; yup

    ;; here:  eax = offset into vertex
    ;;        ebx = vertexPtr
    ;;        ecx = fifo
    ;;        edx = vertex
    ;;        edi = gc
    ;;        ebp = dataElem
    ;;        mm1 = gc->state.Viewport.oy|gc->state.Viewport.ox
    ;;        mm2 = gc->state.Viewport.hheight|gc->state.Viewport.hwidth
    ;;        mm7 = write buffer
    ;; avail: eax, esi, mm0, mm3, mm4, mm5, mm6

$inner_loop_WB1_D:
    mov       esi, [gc+dataElem+tsuDatLstScal]; iscaler
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    movd      mm4, [edx + eax]           ; TRI_SETF(FARRAY(vPtr, i)
    movd      mm6, atab[esi*4]           ; atable [iscaler]

    movd      mm5, btab[esi*4]           ; btable [iscaler]
    pfmul     mm4, mm6                   ; TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]

    mov       eax, [gc+dataElem+tsuDataList+4]; next offset from offset list
    pfadd     mm4, mm5                   ; TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]+btable [iscaler]

    cmp       eax, 0                     ; offset == 0 (end of offset list) ?
    punpckldq mm7, mm4                   ; current component | previous component

    mov       esi, [gc+dataElem+tsuDatLstScal+4]; next iscaler
    movq      [fifo-8], mm7              ; PCI write  new param | previous param

    jz        $clip_setup_end_WB0_D      ; yup, end of offset list, this vertex done
    movd      mm7, [edx + eax]           ; TRI_SETF(FARRAY(vPtr, i)

    add       dataElem, 8                ; dataElem += 2
    movd      mm6, atab[esi*4]           ; atable [iscaler]

    movd      mm5, btab[esi*4]           ; btable [iscaler]
    pfmul     mm7, mm6                   ; TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]

    mov       eax, [gc+dataElem+tsuDataList]; next offset from offset list
    pfadd     mm7, mm5                   ; TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]+btable [iscaler]

    nop                                  ; filler
    test      eax, eax                   ; offset == 0 (end of offset list) ?

    jnz       $inner_loop_WB1_D          ; nope, get next component (1 DWORD in "write buffer")
    nop                                  ; filler

$clip_setup_end_WB1_D:
    mov       vertexCount, [vertices]    ; get back number of vertices left to process
    mov       eax, [gc + wInfo_offset]   ; gc->state.vData.wInfo.offset

    dec       vertexCount                ; one less vertex to handle
    jnz       $clip_for_begin_WB1_D      ; until all vertices done; 1 DWORD in "write buffer" = MM7

    movd      [fifo], mm7                ; flush "write buffer"
    add       fifo, 4                    ; fifoPtr += sizeof(FxFloat)

    jmp       $clip_setup_done_D         ; all vertices handled, triangles done
    nop                                  ; filler

$clip_fifo_aligned_D:                       

    movd      mm7, edx                   ; write buffer has 1 DWORD now

$clip_for_begin_WB1_D:

    ;; here:  eax = gc->state.vData.wInfo.offset
    ;;        ebx = vertexPtr
    ;;        ecx = fifo
    ;;        eax = gc->state.vData.wInfo.offset
    ;;        esi = vertexCount
    ;;        edi = gc
    ;;        mm1 = gc->state.Viewport.oy|gc->state.Viewport.ox
    ;;        mm2 = gc->state.Viewport.hheight|gc->state.Viewport.hwidth
    ;; avail: edx, ebp, mm0, mm3, mm4, mm5, mm6, mm7

    mov       edx, [vertexPtr]           ; vertex = *vertexPtr
    add       vertexPtr, 4               ; next vertexPtr

    mov       dataElem, 0                ; dataElem = 0
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

    movd      mm0, [edx + eax]           ; 0 | W of current vertex
    movq      mm5, [edx]                 ; y | x of current vertex

    pfrcp     mm4, mm0                   ; 1/W approx | 1/W approx
    mov       eax, [gc + colorType]      ; gc->state.vData.colorType

    punpckldq mm0, mm0                   ; W | W
    movd      mm6, [gc + vp_hdepth]      ; gc->state.Viewport.hdepth

    pfrcpit1  mm0, mm4                   ; 1/W refine | 1/W refine
    cmp       eax, 1                     ; gc->state.vData.colorType == GR_U8 

    pfmul     mm5, mm2                   ; TRI_SETF(FARRAY(vPtr,0)*state.Viewport.hheight | TRI_SETF(FARRAY(vPtr,4)*state.Viewport.hwidth
    mov       eax, [gc + tsuDataList]    ; get offset of first component from offset list

    pfrcpit2  mm0, mm4                   ; oow = 1.0f / FARRAY(vPtr, gc->state.vData.wInfo.offset
    mov       [vertices], vertexCount    ; save off vertex count 

    movq      mm3, [gc + tmu0_s_scale]   ; gc->state.tmu_config[0].t_scale | gc->state.tmu_config[0].s_scale 
    pfmul     mm5, mm0                   ; TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.height | TRI_SETF(FARRAY(vPtr, 0)*oow*gc->state.Viewport.hwidth

    movd      [atab+28], mm0             ; atable[7] = oow
    pfmul     mm6, mm0                   ; oow*gc->state.Viewport.hdepth

    movd      [btab], mm0                ; btable[0] = oow
    pfmul     mm3, mm0                   ; oow * gc->state.tmu_config[1].t_scale | oow * gc->state.tmu_config[1].s_scale

    pfadd     mm5, mm1                   ; TRI_SETF(FARRAY(vPtr, 4)*oow*gc->state.Viewport.hheight + gc->state.Viewport.oy) |
    movd      [atab+4], mm6              ; atable[1] = oow*gc->state.Viewport.hdepth

    movq      QWORD PTR [atab+12], mm3   ; atable[4] = oow * gc->state.tmu_config[0].t_scale | atable[3] = oow * gc->state.tmu_config[0].s_scale
    movq      mm3, [gc + tmu1_s_scale]   ; gc->state.tmu_config[1].t_scale | gc->state.tmu_config[1].s_scale

    punpckldq mm7, mm5                   ; header | transformed x
    punpckhdq mm5, mm5                   ; transformed y | transformed y

    pfmul     mm3, mm0                   ; oow * gc->state.tmu_config[1].t_scale | oow * gc->state.tmu_config[1].s_scale
    movq      [fifo-8], mm7              ; PCI write header | transformed x

    movq      mm7, mm5                   ; 1 DWORD in "write buffer" (transformed y)
    nop                                  ; filler

    movq      QWORD PTR [atab+20], mm3   ; atable[6] = oow * gc->state.tmu_config[1].t_scale | atable[5] = oow * gc->state.tmu_config[1].s_scale
    jne       $not_pargb_WB1_D           ; nope, gc->state.vData.colorType != GR_U8

    movd      mm6, [edx + eax]           ; get packed ARGB data;
    mov       dataElem, 4                ; dataElem = 1 (namely pargb)

    punpckldq mm7, mm6                   ; mm7 = pargb | transformed y
    mov       eax, [gc + tsuDataList + 4]; get offset of next vertex component

    movq      [fifo], mm7                ; PCI write pargb | transformed y
    add       fifo, 8                    ; fifoPtr += 2*sizeof(FxFloat)

$not_pargb_WB0_D:
    test      eax, eax                   ; end of data offset list ?
    jz        $clip_setup_end_WB0_D      ; yup, this vertex done

    ;; here:  eax = offset into vertex
    ;;        ebx = vertexPtr
    ;;        ecx = fifo
    ;;        edx = vertex
    ;;        edi = gc
    ;;        ebp = dataElem
    ;;        mm1 = gc->state.Viewport.oy|gc->state.Viewport.ox
    ;;        mm2 = gc->state.Viewport.hheight|gc->state.Viewport.hwidth
    ;; avail: eax, esi, mm0, mm3, mm4, mm5, mm6, mm7

$inner_loop_WB0_D:
    mov       esi, [gc+dataElem+tsuDatLstScal]; iscaler
    movd      mm7, [edx+eax]             ; TRI_SETF(FARRAY(vPtr, i)

    movd      mm6, atab[esi*4]           ; atable[iscaler]
    movd      mm5, btab[esi*4]           ; btable[iscaler]

    pfmul     mm7, mm6                   ; TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]
    mov       eax, [gc+dataElem+tsuDataList+4]

    pfadd     mm7, mm5                   ; TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]+btable[iscaler]
    mov       esi, [gc+dataElem+tsuDatLstScal+4]; next iscaler

    test      eax, eax                   ; offset == 0 (end of offset list) ?
    jz        $clip_setup_end_WB1_D      ; yup, vertex done, 1 DWORD in "write buffer"

    movd      mm4, [edx+eax]             ; TRI_SETF(FARRAY(vPtr, i)
    movd      mm6, atab[esi*4]           ; atable[iscaler]

    movd      mm5, btab[esi*4]           ; btable[iscaler]
    add       dataElem, 8                ; dataElem += 2

    add       fifo, 8                    ; fifoPtr += 2*sizeof*FxFloat)
    pfmul     mm4, mm6                   ; TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]

    mov       eax, [gc+dataElem+tsuDataList]; get next offset from offset list
    pfadd     mm4, mm5                   ; TRI_SETF(FARRAY(vPtr, i)*atable[iscaler]+btable[iscaler]

    cmp       eax, 0                     ; offset == 0 (end of offset list) ? 
    punpckldq mm7, mm4                   ; current component | previous component

    movq      [fifo-8], mm7              ; PCI write current component | previous component
    jnz       $inner_loop_WB0_D          ; nope, vertex not done yet; "write buffer" empty

$clip_setup_end_WB0_D:
    mov       vertexCount, [vertices]    ; get back number of vertices left to process
    mov       eax, [gc + wInfo_offset]   ; gc->state.vData.wInfo.offset

    dec       vertexCount                ; one less vertex to process, any left?
    jnz       $clip_for_begin_WB0_D      ; yup, vertices left, push out next vertex

$clip_setup_done_D:
    mov       eax, [gc + fifoPtr]        ; old fifoPtr
    mov       ebp, [gc + fifoRoom]       ; old number of bytes available in fifo

    mov       vertexCount, [esp + _count]; remaining vertices before previous loop
    sub       eax, fifo                  ; old fifoPtr - new fifoPtr (fifo room used)

    mov       [gc + fifoPtr], fifo       ; save current fifoPtr 
    add       ebp, eax                   ; new number of bytes available in fifo

    sub       vertexCount, 15            ; remaining number of vertices to process
    nop                                  ; filler

    mov       [gc + fifoRoom], ebp       ; save current number of bytes available in fifo
    mov       [esp + _count], vertexCount; remaining number of vertices to process 

    cmp       vertexCount, 0             ; any vertices left to process
    jg        $clip_coords_begin_D       ; loop if number of vertices to process >= 0

    femms                                ; no more MMX code; clear MMX/FPU state

endif ; GLIDE3_SCALER

$tris_done:
    pop       ebp                        ; restore frame pointer
    pop       ebx                        ; restore caller's register variable

    pop       esi                        ; restore caller's register variable
    pop       edi                        ; restore caller's register variable

    ret       12                         ; return, pop 3 DWORD parameters
__grDrawTriangles_3DNow@12 ENDP        

_TEXT   ENDS

;;--------------------------------------------------------------------------
;; end AMD3D version
;;--------------------------------------------------------------------------
endif ; GL_AMD3D

;;--------------------------------------------------------------------------
;; start original code
;;--------------------------------------------------------------------------

ifndef GL_AMD3D

TITLE   xdraw3.asm
.586P
;;; include listing.inc
INCLUDE fxgasm.h

.model FLAT,C                   ; Flat memory, mangle publics with leading '_'

EXTRN   _GlideRoot:DWORD
EXTRN   _FifoMakeRoom:NEAR

CONST   SEGMENT
_F1    DD      03f800000r                      ; 1
_F256  DD      043800000r                      ; 256

_VPF1    DD      03f800000r                      ; 1
_VPF256  DD      043800000r                      ; 256    
CONST   ENDS

_DATA   SEGMENT
vSize           DD    0
ccoow           DD    0
packetVal       DD    0
strideinbytes   DD    0


oow0            DD    0
oow1            DD    0
oow2            DD    0
oow3            DD    0
oowa            DD    0
vPtr0           DD    0
vPtr1           DD    0
vPtr2           DD    0
xa              DD    0
ya              DD    0
xb              DD    0
yb              DD    0
xc              DD    0
yc              DD    0
zArea           DD    0
_DATA    ENDS

_TEXT   SEGMENT
_pktype = 20
_type = 24
_mode = 28
_count = 32
_pointers = 36

gc                 TEXTEQU     <esi>       ; points to graphics context
fifo               TEXTEQU     <ecx>       ; points to next entry in fifo
dlp                TEXTEQU     <ebp>       ; points to dataList structure
vertexCount        TEXTEQU     <ebx>       ; Current vertex counter in the packet
vertexPtr          TEXTEQU     <edi>       ; Current vertex pointer

_drawvertexlist@20 PROC NEAR
; 132  : {

        push    esi
        push    edi
        push    ebx
        push    ebp

;;;     GR_DCL_GC
;;;     vSize = gc->state.vData.vSize
;;;     if (stride == 0)
;;;       stride = gc->state.vData.vStride;
            
        mov     gc, DWORD PTR _GlideRoot+curGC
        mov     ecx, DWORD PTR [gc+vertexSize]
    
        mov     edx, DWORD PTR [esp+_mode]
        mov     vertexCount, DWORD PTR [esp+_count]
    
        mov     vertexPtr, DWORD PTR [esp+_pointers]
        mov     DWORD PTR vSize, ecx

        shl     edx, 2
;;;     mov     ecx, DWORD PTR [gc+CoordinateSpace]
          test    edx, edx
        jne     SHORT no_stride
        mov     edx, DWORD PTR [gc+vertexStride]
        shl     edx, 2

        align 4
no_stride:

;;;     Draw the first (or possibly only) set.  This is necessary because
;;;     the packet is 3_BDDDDDD, and in the next set, the packet is 3_DDDDDD
;;;     We try to make tstrip code simple to read. We combine the original code
;;;     into a single loop by adding an extra packet type assignment at the end of the loop.
;;; 
;;;     if (gc->state.grCoordinateSpaceArgs.coordinate_space_mode == GR_WINDOW_COORDS) {

;;;     test    ecx, ecx
          mov     DWORD PTR strideinbytes, edx

;;;       while (count > 0) {
;;;         FxI32 k, vcount = count >= 15 ? 15 : count;
;;;         GR_SET_EXPECTED_SIZE(vcount * vSize, 1);
;;;         TRI_STRIP_BEGIN(type, vcount, vSize, pktype);

        mov     eax, DWORD PTR [esp+_count]
;;;       jne     clip_coordinates

        test    eax, eax
          jle     strip_done

        align 4
window_coords_begin:

        cmp     vertexCount, 15                 ; 0000000fH
          jl      SHORT win_partial_packet
        mov     vertexCount, 15                 ; 0000000fH

        align 4
win_partial_packet:

        mov     eax, DWORD PTR vSize
        mov     ecx, DWORD PTR [gc+fifoRoom]
        imul    eax, vertexCount
        add     eax, 4
        cmp     ecx, eax
        jge     SHORT win_strip_begin
        push    @Line
        push    0h
        push    eax
        call    _FifoMakeRoom
        add     esp, 12                 ; 0000000cH
        
        align 4
win_strip_begin:

;;;     Setup pacet header
;;;
        mov     fifo, DWORD PTR [gc+fifoPtr]
          mov     eax, vertexCount
        mov     edx, DWORD PTR [esp+_type]
          mov     ebp, DWORD PTR [gc+cullStripHdr]
        shl     edx, 22                 ; 00000010H
          add     fifo, 4
        shl     eax, 6
          or    ebp, edx
        or      eax, ebp
          mov     edx, DWORD PTR [esp+_pktype]
        or      eax, edx
          nop
        mov     DWORD PTR [fifo-4], eax

;;;     for (k = 0; k < vcount; k++) {
;;;       FxI32 i;
;;;       FxU32 dataElem;
;;;       float *vPtr;
;;;       vPtr = pointers;
;;;       if (mode)
;;;         vPtr = *(float **)vPtr
;;;       (float *)pointers += stride;
;;;       TRI_SETF(FARRAY(vPtr, 0));
;;;       dataElem = 0;
;;;       TRI_SETF(FARRAY(vPtr, 4));
;;;       i = gc->tsuDataList[dataElem];

        align 4
win_for_begin:

        mov     edx, vertexPtr
          mov     eax, DWORD PTR strideinbytes
        cmp     eax, 4
          jne     SHORT win_no_deref
        mov     edx, DWORD PTR [vertexPtr]

        align 4
win_no_deref:

        add     fifo, 8
          add     vertexPtr, eax

        mov     eax, DWORD PTR [edx]
          mov     ebp, DWORD PTR [edx+4]
        
        mov     DWORD PTR [fifo-8], eax
          mov     eax, DWORD PTR [gc+tsuDataList]

        mov     DWORD PTR [fifo-4], ebp
  
          test    eax, eax
        lea     dlp, DWORD PTR [gc+tsuDataList]
          je      SHORT win_datalist_end
  
        align 4

;;;       while (i != GR_DLIST_END) {
;;;         TRI_SETF(FARRAY(vPtr, i));
;;;         dataElem++;
;;;         i = gc->tsuDataList[dataElem];
;;;       }

win_datalist_begin:

        add     fifo, 4
          add     dlp, 4

        mov     eax, DWORD PTR [edx+eax]
          nop

        mov     DWORD PTR [fifo-4], eax
          mov     eax, DWORD PTR [dlp]


        test    eax, eax
          jne     SHORT win_datalist_begin
win_datalist_end:

        dec     vertexCount
          jne     SHORT win_for_begin
win_for_end:

;;;       TRI_END;
;;;     Prepare for the next packet (if the strip size is longer than 15)
;;;       GR_CHECK_SIZE();
;;;       count -= 15;
;;;       pktype = SSTCP_PKT3_DDDDDD;
;;;     }
  
        mov     eax, DWORD PTR [gc+fifoPtr]
          mov     edx, DWORD PTR [gc+fifoRoom]
        sub     eax, fifo
          mov     vertexCount, DWORD PTR [esp+_count]
        add     edx, eax
          sub     vertexCount, 15                 ; 0000000fH
  
        mov     DWORD PTR [gc+fifoRoom], edx
          mov     DWORD PTR [esp+_count], vertexCount
  
        mov     DWORD PTR [gc+fifoPtr], fifo
          test    vertexCount, vertexCount
  
        mov     DWORD PTR [esp+_pktype], 16 ; 00000010H
          jg      window_coords_begin

strip_done:
        pop     ebp
          pop     ebx
        pop     edi
          pop     esi
        ret     20                      ; 00000014H

_drawvertexlist@20 ENDP

_pktype = 20
_type = 24
_mode = 28
_count = 32
_pointers = 36

gc                 TEXTEQU     <esi>       ; points to graphics context
fifo               TEXTEQU     <ecx>       ; points to next entry in fifo
vertexPtr          TEXTEQU     <edx>       ; pointer to vertex or vertex array

_vpdrawvertexlist@20 PROC NEAR

        push    esi
        push    edi

        push    ebx        
        mov     gc, DWORD PTR _GlideRoot+curGC
        
        push    ebp
        mov     ecx, DWORD PTR [esp+_mode]
        
        mov     edi, DWORD PTR [esp+_pointers]
        mov     eax, DWORD PTR [gc+wInfo_offset]
    
        test    ecx, ecx
        je      w_no_dref
    
        mov     edi, DWORD PTR [edi]
        
        align   4
w_no_dref:
        
;;;     load first w
        
        fld     DWORD PTR [edi+eax]
        fdivr   DWORD PTR _F1
        
        mov     ecx, DWORD PTR [gc+vertexSize]
        mov     edx, DWORD PTR [esp+_mode]

        mov     edi, DWORD PTR [esp+_count]
;;;     mov     vertexArray, DWORD PTR [esp+_pointers]

        shl     edx, 2
        mov     DWORD PTR vSize, ecx

        test    edx, edx

        jne     SHORT no_stride

        mov     edx, DWORD PTR [gc+vertexStride]
        shl     edx, 2

        align 4
no_stride:

        mov     DWORD PTR strideinbytes, edx
        mov     eax, DWORD PTR [esp+_type]

        shl     eax, 16                 ; 00000010H
        mov     DWORD PTR packetVal, eax

clip_coords_begin:

        cmp     edi, 15
        jl      SHORT clip_partial_packet
        mov     edi, 15
clip_partial_packet:

;;;     GR_SET_EXPECTED_SIZE(vcount * vSize, 1)

        mov     eax, DWORD PTR vSize
        mov     ecx, DWORD PTR [gc+fifoRoom]
        
        imul    eax, edi
        add     eax, 4
        cmp     ecx, eax
        jge     SHORT clip_strip_begin
        push    @Line
        push    0h
        push    eax
        call    _FifoMakeRoom
        add     esp, 12                 ; 0000000cH
clip_strip_begin:

;;;     TRI_STRIP_BEGIN(type, vcount, vSize, pktype)

  
        mov     fifo, DWORD PTR [gc+fifoPtr]
        mov     eax, edi
        
        mov     edx, DWORD PTR packetVal
        mov     ebp, DWORD PTR [gc+cullStripHdr]
        
        or      eax, edx
        add     fifo, 4
        
        shl     eax, 6
        mov     edx, DWORD PTR [esp+_pktype]
        
        or      eax, ebp

        or      eax, edx
        mov     DWORD PTR [fifo-4], eax

  
        mov     vertexPtr, DWORD PTR [esp+_pointers]
        mov     eax, DWORD PTR [esp+_mode]

        test    eax, eax
        
        je      SHORT clip_for_begin
        mov     vertexPtr, DWORD PTR [vertexPtr]

        align   4
clip_for_begin:

        add     fifo, 8
        mov     ebp, DWORD PTR strideinbytes

        add     DWORD PTR [esp+_pointers], ebp
        mov     eax, DWORD PTR [gc+paramIndex]
        
        xor     ebp, ebp
        mov     ebx, DWORD PTR [gc+tsuDataList]

;;; ;   setup x and y

        fld     DWORD PTR [gc+vp_hwidth]
        fmul    DWORD PTR [vertexPtr]
        test    al, 3   
        fld     DWORD PTR [gc+vp_hheight]
        fmul    DWORD PTR [vertexPtr+4]
        fxch
        fmul    st, st(2)
        fxch
        fmul    st, st(2)
        fxch
        fadd    DWORD PTR [gc+vp_ox]
        fxch
        fadd    DWORD PTR [gc+vp_oy]
        fxch    st(2)
        fstp    DWORD PTR ccoow
        fstp    DWORD PTR [fifo-8]
        fstp    DWORD PTR [fifo-4]
        
;;; ;   set up color

        je      clip_setup_ooz
  
        cmp     DWORD PTR [gc+colorType], ebp
        jne     SHORT clip_setup_pargb
  
        test    al, 1
        je      SHORT clip_setup_a
        
        add     fifo, 12
        mov     ebp, 3

        fld     DWORD PTR _GlideRoot+pool_f255
        fmul    DWORD PTR [ebx+vertexPtr]
        fld     DWORD PTR _GlideRoot+pool_f255
        fmul    DWORD PTR [ebx+vertexPtr+4]
        fld     DWORD PTR _GlideRoot+pool_f255
        fmul    DWORD PTR [ebx+vertexPtr+8]
        fxch    st(2)
        fstp    DWORD PTR [fifo-12]
        fstp    DWORD PTR [fifo-8]
        fstp    DWORD PTR [fifo-4]
        mov     ebx, DWORD PTR [gc+tsuDataList+12]
        
        align 4
clip_setup_a:
  
        test    al, 2
        je      SHORT clip_setup_ooz

        add     fifo, 4
        inc     ebp

        fld     DWORD PTR [ebx+vertexPtr]  
        fmul    DWORD PTR _GlideRoot+pool_f255
        fstp    DWORD PTR [fifo-4]
  
        mov     ebx, DWORD PTR [gc+ebp*4+tsuDataList]
        jmp     SHORT clip_setup_ooz
        
        align 4
clip_setup_pargb:
        add     fifo, 4
        mov     ebx, DWORD PTR [ebx+vertexPtr]

        mov     DWORD PTR [fifo-4], ebx
        nop
        
        mov     ebp, 1  
        mov     ebx, DWORD PTR [gc+tsuDataList+4]
clip_setup_ooz:
  
        test    al, 4
        je      SHORT clip_setup_qow

        add     fifo, 4
        inc     ebp

        fld     DWORD PTR [ebx+vertexPtr]  
        fmul    DWORD PTR [gc+vp_hdepth]
        fmul    DWORD PTR ccoow  
        fadd    DWORD PTR [gc+vp_oz]
        fstp    DWORD PTR [fifo-4]
  
        mov     ebx, DWORD PTR [gc+ebp*4+tsuDataList]
        align 4
clip_setup_qow:
  
        test    al, 8
        je      SHORT clip_setup_qow0
  
        mov     ebx, DWORD PTR [gc+qInfo_mode]
        test    ebx, ebx
        je      SHORT clip_setup_oow
        mov     ebx, DWORD PTR [gc+qInfo_offset]

        fld     DWORD PTR [vertexPtr+ebx]
        fmul    DWORD PTR ccoow
        fstp    DWORD PTR [fifo]

        jmp     SHORT clip_setup_oow_inc
        align 4
clip_setup_oow:
        mov     ebx, DWORD PTR ccoow

        mov     DWORD PTR [fifo], ebx
        align 4
clip_setup_oow_inc:
  
        mov     ebx, DWORD PTR [gc+ebp*4+tsuDataList+4]
        add     fifo, 4
        
        inc     ebp
        align 4
clip_setup_qow0:
  
        test    al, 16
        je      SHORT clip_setup_stow0
  
        mov     ebx, DWORD PTR [gc+q0Info_mode]
        cmp     ebx, 1
        jne     SHORT clip_setup_oow0
  
        mov     ebx, DWORD PTR [gc+q0Info_offset]
        
        fld     DWORD PTR [ebx+vertexPtr]
        fmul    DWORD PTR ccoow
        fstp    DWORD PTR [fifo]
        
        jmp     SHORT clip_setup_oow0_inc
        align 4
clip_setup_oow0:
        mov     ebx, DWORD PTR ccoow
        
        mov     DWORD PTR [fifo], ebx
        align 4
clip_setup_oow0_inc:
        mov     ebx, DWORD PTR [gc+ebp*4+tsuDataList+4]
        add     fifo, 4
        
        inc     ebp
        align 4
clip_setup_stow0:
  
        test    al, 32
        je      SHORT clip_setup_qow1
        

        fld     DWORD PTR ccoow
        fmul    DWORD PTR [ebx+vertexPtr]

        add     fifo, 8
        add     ebp, 2

        fmul    DWORD PTR [gc+tmu0_s_scale]
        fld     DWORD PTR ccoow
        fmul    DWORD PTR [ebx+vertexPtr+4]
        mov     ebx, DWORD PTR [gc+ebp*4+tsuDataList]
        fmul    DWORD PTR [gc+tmu0_t_scale]
        fxch
        fstp    DWORD PTR [fifo-8]
        fstp    DWORD PTR [fifo-4]
        
        align 4
clip_setup_qow1:

        test    al, 64
        je      SHORT clip_setup_stow1

        mov     ebx, DWORD PTR [gc+q1Info_mode]
        cmp     ebx, 1
        jne     SHORT clip_setup_oow1

        mov     ebx, DWORD PTR [gc+q1Info_offset]
        
        fld     DWORD PTR [ebx+vertexPtr]
        fmul    DWORD PTR ccoow
        fstp    DWORD PTR [fifo]
        
        jmp     SHORT clip_setup_oow1_inc
        align 4
clip_setup_oow1:
        mov     ebx, DWORD PTR ccoow

        mov     DWORD PTR [fifo], ebx
        align 4
clip_setup_oow1_inc:
  
        mov     ebx, DWORD PTR [gc+ebp*4+tsuDataList+4]
        add     fifo, 4

        inc     ebp

        align 4
clip_setup_stow1:
  
        test    al, 128
        je      SHORT clip_setup_end

        fld     DWORD PTR ccoow
        fmul    DWORD PTR [ebx+vertexPtr]
        add     fifo, 8
        fmul    DWORD PTR [gc+tmu1_s_scale]
        fld     DWORD PTR ccoow
        fmul    DWORD PTR [ebx+vertexPtr+4]
        mov     ebx, DWORD PTR [gc+ebp*4+tsuDataList+4]
        fmul    DWORD PTR [gc+tmu1_t_scale]
        fxch        
        fstp    DWORD PTR [fifo-8]
        fstp    DWORD PTR [fifo-4]

        align 4
clip_setup_end:

        dec     edi        
        jz      clip_for_end

        mov     vertexPtr, DWORD PTR [esp+_pointers]
        mov     ebx, DWORD PTR [esp+_mode]

        test    ebx, ebx
        je      SHORT w_clip_no_deref


        mov     vertexPtr, DWORD PTR [vertexPtr]
        align 4
w_clip_no_deref:

        mov     ebx, DWORD PTR [gc+wInfo_offset]
        
        fld     DWORD PTR [ebx+vertexPtr]  
        fdivr   DWORD PTR _F1
        
        jmp     clip_for_begin
        align 4
clip_for_end:
  
        mov     ebx, DWORD PTR [gc+fifoPtr]
        mov     edx, DWORD PTR [gc+fifoRoom]
        
        sub     ebx, fifo
        mov     edi, DWORD PTR [esp+_count]
        
        add     edx, ebx
        sub     edi, 15                 ; 0000000fH
  
        mov     DWORD PTR [gc+fifoRoom], edx
        mov     DWORD PTR [esp+_count], edi
  
        mov     DWORD PTR [gc+fifoPtr], fifo
        mov     DWORD PTR [esp+_pktype], 16 ; 00000010H

        jle     strip_done
        mov     edx, DWORD PTR [esp+_pointers]

        mov     ebx, DWORD PTR [esp+_mode]
        test    ebx, ebx
        
        je      SHORT w1_clip_no_deref
        mov     edx, DWORD PTR [edx]
        
        align 4
w1_clip_no_deref:

        mov     ebx, DWORD PTR [gc+wInfo_offset]
        fld     DWORD PTR [ebx+edx]  
        fdivr   DWORD PTR _F1
  
        jmp     clip_coords_begin
        align 4
strip_done:

        pop     ebp
        pop     ebx
        pop     edi
        pop     esi
        ret     20                      ; 00000014H
_vpdrawvertexlist@20 ENDP

gc                 TEXTEQU     <esi>       ; points to graphics context
fifo               TEXTEQU     <ecx>       ; points to next entry in fifo
vertexPtr          TEXTEQU     <edi>       ; Current vertex pointer

    PUBLIC  _vptrisetup_nocull@12
_vptrisetup_nocull@12 PROC NEAR
_va = 20
_vb = 24
_vc = 28
        push    ebx
        push    esi
        
        push    edi
        mov     gc, DWORD PTR _GlideRoot+curGC
        
        mov     ecx, DWORD PTR [esp+_va-4]
        mov     eax, DWORD PTR [gc+wInfo_offset]
        
        push    ebp
        nop
        
;;; ;   oow[0] = 1.0f / FARRAY(va, gc->state.vData.wInfo.offset)

        fld     DWORD PTR [eax+ecx]

        fdivr   DWORD PTR _F1

        mov     ecx, DWORD PTR [esp+_vb]
        mov     ebx, DWORD PTR [esp+_vc]

        nop
        nop
        
        mov     ebp, DWORD PTR [eax+ecx]
        mov     edi, DWORD PTR [eax+ebx]

        mov     DWORD PTR vPtr1, ebp
        mov     DWORD PTR vPtr2, edi
        
;;; ;   GR_SET_EXPECTED_SIZE(_GlideRoot.curTriSize, 1)

        mov     eax, DWORD PTR _GlideRoot+curTriSize
        mov     ecx, DWORD PTR [gc+fifoRoom]
        
        add     eax, 4
        nop
        
        cmp     ecx, eax
        jge     SHORT setup_pkt_hdr
        
        push    @Line                      ; line number inside this function
        push    0h                         ; pointer to function name = NULL

        push    eax
        call    _FifoMakeRoom

        add     esp, 12                 ; 0000000cH

        align 4
setup_pkt_hdr:  

;;; ;   TRI_STRIP_BEGIN(kSetupStrip, 3, gc->state.vData.vSize, SSTCP_PKT3_BDDBDD)


        mov     fifo, DWORD PTR [gc+fifoPtr]
        mov     eax, DWORD PTR [gc+cullStripHdr]

        add     fifo, 4
        lea     ebp, DWORD PTR [esp+_va]

        or      eax, 192                ; 000000c0H
        mov     edx, 0                

        mov     DWORD PTR [fifo-4], eax        
        mov     vertexPtr, DWORD PTR [ebp]

        mov     eax, DWORD PTR [gc+paramIndex]
        nop

;;; Begin loop
        
        align 4
begin_for_loop:

        add     edx, 4
        add     fifo, 8
        
        xor     ebx, ebx
        mov     ebp, DWORD PTR [gc+tsuDataList]

;;; ;   setup x and y

        fld     DWORD PTR [gc+vp_hwidth]
        fmul    DWORD PTR [vertexPtr]
        test    al, 3   
        fld     DWORD PTR [gc+vp_hheight]
        fmul    DWORD PTR [vertexPtr+4]
        fxch
        fmul    st, st(2)
        fxch
        fmul    st, st(2)
        fxch
        fadd    DWORD PTR [gc+vp_ox]
        fxch
        fadd    DWORD PTR [gc+vp_oy]
        fxch    st(2)
        fstp    DWORD PTR oowa    
        fstp    DWORD PTR [fifo-8]
        fstp    DWORD PTR [fifo-4]

                
;;; ;   set up color

        je      clip_setup_ooz

        cmp     DWORD PTR [gc+colorType], ebx
        jne     SHORT clip_setup_pargb
        
        test    al, 1
        je      SHORT clip_setup_a

        add     fifo, 12
        add     ebx, 3

        fld     DWORD PTR _GlideRoot+pool_f255
        fmul    DWORD PTR [vertexPtr+ebp]
        fld     DWORD PTR _GlideRoot+pool_f255
        fmul    DWORD PTR [vertexPtr+ebp+4]
        fld     DWORD PTR _GlideRoot+pool_f255
        fmul    DWORD PTR [vertexPtr+ebp+8]
        fxch    st(2)
        fstp    DWORD PTR [fifo-12]
        fstp    DWORD PTR [fifo-8]
        fstp    DWORD PTR [fifo-4]
        mov     ebp, DWORD PTR [gc+tsuDataList+12]
        
        align 4
clip_setup_a:          

        test    al, 2
        je      SHORT clip_setup_ooz
        
        add     fifo, 4
        inc     ebx
        
        fld     DWORD PTR [vertexPtr+ebp]
        fmul    DWORD PTR _GlideRoot+pool_f255
        fstp    DWORD PTR [fifo-4]
        
        mov     ebp, DWORD PTR [gc+ebx*4+tsuDataList]
        jmp     SHORT clip_setup_ooz
        align 4
clip_setup_pargb:
        add     fifo, 4
        mov     ebx, DWORD PTR [vertexPtr+ebp]
        
        mov     DWORD PTR [fifo-4], ebx
        nop
        
        mov     ebx, 1
        mov     ebp, DWORD PTR [gc+tsuDataList+4]
        align 4
clip_setup_ooz:

        test    al, 4
        je      SHORT clip_setup_qow
        
        add     fifo, 4
        inc     ebx
        
        fld     DWORD PTR [vertexPtr+ebp]
        fmul    DWORD PTR [gc+vp_hdepth]
        fmul    DWORD PTR oowa
        fadd    DWORD PTR [gc+vp_oz]
        fstp    DWORD PTR [fifo-4]
        
        mov     ebp, DWORD PTR [gc+ebx*4+tsuDataList]
        align 4
clip_setup_qow: 

        test    al, 8
        je      SHORT clip_setup_qow0

        cmp     DWORD PTR [gc+qInfo_mode], 1
        jne     SHORT clip_setup_oow

        mov     ebp, DWORD PTR [gc+qInfo_offset]
        fld     DWORD PTR oowa
        fmul    DWORD PTR [ebp+vertexPtr]
        fstp    DWORD PTR [fifo]
        
        jmp     SHORT clip_setup_oow_inc
        align 4
clip_setup_oow:
        
        mov     ebp, DWORD PTR oowa
        
        mov     DWORD PTR [fifo], ebp
        align 4
clip_setup_oow_inc:
        mov     ebp, DWORD PTR [gc+ebx*4+tsuDataList+4]   
        add     fifo, 4
        
        inc     ebx
        align 4
clip_setup_qow0:

        test    al, 16                  ; 00000010H
        je      SHORT clip_setup_stow0

        cmp     DWORD PTR [gc+q0Info_mode], 1        
        jne     SHORT clip_setup_oow0
                        
        mov     ebp, DWORD PTR [gc+q0Info_offset]
        
        fld     DWORD PTR oowa
        fmul    DWORD PTR [ebp+vertexPtr]
        fstp    DWORD PTR [fifo]
        
        jmp     SHORT clip_setup_oow0_inc
        align 4
clip_setup_oow0:
        mov     ebp, DWORD PTR oowa
        
        mov     DWORD PTR [fifo], ebp
        align 4
clip_setup_oow0_inc:
        mov     ebp, DWORD PTR [gc+ebx*4+tsuDataList+4]
        add     fifo, 4
        
        inc     ebx
        align 4
clip_setup_stow0:
                        
        test    al, 32
        je      SHORT clip_setup_qow1

        
        fld     DWORD PTR oowa
        fmul    DWORD PTR [vertexPtr+ebp]

        add     fifo, 8
        add     ebx, 2

        fmul    DWORD PTR [gc+tmu0_s_scale]
        fld     DWORD PTR oowa
        fmul    DWORD PTR [vertexPtr+ebp+4]
        mov     ebp, DWORD PTR [gc+ebx*4+tsuDataList]
        fmul    DWORD PTR [gc+tmu0_t_scale]
        fxch    
        fstp    DWORD PTR [fifo-8]
        fstp    DWORD PTR [fifo-4]
        
        align 4
clip_setup_qow1:

        test    al, 64
        je      SHORT clip_setup_stow1

        cmp     DWORD PTR [gc+q1Info_mode], 1
        jne     SHORT clip_setup_oow1

        mov     ebp, DWORD PTR [gc+q1Info_offset]
        
        fld     DWORD PTR [ebp+vertexPtr]
        fmul    DWORD PTR oowa
        fstp    DWORD PTR [fifo]
        
        jmp     SHORT clip_setup_oow1_inc
        align 4
clip_setup_oow1:
        mov     ebp, DWORD PTR oowa
        
        mov     DWORD PTR [fifo], ebp
        align 4
clip_setup_oow1_inc:
        mov     ebp, DWORD PTR [gc+ebx*4+tsuDataList+4]
        add     fifo, 4
        
        inc     ebx
        align 4
clip_setup_stow1:

        test    al, 128
        je      SHORT clip_setup_end
        

        fld     DWORD PTR oowa
        fmul    DWORD PTR [vertexPtr+ebp]
        add     fifo, 8  
        fmul    DWORD PTR [gc+tmu1_s_scale]
        fld     DWORD PTR oowa
        fmul    DWORD PTR [vertexPtr+ebp+4]
        fmul    DWORD PTR [gc+tmu1_t_scale]
        fxch    
        fstp    DWORD PTR [fifo-8]
        fstp    DWORD PTR [fifo-4]

        align 4
clip_setup_end:

        cmp     edx, 12
        je      update_fifo_ptr
        
        fld     DWORD PTR vPtr0[edx]
        fdivr   DWORD PTR _F1
        
        lea     ebx, DWORD PTR [esp+_va]
        mov     ebp, DWORD PTR [gc+wInfo_offset]
        
        mov     vertexPtr, DWORD PTR [ebx+edx]
        jmp     begin_for_loop
       
        align 4
update_fifo_ptr:        

        mov     ebx, DWORD PTR [gc+fifoPtr]
        mov     edx, DWORD PTR [gc+fifoRoom]
        
        sub     ebx, fifo
        mov     eax, 1

        add     edx, ebx
        pop     ebp

        mov     DWORD PTR [gc+fifoRoom], edx
        pop     edi

        mov     DWORD PTR [gc+fifoPtr], fifo
        mov     ebx, DWORD PTR _GlideRoot+trisProcessed

;;; ;   _GlideRoot.stats.trisProcessed++


        pop     esi
        inc     ebx

        mov     DWORD PTR _GlideRoot+trisProcessed, ebx
        pop     ebx
        
        ret     12                      ; 0000000cH
        
_vptrisetup_nocull@12 ENDP

    PUBLIC  _vptrisetup_cull@12
_vptrisetup_cull@12 PROC NEAR
        
_va = 20
_vb = 24
_vc = 28
        push    ebx
        push    esi
        
        push    edi
        mov     gc, DWORD PTR _GlideRoot+curGC
        
        mov     ecx, DWORD PTR [esp+_va-4]
        push    ebp

        mov     eax, DWORD PTR [gc+wInfo_offset]
        nop
        
;;; ;   oow[0] = 1.0f / FARRAY(va, gc->state.vData.wInfo.offset)

        fld     DWORD PTR [eax+ecx]

        fdivr   DWORD PTR _F1

        mov     DWORD PTR vPtr0, ecx

        mov     ecx, DWORD PTR [esp+_vb]
        mov     ebx, DWORD PTR [esp+_vc]

        fld     DWORD PTR [eax+ecx]
        fdivr   DWORD PTR _F1

        mov     DWORD PTR vPtr1, ecx
        mov     DWORD PTR vPtr2, ebx

        fld     DWORD PTR [eax+ebx]
        fdivr   DWORD PTR _F1
        fxch    st(2)                           ;  oow2  oow1  oow0

;;; ;   GR_SET_EXPECTED_SIZE(_GlideRoot.curTriSize, 1)
        mov     eax, DWORD PTR _GlideRoot+curTriSize
        mov     ecx, DWORD PTR [gc+fifoRoom]
        
        add     eax, 4
        nop
        
        cmp     ecx, eax
        jge     SHORT setup_pkt_hdr
        
        push    @Line                      ; line number inside this function
        push    0h                         ; pointer to function name = NULL

        push    eax
        call    _FifoMakeRoom

        add     esp, 12                 ; 0000000cH

        align 4
setup_pkt_hdr: 

        mov     edx, [gc + cull_mode]    
        shl     edx, 31                    ; culltest << 31    

;;; ;   culling

        mov     vertexPtr, DWORD PTR [esp+_va]  ;
        fld     DWORD PTR [gc+vp_hwidth]        ;  oow2  oow1  oow0  tx
        fmul    DWORD PTR [vertexPtr]           ;  |     |     |     |
        fld     DWORD PTR [gc+vp_hheight]       ;  |     |     |     |     ty
        fmul    DWORD PTR [vertexPtr+4]         ;  |     |     |     |     |
        fxch                                    ;  |     |     |     ty    tx
        fmul    st, st(2)                       ;  |     |     |     |     |
        fxch                                    ;  |     |     |     tx    ty
        fmul    st, st(2)                       ;  |     |     |     |     |
        fxch                                    ;  |     |     |     ty    tx
        fadd    DWORD PTR [gc+vp_ox]            ;  |     |     |     |     xa
        fxch                                    ;  |     |     |     xa    ty
        fadd    DWORD PTR [gc+vp_oy]            ;  |     |     |     |     ya
        fxch    st(2)                           ;  |     |     ya    |     oow0
        fstp    DWORD PTR oow0                  ;  |     |     |     |
        fstp    DWORD PTR xa                    ;  |     |     |
        fstp    DWORD PTR ya                    ;  |     |
                
        mov     vertexPtr, DWORD PTR [esp+_vb]  ;
        fld     DWORD PTR [gc+vp_hwidth]        ;  oow2  oow1  tx
        fmul    DWORD PTR [vertexPtr]           ;  |     |     |
        fld     DWORD PTR [gc+vp_hheight]       ;  |     |     |     ty
        fmul    DWORD PTR [vertexPtr+4]         ;  |     |     |     |
        fxch                                    ;  |     |     ty    tx
        fmul    st, st(2)                       ;  |     |     |     |
        fxch                                    ;  |     |     tx    ty
        fmul    st, st(2)                       ;  |     |     |     |
        fxch                                    ;  |     |     ty    tx
        fadd    DWORD PTR [gc+vp_ox]            ;  |     |     |     xb
        fxch                                    ;  |     |     xb    ty
        fadd    DWORD PTR [gc+vp_oy]            ;  |     |     |     yb
        fxch    st(2)                           ;  |     yb    |     oow1
        fstp    DWORD PTR oow1                  ;  |     |     |
        fstp    DWORD PTR xb                    ;  |     |
        fstp    DWORD PTR yb                    ;  |

        mov     vertexPtr, DWORD PTR [esp+_vc]  ;
        fld     DWORD PTR [gc+vp_hwidth]        ;  |     tx
        fmul    DWORD PTR [vertexPtr]           ;  |     |
        fld     DWORD PTR [gc+vp_hheight]       ;  |     |     ty
        fmul    DWORD PTR [vertexPtr+4]         ;  |     |     |
        fxch                                    ;  |     ty    tx
        fmul    st, st(2)                       ;  |     |     |
        fxch                                    ;  |     tx    ty
        fmul    st, st(2)                       ;  |     |     |
        fxch                                    ;  |     ty    tx
        fadd    DWORD PTR [gc+vp_ox]            ;  |     |     xc
        fxch                                    ;  |     xc    ty
        fadd    DWORD PTR [gc+vp_oy]            ;  |     |     yc
        fxch    st(2)                           ;  yc    |     oow2
        fstp    DWORD PTR oow1                  ;  |     |
        fstp    DWORD PTR xc                    ;  |
        fstp    DWORD PTR yc                    ;

        fld     DWORD PTR xa                    ;  xa
        fsub    DWORD PTR xb                    ;  dxAB
        fld     DWORD PTR yb                    ;  |    yb
        fsub    DWORD PTR yc                    ;  |    dyBC
        fld     DWORD PTR xb                    ;  |    |    xb
        fsub    DWORD PTR xc                    ;  |    |    dxBC
        fxch                                    ;  |    dxBC dyBC
        fld     DWORD PTR ya                    ;  |    |    |    ya
        fsub    DWORD PTR yb                    ;  |    |    |    dyAB
        fxch    st(3)                           ;  dyAB |    |    dxAB
        fmulp   st(1), st                       ;  |    |    t0=dxAB*dyBC
        fxch    st(2)                           ;  t0   |    dxAB
        fmulp   st(1), st                       ;  t0   t1
        fsubp   st(1),st                        ;  area
        fstp    DWORD PTR zArea
        
        mov     ecx, DWORD PTR zArea        ; j = *(long *)&area
        xor     eax, eax              ; Clear the return value (0 == culled)

        ; Zero Area Triangle Check
        and     ecx, 7fffffffh    ; if ((j & 0x7FFFFFFF) == 0)
        jz      __triDone

        ;; Triangle area check vs culling mode
        mov     ecx, DWORD PTR zArea              ; reload area just in case we're culling
        xor     ecx, edx               ; if (j ^ (culltest << 31))
    
        jge     __triDone


;;; ;   TRI_STRIP_BEGIN(kSetupStrip, 3, gc->state.vData.vSize, SSTCP_PKT3_BDDBDD)


        mov     fifo, DWORD PTR [gc+fifoPtr]
        mov     eax, DWORD PTR [gc+cullStripHdr]

        add     fifo, 4
        mov     edx, 0                

        or      eax, 192                ; 000000c0H
        nop

        mov     DWORD PTR [fifo-4], eax        
        mov     eax, DWORD PTR [gc+paramIndex]


;;; Begin loop
        
        align 4
begin_for_loop:
        mov     ebp, DWORD PTR oow0[edx]
        add     fifo, 8        

        mov     vertexPtr, DWORD PTR vPtr0[edx]
        mov     DWORD PTR oowa, ebp


;;; ;   setup x and y

        mov     ebx, DWORD PTR xa[edx*2]
        mov     ebp, DWORD PTR xa[edx*2+4]

        mov     DWORD PTR [fifo-8], ebx        
        add     edx, 4

        mov     DWORD PTR [fifo-4], ebp
        xor     ebx, ebx

        
        mov     ebp, DWORD PTR [gc+tsuDataList]
        test    al, 3
        
;;; ;   set up color

        je      clip_setup_ooz

        cmp     DWORD PTR [gc+colorType], ebx
        jne     SHORT clip_setup_pargb
        
        test    al, 1
        je      SHORT clip_setup_a

        add     fifo, 12
        add     ebx, 3

        fld     DWORD PTR _GlideRoot+pool_f255
        fmul    DWORD PTR [vertexPtr+ebp]
        fld     DWORD PTR _GlideRoot+pool_f255
        fmul    DWORD PTR [vertexPtr+ebp+4]
        fld     DWORD PTR _GlideRoot+pool_f255
        fmul    DWORD PTR [vertexPtr+ebp+8]
        fxch    st(2)
        fstp    DWORD PTR [fifo-12]
        fstp    DWORD PTR [fifo-8]
        fstp    DWORD PTR [fifo-4]
        mov     ebp, DWORD PTR [gc+tsuDataList+12]
        
        align 4
clip_setup_a:          

        test    al, 2
        je      SHORT clip_setup_ooz
        
        add     fifo, 4
        inc     ebx
        
        fld     DWORD PTR [vertexPtr+ebp]
        fmul    DWORD PTR _GlideRoot+pool_f255
        fstp    DWORD PTR [fifo-4]
        
        mov     ebp, DWORD PTR [gc+ebx*4+tsuDataList]
        jmp     SHORT clip_setup_ooz
        align 4
clip_setup_pargb:
        add     fifo, 4
        mov     ebx, DWORD PTR [vertexPtr+ebp]
        
        mov     DWORD PTR [fifo-4], ebx
        nop
        
        mov     ebx, 1
        mov     ebp, DWORD PTR [gc+tsuDataList+4]
        align 4
clip_setup_ooz:

        test    al, 4
        je      SHORT clip_setup_qow
        
        add     fifo, 4
        inc     ebx
        
        fld     DWORD PTR [vertexPtr+ebp]
        fmul    DWORD PTR [gc+vp_hdepth]
        fmul    DWORD PTR oowa
        fadd    DWORD PTR [gc+vp_oz]
        fstp    DWORD PTR [fifo-4]
        
        mov     ebp, DWORD PTR [gc+ebx*4+tsuDataList]
        align 4
clip_setup_qow: 

        test    al, 8
        je      SHORT clip_setup_qow0

        cmp     DWORD PTR [gc+qInfo_mode], 1
        jne     SHORT clip_setup_oow

        mov     ebp, DWORD PTR [gc+qInfo_offset]
        fld     DWORD PTR oowa
        fmul    DWORD PTR [ebp+vertexPtr]
        fstp    DWORD PTR [fifo]
        
        jmp     SHORT clip_setup_oow_inc
        align 4
clip_setup_oow:
        
        mov     ebp, DWORD PTR oowa
        
        mov     DWORD PTR [fifo], ebp
        align 4
clip_setup_oow_inc:
        mov     ebp, DWORD PTR [gc+ebx*4+tsuDataList+4]   
        add     fifo, 4
        
        inc     ebx
        align 4
clip_setup_qow0:

        test    al, 16                  ; 00000010H
        je      SHORT clip_setup_stow0

        cmp     DWORD PTR [gc+q0Info_mode], 1        
        jne     SHORT clip_setup_oow0
                        
        mov     ebp, DWORD PTR [gc+q0Info_offset]
        
        fld     DWORD PTR oowa
        fmul    DWORD PTR [ebp+vertexPtr]
        fstp    DWORD PTR [fifo]
        
        jmp     SHORT clip_setup_oow0_inc
        align 4
clip_setup_oow0:
        mov     ebp, DWORD PTR oowa
        
        mov     DWORD PTR [fifo], ebp
        align 4
clip_setup_oow0_inc:
        mov     ebp, DWORD PTR [gc+ebx*4+tsuDataList+4]
        add     fifo, 4
        
        inc     ebx
        align 4
clip_setup_stow0:
                        
        test    al, 32
        je      SHORT clip_setup_qow1

        
        fld     DWORD PTR oowa
        fmul    DWORD PTR [vertexPtr+ebp]

        add     fifo, 8
        add     ebx, 2

        fmul    DWORD PTR [gc+tmu0_s_scale]
        fld     DWORD PTR oowa
        fmul    DWORD PTR [vertexPtr+ebp+4]
        mov     ebp, DWORD PTR [gc+ebx*4+tsuDataList]
        fmul    DWORD PTR [gc+tmu0_t_scale]
        fxch    
        fstp    DWORD PTR [fifo-8]
        fstp    DWORD PTR [fifo-4]
        
        align 4
clip_setup_qow1:

        test    al, 64
        je      SHORT clip_setup_stow1

        cmp     DWORD PTR [gc+q1Info_mode], 1
        jne     SHORT clip_setup_oow1

        mov     ebp, DWORD PTR [gc+q1Info_offset]
        
        fld     DWORD PTR [ebp+vertexPtr]
        fmul    DWORD PTR oowa
        fstp    DWORD PTR [fifo]
        
        jmp     SHORT clip_setup_oow1_inc
        align 4
clip_setup_oow1:
        mov     ebp, DWORD PTR oowa
        
        mov     DWORD PTR [fifo], ebp
        align 4
clip_setup_oow1_inc:
        mov     ebp, DWORD PTR [gc+ebx*4+tsuDataList+4]
        add     fifo, 4
        
        inc     ebx
        align 4
clip_setup_stow1:

        test    al, 128
        je      SHORT clip_setup_end
        
        fld     DWORD PTR oowa
        fmul    DWORD PTR [vertexPtr+ebp]
        add     fifo, 8  
        fmul    DWORD PTR [gc+tmu1_s_scale]
        fld     DWORD PTR oowa
        fmul    DWORD PTR [vertexPtr+ebp+4]
        fmul    DWORD PTR [gc+tmu1_t_scale]
        fxch    
        fstp    DWORD PTR [fifo-8]
        fstp    DWORD PTR [fifo-4]

        align 4
clip_setup_end:

        cmp     edx, 12
        jl      begin_for_loop
        
        align 4
update_fifo_ptr:        

        mov     ebx, DWORD PTR [gc+fifoPtr]
        mov     edx, DWORD PTR [gc+fifoRoom]
        
        sub     ebx, fifo
        mov     eax, 1

        add     edx, ebx
        pop     ebp

        mov     DWORD PTR [gc+fifoRoom], edx
        pop     edi

        mov     DWORD PTR [gc+fifoPtr], fifo
        mov     ebx, DWORD PTR _GlideRoot+trisProcessed

;;; ;   _GlideRoot.stats.trisProcessed++


        pop     esi
        inc     ebx

        mov     DWORD PTR _GlideRoot+trisProcessed, ebx
        pop     ebx
        
        ret     12                      ; 0000000cH
        align 4
__triDone:    
        ;; Restore trashed registers
        mov     esi, [_GlideRoot + trisProcessed]
        pop     ebp
        
        add     esi, 1    ; _GlideRoot.stats.trisProcessed++;    
        pop     edi
    
        pop     esi
        mov     [_GlideRoot + trisProcessed], esi
        
        pop     ebx
        ret     12

_vptrisetup_cull@12 ENDP

_TEXT   ENDS
endif  ; !GL_AMD3D

END

