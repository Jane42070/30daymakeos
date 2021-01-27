[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api024.asm"]

		GLOBAL	_api_fsize

[SECTION .text]
_api_fsize:				; int api_fsize(int fhandle, int mode);
		PUSH	EBX
		MOV		EDX,25
		MOV		EAX,[ESP+16]	; fhandle
		MOV		ECX,[ESP+12]	; maxsize
		MOV		EBX,[ESP+8]		; buf
		INT		0x40
		POP		EBX
		RET
