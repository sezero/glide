# 1 "xdraw2.S"
 









































 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 


.file "xdraw2.asm"


 
# 1 "fxgasm.h" 1

 
 
 



 
 
 

























































 
 
 












# 111 "xdraw2.S" 2






.data
	.type	One,@object
	.size	One,4
One:	.int	0x3f800000
	.type	Area,@object
	.size	Area,4
Area:	.int	0

.data
.section	.rodata
	.type	T2003,@object
	.size	T2003,4
T2003:	.int	0x46400000	 
	.type	T2005,@object
	.size	T2005,4
T2005:	.int	0x3f800000	 
	.type	T2006,@object
	.size	T2006,4
T2006:	.int	0x43800000	 

 






 
 
 
 











 


 



 
 
 
 

 

 

.text


.align 32
.globl _trisetup_Default_clip_nocull_invalid  
.type _trisetup_Default_clip_nocull_invalid  ,@function
_trisetup_Default_clip_nocull_invalid  :





	






# 1 "xdraw2.inc.s" 1
 



















 

 
 
 
 
 
/
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

# 674 "xdraw2.inc.s"





 
 
 

.file "xdraw2.inc.S"




 
 
 











 
 
 




# 960 "xdraw2.inc.s"


# 194 "xdraw2.S" 2








.L_END_trisetup_Default_clip_nocull_invalid  :
.size _trisetup_Default_clip_nocull_invalid  ,.L_END_trisetup_Default_clip_nocull_invalid  - _trisetup_Default_clip_nocull_invalid  

.align 32
.globl _trisetup_Default_clip_cull_invalid  
.type _trisetup_Default_clip_cull_invalid  ,@function
_trisetup_Default_clip_cull_invalid  :


	






# 1 "xdraw2.inc.s" 1
 



















 

 
 
 
 
 
/
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

# 674 "xdraw2.inc.s"





 
 
 

.file "xdraw2.inc.S"




 
 
 











 
 
 




# 960 "xdraw2.inc.s"


# 218 "xdraw2.S" 2








.L_END_trisetup_Default_clip_cull_invalid  :
.size _trisetup_Default_clip_cull_invalid  ,.L_END_trisetup_Default_clip_cull_invalid  - _trisetup_Default_clip_cull_invalid  

.align 32
.globl _trisetup_Default_clip_cull_valid  
.type _trisetup_Default_clip_cull_valid  ,@function
_trisetup_Default_clip_cull_valid  :





	






# 1 "xdraw2.inc.s" 1
 



















 

 
 
 
 
 
/
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

# 674 "xdraw2.inc.s"





 
 
 

.file "xdraw2.inc.S"




 
 
 











 
 
 




# 960 "xdraw2.inc.s"


# 245 "xdraw2.S" 2








.L_END_trisetup_Default_clip_cull_valid  :
.size _trisetup_Default_clip_cull_valid  ,.L_END_trisetup_Default_clip_cull_valid  - _trisetup_Default_clip_cull_valid  

.align 32
.globl _trisetup_Default_clip_nocull_valid  
.type _trisetup_Default_clip_nocull_valid  ,@function
_trisetup_Default_clip_nocull_valid  :





	






# 1 "xdraw2.inc.s" 1
 



















 

 
 
 
 
 
/
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

# 674 "xdraw2.inc.s"





 
 
 

.file "xdraw2.inc.S"




 
 
 











 
 
 




# 960 "xdraw2.inc.s"


# 272 "xdraw2.S" 2








.L_END_trisetup_Default_clip_nocull_valid  :
.size _trisetup_Default_clip_nocull_valid  ,.L_END_trisetup_Default_clip_nocull_valid  - _trisetup_Default_clip_nocull_valid  

.align 32
.globl _trisetup_Default_win_nocull_invalid  
.type _trisetup_Default_win_nocull_invalid  ,@function
_trisetup_Default_win_nocull_invalid  :





	






# 1 "xdraw2.inc.s" 1
 



















 

 
 
 
 
 
/
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

# 674 "xdraw2.inc.s"





 
 
 

.file "xdraw2.inc.S"




 
 
 










# 709 "xdraw2.inc.s"

 
	push %esi
	push %edi

	push %ebx
	push %ebp

	mov 4  + 16  (%esp) , %esi 
	
 
 

# 737 "xdraw2.inc.s"

 

# 751 "xdraw2.inc.s"

	
L_win_nocull_invalid_pastContextTest  :

 
	movl 0x00000b78 (%esi ) , %edx
	test %edx , %edx
	je L_win_nocull_invalid_no_validation  
	call _grValidateState
