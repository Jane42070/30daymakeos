#include "bootpack.h"
extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

void wait_KBC_sendready(void);
void init_keyboard(void);
void enable_mouse(void);

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	extern char hankaku[4096];
	// Define FIFO buffer
	unsigned char s[40], mcursor[256], keybuf[32], mousebuf[128];
	int mx = (binfo->scrnx - 16) / 2; /* 屏幕 */
	int my = (binfo->scrny - 28 - 16) / 2;
	int i,j;

	// 初始化16位颜色
	init_palette();
	// 初始化 全局段记录表，中断记录表
	init_gdtidt();
	// 初始化 PIC
	init_pic();
	// IDT/PIC 的初始化已经完成，开放 CPU 的中断
	io_sti();

	init_screen((unsigned char *)binfo->vram, binfo->scrnx, binfo->scrny);
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf((char *)s, "(%d, %d)", mx, my);
	putfont8_str(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);



	putfont8_pos(binfo->vram, binfo->scrnx, 0, 30, COL8_FFFFFF, (unsigned char *)"hello world!");
	putfont8_pos(binfo->vram, binfo->scrnx, 0, 50, COL8_FFFFFF, (unsigned char *)"CLAY");
	putfont8_pos(binfo->vram, binfo->scrnx, 0, 92, COL8_FFFFFF, (unsigned char *)"HARIBOTE.SYS");

	// 开放PIC1和键盘中断(11111001)
	io_out8(PIC0_IMR, 0xf9);
	// 开放鼠标中断(11101111)
	io_out8(PIC1_IMR, 0xef);
	// 初始化keybuf缓冲区
	fifo8_init(&keyfifo, 32, keybuf);
	// 初始化mousebuf缓冲区
	fifo8_init(&mousefifo, 128, mousebuf);

	init_keyboard();
	enable_mouse();

	// 储存键盘数据
	for (;;) {
		// 屏蔽其他中断
		io_cli();
			// 接收中断并进入等待
		if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) { io_stihlt(); }
		else {
			if (fifo8_status(&keyfifo) != 0) {
				i = fifo8_get(&keyfifo);
				io_sti();
				sprintf((char *)s, "%02X", i);
				boxfill8((unsigned char *)binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 16, 16);
				putfont8_str(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
			}
			else if (fifo8_status(&mousefifo) != 0)
			{
				i = fifo8_get(&mousefifo);
				io_sti();
				sprintf((char *)s, "%02X", i);
				boxfill8((unsigned char *)binfo->vram, binfo->scrnx, COL8_008484, 0, 32, 16, 48);
				putfont8_str(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
			}
		}
	}
}

void wait_KBC_sendready(void)
{
	/** 等待键盘控制电路准备完毕 */
	for(;;) { if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) { break; } }
}

void init_keyboard(void)
{
	/** 初始化键盘控制电路
	 *  鼠标控制电路集成在键盘控制电路上 */
	wait_KBC_sendready();
	// 对鼠标发送模式设定指令 0x60
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	// 等待电路响应
	wait_KBC_sendready();
	// 开启鼠标模式 0x47
	io_out8(PORT_KEYDAT, KBC_MODE);
}

void enable_mouse(void)
{
	/** 激活鼠标 */
	wait_KBC_sendready();
	// 对键盘电路发送指令 0xd4
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	// 等待电路响应
	wait_KBC_sendready();
	// 发送鼠标命令
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE); /** 如果成功，键盘电路会返回ACK(0xfa) */
}
