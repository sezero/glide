;
; compulsory header for h5/glide3/xdraw* assembly specializations (NASM)
;
; $Header$
; $Log$
;

%undef STDCALL

%ifdef __WIN32__
%define STDCALL 1

%define TEXT  .text   align=32
%define DATA  .data   align=32
%define CONST .rdata  align=32

%macro SET_TLSBASE 1
	mov	%1, dword [fs:18h] ; get thread local storage base pointer
%endmacro

%macro SET_TLSOFFSET 1
	mov	%1, [_GlideRoot+tlsOffset] ; offset of GC into tls
%endmacro

%macro GET_GC 2
	mov	gc, [%1 + %2]
%endmacro

%endif

%ifdef __DJGPP__

%define TEXT  .text
%define DATA  .data
%define CONST .rodata

extern threadValueDJGPP

%macro SET_TLSBASE 1
%endmacro

%macro SET_TLSOFFSET 1
%endmacro

%macro GET_GC 2
	mov	gc, [threadValueDJGPP]
%endmacro
%endif

%ifdef __linux__

%define TEXT  .text   align=32
%define DATA  .data   align=32
%define CONST .rodata align=32

extern threadValueLinux

%macro SET_TLSBASE 1
%endmacro

%macro SET_TLSOFFSET 1
%endmacro

%macro GET_GC 2
	mov	gc, [threadValueLinux]
%endmacro
%endif

%macro ret 0-1
%ifdef STDCALL
	ret	%1
%else
	ret
%endif
%endmacro

%macro invoke 1-*
	%rep	%0 - 1
	%rotate -1
	push	%1
	%endrep
	%rotate -1
	call	%1
%ifndef STDCALL
%if	%0 > 1
	add	esp, 4 * (%0 - 1)
%endif
%endif
%endmacro