.align 4

	
L_win_nocull_invalid_no_validation  :
# 823 "xdraw2.inc.s"


L_win_nocull_invalid_nocull  :
 
 
	mov 0x0000004c (%esi ) , %eax
	mov 0x00000dec (%esi ) , %ebx

	add $4 , %eax
	cmp %eax , %ebx

	jge L_win_nocull_invalid___triBegin  

	pushl $836
	push $0x0

	push %eax
	call _grCommandTransportMakeRoom
	add $12, %esp

 









.macro GR_FIFO_WRITE __addr __offset __data
# 866 "xdraw2.inc.s"

	mov \__data , \__offset(\__addr)

.endm	 

.align 4
L_win_nocull_invalid___triBegin  :
	mov 0x00000de4 (%esi ) , %ebp 	 
	mov $8 + 16  - 16  , %ecx 	 

	mov 0x00000dd8 (%esi ) , %eax	 
	nop 

	GR_FIFO_WRITE %ebp  , 0 , %eax	 
	add $4 , %ebp 	 

.align 4
L_win_nocull_invalid___vertexStart  :
	mov 16 (%esp, %ecx ) , %edx 	 
	add $8 , %ebp 

	nop 	 
	nop 

	movl (%edx ) , %eax	 
	lea 0x00000124 (%esi ) , %ebx 	 

	GR_FIFO_WRITE %ebp  , -8 , %eax	 
	movl 4(%edx ) , %eax	 

	xor %edi  , %edi 	 
	GR_FIFO_WRITE %ebp  , -4 , %eax	 

L_win_nocull_invalid___doParams  :
	movl (%ebx ) , %eax	 
	add $4 , %ebx 	 

	cmp $0 , %eax	 
	je L_win_nocull_invalid___nextVertex  

 
 
	nop 
	nop 

L_win_nocull_invalid___paramLoop  :
	movl (%eax,%edx ) , %edi 	 
	add $4 , %ebp 	 

	movl (%ebx ) , %eax	 
	add $4 , %ebx 	 

	cmp $0 , %eax	 
	GR_FIFO_WRITE %ebp  , -4 , %edi 	 

	jne L_win_nocull_invalid___paramLoop  

.align 4
L_win_nocull_invalid___nextVertex  :
 
	add $4 , %ecx 

	cmp $16 + 16  - 16 +4 , %ecx 	 
	jne L_win_nocull_invalid___vertexStart  

 
	mov %ebp  , %eax
	mov 0x00000de4 (%esi ) , %ebx

	mov %ebp  , 0x00000de4 (%esi )
	sub %ebx , %eax

	mov 0x00000010 (%esi ) , %ebx	 
	sub %eax , 0x00000dec (%esi )

	add $1 , %ebx
	mov %ebx , 0x00000010 (%esi )

 
	mov $0x1 , %eax

L_win_nocull_invalid___triDone  :
 
	mov 0x0000000c (%esi ) , %ecx
	pop %ebp

	add $1 , %ecx	 
	pop %ebx

	pop %edi
	mov %ecx , 0x0000000c (%esi )

	pop %esi
	ret


# 299 "xdraw2.S" 2








.L_END_trisetup_Default_win_nocull_invalid  :
.size _trisetup_Default_win_nocull_invalid  ,.L_END_trisetup_Default_win_nocull_invalid  - _trisetup_Default_win_nocull_invalid  

.align 32
.globl _trisetup_Default_win_cull_invalid  

.type _trisetup_Default_win_cull_invalid  ,@function
_trisetup_Default_win_cull_invalid  :





	






# 1 "xdraw2.inc.s" 1
 



















 

 
 
 
 
 
/
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

# 674 "xdraw2.inc.s"





 
 
 

.file "xdraw2.inc.S"




 
 
 










# 709 "xdraw2.inc.s"

 
	push %esi
	push %edi

	push %ebx
	push %ebp

	mov 4  + 16  (%esp) , %esi 
	
 
 

# 737 "xdraw2.inc.s"

 

# 751 "xdraw2.inc.s"

	
L_win_cull_invalid_pastContextTest  :

 
	movl 0x00000b78 (%esi ) , %edx
	test %edx , %edx
	je L_win_cull_invalid_no_validation  
	call _grValidateState
.align 4

	
L_win_cull_invalid_no_validation  :








 
 
	mov 8 + 16  (%esp) , %eax 
	mov 12 + 16  (%esp) , %ebx 

	mov 0x000001e0 (%esi ) , %edx 
	mov 16 + 16  (%esp) , %ecx 

	test %edx  , %edx 
	jz L_win_cull_invalid_nocull  

	shl $31 , %edx 	 

