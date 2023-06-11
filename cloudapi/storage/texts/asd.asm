global _start

_start:
	mov eax, 4
	mov ebx, 1
	mov ecx, msg
	mov edx, len
	int 0x80

	mov eax, 1
	mov ebx, 1
	int 0x80

section data:

	msg db "testeseet", 0x0a
	len equ $ - msg
