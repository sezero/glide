;;
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
;; $Log$
;;
;; Revision 1.1  2000/06/15 00:27:42  joseph
;; Initial checkin into SourceForge.
;; 
;; 9     3/10/99 10:40a Peter
;; detect katmai-ness
;; 
;; 8     1/20/99 5:50p Peter
;; norbert's code for k6 and k7 (?) wc
;; 
;; 7     12/09/98 1:11p Peter
;; cpu detection from Norbert for teh rest of the 3DNow!(tm) partners
;; 
;; 6     10/03/98 3:35p Peter
;; First pass at dynamic detection of 3DNow!(tm)
;; 
;; 5     7/24/98 1:40p Hohn
; 
; 4     5/28/97 8:23a Peter
; Merge w/ original glide source
; 
; 2     3/04/97 9:10p Dow
;;
;;

TITLE  cpudtect.asm

.586P
.model FLAT,C                   ; Flat memory, mangle publics with leading '_'

;;      Data for data segment goes here
;_DATA   SEGMENT DWORD USE32 PUBLIC 'DATA'; 
;_DATA   ENDS
    
;;; Some useful constants
; CPU Type
CPUTypeUnknown   = 0ffffffffh
CPUTypeIntel     = 000000000h
CPUTypeAMD       = 080010000h
CPUTypeCyrix     = 080020000h
CPUTypeIDT       = 080030000h
CPUTypeTransmeta = 080040000h

;;; References to external data:
    
_TEXT   SEGMENT
;;
;;  _cpu_detect_asm - detect the type of CPU 
;; 
;;  USAGE:
;;
;;  int __cdecl _cpu_detect_asm(void);
;;

;; The return value is split into two 16-bit fields. Bits [31:16]
;; identify the processor vendor as follows:
;;
;; 0000h   Intel
;; 8001h   AMD
;; 8002h   Cyrix
;; 8003h   IDT
;; 8004h   Transmeta
;; ffffh   Unknown
;;
;; Bits [15:0] identify processor features as follows:
;;
;; 0001h   MMX
;; 0002h   3DNow! (tm)
;; 0004h   K6-style MTRRs
;; 0008h   PentiumII-style MTRRs
;; 0010h   SSE FP
;; 0020h   SSE MMX
;; 0040h   SSE2
;;
;; The FP part of SSE introduces a new architectural state and therefore
;; requires support from the operating system. So even if CPUID indicates
;; support for SSE FP, the application might not be able to use it. If
;; CPUID indicates support for SSE FP, check whether it is also
;; supported by the OS, and turn off the SSE FP feature bit if there 
;; is no OS support for SSE FP.
;;
;; Operating systems that do not support SSE FP return an illegal
;; instruction exception if execution of an SSE FP instruction is performed. 
;; Here, a sample SSE FP instruction is executed, and is checked for an 
;; exception using the (non-standard) __try/__except mechanism 
;; of Microsoft Visual C. 
;;
;; if((_GlideRoot.CPUType & 0x10L) == 0x10UL) {
;;   __try {
;;     __asm _emit 0x0f 
;;     __asm _emit 0x56 
;;     __asm _emit 0xC0    ;; orps xmm0, xmm0
;;   }
;;   __except(EXCEPTION_EXECUTE_HANDLER) {
;;     GDBG_INFO(0,"SSE is not supported by OS\n");
;;     _GlideRoot.CPUType &= ~0x10UL;
;;   }
;; }
;; if((_GlideRoot.CPUType & 0x40L) == 0x40UL) {
;;   __try {
;;     __asm _emit 0x66 
;;     __asm _emit 0x0f 
;;     __asm _emit 0x57 
;;     __asm _emit 0xc0    ;; xorpd xmm0, xmm0
;;   }
;;   __except(EXCEPTION_EXECUTE_HANDLER) {
;;     GDBG_INFO(0,"SSE2 is not supported by OS\n");
;;     _GlideRoot.CPUType &= ~0x40UL;
;;   }
;; }
;;
;; Although CR0 can be called from ring3, CR4 must be called from ring0
;; which prevents us from using the CR0.EM and the CR4.OSFXSR bits. The
;; main reason for this indirect method.
;;

PUBLIC  _cpu_detect_asm
_cpu_detect_asm PROC NEAR
P6Stuff:
    .586
    push   esi              ; save registers that are not volatile
    push   edi
    push   ebx
    push   ebp

    ; First, determine whether CPUID instruction is available.
    ; If it's not, then it's a 386 or 486.
    pushfd                  ; push original EFLAGS.
    pop eax                 ; pop into eax
    mov ecx, eax            ; save original EFLAGS in ecx
    xor eax, 0200000h       ; flip ID bit in EFLAGS
    push    eax             ; put it back on stack
    popfd                   ; pop into EFLAGS
    pushfd                  ; get EFLAGS back
    pop eax                 ; into eax
    xor eax, ecx            ; check to see if we could toggle ID
    jz  UnknownVendor       ; Sorry, older than P5
    
    xor esi, esi            ; default feature flags
    xor edi, edi            ; default extended feature flags

    ;;  Verifying architecture family
    ;;      eax - type[13:12] family[11:8] model[7:4] revision[3:0]
    ;;      edx - feature bits
    mov eax, 1
    cpuid                  ; get family/model/stepping
    mov esi, edx
    mov ebp, eax           ; save family/model/stepping

;; get the vendor string 
ChkIntel:
    mov eax, 0             ; eax = 0.
    cpuid                  ; get cpuid
    cmp ebx, 0756e6547h    ; "Genu"
    jne NoneIntel
    cmp edx, 049656e69h    ; "ineI"
    jne NoneIntel
    cmp ecx, 06c65746eh    ; "ntel"
    je  CPUisIntel

