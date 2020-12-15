#include "bootpack.h"

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;	// 结构体指针指向储存系统信息的地址
	// 内存管理
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	// 图层管理
	struct SHTCTL *shtctl;
	// 图层背景，鼠标
	struct SHEET *sht_bak, *sht_mouse;
	// 定义背景缓冲区、鼠标缓冲区
	unsigned char *buf_back, buf_mouse[256];

	unsigned int memtotal;
	char s[40];
	unsigned char mcursor[256], keybuf[32], mousebuf[128];		// Define FIFO buffer
	int i, j, mx, my;

	init_gdtidt();							// 初始化 全局段记录表，中断记录表
	init_pic();								// 初始化 PIC
	io_sti();								// IDT/PIC 的初始化已经完成，开放 CPU 的中断
	io_out8(PIC0_IMR, 0xf9);				// 开放PIC1和键盘中断(11111001)
	io_out8(PIC1_IMR, 0xef);				// 开放鼠标中断(11101111)
	fifo8_init(&keyfifo, 32, keybuf);		// 初始化keybuf缓冲区
	fifo8_init(&mousefifo, 128, mousebuf);	// 初始化mousebuf缓冲区

	init_keyboard();
	enable_mouse(&mdec);
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); // 0x00001000 - 0x0009efff
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	init_palette();
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	// 分配图层
	sht_bak = sheet_alloc(shtctl);
	sht_mouse = sheet_alloc(shtctl);
	// 分配内存
	buf_back = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	sheet_setbuf(sht_bak, buf_back, binfo->scrnx, binfo->scrny, -1);	// 没有透明色
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);	// 透明色号 99
	init_screen(buf_back, binfo->scrnx, binfo->scrny);
	init_mouse_cursor8(buf_mouse, 99);	// 背景色号 99
	sheet_slide(shtctl, sht_bak, 0, 0);

	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 28 - 16) / 2;
	sheet_slide(shtctl, sht_mouse, mx, my);
	sheet_updown(shtctl, sht_bak, 0);
	sheet_updown(shtctl, sht_mouse, 1);
	// putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf(s, "memory %dMB free: %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfont8_str(buf_back, binfo->scrnx, 0, 50, COL8_FFFFFF, s);
	putfont8_pos(buf_back, binfo->scrnx, 0, 30, COL8_FFFFFF, "CLAY");
	sheet_refresh(shtctl);

	for (;;) {
		// 屏蔽其他中断
		io_cli();
			// 接收中断并进入等待
		if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) io_stihlt();
		else {
			if (fifo8_status(&keyfifo) != 0) {
				i = fifo8_get(&keyfifo);
				io_sti();
				sprintf(s, "%02X", i);
				boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 16, 16);
				putfont8_str(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
				sheet_refresh(shtctl);
			}
			else if (fifo8_status(&mousefifo) != 0)
			{
				i = fifo8_get(&mousefifo);
				io_sti();
				if (mouse_decode(&mdec, i) != 0) {
					sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					if ((mdec.btn & 0x01) != 0) s[1] = 'L';
					if ((mdec.btn & 0x02) != 0) s[3] = 'R';
					if ((mdec.btn & 0x04) != 0) s[2] = 'C';
					boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 32, 16 + 15 * 8 - 1, 48);
					putfont8_str(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
					// 鼠标移动
					mx += mdec.x;
					my += mdec.y;
					// 防止超出屏幕
					if (mx < 0) mx = 0;
					if (my < 0) my = 0;
					if (mx > binfo->scrnx - 16) mx = binfo->scrnx - 16;
					if (my > binfo->scrny - 16) my = binfo->scrny - 16;
					sprintf(s, "(%3d %3d)", mx, my);
					// 隐藏坐标
					boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 16, 100, 32);
					// 显示坐标
					putfont8_str(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
					// 描绘鼠标
					// 包含 sheet_refresh
					sheet_slide(shtctl, sht_mouse, mx, my);
				}
			}
		}
	}
}
