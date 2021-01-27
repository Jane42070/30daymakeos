[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api003.asm"]

		GLOBAL	_api_putnstr

[SECTION .text]

_api_putnstr:	; void api_putnstr(char *s, int l);
		PUSH	EBX
		MOV		EDX,3
		MOV		EBX,[ESP+ 8]	; s
		MOV		ECX,[ESP+12]	; l
		INT		0x40
		POP		EBX
		RET
