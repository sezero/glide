# 1 "cpudtect.S"
 



















 
 







 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 



.file "cpudtect.asm"


 
 
 

 
 





 

.text


 
 
 

 

 

.globl _cpu_detect_asm
.type _cpu_detect_asm,@function
_cpu_detect_asm:
.L_cpu_detect_asm_P6Stuff:
	push %esi	 
	push %edi
	push %ebx
	push %ebp

 
 
	pushf 	 
	pop %eax	 
	mov %eax , %ecx	 
	xor $0x0200000 , %eax	 
	push %eax	 
	popf 	 
	pushf 	 
	pop %eax	 
	xor %ecx , %eax	 
	jz .L_cpu_detect_asm_NotPentium	 


 

 
	xor %eax , %eax	 
	cpuid 	 
	xor $0x0756e6547 , %ebx	 
	jnz .L_cpu_detect_asm_NotIntel
	xor $0x049656e69 , %edx	 
	jnz .L_cpu_detect_asm_NotIntel
	xor $0x06c65746e , %ecx	 
	jnz .L_cpu_detect_asm_NotIntel

 
 
 
	mov $1 , %eax
	cpuid 	 

	shr $8 , %eax	 
	and $0x0f , %eax	 
	cmp $6 , %eax
	jl .L_cpu_detect_asm_IsP5	 
 

 
 
.L_cpu_detect_asm_IsP6:
 
	mov %edx , %ebx
	test $0x1000 , %ebx

 
	jz .L_cpu_detect_asm_IsP5

 
	test $0x2000000 , %edx
	jz .L_cpu_detect_asm_noKNI

	mov $7 , %eax
	jmp .L_cpu_detect_asm_DoneCpu	 

.L_cpu_detect_asm_noKNI:
	mov $6 , %eax	 
	jmp .L_cpu_detect_asm_DoneCpu	 

.L_cpu_detect_asm_IsP5:
	mov $5 , %eax	 
	jmp .L_cpu_detect_asm_DoneCpu

.L_cpu_detect_asm_NotPentium:
	mov $4 , %eax

.L_cpu_detect_asm_DoneCpu:
	pop %ebp
	pop %ebx
	pop %edi
	pop %esi
	ret


.L_cpu_detect_asm_NotIntel:

 
 
 

 
 

 
 
 

 

 
 
 
 

	xor %esi , %esi	 
	xor %edi , %edi	 

 

	mov $0x80000000 , %eax
	cpuid 
	cmp $0x80000000 , %eax
	jbe .L_cpu_detect_asm_NoExtendedFeatures

 

	mov $0x80000001 , %eax
	cpuid 
	mov %edx , %edi

.L_cpu_detect_asm_NoExtendedFeatures:

 

	mov $1 , %eax
	cpuid 
	mov %edx , %esi
	mov %eax , %ebp	 

 

	mov $0 , %eax
	cpuid 

.L_cpu_detect_asm_ChkAMD:
	cmp $0x68747541 , %ebx	 
	jne .L_cpu_detect_asm_ChkCyrix
	cmp $0x69746E65 , %edx	 
	jne .L_cpu_detect_asm_ChkCyrix
	cmp $0x444D4163 , %ecx	 
	je .L_cpu_detect_asm_CPUisAMD

.L_cpu_detect_asm_ChkCyrix:
	cmp $0x69727943 , %ebx	 
	jne .L_cpu_detect_asm_ChkIDT
	cmp $0x736E4978 , %edx	 
	jne .L_cpu_detect_asm_ChkIDT
	cmp $0x64616574 , %ecx	 
	je .L_cpu_detect_asm_CPUisCyrix

.L_cpu_detect_asm_ChkIDT:
	cmp $0x746E6543 , %ebx	 
	jne .L_cpu_detect_asm_UnknownVendor
	cmp $0x48727561 , %edx	 
	jne .L_cpu_detect_asm_UnknownVendor
	cmp $0x736C7561 , %ecx	 
	jne .L_cpu_detect_asm_UnknownVendor

.L_cpu_detect_asm_CPUisIDT:
	mov $0x80030000 , %eax	 
	test $0x00800000 , %esi	 
	jz .L_cpu_detect_asm_DoneCpu
	or $1 , %eax	 
	test $0x80000000 , %edi	 
	jz .L_cpu_detect_asm_DoneCpu
	or $2 , %eax	 
	jmp .L_cpu_detect_asm_DoneCpu

.L_cpu_detect_asm_CPUisAMD:
	mov $0x80010000 , %eax	 
	mov %ebp , %edx	 
	and $0x00000FFF , %edx	 
	cmp $0x00000588 , %edx	 
	jb .L_cpu_detect_asm_AmdMTRRchkDone	 
	cmp $0x00000600 , %edx	 
	jb .L_cpu_detect_asm_AmdHasK6MTRR	 
	or $8 , %eax	 
	jmp .L_cpu_detect_asm_AmdMTRRchkDone
.L_cpu_detect_asm_AmdHasK6MTRR:
	or $4 , %eax	 
.L_cpu_detect_asm_AmdMTRRchkDone:
	test $0x00800000 , %esi	 
	jz .L_cpu_detect_asm_DoneCpu
	or $1 , %eax	 
	test $0x80000000 , %edi	 
	jz .L_cpu_detect_asm_DoneCpu
	or $2 , %eax	 
	jmp .L_cpu_detect_asm_DoneCpu

.L_cpu_detect_asm_CPUisCyrix:
	mov $0x80020000 , %eax	 
	test $0x00800000 , %esi	 
	jz .L_cpu_detect_asm_DoneCpu
	or $1 , %eax	 
	test $0x80000000 , %edi	 
	jz .L_cpu_detect_asm_DoneCpu
	or $2 , %eax	 
	jmp .L_cpu_detect_asm_DoneCpu

.L_cpu_detect_asm_UnknownVendor:
	mov $0x0ffffffff , %eax
	jmp .L_cpu_detect_asm_DoneCpu


.L_END__cpu_detect_asm:
.size _cpu_detect_asm,.L_END__cpu_detect_asm-_cpu_detect_asm


 
 
 
.align 4	 
.globl single_precision_asm
.type single_precision_asm,@function
single_precision_asm:
	push %eax	 
	fnclex 	 
	fstcw (%esp)
	movl (%esp) , %eax
	and $0x0000fcff , %eax	 
	movl %eax , (%esp)
	fldcw (%esp)
	pop %eax
	ret
.L_END_single_precision_asm:
.size single_precision_asm,.L_END_single_precision_asm-single_precision_asm

 
 
 
.align 4	 
.globl double_precision_asm
.type double_precision_asm,@function
double_precision_asm:
	push %eax	 
	fnclex 	 
	fstcw (%esp)
	movl (%esp) , %eax
	and $0x0000fcff , %eax	 
	or $0x000002ff , %eax	 
	movl %eax , (%esp)
	fldcw (%esp)
	pop %eax
	ret
.L_END_double_precision_asm:
.size double_precision_asm,.L_END_double_precision_asm-double_precision_asm

.end