NoneIntel:
    ;; Test whether extended feature function is supported
    mov eax, 80000000h
    cpuid
    cmp eax, 80000000h
    jbe ChkAMD

    ;; execute extended feature function
    mov eax, 80000001h
    cpuid
    mov edi, edx

ChkAMD:
    mov eax, 0             ; eax = 0.
    cpuid                  ; get cpuid
    cmp ebx, 68747541h     ; 'htuA'
    jne ChkCyrix
    cmp edx, 69746E65h     ; 'itne'
    jne ChkCyrix
    cmp ecx, 444D4163h     ; 'DMAc'
    je  CPUisAMD

ChkCyrix:
    cmp ebx, 69727943h     ; 'iryC'
    jne ChkIDT
    cmp edx, 736E4978h     ; 'snIx'
    jne ChkIDT
    cmp ecx, 64616574h     ; 'deat'
    je  CPUisCyrix

ChkIDT:
    cmp ebx, 746E6543h     ; 'tneC'
    jne ChkTransmeta
    cmp edx, 48727561h     ; 'Hrua'
    jne ChkTransmeta
    cmp ecx, 736C7561h     ; 'slua'
    je  CPUisIDT

ChkTransmeta:
    cmp ebx, 6E617254h     ; 'narT'
    jne UnknownVendor
    cmp edx, 74656D73h     ; 'tems'
    jne UnknownVendor
    cmp ecx, 74656D73h     ; 'UPCa'
    je  CPUisTransmeta

CPUisIntel:
    mov  eax, CPUTypeIntel ; vendor = Intel, features = none
    test esi, 1000h        ; check P2_MTRR bit in features
    jz   IntelMTRRchkDone
    or   eax, 8h           ; set P2_MTRR feature flag
IntelMTRRchkDone:
    jmp  ChkMMX

CPUisAMD:
    mov  eax, CPUTypeAMD   ; vendor = AMD, features = none
    mov  edx, ebp          ; family/model/stepping information
    and  edx, 00000FFFh    ; extract family/model/stepping
    cmp  edx, 00000588h    ; CXT, Sharptooth, or K7 ?
    jb   AmdMTRRchkDone    ; nope, definitely no MTRRs
    cmp  edx, 00000600h    ; K7 or better ?
    jb   AmdHasK6MTRR      ; nope, but supports K6 MTRRs
    or   eax, 8h           ; set P2_MTRR feature flag
    jmp  AmdMTRRchkDone    ;
AmdHasK6MTRR:
    or   eax, 4h           ; set K6_MTRR feature flag
AmdMTRRchkDone:
    jmp  Chk3DNOW

CPUisCyrix:
    mov  eax, CPUTypeCyrix ; vendor = Cyrix, features = none
    jmp  Chk3DNOW

CPUisIDT:
    mov  eax, CPUTypeIDT   ; vendor = IDT, features = none
    jmp  Chk3DNOW

CPUisTransmeta:
    mov  eax, CPUTypeTransmeta ; vendor = Transmeta, features = none

;; none intel cpu features
Chk3DNOW:
    test edi, 80000000h    ; check for 3DNow! bit in extended features
    jz   ChkMMX
    or   eax, 2h           ; set 3DNow! feature flag

;; intel cpu features
ChkMMX:
    test esi, 00800000h    ; check for MMX bit in features
    jz   ChkSSE
    or   eax, 1h           ; set MMX feature flag

ChkSSE:
    test esi, 02000000h    ; check for SSE FP bit in features
    jz   ChkSSE2
    or   eax, 10h          ; set SSE FP feature flag

;ChkSSEMMX:
;    test edi, 00400000h    ; check for SSE MMX bit in extended features
;    jz   ChkSSE2
;    or   eax, 20h          ; set SSE MMX feature flag

ChkSSE2:
    test esi, 04000000h    ; check for SSE2 feature flag
    jz   DoneCpu
    or   eax, 40h          ; set SSE2 feature flag
    jmp  DoneCpu

UnknownVendor:
    mov  eax, CPUTypeUnknown

DoneCpu:
    pop  ebp
    pop  ebx
    pop  edi
    pop  esi
    ret

_cpu_detect_asm ENDP


;------------------------------------------------------------------------------   
; this routine sets the precision to single
; which effects all adds, mults, and divs
    align 4                 ; 
    PUBLIC  single_precision_asm
single_precision_asm PROC NEAR
.586
    push  eax       ; make room
    fnclex          ; clear pending exceptions    
    fstcw WORD PTR [esp]
    mov   eax, DWORD PTR [esp]
    and   eax, 0000fcffh  ; clear bits 9:8
    mov   DWORD PTR [esp], eax
    fldcw WORD PTR [esp]
    pop   eax
    ret   0
single_precision_asm ENDP

;------------------------------------------------------------------------------   
; this routine sets the precision to double
; which effects all adds, mults, and divs
    align 4                 ; 
    PUBLIC  double_precision_asm
double_precision_asm PROC NEAR
.586
    push  eax       ; make room
    fnclex          ; clear pending exceptions    
    fstcw WORD PTR [esp]
    mov   eax, DWORD PTR [esp]
    and   eax, 0000fcffh  ; clear bits 9:8
    or    eax, 000002ffh  ; set 9:8 to 10
    mov   DWORD PTR [esp], eax
    fldcw WORD PTR [esp]
    pop   eax
    ret   0
double_precision_asm ENDP
    
_TEXT ENDS
END
