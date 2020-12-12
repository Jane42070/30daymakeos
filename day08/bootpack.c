#include "bootpack.h"
extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

struct MOUSE_DEC {
	unsigned char buf[3], phase;
	// 鼠标移动、按键状态
	int x, y, btn;
} mdec;

void wait_KBC_sendready(void);
void init_keyboard(void);
void enable_mouse(struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data);

void HariMain(void)
{
	// 结构体指针指向储存系统信息的地址
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	extern char hankaku[4096];
	// Define FIFO buffer
	unsigned char mcursor[256], keybuf[32], mousebuf[128];
	char s[40];
	int mx = (binfo->scrnx - 16) / 2; /* 屏幕 */
	int my = (binfo->scrny - 28 - 16) / 2;
	int i, j;

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
	putfont8_pos(binfo->vram, binfo->scrnx, 0, 50, COL8_FFFFFF, "CLAY");
	putfont8_pos(binfo->vram, binfo->scrnx, 0, 92, COL8_FFFFFF, "HARIBOTE.SYS");

	// 开放PIC1和键盘中断(11111001)
	io_out8(PIC0_IMR, 0xf9);
	// 开放鼠标中断(11101111)
	io_out8(PIC1_IMR, 0xef);
	// 初始化keybuf缓冲区
	fifo8_init(&keyfifo, 32, keybuf);
	// 初始化mousebuf缓冲区
	fifo8_init(&mousefifo, 128, mousebuf);

	init_keyboard();
	enable_mouse(&mdec);

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
				sprintf(s, "%02X", i);
				boxfill8((unsigned char *)binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 16, 16);
				putfont8_str(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
			}
			else if (fifo8_status(&mousefifo) != 0)
			{
				i = fifo8_get(&mousefifo);
				io_sti();
				if (mouse_decode(&mdec, i) != 0) {
					sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					if ((mdec.btn & 0x01) != 0) s[1] = 'L' ;
					if ((mdec.btn & 0x02) != 0) s[3] = 'R' ;
					if ((mdec.btn & 0x04) != 0) s[2] = 'C' ;
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 32, 16 + 15 * 8 - 1, 48);
					putfont8_str(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
					// 鼠标移动
					// 隐藏鼠标
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);
					mx += mdec.x;
					my += mdec.y;
					// 防止超出屏幕
					if (mx < 0) mx = 0;
					if (my < 0) my = 0;
					if (mx > binfo->scrnx - 16) mx = binfo->scrnx - 16;
					if (my > binfo->scrny - 16) my = binfo->scrny - 16;
					sprintf(s, "(%3d %3d)", mx, my);
					// 隐藏坐标
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 100, 32);
					// 显示坐标
					putfont8_str(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
					// 描绘鼠标
					putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
				}
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

void enable_mouse(struct MOUSE_DEC *mdec)
{
	/** 激活鼠标 */
	wait_KBC_sendready();
	// 对键盘电路发送指令 0xd4
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	// 等待电路响应
	wait_KBC_sendready();
	// 发送鼠标命令
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE); /** 如果成功，键盘电路会返回ACK(0xfa) */
	mdec->phase = 0;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data)
{
	switch (mdec->phase) {
		/** 等待鼠标发送第一个字节 */
		case 1:
			mdec->buf[0] = data;
			mdec->phase = 2;
			return 0;
		/** 等待鼠标发送第二个字节 */
		case 2:
			mdec->buf[1] = data;
			mdec->phase = 3;
			return 0;
		/** 等待鼠标发送第三个字节
		 *	并处理所有数据 */
		case 3:
			mdec->buf[2] = data;
			mdec->phase = 1;
			mdec->btn = mdec->buf[0] & 0x07;
			mdec->x = mdec->buf[1];
			mdec->y = mdec->buf[2];
			if ((mdec->buf[0] & 0x10) != 0) mdec->x |= 0xffffff00;
			if ((mdec->buf[0] & 0x20) != 0) mdec->y |= 0xffffff00;
			// 鼠标的y方向与画面符号相反
			mdec->y = - mdec->y;
			return 1;
		default:
			// 等待鼠标就绪 (0xfa)
			if (data == 0xfa) mdec->phase = 1;
	}
	return -1;
}
