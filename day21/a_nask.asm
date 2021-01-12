[FORMAT "WCOFF"]			; 生成对象文件格式
[INSTRSET "i486p"]			; 表示使用 486 兼容指令集
[BITS 32]					; 生成 32 位模式机器语言
[FILE "a_nask.nas"]			; 源文件名信息

	GLOBAL _api_putchar

[SECTION .text]

_api_putchar:	; void api_putchar(int c)
	MOV		EDX,1
	MOV		AL,[ESP+4]		; int c
	INT		0x40
	RET