L_win_cull_invalid_Area_Computation  :
 
 
	flds 0 (%eax )	 
	fsubs 0 (%ebx )	 
	flds 0 (%ebx )	 
	fsubs 0 (%ecx )	 
	flds 4 (%ebx )	 
	fsubs 4 (%ecx )	 
	flds 4 (%eax )	 
	fsubs 4 (%ebx )	 
	fld %st(3)	 
	fmul %st(2) , %st	 
	fld %st(3)	 
	fmul %st(2) , %st	 
	fsubrp %st , %st(1)	 
	fsts One+0x04 	 

 
	fstp %st(0)	 
	fstp %st(0)	 
	fstp %st(0)	 
	fstp %st(0)	 
	fstp %st(0)	 

	mov One+0x04  , %ebp 	 
	xor %eax , %eax	 

 
	and $0x7fffffff , %ebp 	 
	jz L_win_cull_invalid___triDone  

 
	mov One+0x04  , %ebp 	 
	xor %edx  , %ebp 	 

	jge L_win_cull_invalid___triDone  
.align 4


L_win_cull_invalid_nocull  :
 
 
	mov 0x0000004c (%esi ) , %eax
	mov 0x00000dec (%esi ) , %ebx

	add $4 , %eax
	cmp %eax , %ebx

	jge L_win_cull_invalid___triBegin  

	pushl $836
	push $0x0

	push %eax
	call _grCommandTransportMakeRoom
	add $12, %esp

 









.macro GR_FIFO_WRITE __addr __offset __data
# 866 "xdraw2.inc.s"

	mov \__data , \__offset(\__addr)

.endm	 

.align 4
L_win_cull_invalid___triBegin  :
	mov 0x00000de4 (%esi ) , %ebp 	 
	mov $8 + 16  - 16  , %ecx 	 

	mov 0x00000dd8 (%esi ) , %eax	 
	nop 

	GR_FIFO_WRITE %ebp  , 0 , %eax	 
	add $4 , %ebp 	 

.align 4
L_win_cull_invalid___vertexStart  :
	mov 16 (%esp, %ecx ) , %edx 	 
	add $8 , %ebp 

	nop 	 
	nop 

	movl (%edx ) , %eax	 
	lea 0x00000124 (%esi ) , %ebx 	 

	GR_FIFO_WRITE %ebp  , -8 , %eax	 
	movl 4(%edx ) , %eax	 

	xor %edi  , %edi 	 
	GR_FIFO_WRITE %ebp  , -4 , %eax	 

L_win_cull_invalid___doParams  :
	movl (%ebx ) , %eax	 
	add $4 , %ebx 	 

	cmp $0 , %eax	 
	je L_win_cull_invalid___nextVertex  

 
 
	nop 
	nop 

L_win_cull_invalid___paramLoop  :
	movl (%eax,%edx ) , %edi 	 
	add $4 , %ebp 	 

	movl (%ebx ) , %eax	 
	add $4 , %ebx 	 

	cmp $0 , %eax	 
	GR_FIFO_WRITE %ebp  , -4 , %edi 	 

	jne L_win_cull_invalid___paramLoop  

.align 4
L_win_cull_invalid___nextVertex  :
 
	add $4 , %ecx 

	cmp $16 + 16  - 16 +4 , %ecx 	 
	jne L_win_cull_invalid___vertexStart  

 
	mov %ebp  , %eax
	mov 0x00000de4 (%esi ) , %ebx

	mov %ebp  , 0x00000de4 (%esi )
	sub %ebx , %eax

	mov 0x00000010 (%esi ) , %ebx	 
	sub %eax , 0x00000dec (%esi )

	add $1 , %ebx
	mov %ebx , 0x00000010 (%esi )

 
	mov $0x1 , %eax

L_win_cull_invalid___triDone  :
 
	mov 0x0000000c (%esi ) , %ecx
	pop %ebp

	add $1 , %ecx	 
	pop %ebx

	pop %edi
	mov %ecx , 0x0000000c (%esi )

	pop %esi
	ret


# 327 "xdraw2.S" 2








.L_END_trisetup_Default_win_cull_invalid  :
.size _trisetup_Default_win_cull_invalid  ,.L_END_trisetup_Default_win_cull_invalid  - _trisetup_Default_win_cull_invalid  

.align 32
.globl _trisetup_Default_win_cull_valid  
.type _trisetup_Default_win_cull_valid  ,@function
_trisetup_Default_win_cull_valid  :





	






# 1 "xdraw2.inc.s" 1
 



















 

 
 
 
 
 
/
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

