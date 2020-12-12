#include "bootpack.h"

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	extern struct FIFO8 keyfifo;
	extern char hankaku[4096];
	// Define FIFO buffer
	unsigned char s[40], mcursor[256], keybuf[32];
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

	putfont8_pos(binfo->vram, binfo->scrnx, 0, 8, COL8_FFFFFF, (unsigned char *)"hello world!");
	putfont8_pos(binfo->vram, binfo->scrnx, 0, 40, COL8_FFFFFF, (unsigned char *)"CLAY");
	putfont8_pos(binfo->vram, binfo->scrnx, 0, 92, COL8_FFFFFF, (unsigned char *)"HARIBOTE.SYS");

	// 开放PIC1和键盘中断(11111001)
	io_out8(PIC0_IMR, 0xf9);
	// 开放鼠标中断(11101111)
	io_out8(PIC1_IMR, 0xef);
	// 初始化keybuf缓冲区
	fifo8_init(&keyfifo, 32, keybuf);

	// 储存键盘数据
	for (;;) {
		// 屏蔽其他中断
		io_cli();
		if (fifo8_status(&keyfifo) == 0) {
			// 接收中断并进入等待
			io_stihlt();
		} else {
			i = fifo8_get(&keyfifo);
			io_sti();
			sprintf((char *)s, "%02X", i);
			boxfill8((unsigned char *)binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 16, 16);
			putfont8_str(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
		}
	}
}
