[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api004.asm"]

		GLOBAL	_api_end

[SECTION .text]

_api_end:	; void api_end(void);
		MOV		EDX,4
		INT		0x40
