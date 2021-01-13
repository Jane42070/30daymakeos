[FORMAT "WCOFF"]			; 生成对象文件格式
[INSTRSET "i486p"]			; 表示使用 486 兼容指令集
[BITS 32]					; 生成 32 位模式机器语言
[FILE "a_nask.nas"]			; 源文件名信息

	GLOBAL _api_putchar, _api_putstr
	GLOBAL _api_end

[SECTION .text]

_api_putchar:	; void api_putchar(int c)
	MOV		EDX,1
	MOV		AL,[ESP+4]		; int c
	INT		0x40
	RET

_api_putstr:	; void api_putstr(char *s)
	PUSH	EBX
	MOV		EDX,2
	MOV		EBX,[ESP+8]		; s
	INT		0x40
	POP		EBX
	RET

_api_end:	; void api_end()
	MOV		EDX,4
	INT		0x40
