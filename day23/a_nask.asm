[FORMAT "WCOFF"]			; 生成对象文件格式
[INSTRSET "i486p"]			; 表示使用 486 兼容指令集
[BITS 32]					; 生成 32 位模式机器语言
[FILE "a_nask.asm"]			; 源文件名信息

		GLOBAL _api_putchar, _api_putstr
		GLOBAL _api_end
		GLOBAL _api_openwin, _api_putstrwin, _api_boxfilwin
		GLOBAL _api_initmalloc, _api_malloc, _api_free

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

_api_openwin:	; int api_openwin(char *buf, int sxiz, int yxiz, int col_inv, char *title)
		PUSH	EDI
		PUSH	ESI
		PUSH	EBX
		MOV		EDX,5
		MOV		EBX,[ESP+16]	; buf
		MOV		ESI,[ESP+20]	; sxiz
		MOV		EDI,[ESP+24]	; yxiz
		MOV		EAX,[ESP+28]	; col_inv
		MOV		ECX,[ESP+32]	; title
		INT		0x40
		POP		EBX
		POP		ESI
		POP		EDI
		RET

_api_putstrwin:	; void api_putstrwin(int win, int x, int y, int col, int len, char *str)
		PUSH	EDI
		PUSH	ESI
		PUSH	EBP
		PUSH	EBX
		MOV		EDX,6
		MOV		EBX,[ESP+20]	; win
		MOV		ESI,[ESP+24]	; x
		MOV		EDI,[ESP+28]	; y
		MOV		EAX,[ESP+32]	; col
		MOV		ECX,[ESP+36]	; len
		MOV		EBP,[ESP+40]	; str
		INT		0x40
		POP		EBX
		POP		EBP
		POP		ESI
		POP		EDI
		RET

_api_boxfilwin:	; void api_boxfilwin(int win, int x0, int y0, int x1, int y1, int col)
		PUSH	EDI
		PUSH	ESI
		PUSH	EBP
		PUSH	EBX
		MOV		EDX,7
		MOV		EBX,[ESP+20]	; win
		MOV		EAX,[ESP+24]	; x0
		MOV		ECX,[ESP+28]	; y0
		MOV		ESI,[ESP+32]	; x1
		MOV		EDI,[ESP+36]	; y1
		MOV		EBP,[ESP+40]	; col
		INT		0x40
		POP		EBX
		POP		EBP
		POP		ESI
		POP		EDI
		RET

_api_initmalloc:	; void api_initmalloc()
		PUSH	EBX
		MOV		EDX,8
		MOV		EBX,[CS:0x0020]	; malloc 内存空间的地址
		MOV		EAX,EBX
		ADD		EAX,32*1024		; 加上 32KB
		MOV		ECX,[CS:0x0000]	; 数据段的大小
		SUB		ECX,EAX
		INT		0x40
		POP		EBX
		RET

_api_malloc:		; char *api_malloc(int size)
		PUSH	EBX
		MOV		EDX,9
		MOV		EBX,[CS:0x0020]
		MOV		ECX,[ESP+8]		; size
		INT		0x40
		POP		EBX
		RET

_api_free:			; void api_free(char *addr, int size)
		PUSH	EBX
		MOV		EDX,10
		MOV		EBX,[CS:0x0020]
		MOV		EAX,[ESP+8]		; addr
		MOV		ECX,[ESP+12]	; size
		INT		0x40
		POP		EBX
		RET

_api_end:	; void api_end()
		MOV		EDX,4
		INT		0x40