# 674 "xdraw2.inc.s"





 
 
 

.file "xdraw2.inc.S"




 
 
 










# 709 "xdraw2.inc.s"

 
	push %esi
	push %edi

	push %ebx
	push %ebp

	mov 4  + 16  (%esp) , %esi 
	
 
 

# 737 "xdraw2.inc.s"

 

# 751 "xdraw2.inc.s"

	
L_win_cull_valid_pastContextTest  :








	
L_win_cull_valid_no_validation  :








 
 
	mov 8 + 16  (%esp) , %eax 
	mov 12 + 16  (%esp) , %ebx 

	mov 0x000001e0 (%esi ) , %edx 
	mov 16 + 16  (%esp) , %ecx 

	test %edx  , %edx 
	jz L_win_cull_valid_nocull  

	shl $31 , %edx 	 

L_win_cull_valid_Area_Computation  :
 
 
	flds 0 (%eax )	 
	fsubs 0 (%ebx )	 
	flds 0 (%ebx )	 
	fsubs 0 (%ecx )	 
	flds 4 (%ebx )	 
	fsubs 4 (%ecx )	 
	flds 4 (%eax )	 
	fsubs 4 (%ebx )	 
	fld %st(3)	 
	fmul %st(2) , %st	 
	fld %st(3)	 
	fmul %st(2) , %st	 
	fsubrp %st , %st(1)	 
	fsts One+0x04 	 

 
	fstp %st(0)	 
	fstp %st(0)	 
	fstp %st(0)	 
	fstp %st(0)	 
	fstp %st(0)	 

	mov One+0x04  , %ebp 	 
	xor %eax , %eax	 

 
	and $0x7fffffff , %ebp 	 
	jz L_win_cull_valid___triDone  

 
	mov One+0x04  , %ebp 	 
	xor %edx  , %ebp 	 

	jge L_win_cull_valid___triDone  
.align 4


L_win_cull_valid_nocull  :
 
 
	mov 0x0000004c (%esi ) , %eax
	mov 0x00000dec (%esi ) , %ebx

	add $4 , %eax
	cmp %eax , %ebx

	jge L_win_cull_valid___triBegin  

	pushl $836
	push $0x0

	push %eax
	call _grCommandTransportMakeRoom
	add $12, %esp

 









.macro GR_FIFO_WRITE __addr __offset __data
# 866 "xdraw2.inc.s"

	mov \__data , \__offset(\__addr)

.endm	 

.align 4
L_win_cull_valid___triBegin  :
	mov 0x00000de4 (%esi ) , %ebp 	 
	mov $8 + 16  - 16  , %ecx 	 

	mov 0x00000dd8 (%esi ) , %eax	 
	nop 

	GR_FIFO_WRITE %ebp  , 0 , %eax	 
	add $4 , %ebp 	 

.align 4
L_win_cull_valid___vertexStart  :
	mov 16 (%esp, %ecx ) , %edx 	 
	add $8 , %ebp 

	nop 	 
	nop 

	movl (%edx ) , %eax	 
	lea 0x00000124 (%esi ) , %ebx 	 

	GR_FIFO_WRITE %ebp  , -8 , %eax	 
	movl 4(%edx ) , %eax	 

	xor %edi  , %edi 	 
	GR_FIFO_WRITE %ebp  , -4 , %eax	 

L_win_cull_valid___doParams  :
	movl (%ebx ) , %eax	 
	add $4 , %ebx 	 

	cmp $0 , %eax	 
	je L_win_cull_valid___nextVertex  

 
 
	nop 
	nop 

L_win_cull_valid___paramLoop  :
	movl (%eax,%edx ) , %edi 	 
	add $4 , %ebp 	 

	movl (%ebx ) , %eax	 
	add $4 , %ebx 	 

	cmp $0 , %eax	 
	GR_FIFO_WRITE %ebp  , -4 , %edi 	 

	jne L_win_cull_valid___paramLoop  

.align 4
L_win_cull_valid___nextVertex  :
 
	add $4 , %ecx 

	cmp $16 + 16  - 16 +4 , %ecx 	 
	jne L_win_cull_valid___vertexStart  

 
	mov %ebp  , %eax
	mov 0x00000de4 (%esi ) , %ebx

	mov %ebp  , 0x00000de4 (%esi )
	sub %ebx , %eax

	mov 0x00000010 (%esi ) , %ebx	 
	sub %eax , 0x00000dec (%esi )

	add $1 , %ebx
	mov %ebx , 0x00000010 (%esi )

 
	mov $0x1 , %eax

