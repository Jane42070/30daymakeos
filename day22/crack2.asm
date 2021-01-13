[INSTRSET "i486p"]
[BITS 32]
		MOV		EAX,1*8		; OS 用的段号
		MOV		DS,AX		; 将其存入 DS
		MOV		BYTE [0x102600],0
		MOV		EDX,4
		INT		0x40
