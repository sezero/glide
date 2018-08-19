; this is here so that the win9x parts of minihwc.c &&
; win_mode.c can compile without the win9x ddk headers.

extern		__imp__CreateFileA@28
extern		__imp__CloseHandle@4
extern		__imp__DeviceIoControl@32

global		_CM_Get_DevNode_Key

section .text
align 16
;CM_Get_DevNode_Key(DWORD, PCHAR, PVOID, ULONG, ULONG)
_CM_Get_DevNode_Key:
	push	ebp
	mov	ebp,esp
	;struct _IOCTLPKT { DWORD stack, servicenum; } pkt;
	;DWORD stack;
	sub	esp,12
	mov	dword [ebp-4],ebp
	mov	eax,dword [ebp-4]
	mov	dword [ebp-8],0x8000003d	; servicenum
	add	eax,8
	mov	dword [ebp-12],eax
	lea	eax,[ebp-12]
	push	eax
	call	_CM_IoctlHandler@4
	mov	esp,ebp
	pop	ebp
	ret
align 16
;DWORD __stdcall CM_IOCTLHandler(struct _IOCTLPKT *)
_CM_IoctlHandler@4:
	;HANDLE hCONFIGMG; DWORD rc,rcsize;
	sub	esp,8
	push	esi
	push	0
	push	0
	push	3
	push	0
	push	3
	push	0xc0000000		; GENERIC_READ|GENERIC_WRITE
	push	LC0
	; rc= ERR, rcsize = 0;
	mov	dword [esp+0x20],19	; CONFIGMG error
	mov	dword [esp+0x24],0
	call	[__imp__CreateFileA@28]
	mov	esi,eax
	cmp	esi,0xffffffff
	jne	.L0
	mov	eax,19
	pop	esi
	add	esp,8
	ret	4
.L0:
	lea	eax,[esp+8]
	push	0
	push	eax
	mov	eax,dword [esp+24]
	lea	ecx,[esp+12]
	push	4
	mov	edx,dword [eax+4]
	push	ecx
	push	4
	push	eax
	push	edx
	push	esi
	;DeviceIoControl(hCONFIGMG,
	;		 pkt->servicenum, &pkt->stack, sizeof(pkt->stack),
	;		 &rc, sizeof(rc), &rcsize,NULL);
	call	[__imp__DeviceIoControl@32]
	test	eax,eax
	jne	.L1
	mov	dword [esp+0x4],19
.L1:
	push	esi
	call	[__imp__CloseHandle@4]
	mov	eax,dword [esp+0x8]
	cmp	eax,4
	mov	eax,19
	jne	.L2
	mov	eax,dword [esp+4]
.L2:
	pop	esi
	add	esp,8
	ret	4
align 16

section .rdata
LC0:	db	`\\\\.\\CONFIGMG\0`
	align 16,db 0