L_win_cull_valid___triDone  :
 
	mov 0x0000000c (%esi ) , %ecx
	pop %ebp

	add $1 , %ecx	 
	pop %ebx

	pop %edi
	mov %ecx , 0x0000000c (%esi )

	pop %esi
	ret


# 354 "xdraw2.S" 2








.L_END_trisetup_Default_win_cull_valid  :
.size _trisetup_Default_win_cull_valid  ,.L_END_trisetup_Default_win_cull_valid  - _trisetup_Default_win_cull_valid  

.align 32
.globl _trisetup_Default_win_nocull_valid  
.type _trisetup_Default_win_nocull_valid  ,@function
_trisetup_Default_win_nocull_valid  :





	






# 1 "xdraw2.inc.s" 1
 



















 

 
 
 
 
 
/
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

# 674 "xdraw2.inc.s"





 
 
 

.file "xdraw2.inc.S"




 
 
 










# 709 "xdraw2.inc.s"

 
	push %esi
	push %edi

	push %ebx
	push %ebp

	mov 4  + 16  (%esp) , %esi 
	
 
 

# 737 "xdraw2.inc.s"

 

# 751 "xdraw2.inc.s"

	
L_win_nocull_valid_pastContextTest  :








	
L_win_nocull_valid_no_validation  :
# 823 "xdraw2.inc.s"


L_win_nocull_valid_nocull  :
 
 
	mov 0x0000004c (%esi ) , %eax
	mov 0x00000dec (%esi ) , %ebx

	add $4 , %eax
	cmp %eax , %ebx

	jge L_win_nocull_valid___triBegin  

	pushl $836
	push $0x0

	push %eax
	call _grCommandTransportMakeRoom
	add $12, %esp

 









.macro GR_FIFO_WRITE __addr __offset __data
# 866 "xdraw2.inc.s"

	mov \__data , \__offset(\__addr)

.endm	 

.align 4
L_win_nocull_valid___triBegin  :
	mov 0x00000de4 (%esi ) , %ebp 	 
	mov $8 + 16  - 16  , %ecx 	 

	mov 0x00000dd8 (%esi ) , %eax	 
	nop 

	GR_FIFO_WRITE %ebp  , 0 , %eax	 
	add $4 , %ebp 	 

.align 4
L_win_nocull_valid___vertexStart  :
	mov 16 (%esp, %ecx ) , %edx 	 
	add $8 , %ebp 

	nop 	 
	nop 

	movl (%edx ) , %eax	 
	lea 0x00000124 (%esi ) , %ebx 	 

	GR_FIFO_WRITE %ebp  , -8 , %eax	 
	movl 4(%edx ) , %eax	 

	xor %edi  , %edi 	 
	GR_FIFO_WRITE %ebp  , -4 , %eax	 

L_win_nocull_valid___doParams  :
	movl (%ebx ) , %eax	 
	add $4 , %ebx 	 

	cmp $0 , %eax	 
	je L_win_nocull_valid___nextVertex  

 
 
	nop 
	nop 

L_win_nocull_valid___paramLoop  :
	movl (%eax,%edx ) , %edi 	 
	add $4 , %ebp 	 

	movl (%ebx ) , %eax	 
	add $4 , %ebx 	 

	cmp $0 , %eax	 
	GR_FIFO_WRITE %ebp  , -4 , %edi 	 

	jne L_win_nocull_valid___paramLoop  

.align 4
L_win_nocull_valid___nextVertex  :
 
	add $4 , %ecx 

	cmp $16 + 16  - 16 +4 , %ecx 	 
	jne L_win_nocull_valid___vertexStart  

 
	mov %ebp  , %eax
	mov 0x00000de4 (%esi ) , %ebx

	mov %ebp  , 0x00000de4 (%esi )
	sub %ebx , %eax

	mov 0x00000010 (%esi ) , %ebx	 
	sub %eax , 0x00000dec (%esi )

	add $1 , %ebx
	mov %ebx , 0x00000010 (%esi )

 
	mov $0x1 , %eax

L_win_nocull_valid___triDone  :
 
	mov 0x0000000c (%esi ) , %ecx
	pop %ebp

	add $1 , %ecx	 
	pop %ebx

	pop %edi
	mov %ecx , 0x0000000c (%esi )

	pop %esi
	ret


# 381 "xdraw2.S" 2








.L_END_trisetup_Default_win_nocull_valid  :
.size _trisetup_Default_win_nocull_valid  ,.L_END_trisetup_Default_win_nocull_valid  - _trisetup_Default_win_nocull_valid  

# 428 "xdraw2.S"


.end

