; haribote-os boot asm
; TAB=4

[INSTRSET "i486p"]

VBEMODE	EQU		0x105			; 1024x768x8bit 分辨率

BOTPAK	EQU		0x00280000		; 加载 bootpack
DSKCAC	EQU		0x00100000		; 磁盘缓存的位置
DSKCAC0	EQU		0x00008000		; 磁盘缓存的位置（实模式）

; BOOT_INFO 相关
CYLS	EQU		0x0ff0			; 引导扇区设置
LEDS	EQU		0x0ff1
VMODE	EQU		0x0ff2			; 关于颜色的信息
SCRNX	EQU		0x0ff4			; 分辨率 X
SCRNY	EQU		0x0ff6			; 分辨率 Y
VRAM	EQU		0x0ff8			; 图像缓冲区的起始地址

		ORG		0xc200			;  这个的程序要被装载的内存地址

; 确认 VBE 是否存在
		MOV		AX,0x9000
		MOV		ES,AX
		MOV		DI,0
		MOV		AX,0x4f00
		INT		0x10
		CMP		AX,0x004f	; 如果有 VBE，AX 的值会变为 0x004f
		JNE		scrn320		; AX != 0x004f 跳转到 scrn320 显示模式

; 检查 VBE 的版本
		MOV		AX,[ES:DI+4]
		CMP		AX,0x0200	; VBE 不是 2.0 以上，不能使用高分辨率
		JB		scrn320		; if (AX < 0x0200) goto scrn320

; 取得画面模式信息
		MOV		CX,VBEMODE
		MOV		AX,0x4f01
		INT		0x10
		CMP		AX,0x004f	; 判断画面模式是否能使用，不能则跳转到 scrn320
		JNE		scrn320

; 确认画面模式信息
		CMP		BYTE [ES:DI+0x19],8
		JNE		scrn320
		CMP		BYTE [ES:DI+0x1b],4
		JNE		scrn320
		MOV		AX,[ES:DI+0x00]
		AND		AX,0x0080
		JZ		scrn320		; 模式属性的bit7是0，所以放弃

; 画面模式切换

		MOV		BX,VBEMODE+0x4000		; VBE 的 640x480x8bit 彩色
		MOV		AX,0x4f02
		INT		0x10
		MOV		BYTE [VMODE],8			; 屏幕的模式（参考 C 语言的引用）
		MOV		AX,[ES:DI+0x12]
		MOV		[SCRNX],AX
		MOV		AX,[ES:DI+0x14]
		MOV		WORD [SCRNY],AX
		MOV		[SCRNY],AX
		MOV		EAX,[ES:DI+0x28]
		MOV		[VRAM],EAX
		JMP		keystatus

scrn320:
		MOV		AL,0x13			; VGA，320x200x8bit模式
		MOV		AH,0x00
		INT		0x10
		MOV		BYTE [VMODE],8	; 记下画面模式（参考 C 语言）
		MOV		WORD [SCRNX],320
		MOV		WORD [SCRNY],200
		MOV		DWORD [VRAM],0x000a0000

; 通过 BIOS 获取指示灯状态
keystatus:
		MOV		AH,0x02
		INT		0x16 			; keyboard BIOS
		MOV		[LEDS],AL

; 防止 PIC 接受所有中断
;	AT 兼容机的规范、PIC 初始化
;	然后之前在 CLI 不做任何事就挂起
;	PIC 在同意后初始化

		MOV		AL,0xff
		OUT		0x21,AL			; 输出 0xff 到 0x21
		NOP						; 不断执行 OUT 指令
		OUT		0xa1,AL

		CLI						; 进一步中断 CPU

; 让 CPU 支持 1M 以上内存、设置 A20GATE
; 最初 CPU 只有 16 位模式，为了兼容旧版 OS，激活 A20 信号线之前，只能使用 1MB 内存
		CALL	waitkbdout
		MOV		AL,0xd1
		OUT		0x64,AL
		CALL	waitkbdout
		MOV		AL,0xdf			; enable A20
		OUT		0x60,AL
		CALL	waitkbdout

; 保护模式转换
; 程序不能随意更改段的设定，也不能使用操作系统专用的段，系统受到 CPU 的保护
; 保护模式 protected virtual address mode
; 16 位模式称为 real mode
[INSTRSET "i486p"]				; 说明使用 486 指令

		LGDT	[GDTR0]			; LGDT 读入临时 GDT
		MOV		EAX,CR0
		AND		EAX,0x7fffffff	; 使用 bit31（禁用分页）
		OR		EAX,0x00000001	; bit0 到 1 转换（保护模式过渡）
		MOV		CR0,EAX
		JMP		pipelineflush	; 模式改变，使用 JMP 重新解释指令
pipelineflush:
		MOV		AX,1*8			;  写 32bit 的段
		MOV		DS,AX
		MOV		ES,AX
		MOV		FS,AX
		MOV		GS,AX
		MOV		SS,AX

; bootpack 传递

		MOV		ESI,bootpack	; 源
		MOV		EDI,BOTPAK		; 目标
		MOV		ECX,512*1024/4
		CALL	memcpy

; 传输磁盘数据

; 从引导区开始

		MOV		ESI,0x7c00		; 源
		MOV		EDI,DSKCAC		; 目标
		MOV		ECX,512/4
		CALL	memcpy

; 剩余的全部

		MOV		ESI,DSKCAC0+512	; 源
		MOV		EDI,DSKCAC+512	; 目标
		MOV		ECX,0
		MOV		CL,BYTE [CYLS]
		IMUL	ECX,512*18*2/4	; 除以 4 得到字节数
		SUB		ECX,512/4		; IPL 偏移量
		CALL	memcpy

; 由于还需要 asmhead 才能完成
; 完成其余的 bootpack 任务

; bootpack 启动

		MOV		EBX,BOTPAK
		MOV		ECX,[EBX+16]
		ADD		ECX,3			; ECX += 3;
		SHR		ECX,2			; ECX /= 4;
		JZ		skip			; 传输完成
		MOV		ESI,[EBX+20]	; 源
		ADD		ESI,EBX
		MOV		EDI,[EBX+12]	; 目标
		CALL	memcpy
skip:
		MOV		ESP,[EBX+12]	; 堆栈的初始化
		JMP		DWORD 2*8:0x0000001b

waitkbdout:
		IN		 AL,0x64
		AND		 AL,0x02
		JNZ		waitkbdout		; AND 结果不为 0 跳转到 waitkbdout
		RET

memcpy:
		MOV		EAX,[ESI]
		ADD		ESI,4
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy			; 运算结果不为 0 跳转到 memcpy
		RET
; memcpy 地址前缀大小

		ALIGNB	16
GDT0:
		RESB	8				; 初始值
		DW		0xffff,0x0000,0x9200,0x00cf	; 写 32bit 位段寄存器
		DW		0xffff,0x0000,0x9a28,0x0047	; 可执行的文件的 32bit 寄存器（bootpack 用）

		DW		0
GDTR0:
		DW		8*3-1
		DD		GDT0

		ALIGNB	16
bootpack:
