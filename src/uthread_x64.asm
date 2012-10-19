.text

_netlib_after_swap:
		mov -0x8(%rsp), %rbx
		mov -0x10(%rsp), %rcx
		mov -0x18(%rsp), %rdx
		mov -0x20(%rsp), %rbp
		mov -0x28(%rsp), %r8
		mov -0x30(%rsp), %r9
		mov -0x38(%rsp), %r10
		mov -0x40(%rsp), %r11
		mov -0x48(%rsp), %r12
		mov -0x50(%rsp), %r13
		mov -0x58(%rsp), %r14
		mov -0x60(%rsp), %r15

		mov -0x68(%rsp), %rsi
		mov -0x70(%rsp), %rdi
		
		ret
		
		.global netlib_swap_context
netlib_swap_context:
		mov %rdi, -0x70(%rsp)
		mov %rsi, -0x68(%rsp)
		
		mov %rbx, -0x8(%rsp)
		mov %rcx, -0x10(%rsp)
		mov %rdx, -0x18(%rsp)
		mov %rbp, -0x20(%rsp)
		mov %r8, -0x28(%rsp)
		mov %r9, -0x30(%rsp)
		mov %r10, -0x38(%rsp)
		mov %r11, -0x40(%rsp)
		mov %r12, -0x48(%rsp)
		mov %r13, -0x50(%rsp)
		mov %r14, -0x58(%rsp)
		mov %r15, -0x60(%rsp)
		
		mov %rsp, (%rdi)
		mov %rsi, %rsp

		jmp _netlib_after_swap

		.global netlib_make_context
netlib_make_context:
		lea -0x80(%rdi), %rax
		mov %rsi, (%rax)				# store param2 as return
		mov %rdx, -0x70(%rax)			# store param3 as param1
		ret
