[BITS 32]
	MOV		AL,'A'
	CALL	2*8:0xBFC
fin:
	HLT
	JMP fin
